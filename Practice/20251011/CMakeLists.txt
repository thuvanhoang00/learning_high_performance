cmake_minimum_required(VERSION 3.28)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED true)
set(CMAKE_CXX_FLAGS "-std=c++2a -Wall -Wextra -Werror -Wpedantic -O3")
project(day3)

add_executable(${PROJECT_NAME} main.cpp)

find_package(benchmark REQUIRED)
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/common)
target_link_libraries(${PROJECT_NAME} benchmark::benchmark)