#pragma once
#include "common/types.h"
#include "common/thread_utils.h"
#include "common/lockfree_queue.h"
#include "common/macros.h"
#include "common/mcast_socket.h"
#include "common/memory_pool.h"
#include "common/logging.h"
#include "market_update.h"
#include "me_order.h"
using namespace thu;

namespace Exchange{
class SnapshotSynthesizer{
private:
    MDPMarketUpdateLFQueue *snapshot_md_updates_ = nullptr;
    Logger logger_;
    volatile bool run_ = false;
    std::string time_str_;
    McastSocket snapshot_socket_;
    std::array<std::array<MEMarketUpdate *, ME_MAX_ORDER_IDS>, ME_MAX_TICKETS> ticker_orders_;
    size_t last_inc_seq_num_ = 0;
    Nanos last_snapshot_time_ = 0;
    MemPool<MEMarketUpdate> order_pool_;
public:
    SnapshotSynthesizer(MDPMarketUpdateLFQueue *market_updates, const std::string &iface, const std::string &snapshot_ip, int snapshot_port);
    ~SnapshotSynthesizer();
    auto start() -> void;
    auto stop() -> void;
    auto run() -> void;
    auto addToSnapshot(const MDPMarketUpdate *market_update) -> void;
    auto publishSnapshot() -> void;
};

}// end namespace