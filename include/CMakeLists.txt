target_sources(${SERVER} PRIVATE
    sblist.h
    server.h
    sockets.h
)
target_include_directories(${SERVER} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})