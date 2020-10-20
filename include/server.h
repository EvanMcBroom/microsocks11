// Copyright (C) 2020 Evan McBroom

#pragma once

#include <sockets.h>

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

class Server {
public:
	struct Client {
		union sockAddress address;
		Socket socket_;
		bool valid;
	};

	bool start(const char* host, unsigned short port);
	Client waitForClient();

private:
	Socket socket_;
};