cmake_minimum_required(VERSION 3.14)
set(CMAKE_CXX_COMPILER "/usr/bin/clang++-17")
# set(CMAKE_CXX_FLAGS "-std=c++2a -O3 -g -Wall -fsanitize=thread -fsanitize=undefined -Werror -Wpedantic")
set(CMAKE_CXX_FLAGS "-std=c++2a -O3 -g -Wall -pthread")

find_package(benchmark REQUIRED)

add_executable(P1_hello_async "P1_hello_async.cpp")
add_executable(thread_pool "thread_pool.cpp")

target_link_libraries(P1_hello_async PRIVATE benchmark::benchmark)
