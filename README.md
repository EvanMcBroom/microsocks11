# MicroSocks11 - A cross-platform SOCKS5 server.

[![MIT License](https://img.shields.io/badge/license-MIT-blue.svg?style=flat)](LICENSE)

MicroSocks11 is based on the [microsocks](https://github.com/rofl0r/microsocks) project and uses CMake and C++11 for cross-platform support.

This project is under an [MIT license](https://github.com/EvanMcBroom/microsocks11/blob/master/LICENSE.txt).
Any sections of code that are included from the original project are also under an [MIT license](https://github.com/rofl0r/microsocks/blob/master/COPYING) and copyrighted to [rofl0r](https://github.com/rofl0r/).

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
cmake --build . --target microsocks
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