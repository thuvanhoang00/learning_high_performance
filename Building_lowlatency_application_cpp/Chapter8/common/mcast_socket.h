#pragma once
#include <functional>
#include "socket_utils.h"
#include "logging.h"
namespace thu{
    constexpr size_t McastBufferSize = 64 * 1024 * 1024;
    struct McastSocket{
        McastSocket(Logger &logger) : logger_(logger){
            outbound_data_.resize(McastBufferSize);
            inbound_data_.resize(McastBufferSize);
        }

        // initialize multicast socket to read from or publish to a stream
        // does not join the multicast stream yet
        auto init(const std::string &ip, const std::string &iface, int port, bool is_listening) -> int;
        
        // add/join membership/subscription to a multicast stream
        auto join(const std::string &ip) -> bool;

        // remove/leave membership/subscription to a multicast stream
        auto leave(const std::string &ip, int port)->void;

        // publish outgoing data and read incoming data
        auto sendAndRecv() noexcept -> bool;

        // copy data to send buffers - does not send them out yet
        auto send(const void *data, size_t len) noexcept->void;

        int socket_fd_ = -1;
        
        // send and receive buffers, typically only one or the other is needed, not both
        std::vector<char> outbound_data_;
        size_t next_send_valid_index_ = 0;
        std::vector<char> inbound_data_;
        size_t next_rcv_valid_index_ = 0;

        // function wrapper for the method to call when data is read
        std::function<void(McastSocket *)> recv_callback_ = nullptr;

        std::string time_str_;
        Logger &logger_;
    };
}