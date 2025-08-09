set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_COMPILER g++)
# set(CMAKE_CXX_FLAGS "-std=c++2a -O3 -march=native -fno-if-conversion -fno-tree-vectorize")
project(test_benchmark)
find_package(benchmark REQUIRED)
add_executable(${PROJECT_NAME} branch_prediction.cpp)
target_link_libraries(${PROJECT_NAME} benchmark::benchmark)
add_executable(branch_less branch_less.cpp)
target_link_libraries(branch_less benchmark::benchmark)
# target_compile_options(${PROJECT_NAME} )





