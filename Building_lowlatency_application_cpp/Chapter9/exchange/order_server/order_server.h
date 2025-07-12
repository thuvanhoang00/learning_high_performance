#pragma once
#include <functional>
#include "thread_utils.h"
#include "macros.h"
#include "tcp_server.h"
#include "client_request.h"
#include "client_response.h"
#include "fifo_sequencer.h"

namespace Exchange{
class OrderServer{
private:
    const std::string iface_;
    const int port_ = 0;
    ClientResponseLFQueue *outgoing_responses_ = nullptr;
    volatile bool run_ = false;
    std::string time_str_;
    Logger logger_;
    std::array<size_t, ME_MAX_NUM_CLIENTS> cid_next_outgoing_seq_num_;
    std::array<size_t, ME_MAX_NUM_CLIENTS> cid_next_exp_seq_num_;
    std::array<thu::TCPSocket *, ME_MAX_NUM_CLIENTS> cid_tcp_socket_;
    thu::TCPServer tcp_server_;
    FIFOSequencer fifo_sequencer_;

public:
    OrderServer(ClientRequestLFQueue *client_requests, ClientResponseLFQueue *client_responses, const std::string &iface, int port);
    ~OrderServer();
    auto start() -> void;
    auto stop() -> void;
    auto recvCallback(TCPSocket *socket, Nanos rx_time) noexcept -> void;
    auto recvFinishedCallback() noexcept -> void;
    auto run() -> void;
};
}