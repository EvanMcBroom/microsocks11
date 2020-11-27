target_sources(${LIBRARY} PRIVATE
    server.h
    sockets.h
    socks5.h
)
target_include_directories(${LIBRARY} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})