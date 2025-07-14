#pragma once
#include "common/macros.h"
#include "common/logging.h"
#include "order_manager.h"
#include "common/types.h"
#include "feature_engine.h"
using namespace thu;

namespace Trading{
class MarketMaker{
private:
    const FeatureEngine *feature_engine_ = nullptr;
    OrderManager *order_manager_ = nullptr;
    std::string time_str_;
    Logger *logger_ = nullptr;
    const TradeEngineCfgHashMap ticker_cfg_;
public:
    MarketMaker(Logger *logger, TradeEngine *trade_engine, const FeatureEngine *feature_engine
        , OrderManager *order_manager, const TradeEngineCfgHashMap &ticker_cfg)
        : logger_(logger), feature_engine_(feature_engine), order_manager_(order_manager), ticker_cfg_(ticker_cfg)
    {
        trade_engine->algoOnOrderBookUpdate_ = [this](auto ticker_id, auto price, auto side, auto book){
            onOrderBookUpdate(ticker_id, price, side, book);
        };
        trade_engine->algoOnTradeUpdate_ = [this](auto market_update, auto book){
            onTradeUpdate(market_update, book);
        };
        trade_engine->algoOnOrderUpdate_ = [this](auto client_response){
            onOrderUpdate(client_response);
        };
    }
};
}; // end namespace