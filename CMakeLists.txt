cmake_minimum_required(VERSION 3.14)
project(ParcelLockerNetwork)

set(CMAKE_CXX_STANDARD 17)

include_directories(include server)

add_executable(server
    server/main.cpp
    server/Server.cpp
)

add_executable(client
    client/client.cpp
)

target_link_libraries(server PRIVATE ws2_32 sqlite3)
target_link_libraries(client PRIVATE ws2_32)