target_sources(${SERVER} PRIVATE
    server.h
    sockets.h
    socks5.h
)
target_include_directories(${SERVER} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})