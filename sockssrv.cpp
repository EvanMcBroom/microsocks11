/* Copyright (C) 2017 rofl0r. */

#include <algorithm>
#include <cxxopts.hpp>
#include <errno.h>
#include <future>
#include <limits.h>
#include <memory>
#include <mutex>
#include <sblist.h>
#include <server.h>
#include <signal.h>
#include <sockets.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <thread>
#include <vector>

static const char* auth_user;
static const char* auth_pass;
static sblist* auth_ips;
const char* listenip;
unsigned port;
static std::mutex auth_ips_mutex;
static const struct server* server;
static union sockaddr_union bind_addr{ AF_UNSPEC };
static bool verbose;

enum socksstate {
	SS_1_CONNECTED,
	SS_2_NEED_AUTH, /* skipped if NO_AUTH method supported */
	SS_3_AUTHED,
};

enum authmethod {
	AM_NO_AUTH = 0,
	AM_GSSAPI = 1,
	AM_USERNAME = 2,
	AM_INVALID = 0xFF
};

enum errorcode {
	EC_SUCCESS = 0,
	EC_GENERAL_FAILURE = 1,
	EC_NOT_ALLOWED = 2,
	EC_NET_UNREACHABLE = 3,
	EC_HOST_UNREACHABLE = 4,
	EC_CONN_REFUSED = 5,
	EC_TTL_EXPIRED = 6,
	EC_COMMAND_NOT_SUPPORTED = 7,
	EC_ADDRESSTYPE_NOT_SUPPORTED = 8,
};

struct context_t {
	struct client client;
	enum socksstate state;
};

static int connect_socks_target(unsigned char *buf, size_t n, struct client *client) {
	if(n < 5) return -EC_GENERAL_FAILURE;
	if(buf[0] != 5) return -EC_GENERAL_FAILURE;
	if(buf[1] != 1) return -EC_COMMAND_NOT_SUPPORTED; /* we support only CONNECT method */
	if(buf[2] != 0) return -EC_GENERAL_FAILURE; /* malformed packet */

	int af = AF_INET;
	size_t minlen = 4 + 4 + 2, l;
	char namebuf[256];
	struct addrinfo* remote;

	switch(buf[3]) {
		case 4: /* ipv6 */
			af = AF_INET6;
			minlen = 4 + 2 + 16;
			/* fall through */
		case 1: /* ipv4 */
			if(n < minlen) return -EC_GENERAL_FAILURE;
			if(namebuf != inet_ntop(af, buf+4, namebuf, sizeof namebuf))
				return -EC_GENERAL_FAILURE; /* malformed or too long addr */
			break;
		case 3: /* dns name */
			l = buf[4];
			minlen = 4 + 2 + l + 1;
			if(n < 4 + 2 + l + 1) return -EC_GENERAL_FAILURE;
			memcpy(namebuf, buf+4+1, l);
			namebuf[l] = 0;
			break;
		default:
			return -EC_ADDRESSTYPE_NOT_SUPPORTED;
	}
	unsigned short port;
	port = (buf[minlen-2] << 8) | buf[minlen-1];
	/* there's no suitable errorcode in rfc1928 for dns lookup failure */
	if(resolve(namebuf, port, &remote)) return -EC_GENERAL_FAILURE;
	int fd = socket(remote->ai_addr->sa_family, SOCK_STREAM, 0);
	if(fd == -1) {
		eval_errno:
		if(fd != -1) closesocket_(fd);
		freeaddrinfo(remote);
		switch(errno) {
			case ETIMEDOUT:
				return -EC_TTL_EXPIRED;
			case EPROTOTYPE:
			case EPROTONOSUPPORT:
			case EAFNOSUPPORT:
				return -EC_ADDRESSTYPE_NOT_SUPPORTED;
			case ECONNREFUSED:
				return -EC_CONN_REFUSED;
			case ENETDOWN:
			case ENETUNREACH:
				return -EC_NET_UNREACHABLE;
			case EHOSTUNREACH:
				return -EC_HOST_UNREACHABLE;
			case EBADF:
			default:
			perror("socket/connect");
			return -EC_GENERAL_FAILURE;
		}
	}
	if(SOCKADDR_UNION_AF(&bind_addr) != AF_UNSPEC && bindtoip(fd, &bind_addr) == -1)
		goto eval_errno;
	if(connect(fd, remote->ai_addr, remote->ai_addrlen) == -1)
		goto eval_errno;

	freeaddrinfo(remote);
	char clientname[256];
	af = SOCKADDR_UNION_AF(&client->addr);
	void *ipdata = SOCKADDR_UNION_ADDRESS(&client->addr);
	inet_ntop(af, ipdata, clientname, sizeof clientname);
	if (verbose)
		fprintf(stderr, "client[%d] %s: connected to %s:%d\n", client->fd, clientname, namebuf, port);
	return fd;
}

