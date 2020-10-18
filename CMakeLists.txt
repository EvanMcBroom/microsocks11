cmake_minimum_required(VERSION 3.14.0)
project(microsocks11 C CXX)
set(TARGET microsocks)
add_executable(${TARGET})
add_subdirectory(include)
target_sources(${TARGET} PRIVATE
    sblist.c
    sblist_delete.c
    server.c
    sockssrv.c
)