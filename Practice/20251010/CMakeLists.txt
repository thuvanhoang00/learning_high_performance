cmake_minimum_required(VERSION 3.8)
project(day2)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED true)
add_executable(${PROJECT_NAME} main.cpp)

find_package(benchmark REQUIRED)
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/common)
target_link_libraries(${PROJECT_NAME} benchmark::benchmark)