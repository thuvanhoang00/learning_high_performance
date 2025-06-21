set(CMAKE_CXX_STANDARD 20)

add_executable("socket_example"
    socket_example.cpp
    tcp_socket.cpp
    tcp_server.cpp
    socket_utils.cpp
)