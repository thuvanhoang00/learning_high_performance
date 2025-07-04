#pragma once
#include <functional>
#include "market_data/snapshot_synthesizer.h"
#include "market_update.h"
#include "thread_utils.h"

namespace Exchange{
class MarketDataPublisher{
private:
    size_t next_inc_seq_num_ = 1;
    MEMarketUpdateLFQueue *outgoing_md_updates_ = nullptr;
    MDPMarketUpdateLFQueue snapshot_md_updates_;
    volatile bool run_ = false;
    std::string time_str_;
    Logger logger_;
    thu::McastSocket incremental_socket_;
    SnapshotSynthesizer *snapshot_synthesizer_ = nullptr;
public:
    MarketDataPublisher(MEMarketUpdateLFQueue *market_updates, const std::string &iface, const std::string &snapshot_ip, int snapshot_port
                        , const std::string &incremental_ip, int incremental_port)
                        : outgoing_md_updates_(market_updates), snapshot_md_updates_(ME_MAX_MARKET_UPDATES)
                        , run_(false)
                        , logger_("exchange_market_data_publisher.log")
                        , incremental_socket_(logger_)
                        {
                            ASSERT(incremental_socket_.init(incremental_ip, iface, incremental_port, /*is_listening*/false) >=0, 
                                    "Unable to create incremental mcast socket. error:"+std::string(std::strerror(errno)));
                        
                            snapshot_synthesizer_ = new SnapshotSynthesizer(&snapshot_md_updates_, snapshot_ip, snapshot_port);
                        }
    auto start(){
        run_ = true;
        ASSERT(createAndStartThread(-1, "MarketDataPublisher", [this](){run();}) != nullptr, "Failed to start MarketData thread.");
        snapshot_synthesizer_->start();
    }
    auto stop(){
        run_ = false;
        snapshot_synthesizer_->stop();
    }
    auto run() noexcept -> void;

    ~MarketDataPublisher(){
        stop();
        using namespace std::literals::chrono_literals;
        std::this_thread::sleep_for(5s);
        delete snapshot_synthesizer_;
        snapshot_synthesizer_ = nullptr;
    }
};
}