// Copyright (C) 2020 Evan McBroom
#include <cerrno>
#include <limits.h>
#include <memory>
#include <server.h>
#include <signal.h>
#include <sockets.h>
#include <socks5.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <socks5.h>

std::pair<Socks5IoStream::ClientId, Socks5IoStream::Buffer> Socks5IoStream::read() {
	return std::pair<Socks5IoStream::ClientId, Socks5IoStream::Buffer>{};
}

std::pair<ErrorCode, size_t> Socks5IoStream::waitForData(size_t seconds) {
	return std::pair<ErrorCode, size_t>{};
}

void Socks5IoStream::write(ClientId id, const Buffer& buffer) {

}

void Socks5Server::Buffer::recieve() {
	auto [error, clientCount] { waitForClients(socket_) };
	recieved = recv(socket_, reinterpret_cast<char*>(data), sizeof(data), 0);
}

bool Socks5Server::start(const char* host, unsigned short port, std::unique_ptr<std::promise<int>> error) {
#ifdef WINDOWS
	WSADATA wsaData = { 0 };
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		return false;
#else
	signal(SIGPIPE, SIG_IGN);
#endif

	Server server;
	auto success{ server.start(host, port) };
	if (error)
		error->set_value(errno);
	if (!success)
		return false;

	do {
		auto[error, clientCount]{ server.waitForClients() };
		if ((error == ErrorCode::TTL_EXPIRED && stopListening) || error == ErrorCode::GENERAL_FAILURE)
			break;

		while (clientCount--) {
			Client client;
			client.socketInfo = server.acceptClient();
			if (!client.socketInfo.valid) continue;
			std::thread handleRequest{ [this, &client]() { proxyRequest(client); } };
			handleRequest.detach();
		}
	} while (true);
	server.stop();

#ifdef WINDOWS
	WSACleanup();
#endif
	return true;
}

AuthMethod Socks5Server::checkAuthMethod(Buffer& buffer, Socks5Server::Client& client) {
	if (buffer.data[0] != 5) return AuthMethod::INVALID;
	size_t index = 1;
	if (index >= buffer.recieved) return AuthMethod::INVALID;
	int n_methods = buffer.data[index];
	index++;
	while (index < buffer.recieved && n_methods > 0) {
		if (static_cast<AuthMethod>(buffer.data[index]) == AuthMethod::NO_AUTH) {
			if (!username) return AuthMethod::NO_AUTH;
			else if (maintainAuthentication) {
				size_t i;
				int authed = 0;
				const std::lock_guard<std::mutex> lock(authenticatedClientsMutex);
				for (i = 0; i < authenticatedClients.size(); i++) {
					if ((authed = isAuthenticated(client.socketInfo.address, authenticatedClients.at(i).socketInfo.address)))
						break;
				}
				if (authed) return AuthMethod::NO_AUTH;
			}
		}
		else if (static_cast<AuthMethod>(buffer.data[index]) == AuthMethod::USERNAME) {
			if (username) return AuthMethod::USERNAME;
		}
		index++;
		n_methods--;
	}
	return AuthMethod::INVALID;
}

// returns Socket on success and ErrorCode on failure