static int is_authed(union sockaddr_union *client, union sockaddr_union *authedip) {
	int af = SOCKADDR_UNION_AF(authedip);
	if(af == SOCKADDR_UNION_AF(client)) {
		size_t cmpbytes = af == AF_INET ? 4 : 16;
		void *cmp1 = SOCKADDR_UNION_ADDRESS(client);
		void *cmp2 = SOCKADDR_UNION_ADDRESS(authedip);
		if(!memcmp(cmp1, cmp2, cmpbytes)) return 1;
	}
	return 0;
}

static enum authmethod check_auth_method(unsigned char *buf, size_t n, struct client*client) {
	if(buf[0] != 5) return AM_INVALID;
	size_t idx = 1;
	if(idx >= n ) return AM_INVALID;
	int n_methods = buf[idx];
	idx++;
	while(idx < n && n_methods > 0) {
		if(buf[idx] == AM_NO_AUTH) {
			if(!auth_user) return AM_NO_AUTH;
			else if(auth_ips) {
				size_t i;
				int authed = 0;
				const std::lock_guard<std::mutex> lock(auth_ips_mutex);
				for(i=0;i<sblist_getsize(auth_ips);i++) {
					if((authed = is_authed(&client->addr, reinterpret_cast<sockaddr_union*>(sblist_get(auth_ips, i)))))
						break;
				}
				if(authed) return AM_NO_AUTH;
			}
		} else if(buf[idx] == AM_USERNAME) {
			if(auth_user) return AM_USERNAME;
		}
		idx++;
		n_methods--;
	}
	return AM_INVALID;
}

static void add_auth_ip(struct client*client) {
	const std::lock_guard<std::mutex> lock(auth_ips_mutex);
	sblist_add(auth_ips, &client->addr);
}

static void send_auth_response(int fd, int version, enum authmethod meth) {
	unsigned char buf[2];
	buf[0] = version;
	buf[1] = meth;
	send(fd, reinterpret_cast<const char *>(buf), 2, 0);
}

static void send_error(int fd, enum errorcode ec) {
	/* position 4 contains ATYP, the address type, which is the same as used in the connect
	   request. we're lazy and return always IPV4 address type in errors. */
	char buf[10] = { 5, ec, 0, 1 /*AT_IPV4*/, 0,0,0,0, 0,0 };
	send(fd, reinterpret_cast<const char*>(buf), 10, 0);
}

static void copyloop(int fd1, int fd2) {
	int maxfd = fd2;
	if(fd1 > fd2) maxfd = fd1;
	fd_set fdsc, fds;
	FD_ZERO(&fdsc);
	FD_SET(fd1, &fdsc);
	FD_SET(fd2, &fdsc);

	while(1) {
		memcpy(&fds, &fdsc, sizeof(fds));
		/* inactive connections are reaped after 15 min to free resources.
		   usually programs send keep-alive packets so this should only happen
		   when a connection is really unused. */
		struct timeval timeout;
		timeout.tv_sec = 60 * 15;
		timeout.tv_usec = 0;
		switch(select(maxfd+1, &fds, 0, 0, &timeout)) {
			case 0:
				send_error(fd1, EC_TTL_EXPIRED);
				return;
			case -1:
				if(errno == EINTR) continue;
				else perror("select");
				return;
		}
		int infd = FD_ISSET(fd1, &fds) ? fd1 : fd2;
		int outfd = infd == fd2 ? fd1 : fd2;
		char buf[1024];
		ssize_t sent = 0, n = recv(infd, buf, sizeof(sizeof buf), 0);
		if(n <= 0) return;
		while(sent < n) {
			auto m{ send(outfd, reinterpret_cast<const char*>(buf + sent), n - sent, 0) };
			if(m < 0) return;
			sent += m;
		}
	}
}

static enum errorcode check_credentials(unsigned char* buf, size_t n) {
	if(n < 5) return EC_GENERAL_FAILURE;
	if(buf[0] != 1) return EC_GENERAL_FAILURE;
	unsigned ulen, plen;
	ulen=buf[1];
	if(n < 2 + ulen + 2) return EC_GENERAL_FAILURE;
	plen=buf[2+ulen];
	if(n < 2 + ulen + 1 + plen) return EC_GENERAL_FAILURE;
	char user[256], pass[256];
	memcpy(user, buf+2, ulen);
	memcpy(pass, buf+2+ulen+1, plen);
	user[ulen] = 0;
	pass[plen] = 0;
	if(!strcmp(user, auth_user) && !strcmp(pass, auth_pass)) return EC_SUCCESS;
	return EC_NOT_ALLOWED;
}

