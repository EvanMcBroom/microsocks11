// Copyright (C) 2020 Evan McBroom
#pragma once
#include <future>
#include <mutex>
#include <queue>
#include <server.h>
#include <vector>

namespace {
	class Socks5Processor {
	public:
		struct Client {
			enum class State {
				CONNECTED,
				NEED_AUTH, /* skipped if NO_AUTH method supported */
				AUTHED,
			};

			State state;
		};

		void setAuthentication(const char* username, const char* password, bool maintainAuthentication = false) {
			username = username;
			password = password;
			maintainAuthentication = maintainAuthentication;
		}
		void setVerbose(bool verbose) {
			verbose = verbose;
		}

	protected:
		std::mutex authenticatedClientsMutex;
		const char* username = nullptr;
		const char* password = nullptr;
		bool maintainAuthentication = false;
		bool verbose = false;

		template<typename _C>
		inline void addAuthAddress(std::vector<_C> authenticatedClients, _C& client) {
			const std::lock_guard<std::mutex> lock(authenticatedClientsMutex);
			authenticatedClients.emplace_back(client);
		};
	};
}

enum class AuthMethod {
	NO_AUTH = 0,
	USERNAME = 2,
	INVALID = 0xFF
};

class Socks5IoStream : public Socks5Processor {
public:
	using Buffer = std::vector<char>;
	using ClientId = uint32_t;
	struct Client : public Socks5Processor::Client {
		ClientId id;
	};

	Socks5IoStream(Socks5IoStream&&) noexcept = default;
	Socks5IoStream& operator=(Socks5IoStream&& other) noexcept = default;
	~Socks5IoStream() { }

	std::pair<ClientId, Buffer> read();
	std::pair<ErrorCode, size_t> waitForData(size_t seconds = 15);
	void write(ClientId id, const Buffer& buffer);

private:
	using DataQueue = std::queue<std::pair<ClientId, Buffer>>;
	std::vector<Client> authenticatedClients;
	DataQueue inQueue;
	DataQueue outQueue;
};

class Socks5Server : public Socks5Processor {
public:
	struct Buffer {
	public:
		unsigned char data[1024] = { 0 };
		int recieved = 0;

		Buffer(Socket socket_)
			: socket_(socket_) {};
		void recieve();

	private:
		Socket socket_;
	};
	struct Client : public Socks5Processor::Client {
		Server::Client socketInfo;
	};

	Socks5Server() {
		bindAddress.v4.sin_family = AF_UNSPEC;
	}
	Socks5Server(Socks5Server&&) noexcept = default;
	Socks5Server& operator=(Socks5Server&& other) noexcept = default;
	~Socks5Server(){ }

	void setBindAddress(const char* address) {
		resolveSockAddress(address, 0, &bindAddress);
	}
	bool start(const char* host, unsigned short port, std::unique_ptr<std::promise<int>> error = nullptr);
	void stop() { stopListening = true; }

private:
	std::vector<Client> authenticatedClients;
	sockAddress bindAddress;
	bool stopListening = false;

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