ErrorCode Socks5Server::connectClient(Buffer& buffer, Socks5Server::Client& client) {
	if (buffer.recieved < 5) return ErrorCode::GENERAL_FAILURE;
	if (buffer.data[0] != 5) return ErrorCode::GENERAL_FAILURE;
	if (buffer.data[1] != 1) return ErrorCode::COMMAND_NOT_SUPPORTED; /* we only support the CONNECT method */
	if (buffer.data[2] != 0) return ErrorCode::GENERAL_FAILURE; /* malformed packet */

	auto af{ AF_INET };
	size_t minlen = 4 + 4 + 2, l;
	char host[256];
	struct addrinfo* remote;

	switch (buffer.data[3]) {
	case 4: /* ipv6 */
		af = AF_INET6;
		minlen = 4 + 2 + 16;
		/* fall through */
	case 1: /* ipv4 */
		if (buffer.recieved < minlen) return ErrorCode::GENERAL_FAILURE;
		if (host != inet_ntop(af, buffer.data + 4, host, sizeof host))
			return ErrorCode::GENERAL_FAILURE; /* malformed or too long addr */
		break;
	case 3: /* dns name */
		l = buffer.data[4];
		minlen = 4 + 2 + l + 1;
		if (buffer.recieved < 4 + 2 + l + 1) return ErrorCode::GENERAL_FAILURE;
		memcpy(host, buffer.data + 4 + 1, l);
		host[l] = 0;
		break;
	default:
		return ErrorCode::ADDRESSTYPE_NOT_SUPPORTED;
	}
	unsigned short port;
	port = (buffer.data[minlen - 2] << 8) | buffer.data[minlen - 1];
	/* there is no suitable error code in rfc1928 for a dns lookup failure */
	if (resolve(host, port, &remote)) return ErrorCode::GENERAL_FAILURE;
	Socket fd = socket(remote->ai_addr->sa_family, SOCK_STREAM, 0);
	if (fd == -1) {
	eval_errno:
		if (fd != -1) closesocket_(fd);
		freeaddrinfo(remote);
		switch (errno) {
		case ETIMEDOUT:
			return ErrorCode::TTL_EXPIRED;
		case EPROTOTYPE:
		case EPROTONOSUPPORT:
		case EAFNOSUPPORT:
			return ErrorCode::ADDRESSTYPE_NOT_SUPPORTED;
		case ECONNREFUSED:
			return ErrorCode::CONN_REFUSED;
		case ENETDOWN:
		case ENETUNREACH:
			return ErrorCode::NET_UNREACHABLE;
		case EHOSTUNREACH:
			return ErrorCode::HOST_UNREACHABLE;
		case EBADF:
		default:
			if (verbose)
				fprintf(stderr, "socket/connect\n");
			return ErrorCode::GENERAL_FAILURE;
		}
	}
	if (family(&bindAddress) != AF_UNSPEC && bindToSockAddress(fd, &bindAddress) == -1)
		goto eval_errno;
	if (connect(fd, remote->ai_addr, remote->ai_addrlen) == -1)
		goto eval_errno;

	freeaddrinfo(remote);
	char clientname[256];
	af = *family(&client.socketInfo.address);
	void* ipdata = address(&client.socketInfo.address);
	inet_ntop(af, ipdata, clientname, sizeof clientname);
	if (verbose)
		fprintf(stderr, "client[%d] %s: connected to %s:%d\n", client.socketInfo.socket_, clientname, host, port);
	return static_cast<ErrorCode>(fd);
}

ErrorCode Socks5Server::checkCredentials(Buffer& buffer) {
	if (buffer.recieved < 5) return ErrorCode::GENERAL_FAILURE;
	if (buffer.data[0] != 1) return ErrorCode::GENERAL_FAILURE;
	unsigned ulen, plen;
	ulen = buffer.data[1];
	if (buffer.recieved < 2 + ulen + 2) return ErrorCode::GENERAL_FAILURE;
	plen = buffer.data[2 + ulen];
	if (buffer.recieved < 2 + ulen + 1 + plen) return ErrorCode::GENERAL_FAILURE;
	char user[256], pass[256];
	memcpy(user, buffer.data + 2, ulen);
	memcpy(pass, buffer.data + 2 + ulen + 1, plen);
	user[ulen] = 0;
	pass[plen] = 0;
	if (!strcmp(user, username) && !strcmp(pass, password)) return ErrorCode::SUCCESS;
	return ErrorCode::NOT_ALLOWED;
}

