cmake_minimum_required(VERSION 3.8)
project(day1)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED true)

#create executable file
add_executable(${PROJECT_NAME} main.cpp)

#find lib
find_package(benchmark REQUIRED)
target_link_libraries(${PROJECT_NAME} benchmark::benchmark)

