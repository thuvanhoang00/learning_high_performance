cmake_minimum_required(VERSION 3.8)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED true)

project(lockfree_bench)
find_package(benchmark REQUIRED)
add_executable(${PROJECT_NAME}
    lockfree_bench.cpp
)
target_link_libraries(${PROJECT_NAME} "${benchmark_LIBRARIES}")
target_link_libraries(${PROJECT_NAME} benchmark::benchmark)
target_compile_options(${PROJECT_NAME} PRIVATE -O3)