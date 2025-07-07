#pragma once
#include <functional>
#include <map>
#include "common/thread_utils.h"
#include "common/lockfree_queue.h"
#include "common/macros.h"
#include "common/mcast_socket.h"
#include "exchange/market_data/market_update.h"
namespace Trading{
class MarketDataConsumer{
private:
    size_t next_exp_inc_seq_num_ = 1;
    Exchange::MEMarketUpdateLFQueue *incoming_md_updates_ = nullptr;
    volatile bool run_ = false;
    std::string time_str_;
    Logger logger_;
    thu::McastSocket incremental_mcast_socket_, snapshot_mcast_socket_;
    bool in_recovery_ = false;
    const std::string iface_, snapshot_ip_;
    const int snapshot_port_;
    typedef std::map<size_t, Exchange::MEMarketUpdate> QueuedMarketUpdates;
    QueuedMarketUpdates snapshot_queued_msgs_, incremental_queued_msgs_;
public:
    MarketDataConsumer(thu::ClientId client_id, Exchange::MEMarketUpdateLFQueue *market_updates, const std::string &iface, const std::string &snapshot_ip
                        , int snapshot_port, const std::string &incremental_ip, int incremental_port);
    ~MarketDataConsumer();
    auto start()->void;
    auto stop()->void;
    auto run() noexcept -> void;
    auto recvCallback(thu::McastSocket *socket) noexcept->void;
    auto startSnapshotSync() -> void;
};


}// end namespace