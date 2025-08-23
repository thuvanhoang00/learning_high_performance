set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_COMPILER g++)
project(high_performance)
set(MEMORY_BENCH memory_bench)

find_package(benchmark REQUIRED)

add_executable(${PROJECT_NAME} vector_sort.cpp)
add_executable(${MEMORY_BENCH} memory_bench.cpp)
target_link_libraries(${PROJECT_NAME} benchmark::benchmark)
target_link_libraries(${MEMORY_BENCH} benchmark::benchmark)

# add_executable(list_vector list_vector.cpp)
# target_link_libraries(list_vector benchmark::benchmark)