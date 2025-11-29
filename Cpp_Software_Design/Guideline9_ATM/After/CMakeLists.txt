cmake_minimum_required(VERSION 3.15)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED true)

set(CMAKE_CXX_COMPILER "/usr/bin/clang++-18")
set(CMAKE_CXX_FLAGS "-std=c++2a -O3")

project(ATM)
file(GLOB SOURCES "*.cpp")


add_executable(${PROJECT_NAME} ${SOURCES})
