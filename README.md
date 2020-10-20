# MicroSocks11 - A cross-platform SOCKS5 server.

[![MIT License](https://img.shields.io/badge/license-MIT-blue.svg?style=flat)](LICENSE)

MicroSocks11 is a SOCKS5 library and server based on the [microsocks](https://github.com/rofl0r/microsocks) project that use CMake and C++11 for cross-platform support.
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
git clone git@github.com:EvanMcBroom/microsocks11.git
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
