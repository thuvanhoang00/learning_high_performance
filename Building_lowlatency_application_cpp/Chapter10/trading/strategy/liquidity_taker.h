#pragma once
#include "common/macros.h"
#include "common/logging.h"
#include "order_manager.h"
#include "feature_engine.h"

namespace Trading{
class LiquidityTaker{
private:
    const FeatureEngine *feature_engine_ = nullptr;
    OrderManager *order_manager_ = nullptr;
    std::string time_str_;
    Logger *logger_ = nullptr;
    const TradeEngineCfgHashMap ticker_cfg_;
public:
    LiquidityTaker(Logger *logger, TradeEngine *trade_engine, FeatureEngine *feature_engine, OrderManger *order_manager
        , const TradeEngineCfgHashMap &ticker_cfg);
    auto onTradeUpdate(const Exchange::MEMarketUpdate *market_update, MarketOrderBook *book) noexcept->void{
        logger_->log("%:% %() % %\n", __FILE__, __LINE__, __FUNCTION__, thu::getCurrentTimeStr(&time_str_),
                   market_update->toString().c_str());
        const auto bbo = book->getBBO();
        const auto agg_qty_ratio = feature_engine_->getAggTradeQtyRatio();
        if(LIKELY(bbo->bid_price_ != Price_INVALID && bbo->ask_price_ != Price_INVALID && agg_qty_ratio != Feature_INVALID)){
            logger_->log("%:% %() % % agg-qty-ratio:%\n", __FILE__, __LINE__, __FUNCTION__,
                     thu::getCurrentTimeStr(&time_str_),
                     bbo->toString().c_str(), agg_qty_ratio);
            const auto clip = ticker_cfg_.at(market_update->ticker_id_).clip_;
            const auto threshold = ticker_cfg_.at(market_update->ticker_id_).threshold_;
            if(agg_qty_ratio >= threshold){
                if(market_update->side_ == Side::BUY){
                    order_manager_->moveOrders(market_update->ticker_id_, bbo->ask_price_, Price_INVALID, clip);
                }
                else{
                    order_manager_->moveOrders(market_update->ticker_id_, Price_INVALID, bbo->bid_price_, clip);
                }
            }
        }
    }

    auto onOrderBookUpdate(TickerId ticker_id, Price price, Side side, MarketOrderBook *) noexcept->void{
        logger_->log("%:% %() % ticker:% price:% side:%\n",
                     __FILE__, __LINE__, __FUNCTION__,
                     thu::getCurrentTimeStr(&time_str_),
                     ticker_id, thu::priceToString(price).c_str(),
                     thu::sideToString(side).c_str());
    }

    auto onOrderUpdate(const Exchange::MEClientResponse *client_response)noexcept ->void{
        logger_->log("%:% %() % %\n", __FILE__, __LINE__,
        __FUNCTION__, thu::getCurrentTimeStr(&time_str_),
                   client_response->toString().c_str());
        order_manager_->onOrderUpdate(client_response);
    }
};
};