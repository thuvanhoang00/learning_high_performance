#pragma once
#include "tcp_socket.h"
#include <vector>
#include <algorithm>

namespace common{
struct TCPServer{
    int efd_ = -1;
    TCPSocket listener_socket_;
    epoll_event events_[1024];
    std::vector<TCPSocket*> sockets_, receive_sockets_, send_sockets_, disconnected_sockets_;
    std::function<void(TCPSocket* s, Nanos rx_time)> recv_callback_;
    std::function<void()> recv_finished_callback_;
    std::string time_str;

    explicit TCPServer() : listener_socket_(){
        recv_callback_ = [this](auto socket, auto rx_time){
            defaultRecvCallback(socket, rx_time);
        };

        recv_finished_callback_ = [this](){
            defaultRecvFinishedCallback();
        };
    }

    // Deleted constructor

    auto defaultRecvCallback(TCPSocket* socket, Nanos rx_time) noexcept->void;
    auto defaultRecvFinishedCallback() noexcept->void;
    auto destroy();
    auto listen(const std::string& iface, int port)->void;
    auto epoll_add(TCPSocket* socket)->bool;
    auto epoll_del(TCPSocket* socket)->bool;
    auto del(TCPSocket* socket);
    auto poll() noexcept->void;
    auto sendAndRecv() noexcept->void;
};

}