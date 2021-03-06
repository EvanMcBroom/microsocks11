# MicroSocks11 - A cross-platform SOCKS5 server.

[![MIT License](https://img.shields.io/badge/license-MIT-blue.svg?style=flat)](LICENSE.txt)
[![AppVeyor](https://img.shields.io/appveyor/build/EvanMcBroom/microsocks11?logo=AppVeyor)](https://ci.appveyor.com/project/EvanMcBroom/microsocks11/)
[![Travis](https://img.shields.io/travis/EvanMcBroom/microsocks11?logo=Travis)](https://travis-ci.org/EvanMcBroom/microsocks11)
[![GitHub All Releases](https://img.shields.io/github/downloads/EvanMcBroom/microsocks11/total?color=yellow)](https://github.com/EvanMcBroom/microsocks11/releases)
[![Docker Pulls](https://img.shields.io/docker/pulls/evanmcbroom/microsocks11?color=yellow)](https://hub.docker.com/r/evanmcbroom/microsocks11)

MicroSocks11 is a SOCKS5 library and server based on the [microsocks](https://github.com/rofl0r/microsocks) project that uses CMake and C++11 for cross-platform support.
MicroSocks11 supports IPv4, IPv6, and DNS.

This project is released under an [MIT license](https://github.com/EvanMcBroom/microsocks11/blob/master/LICENSE.txt).
Any sections of code that are included from the original project are also released under an [MIT license](https://github.com/rofl0r/microsocks/blob/master/COPYING) and copyrighted to [rofl0r](https://github.com/rofl0r/).

## Building

MicroSocks11 requres [cxxopts](https://github.com/jarro2783/cxxopts) which can be installed using [vcpkg](https://github.com/microsoft/vcpkg).

```
vcpkg install cxxopts
```

MicroSocks11 uses [CMake](https://cmake.org/) to generate and run the build system files for your platform.

```
git clone https://github.com/EvanMcBroom/microsocks11.git
cd microsocks11
mkdir builds
cd builds
cmake .. -DCMAKE_TOOLCHAIN_FILE=$VCPKG_PATH/scripts/buildsystems/vcpkg.cmake
cmake --build .
```

By default CMake will build both the `microsocks` utility and the `socks5` static library it uses. The `socks5` library has a simple api that can be used to build other utilities.

You can set the `BUILD_STATIC` flag when generating the build system files to link against the static version of the runtime library. This will build the `microsocks` utility as a standalone program that can be ran on other hosts.

```
cmake .. -DBUILD_STATIC=TRUE
cmake --build .
```

> :pencil2: On Linux you will need to install `clang` and `libc++` for static builds because `glibc` can not statically link the `getaddrinfo` symbol ([reference](https://stackoverflow.com/a/3087067/11039217)). Please refer to the Travis CI file for how to install `clang` and `libc++`.

## Running

MicroSocks11 is simple to use and can be ran directly with default arguments.

```
./microsocks --help
SOCKS5 Server
Usage:
  MicroSocks [OPTION...]

  -1, --auth-once         Whitelist an ip address after authenticating to not
                          requre a password on subsequent connections
  -b, --bind-address arg  The address for the all forwarded connections
  -h, --help              Print this message
  -l, --listen-ip arg     Ip address to listen on (default: 0.0.0.0)
  -p, --port arg          Port to listen on (default: 1080)
  -u, --user arg          The username to use for authentication
  -v, --verbose           Verbose output
  -P, --password arg      The password to use for authentication
./microsocks
...
```

You can use curl to test if the proxy is running.

```
curl --socks5-hostname localhost:1080 icanhazip.com
```

## Docker Container

MicroSocks11 can also be built and ran as a docker container.
You can either build the image yourself or pull the image from [dockerhub](https://hub.docker.com/u/evanmcbroom).

```
# build option 1 - pull the container from dockerhub
docker pull evanmcbroom/microsocks11

# build option 2 - build the container yourself
git clone https://github.com/EvanMcBroom/microsocks11.git
docker build microsocks11 --tag evanmcbroom/microsocks11:latest

# run example 1 - run the proxy on 0.0.0.0:1080 in the background
docker run --rm -d --net=host --name=proxy evanmcbroom/microsocks11

# run example 2 - run the proxy with custom arguments
docker run --rm --net=host --name=proxy evanmcbroom/microsocks11 --help
```
