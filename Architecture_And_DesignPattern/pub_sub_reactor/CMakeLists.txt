project(simple_pub_sub)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB SRCS "*.cpp")
add_executable(${PROJECT_NAME} ${SRCS})