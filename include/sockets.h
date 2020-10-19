#pragma once
#if defined(_WIN32) || defined(WIN32) || defined(__MINGW32__)
#define WINDOWS

#include <io.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")

typedef const char optval_t;
typedef size_t ssize_t;
const auto closesocket_ = closesocket;

#elif

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>

typedef int optval_t;
const auto closesocket_ = close;

#endif