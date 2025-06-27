set(CMAKE_CXX_STANDARD 20)

add_executable("exchange_example"
    exchange_main.cpp
    matching_engine.cpp
    me_order_book.cpp
    me_order.cpp
    
)
