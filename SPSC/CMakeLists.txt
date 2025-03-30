cmake_minimum_required(VERSION 3.8)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED true)

project(spsc)
find_package(benchmark REQUIRED)
add_executable(${PROJECT_NAME}
    SPSC_bench.cpp
    #SPSC.cpp    
)
target_link_libraries(${PROJECT_NAME} "${benchmark_LIBRARIES}")
target_link_libraries(${PROJECT_NAME} benchmark::benchmark)
target_compile_options(${PROJECT_NAME} PRIVATE -O3)