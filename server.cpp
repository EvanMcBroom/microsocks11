// Copyright (C) 2020 Evan McBroom

#include <cstdint>
#include <server.h>
#include <sockets.h>
#include <stdio.h>
#include <string.h>

int bindToSockAddress(Socket socket_, sockAddress* bindaddr) {
	auto size{ length(bindaddr) };
	return (size) ? bind(socket_, (struct sockaddr*) bindaddr, size) : 0;
}

int resolve(const char* host, unsigned short port, struct addrinfo** addr) {
	struct addrinfo hints = { 0 };
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	char service[8];
	snprintf(service, sizeof(service), "%u", port);
	return getaddrinfo(host, service, &hints, addr);
}

int resolveSockAddress(const char* host, unsigned short port, sockAddress* res) {
	struct addrinfo* ainfo = 0;
	int ret;
	*family(res) = AF_UNSPEC;
	if ((ret = resolve(host, port, &ainfo))) return ret;
	memcpy(res, ainfo->ai_addr, ainfo->ai_addrlen);
	freeaddrinfo(ainfo);
	return 0;
}

bool Server::start(const char* host, unsigned short port) {
	struct addrinfo* ainfo = 0;
	if (resolve(host, port, &ainfo)) return false;
	struct addrinfo* p;
	socket_ = -1;
	for (p = ainfo; p; p = p->ai_next) {
		if ((socket_ = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0)
			continue;
		optval_t yes = 1;
		setsockopt(socket_, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
		size_t on{ true };
		ioctl_(socket_, FIONBIO, reinterpret_cast<ioctlarg_t>(&on));
		if (bind(socket_, p->ai_addr, p->ai_addrlen) < 0) {
			closesocket_(socket_);
			socket_ = -1;
			continue;
		}
		break;
	}
	freeaddrinfo(ainfo);
	if (socket_ < 0) return false;
	if (listen(socket_, SOMAXCONN) < 0) {
		closesocket_(socket_);
		return false;
	}
	return true;
}

Server::Client Server::waitForClient() {
	Client client;
	socklen_t length{ sizeof(Client::address) };
	client.valid = ((client.socket_ = accept(socket_, reinterpret_cast<sockaddr*>(&client.address), &length)) != -1);
	return client;
}
