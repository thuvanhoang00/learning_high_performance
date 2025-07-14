#pragma once
#include <functional>

#include "common/types.h"
#include "common/thread_utils.h"
#include "common/lockfree_queue.h"
#include "macros.h"
#include "logging.h"
#include "time_utils.h"

#include "exchange/order_server/client_request.h"
#include "exchange/order_server/client_response.h"
#include "exchange/market_data/market_update.h"

#include "market_order_book.h"
#include "feature_engine.h"
#include "position_keeper.h"
#include "order_manager.h"
#include "risk_manager.h"

#include "market_maker.h"
#include "liquidity_taker.h"

namespace Trading{
using namespace thu;
class TradeEngine{
private:
    ClientId client_id_;
    MarketOrderBookHashMap ticker_order_book_;
    Exchange::ClientRequestLFQueue *outgoing_ogw_requests_ = nullptr;
    Exchange::ClientResponseLFQueue *incoming_ogw_responses_ = nullptr;
    Exchange::MEMarketUpdateLFQueue *incoming_md_updates_ = nullptr;
    
    Nanos last_event_time_ = 0;
    volatile bool run_ = false;

    std::string time_str_;
    Logger logger_;

    FeatureEngine feature_engine_;
    PositionKeeper position_keeper_;
    OrderManager order_manager_;
    RiskManager risk_manager_;

    MarketMaker *mm_algo_ = nullptr;
    LiquidityTaker *taker_algo_ = nullptr;
public:
    TradeEngine(ClientId client_id, AlgoType type, const TradeEngineCfgHashMap &ticker_cfg
                , Exchange::ClientRequestLFQueue *client_requests
                , Exchange::ClientResponseLFQueue *client_responses
                , Exchange::MEMarketUpdateLFQueue *market_updates);
    ~TradeEngine();

    auto start()->void{
        run_ = true;
        ASSERT(createAndStartThread(-1, "Trading/Engine", [this](){run();}) != nullptr, "Failed to start TradeEngine thread.");
    }

    auto stop()->void{
        while(incoming_ogw_responses_->size() || incoming_md_updates_->size()){
            logger_.log("%:% %() % Sleeping till all updates are consumed ogw-size:% md-size:%\n", __FILE__, __LINE__, __FUNCTION__,
                        thu::getCurrentTimeStr(&time_str_), incoming_ogw_responses_->size(), incoming_md_updates_->size());

            using namespace std::literals::chrono_literals;
            std::this_thread::sleep_for(10ms);
        }
        logger_.log("%:% %() % POSITIONS\n%\n", __FILE__, __LINE__, __FUNCTION__, thu::getCurrentTimeStr(&time_str_),
                    position_keeper_.toString());

        run_ = false;
    }

    auto run() noexcept->void;


    std::function<void(TickerId ticker_id, Price price, Side side, MarketOrderBook *book)> algoOnOrderBookUpdate_;
    std::function<void(const Exchange::MEMarketUpdate *market_update, MarketOrderBook *book)> algoOnTradeUpdate_;
    std::function<void(const Exchange::MEClientResponse *client_response)> algoOnOrderUpdate_;

    auto clientId() const{
        return client_id_;
    }
};
}// end namespace