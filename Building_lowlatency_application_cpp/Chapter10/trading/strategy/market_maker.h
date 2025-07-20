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

    auto onOrderBookUpdate(TickerId ticker_id, Price price, Side side, const MarketOrderBook *book) noexcept->void{
        logger_->log("%:% %() % ticker:% price:% side:%\n",
                     __FILE__, __LINE__, __FUNCTION__, thu::getCurrentTimeStr(&time_str_), ticker_id, priceToString(price).c_str(), sideToString(side).c_str());
        const auto bbo = book->getBBO();
        const auto fair_price = feature_engine_->getMktPrice();
        if(LIKELY(bbo->bid_price_ != Price_INVALID && bbo->ask_price_ != Price_INVALID && fair_price != Feature_INVALID)){
            logger_->log("%:% %() % % fair-price:%\n", __FILE__, __LINE__, __FUNCTION__,
                         thu::getCurrentTimeStr(&time_str_),
                         bbo->toString().c_str(), fair_price);
            const auto clip = ticker_cfg_.at(ticker_id).clip_;
            const auto threshold = ticker_cfg_.at(ticker_id).threshold_;
            const auto bid_price = bbo->bid_price_ - (fair_price - bbo->bid_price_ >= threshold ? 0 : 1);
            const auto ask_price = bbo->ask_price_ - (fair_price - bbo->ask_price_ >= threshold ? 0 : 1);
            order_manager_->moveOrders(ticker_id, bid_price, ask_price, clip);
        }
    }

    auto onTradeUpdate(const Exchange::MEMarketUpdate *market_update, MarketOrderBook *) noexcept->void{
        logger_->log("%:% %() % %\n", __FILE__, __LINE__, __FUNCTION__, thu::getCurrentTimeStr(&time_str_),
                   market_update->toString().c_str());
    }

    auto onOrderUpdate(const Exchange::MEClientResponse *client_response) noexcept->void{
        logger_->log("%:% %() % %\n", __FILE__, __LINE__, __FUNCTION__, thu::getCurrentTimeStr(&time_str_),
                   client_response->toString().c_str());
        order_manager_->onOrderUpdate(client_response);
    }
};
}; // end namespace