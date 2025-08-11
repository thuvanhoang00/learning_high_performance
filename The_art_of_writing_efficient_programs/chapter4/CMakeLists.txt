set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_COMPILER g++)
project(memory_architecture)
find_package(benchmark REQUIRED)

add_executable(${PROJECT_NAME} memory_architecture.cpp)
target_link_libraries(${PROJECT_NAME} benchmark::benchmark)

add_executable(list_vector list_vector.cpp)
target_link_libraries(list_vector benchmark::benchmark)