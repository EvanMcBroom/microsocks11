// Copyright (C) 2020 Evan McBroom

#pragma once

#include <sockets.h>
#include <vector>

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

union sockAddress {
	struct sockaddr_in  v4;
	struct sockaddr_in6 v6;
};

union internetAddress {
	struct in_addr  v4;
	struct in6_addr v6;
};

constexpr auto family(sockAddress* address) {
	return &address->v4.sin_family;
}

constexpr auto length(sockAddress* address) {
	return (*family(address) == AF_INET) ?
		sizeof(address->v4) : (*family(address) == AF_INET6) ?
			sizeof(address->v6) : 0;
}

constexpr auto address(sockAddress* address) {
	return (*family(address) == AF_INET) ?
		reinterpret_cast<internetAddress*>(&address->v4.sin_addr) : (*family(address) == AF_INET6) ?
			reinterpret_cast<internetAddress*>(&address->v6.sin6_addr) : nullptr;
}

constexpr auto port(sockAddress* address) {
	return (*family(address) == AF_INET) ?
		address->v4.sin_port : (*family(address) == AF_INET6) ?
			address->v6.sin6_port : 0;
}

int bindToSockAddress(Socket socket_, sockAddress* bindaddr);
int resolve(const char* host, unsigned short port, struct addrinfo** addr);
int resolveSockAddress(const char* host, unsigned short port, sockAddress* res);
std::pair<ErrorCode, size_t> waitForClients(Socket socket_, size_t seconds = 15);

class Server {
public:
	struct Client {
		union sockAddress address;
		Socket socket_;
		bool valid;
	};

	Client acceptClient();
	bool start(const char* host, unsigned short port);
	void stop();
	std::pair<ErrorCode, size_t> waitForClients(size_t seconds = 15) { return ::waitForClients(socket_, seconds); };

private:
	Socket socket_;
};