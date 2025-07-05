set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_COMPILER g++)
set(CMAKE_CXX_FLAGS "-std=c++2a -Wall -Wextra -Werror -Wpedantic")
set(CMAKE_VERBOSE_MAKEFILE on)

project(LowLatencyApp)

add_subdirectory(common)
add_subdirectory(exchange)

list(APPEND LIBS libexchange)
list(APPEND LIBS libcommon)
list(APPEND LIBS pthread)

include_directories(${PROJECT_SOURCE_DIR})
file(GLOB EXCHANGE_DIRS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "${PROJECT_SOURCE_DIR}/exchange/*")
foreach(dir ${EXCHANGE_DIRS})
    if(IS_DIRECTORY "${PROJECT_SOURCE_DIR}/exchange/${dir}")
        target_include_directories(my_target PRIVATE "${PROJECT_SOURCE_DIR}/exchange/${dir}")
    endif()
endforeach()

# include_directories(${PROJECT_SOURCE_DIR}/exchange/*)

add_executable(exchange_main exchange/exchange_main.cpp)
target_link_libraries(exchange_main PUBLIC ${LIBS})