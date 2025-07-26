#pragma once
#include "common/macros.h"
#include "common/logging.h"
#include "order_manager.h"
#include "feature_engine.h"
#include "trade_engine.h"

namespace Trading{
class LiquidityTaker{
private:
    const FeatureEngine *feature_engine_ = nullptr;
    OrderManager *order_manager_ = nullptr;
    std::string time_str_;
    Logger *logger_ = nullptr;
    const TradeEngineCfgHashMap ticker_cfg_;
public:
    LiquidityTaker(Logger *logger, TradeEngine *trade_engine, FeatureEngine *feature_engine, OrderManager *order_manager
        , const TradeEngineCfgHashMap &ticker_cfg);
    auto onTradeUpdate(const Exchange::MEMarketUpdate *market_update, MarketOrderBook *book) noexcept->void;

    auto onOrderBookUpdate(TickerId ticker_id, Price price, Side side, MarketOrderBook *) noexcept->void;

    auto onOrderUpdate(const Exchange::MEClientResponse *client_response)noexcept ->void;
};
}