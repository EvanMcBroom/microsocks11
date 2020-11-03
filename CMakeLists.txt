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

if (NOT MSVC)
	target_link_libraries(${LIBRARY} pthread)
endif()

if(BUILD_STATIC)
    if (MSVC)
        string(REGEX REPLACE "/M[TD]d?" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
        string(REGEX REPLACE "/M[TD]d?" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
        target_link_options(${SERVER} PRIVATE /INCREMENTAL:NO /NODEFAULTLIB:MSVCRT)
    else ()
	    target_link_options(${SERVER} PRIVATE -static)
    endif()
endif()
