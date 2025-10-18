cmake_minimum_required(VERSION 3.14)
# set(CMAKE_CXX_STANDARD 20)
# set(CMAKE_CXX_STANDARD_REQUIRED true)
set(CMAKE_CXX_COMPILER "/usr/bin/clang++-17")
set(CMAKE_CXX_FLAGS "-std=c++2a -O3 -g -mavx2 -Wall -pedantic")
project(Client_Server)

add_subdirectory(common)
add_subdirectory(server)
add_subdirectory(client)

add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE common)