void* Socks5Server::proxyRequest(Socks5Server::Client client) {
	using State = Socks5Server::Client::State;
	client.state = State::CONNECTED;
	Buffer buffer(client.socketInfo.socket_);
	while (buffer.recieve(), buffer.recieved > 0) {
		switch (client.state) {
		case State::CONNECTED: {
			auto authMethod{ checkAuthMethod(buffer, client) };
			if (authMethod == AuthMethod::NO_AUTH) client.state = State::AUTHED;
			else if (authMethod == AuthMethod::USERNAME) client.state = State::NEED_AUTH;
			sendResponseCode(client.socketInfo.socket_, 5, static_cast<int>(authMethod));
			if (authMethod == AuthMethod::INVALID) goto breakloop;
			break;
		}
		case State::NEED_AUTH: {
			auto value{ checkCredentials(buffer) };
			sendResponseCode(client.socketInfo.socket_, 1, static_cast<int>(value));
			if (value != ErrorCode::SUCCESS)
				goto breakloop;
			client.state = State::AUTHED;
			if (maintainAuthentication)
				addAuthAddress(authenticatedClients, client);
			break;
		}
		case State::AUTHED: {
			auto remoteSocket{ static_cast<Socket>(connectClient(buffer, client)) };
			if (remoteSocket < 0) {
				sendError(client.socketInfo.socket_, static_cast<ErrorCode>(remoteSocket));
				goto breakloop;
			}
			sendError(client.socketInfo.socket_, ErrorCode::SUCCESS);
			copy(client.socketInfo.socket_, remoteSocket);
			closesocket_(remoteSocket);
			goto breakloop;
		}
		}
	}
breakloop:
	closesocket_(client.socketInfo.socket_);
	return 0;
}

namespace {
	int isAuthenticated(union sockAddress& client, sockAddress& authedip) {
		auto af{ family(&authedip) };
		if (af == family(&client)) {
			size_t cmpbytes = (*af == AF_INET) ? 4 : 16;
			void* cmp1 = address(&client);
			void* cmp2 = address(&authedip);
			if (!memcmp(cmp1, cmp2, cmpbytes)) return 1;
		}
		return 0;
	}

	void sendResponseCode(int socket, int version, int code) {
		unsigned char buf[2];
		buf[0] = version;
		buf[1] = static_cast<int>(code);
		send(socket, reinterpret_cast<const char*>(buf), 2, 0);
	}

	void sendError(int socket, ErrorCode errorCode) {
		/* position 4 contains ATYP, the address type, which is the same as used in the connect
		   request. we're lazy and return always IPV4 address type in errors. */
		char buf[10] = { 5, static_cast<char>(errorCode), 0, 1 /*AT_IPV4*/, 0,0,0,0, 0,0 };
		send(socket, reinterpret_cast<const char*>(buf), 10, 0);
	}
	
	void copy(int fd1, int fd2) {
		int maxfd = fd2;
		if (fd1 > fd2) maxfd = fd1;
		fd_set fdsc, fds;
		FD_ZERO(&fdsc);
		FD_SET(fd1, &fdsc);
		FD_SET(fd2, &fdsc);

		while (1) {
			memcpy(&fds, &fdsc, sizeof(fds));
			/* inactive connections are reaped after 15 min to free resources.
			   usually programs send keep-alive packets so this should only happen
			   when a connection is really unused. */
			struct timeval timeout;
			timeout.tv_sec = 60 * 15;
			timeout.tv_usec = 0;
			switch (select(maxfd + 1, &fds, 0, 0, &timeout)) {
			case 0:
				sendError(fd1, ErrorCode::TTL_EXPIRED);
				return;
			case -1:
				if (errno == EINTR) continue;
				else perror("select");
				return;
			}
			int infd = FD_ISSET(fd1, &fds) ? fd1 : fd2;
			int outfd = infd == fd2 ? fd1 : fd2;
			char buf[1024];
			ssize_t sent = 0, n = recv(infd, buf, sizeof(sizeof buf), 0);
			if (n <= 0) return;
			while (sent < n) {
				auto m{ send(outfd, reinterpret_cast<const char*>(buf + sent), n - sent, 0) };
				if (m < 0) return;
				sent += m;
			}
		}
	}
}
