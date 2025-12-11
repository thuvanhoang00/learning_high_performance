project(slogger2)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
file(GLOB SOURCES *.cpp)

add_executable(${PROJECT_NAME} ${SOURCES})