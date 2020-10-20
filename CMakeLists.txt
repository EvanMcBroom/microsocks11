cmake_minimum_required(VERSION 3.14.0)
project(microsocks11 CXX)
find_package(cxxopts CONFIG REQUIRED)

set(PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
)

set(LIBRARY socks5)
add_library(${LIBRARY} STATIC)
set_target_properties(${LIBRARY} PROPERTIES ${PROPERTIES})
target_sources(${LIBRARY} PRIVATE server.cpp socks5.cpp)

set(SERVER microsocks)
add_executable(${SERVER})
set_target_properties(${SERVER} PROPERTIES ${PROPERTIES})
target_link_libraries(${SERVER} PRIVATE ${LIBRARY} cxxopts::cxxopts)
target_sources(${SERVER} PRIVATE main.cpp)

add_subdirectory(include)

if(BUILD_STATIC)
    if (MSVC)
        set(CMAKE_CXX_FLAGS_RELEASE "/MT")
        set(CMAKE_CXX_FLAGS_DEBUG "/MTd")
        target_link_options(${SERVER} PRIVATE /INCREMENTAL:NO /NODEFAULTLIB:MSVCRT)
    else ()
        set(CMAKE_EXE_LINKER_FLAGS " -static")
        target_link_libraries(${SERVER} -static -static-libgcc -static-libstdc++)
    endif()
endif()