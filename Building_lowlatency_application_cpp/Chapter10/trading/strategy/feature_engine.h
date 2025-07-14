#pragma once
#include "common/macros.h"
#include "common/logging.h"
#include "market_order_book.h"
using namespace thu;
namespace Trading{
const auto Feature_INVALID = std::numeric_limits<double>::quiet_NaN();
class FeatureEngine{
private:
    std::string time_str_;
    Logger *logger_ = nullptr;
    double mkt_price_ = Feature_INVALID, agg_trade_qty_ratio_ = Feature_INVALID;
public:
    FeatureEngine(Logger *logger) : logger_(logger)
    {}
    auto getMktPrice() const noexcept{
        return mkt_price_;
    }
    
    auto getAggTradeQtyRatio() const noexcept{
        return agg_trade_qty_ratio_;
    }
    
    auto onOrderBookUpdate(TickerId ticker_id, Price price, Side side, MarketOrderBook *book) noexcept->void{
        const auto bbo = book->getBBO();
        if(LIKELY(bbo->bid_price_ != Price_INVALID && bbo->ask_price_ != Price_INVALID)){
            mkt_price_ = (bbo->bid_price_ * bbo->ask_qty_ + bbo->ask_price_ * bbo->bid_qty_) / static_cast<double>(bbo->bid_qty_ + bbo->ask_qty_);
        }
        logger_->log("%:% %() % ticker:% price:% side:% mkt-price:% agg-trade-ratio:%\n", __FILE__, __LINE__,
          __FUNCTION__, getCurrentTimeStr(&time_str_),
                     ticker_id, priceToString(price).c_str(),
                   sideToString(side).c_str(), mkt_price_, agg_trade_qty_ratio_);        
    }

    auto onTradeUpdate(const Exchange::MEMarketUpdate *market_update, MarketOrderBook *book) noexcept->void{
        const auto bbo = book->getBBO();
        if(LIKELY(bbo->bid_price_ != Price_INVALID && bbo->ask_price_ != Price_INVALID)){
            agg_trade_qty_ratio_ = static_cast<double>(market_update->qty_) / (market_update->side_ == Side::BUY ? bbo->ask_qty_ : bbo->bid_qty_);
        }
        logger_->log("%:% %() % % mkt-price:% agg-trade-ratio:%\n", __FILE__, __LINE__, __FUNCTION__,
                   thu::getCurrentTimeStr(&time_str_),
                   market_update->toString().c_str(),
                     mkt_price_, agg_trade_qty_ratio_);
    }
};
} // end namespace