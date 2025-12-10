cmake_minimum_required(VERSION 3.22)
project(spliting_large_object)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)

find_package(benchmark REQUIRED)

add_executable(${PROJECT_NAME} spliting_large_object.cpp)
target_link_libraries(${PROJECT_NAME} benchmark::benchmark)
