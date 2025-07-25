#include "order_gateway.h"
namespace Trading{
    OrderGateway::OrderGateway(ClientId client_id, Exchange::ClientRequestLFQueue *client_requests, Exchange::ClientResponseLFQueue *client_responses, std::string ip, const std::string &iface, int port)
        : client_id_(client_id)
        , ip_(ip)
        , iface_(iface)
        , port_(port)
        , outgoing_requests_(client_requests)
        , incoming_responses_(client_responses)
        , logger_("trading_order_gateway_"+std::to_string(client_id) +".log")
        , tcp_socket_(logger_)
    {
        tcp_socket_.recv_callback_ = [this](auto socket, auto rx_time){
            recvCallback(socket, rx_time);
        };
    }

    OrderGateway::~OrderGateway()
    {
        stop();
        using namespace std::literals::chrono_literals;
        std::this_thread::sleep_for(5s);
    }

    auto OrderGateway::start()->void{
        run_ = true;
        ASSERT(tcp_socket_.connect(ip_, iface_, port_, false) >= 0, "Unable to connect to ip:" + ip_ + " port:" +
                                                                        std::to_string(port_) + " on iface:" +
                                                                        iface_ + " error:" +
                                                                        std::string(std::strerror(errno)));
        ASSERT(createAndStartThread(-1, "Trading/OrderGateway", [this](){run();}) != nullptr, "Failed to start OrderGateway thread.");
    }
    auto OrderGateway::stop()->void{
        run_ = false;
    }
    auto OrderGateway::run()->void{
        logger_.log("%:% %() %\n", __FILE__, __LINE__,
                    __FUNCTION__, thu::getCurrentTimeStr(&time_str_));
        while (run_)
        {
            tcp_socket_.sendAndRecv();
            for(auto client_request = outgoing_requests_->getNextToRead(); client_request; client_request = outgoing_requests_->getNextToRead()){
                logger_.log("%:% %() % Sending cid:% seq:% %\n",
                            __FILE__, __LINE__, __FUNCTION__,
                            thu::getCurrentTimeStr(&time_str_),
                            client_id_, next_outgoing_seq_num_,
                            client_request->toString());
                tcp_socket_.send(&next_outgoing_seq_num_, sizeof(next_outgoing_seq_num_));
                tcp_socket_.send(client_request, sizeof(Exchange::MEClientRequest));
                outgoing_requests_->updateReadIndex();
                next_outgoing_seq_num_++;
            }
        }        
    }

    auto OrderGateway::recvCallback(TCPSocket *socket, Nanos rx_time) noexcept -> void{
        logger_.log("%:% %() % Received socket:% len:% %\n",
                    __FILE__, __LINE__, __FUNCTION__,
                    thu::getCurrentTimeStr(&time_str_), socket->fd_,
                    socket->next_rcv_valid_index_, rx_time);
        if(socket->next_rcv_valid_index_ >= sizeof(Exchange::OMClientResponse)){
            size_t i = 0;
            for(; i + sizeof(Exchange::OMClientResponse) <= socket->next_rcv_valid_index_; i+= sizeof(Exchange::OMClientResponse)){
                auto response = reinterpret_cast<const Exchange::OMClientResponse*>(socket->rcv_buffer_ + i);
                logger_.log("%:% %() % Received %\n", __FILE__,
                            __LINE__, __FUNCTION__,
                            thu::getCurrentTimeStr(&time_str_), response->toString());
                if(response->me_client_response_.client_id_ != client_id_){
                    logger_.log("%:% %() % ERROR Incorrect client id. ClientId expected:% received:%.\n", __FILE__,
                                __LINE__, __FUNCTION__,
                                thu::getCurrentTimeStr(&time_str_), client_id_, response->me_client_response_.client_id_);
                    continue;
                }
                if(response->seq_num_ != next_exp_seq_num_){
                    logger_.log("%:% %() % ERROR Incorrect sequence number. ClientId:%. SeqNum expected:% received:%.\n", __FILE__, __LINE__,
                                __FUNCTION__, thu::getCurrentTimeStr(&time_str_), client_id_, next_exp_seq_num_, response->seq_num_);
                    continue;
                }
                ++next_exp_seq_num_;
                auto next_write = incoming_responses_->getNextToWriteTo();
                *next_write = std::move(response->me_client_response_);
                incoming_responses_->updateWriteIndex();
            }
            memcpy(socket->rcv_buffer_, socket->rcv_buffer_ + i, socket->next_rcv_valid_index_-i);
            socket->next_rcv_valid_index_ -= i;
        }
    }
}
