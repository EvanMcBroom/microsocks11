// Copyright (C) 2020 Evan McBroom

#pragma once

#include <mutex>
#include <sblist.h>
#include <server.h>

enum class AuthMethod {
	NO_AUTH = 0,
	GSSAPI = 1,
	USERNAME = 2,
	INVALID = 0xFF
};

enum class ErrorCode {
	SUCCESS = 0,
	GENERAL_FAILURE = -1,
	NOT_ALLOWED = -2,
	NET_UNREACHABLE = -3,
	HOST_UNREACHABLE = -4,
	CONN_REFUSED = -5,
	TTL_EXPIRED = -6,
	COMMAND_NOT_SUPPORTED = -7,
	ADDRESSTYPE_NOT_SUPPORTED = -8,
};

struct Buffer {
public:
	Buffer(Socket socket_)
		: socket_(socket_) {};

	unsigned char data[1024] = { 0 };
	int recieved = 0;

	void recieve();

private:
	Socket socket_;
};

class Socks5Server {
public:
	struct Client : Server::Client {
		enum class State {
			CONNECTED,
			NEED_AUTH, /* skipped if NO_AUTH method supported */
			AUTHED,
		};

		State state;
	};

	Socks5Server() {
		bindAddress.v4.sin_family = AF_UNSPEC;
	}

	void setAuthentication(const char* username, const char* password, bool maintainAuthentication = false) {
		username = username;
		password = password;
		maintainAuthentication = maintainAuthentication;
	}

	void setBindAddress(const char* address) {
		resolveSockAddress(address, 0, &bindAddress);
	}

	void setVerbose(bool verbose) {
		verbose = verbose;
	}

	bool start(const char* host, unsigned short port);

private:
	const char* username = nullptr;
	const char* password = nullptr;
	bool maintainAuthentication = false;
	std::mutex authenticatedClientsMutex;
	std::vector<Socks5Server::Client> authenticatedClients;
	sockAddress bindAddress;
	bool verbose = false;

	inline void addAuthAddress(Socks5Server::Client& client) {
		const std::lock_guard<std::mutex> lock(authenticatedClientsMutex);
		authenticatedClients.emplace_back(client);
	};
	AuthMethod checkAuthMethod(Buffer& buffer, Socks5Server::Client& client);
	ErrorCode connectClient(Buffer& buffer, Socks5Server::Client& client);
	ErrorCode checkCredentials(Buffer& buffer);
	void* proxyRequest(Socks5Server::Client client);
};

namespace {
	void copy(int fd1, int fd2);
	int isAuthenticated(sockAddress& client, sockAddress& authedip);
	void sendResponseCode(int socket, int version, int authMethod);
	void sendError(int socket, ErrorCode errorCode);
}