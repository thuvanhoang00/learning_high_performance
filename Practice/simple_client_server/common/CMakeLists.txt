cmake_minimum_required(VERSION 3.14)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED true)
# set(CMAKE_CXX_COMPILER "/usr/bin/g++-13")
set(CMAKE_CXX_COMPILER "/usr/bin/clang++-17")
set(CMAKE_CXX_FLAGS "-std=c++2a -O3 -g -mavx2 -Wall -Werror -pedantic")
file(GLOB SOURCES "*.cpp")


add_library(common ${SOURCES})