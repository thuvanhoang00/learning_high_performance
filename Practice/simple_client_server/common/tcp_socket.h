#pragma once
#include <functional>
#include "socket_utils.h"

namespace common{
constexpr size_t TCPBufferSize = 64 * 1024 * 1024;
typedef int64_t Nanos;
    constexpr Nanos NANOS_TO_MICROS = 1000;
    constexpr Nanos MICROS_TO_MILLIS = 1000;
    constexpr Nanos MILLIS_TO_SECS = 1000;
    constexpr Nanos NANOS_TO_MILLIS = NANOS_TO_MICROS * MICROS_TO_MILLIS;
    constexpr Nanos NANOS_TO_SECS = NANOS_TO_MILLIS * MILLIS_TO_SECS;
struct TCPSocket{
    explicit TCPSocket(){
        send_buffer_ = new char[TCPBufferSize];
        rcv_buffer_ = new char[TCPBufferSize];
        recv_callback_ = [this](auto socket, auto rx_time){
            defaultRecvCallback(socket, rx_time);
        };
    }

    // deleted constructor

    ~TCPSocket(){
        destroy();
        delete[] send_buffer_; send_buffer_ = nullptr;
        delete[] rcv_buffer_; rcv_buffer_ = nullptr;
    }

    auto destroy() noexcept->void;
    auto defaultRecvCallback(TCPSocket* socket, Nanos rx_time) noexcept->void;
    auto connect(const std::string& ip, const std::string& iface, int port, bool is_listening)->int;
    auto send(const void* data, size_t len) noexcept->void;
    auto sendAndRecv() noexcept->bool;


    int fd_ = -1;
    char* send_buffer_ = nullptr;
    size_t next_send_valid_index_ = 0;
    char* rcv_buffer_ = nullptr;
    size_t next_rcv_valid_index_ = 0;
    bool send_disconnected_ = false;
    bool recv_disconnected_ = false;
    struct sockaddr_in  inInAddr;
    std::function<void(TCPSocket* s, Nanos rx_time)> recv_callback_;
    std::string time_str_;
};
}