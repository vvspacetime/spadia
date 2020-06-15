cmake_minimum_required(VERSION 3.13)
project(test)

set(CMAKE_CXX_STANDARD 17)
#add_compile_options(-fsanitize=address)
#add_compile_options(-fno-omit-frame-pointer)

include_directories(./)

add_executable(main example/main.cpp
        peerconnection.cpp
        pool.cpp
        media_stream.cpp
        server.cpp
        rtp_packet.cpp
        stun_packet.cpp
        rtcp_packet.cpp rtp_session_manager.cpp rtp_session_manager.h tools.cpp crc32.h)

target_link_libraries(main glog uuid uv sdptransform crypto pthread dl rt)
#target_link_libraries(server uv pthread dl rt)


