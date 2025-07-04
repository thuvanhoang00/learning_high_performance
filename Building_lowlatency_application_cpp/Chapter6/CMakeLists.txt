set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_COMPILER g++)
set(CMAKE_CXX_FLAGS "-std=c++2a -Wall -Wextra -Werror -Wpedantic")

add_subdirectory(common)
add_subdirectory(order_server)
add_subdirectory(market_data)
include_directories(${PROJECT_SOURCE_DIR})

add_executable("exchange_example"
    exchange_main.cpp
    matching_engine.cpp
    me_order_book.cpp
    me_order.cpp
    
)
