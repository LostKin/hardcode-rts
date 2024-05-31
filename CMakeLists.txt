cmake_minimum_required(VERSION 3.16)
project(rts)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(libhardcode)
add_subdirectory(libhccn)
add_subdirectory(libmatchstate)
add_subdirectory(librtsn)
add_subdirectory(libapi)

add_subdirectory(rts_client)
add_subdirectory(rts_server)
