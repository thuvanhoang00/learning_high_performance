#pragma once
#include <functional>
#include "common/thread_utils.h"
#include "common/macros.h"
#include "common/tcp_server.h"
#include "exchange/order_server/client_request.h"
#include "exchange/order_server/client_response.h"
#include "common/types.h"

namespace Trading{
class OrderGateway{
private:
    const ClientId client_id_;
    std::string ip_;
    const std::string iface_;
    const int port_ = 0;
    Exchange::ClientRequestLFQueue *outgoing_requests_ = nullptr;
    Exchange::ClientResponseLFQueue *incoming_responses_ = nullptr;
    volatile bool run_ = false;
    std::string time_str_;
    Logger logger_;
    size_t next_outgoing_seq_num_ = 1;
    size_t next_exp_seq_num_ = 1;
    thu::TCPSocket tcp_socket_;
public:
    OrderGateway(ClientId client_id
        , Exchange::ClientRequestLFQueue *client_requests
        , Exchange::ClientResponseLFQueue *client_responses
        , std::string ip
        , const std::string &iface
        , int port);
    ~OrderGateway();
    auto start() -> void;
    auto stop() -> void;
    auto run() -> void;
    auto recvCallback(TCPSocket *socket, Nanos rx_time) noexcept -> void;
};
}// end namespace