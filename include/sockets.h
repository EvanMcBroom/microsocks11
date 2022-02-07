// Copyright (C) 2020 Evan McBroom
#pragma once
#if defined(_WIN32) || defined(WIN32) || defined(__MINGW32__)
#define WINDOWS
#include <io.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")

using optval_t = const char;
using ssize_t = size_t;
using ioctlarg_t = u_long*;
const auto closesocket_ = closesocket;
const auto ioctl_ = ioctlsocket;

#else

#include <arpa/inet.h>
#include <cerrno>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

using optval_t = int;
using ioctlarg_t = char*;
const auto closesocket_ = close;
const auto ioctl_ = ioctl;

#endif

using Socket = int;
