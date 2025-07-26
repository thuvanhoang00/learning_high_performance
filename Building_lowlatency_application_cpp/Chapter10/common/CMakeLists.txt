set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_COMPILER g++)
set(CMAKE_CXX_FLAGS "-std=c++2a -Wall -Wextra -Werror -Wpedantic")
set(CMAKE_VERBOSE_MAKEFILE on)

file(GLOB SOURCES "*.cpp")
file(GLOB HEADERS "*.h")

add_library(libcommon STATIC ${SOURCES} ${HEADERS})
target_include_directories(libcommon PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})