static void* clientthread(void *data) {
	struct context_t *context = reinterpret_cast<struct context_t*>(data);
	context->state = SS_1_CONNECTED;
	unsigned char buf[1024];
	ssize_t n;
	int ret;
	int remotefd = -1;
	enum authmethod am;
	while((n = recv(context->client.fd, reinterpret_cast<char*>(buf), sizeof buf, 0)) > 0) {
		switch(context->state) {
			case SS_1_CONNECTED:
				am = check_auth_method(buf, n, &context->client);
				if(am == AM_NO_AUTH) context->state = SS_3_AUTHED;
				else if (am == AM_USERNAME) context->state = SS_2_NEED_AUTH;
				send_auth_response(context->client.fd, 5, am);
				if(am == AM_INVALID) goto breakloop;
				break;
			case SS_2_NEED_AUTH:
				ret = check_credentials(buf, n);
				send_auth_response(context->client.fd, 1, static_cast<authmethod>(ret));
				if(ret != EC_SUCCESS)
					goto breakloop;
				context->state = SS_3_AUTHED;
				if(auth_ips) add_auth_ip(&context->client);
				break;
			case SS_3_AUTHED:
				ret = connect_socks_target(buf, n, &context->client);
				if(ret < 0) {
					send_error(context->client.fd, static_cast<errorcode>(ret*-1));
					goto breakloop;
				}
				remotefd = ret;
				send_error(context->client.fd, EC_SUCCESS);
				copyloop(context->client.fd, remotefd);
				goto breakloop;

		}
	}
breakloop:

	if(remotefd != -1)
		closesocket_(remotefd);

	closesocket_(context->client.fd);
	free(context);
	return 0;
}

static int usage(void) {
	fprintf(stderr,
		"MicroSocks SOCKS5 Server\n"
		"------------------------\n"
		"usage: microsocks -1 -i listenip -p port -u user -P password -b bindaddr\n"
		"all arguments are optional.\n"
		"by default listenip is 0.0.0.0 and port 1080.\n\n"
		"option -b specifies which ip outgoing connections are bound to\n"
		"this is handy for programs like firefox that don't support\n"
		"user/pass auth. for it to work you'd basically make one connection\n"
		"with another program that supports it, and then you can use firefox too.\n"
	);
	return 1;
}

/* prevent username and password from showing up in top. */
static void zero_arg(char *s) {
	size_t i, l = strlen(s);
	for(i=0;i<l;i++) s[i] = 0;
}

int main(int argc, char** argv) {
	cxxopts::Options options("MicroSocks", "SOCKS5 Server");
	options.add_options()
		("1,auth-once", "Whitelist an ip address after authenticating to not requre a password on subsequent connections")
		("b,bind-address", "Int param", cxxopts::value<std::string>()->default_value(""))
		("h,help", "Print this message")
		("l,listen-ip", "Ip address to listen on", cxxopts::value<std::string>()->default_value("0.0.0.0"))
		("p,port", "Port to listen on", cxxopts::value<std::string>()->default_value("1080"))
		("u,user", "The username to use for authentication", cxxopts::value<std::string>())
		("v,verbose", "Verbose output")
		("P,password", "The password to use for authentication", cxxopts::value<std::string>())
	;
	options.allow_unrecognised_options();
	auto result{ options.parse(argc, argv) };

	if (result.count("help")) {
		fprintf(stderr, "%s\n", options.help().c_str());
		exit(0);
	}

	bool auth{ result["1"].count() != 0 };
	bool user{ result["u"].count() != 0 };
	bool password{ result["p"].count() != 0 };

	if (user ^ password) {
		fprintf(stderr, "error: user and pass must be used together\n");
		return 1;
	}

	if (auth && !password) {
		fprintf(stderr, "error: auth-once option must be used together with user/pass\n");
		return 1;
	}

	if (result.count("1"))
		auth_ips = sblist_new(sizeof(union sockaddr_union), 8);

	if (result.count("b"))
		resolve_sa(result["b"].as<std::string>().c_str(), 0, &bind_addr);

	if (user)
		auth_user = strdup(result["u"].as<std::string>().c_str());

	if (password)
		auth_pass = strdup(result["P"].as<std::string>().c_str());

	listenip = strdup(result["l"].as<std::string>().c_str());

	port = atoi(result["p"].as<std::string>().c_str());

	verbose = result.count("v") != 0;

	for (size_t i = 1; i < argc; i++)
		zero_arg(argv[i]);

#ifdef WINDOWS
	WSADATA wsaData = { 0 };
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		return false;
#else
	signal(SIGPIPE, SIG_IGN);
#endif

	struct server s;
	std::vector<std::unique_ptr<std::thread>> threads;
	if (server_setup(&s, listenip, port)) {
		perror("server_setup");
		return 1;
	}
	server = &s;

	while (1) {
		// collect
		std::for_each(threads.begin(), threads.end(), [](std::unique_ptr<std::thread>& thread) { thread->join(); });
		threads.erase(threads.begin(), threads.end());

		struct client c;
		auto curr{ reinterpret_cast<struct context_t*>(malloc(sizeof(struct context_t))) };
		if(server_waitclient(&s, &c)) continue;
		curr->client = c;
		threads.emplace_back(std::make_unique<std::thread>(clientthread, curr));
	}

#ifdef WINDOWS
	WSACleanup();
#endif
}
