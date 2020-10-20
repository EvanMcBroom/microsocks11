cmake_minimum_required(VERSION 3.14.0)
project(microsocks11 CXX)
set(SERVER microsocks)
find_package(cxxopts CONFIG REQUIRED)

add_executable(${SERVER})
set_target_properties(${SERVER} PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
)

target_link_libraries(${SERVER} PRIVATE cxxopts::cxxopts)

add_subdirectory(include)
target_sources(${SERVER} PRIVATE
    main.cpp
    server.cpp
    socks5.cpp
)