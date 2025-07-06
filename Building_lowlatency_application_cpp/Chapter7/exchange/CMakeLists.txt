set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_COMPILER g++)
set(CMAKE_CXX_FLAGS "-std=c++2a -Wall -Wextra -Werror -Wpedantic")
set(CMAKE_VERBOSE_MAKEFILE on)

file(GLOB SOURCES "*/*.cpp")

include_directories(${PROJECT_SOURCE_DIR})
add_library(libexchange STATIC ${SOURCES})

target_include_directories(libexchange PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/market_data
    ${CMAKE_CURRENT_SOURCE_DIR}/order_server
    ${CMAKE_CURRENT_SOURCE_DIR}/matcher
)

target_link_libraries(libexchange PRIVATE libcommon)

