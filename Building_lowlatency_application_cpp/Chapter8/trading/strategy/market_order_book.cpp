#include "market_order_book.h"
#include "trade_engine.h"
namespace Trading{
    MarketOrderBook::MarketOrderBook(TickerId ticker_id, Logger *logger)
        : ticker_id_(ticker_id)
        , orders_at_price_pool_(ME_MAX_PRICE_LEVELS)
        , order_pool_(ME_MAX_ORDER_IDS)
        , logger_(logger)
    {
    }

    MarketOrderBook::~MarketOrderBook()
    {
        logger_->log("%:% %() % OrderBook\n%\n", __FILE__, __LINE__, __FUNCTION__,
                 thu::getCurrentTimeStr(&time_str_), toString(false, true));
        trade_engine_ = nullptr;
        bids_by_price_ = asks_by_price_ = nullptr;
        oid_to_order_.fill(nullptr);
    }
    auto MarketOrder::setTradeEngine(TradeEngine *trade_engine)->void{
        trade_engine_ = trade_engine;
    }
    auto MarketOrderBook::onMarketUpdate(const Exchange::MEMarketUpdate *market_update) noexcept -> void
    {
        const auto bid_updated = (bids_by_price_ && market_update->side_ == Side::BUY && market_update->price_ >= bids_by_price_->price_);
        const auto ask_updated = (asks_by_price_ && market_update->side_ == Side::SELL && market_update->price_ <= asks_by_price_);
        switch (market_update->type_)
        {
        case Exchange::MarketUpdateType::ADD:
            {
                auto order = order_pool_.allocate(market_update->order_id_
                    , market_update->side_
                    , market_update->price_
                    , market_update->qty_
                    , market_update->priority_
                    , nullptr
                    , nullptr);
                addOrder(order);
            }
            break;
        case Exchange::MarketUpdateType::MODIFY:
            {
                auto order = order_pool_.at(market_update->order_id_);
                order->qty_ = market_update->qty_;
            }
            break;
        case Exchange::MarketUpdateType::CANCEL:
            {
                auto order = oid_to_order_.at(market_update->order_id_);
                removeOrder(order);
            }
            break;
        case Exchange::MarketUpdateType::TRADE:
            {
                trade_engine_->onTradeUpdate(market_update, this);
                return;
            }
        case Exchange::MarketUpdateType::CLEAR:
            {
                for(auto &order : oid_to_order_){
                    if(order)
                        order_pool_.deallocate(order);
                }
                oid_to_order_.fill(nullptr);
                if(bids_by_price_){
                    for(auto bid = bids_by_price_->next_entry_; bid != bids_by_price_; bid = bid->next_entry_){
                        orders_at_price_pool_.deallocate(bid);
                    }
                    orders_at_price_pool_.deallocate(bids_by_price_);
                }
                if(asks_by_price_){
                    for(auto asak = asks_by_price_->next_entry_; ask != asks_by_price_; ask = ask->next_entry_){
                        orders_at_price_pool_.deallocate(ask);
                    }
                    orders_at_price_pool_.deallocate(asks_by_price_);
                }
                bids_by_price_ = asks_by_price_ = nullptr;
            }
        case Exchange::MarketUpdateType::INVALID:
        case Exchange::MarketUpdateType::SNAPSHOT_END:
        case Exchange::MarketUpdateType::SNAPSHOT_START:
        default:
            break;
        }

        updateBBO(bid_updated, ask_updated);
        trade_engine_->onOrderBookUpdate(market_update->ticker_id_, market_update->price_, market_update->side_);
        logger_->log("%:% %() % OrderBook\n%\n", __FILE__, __LINE__, __FUNCTION__,
                 thu::getCurrentTimeStr(&time_str_), toString(false, true));
    }
    auto MarketOrderBook::updateBBO(bool update_bid, bool update_ask) noexcept -> void
    {
        if(update_bid){
            if(bids_by_price_){
                bbo_.bid_price_ = bids_by_price_->price_;
                bbo_.bid_qty_ = bids_by_price_->first_mkt_order_->qty_;
                for(auto order = bids_by_price_->first_mkt_order_->next_order_; order != bids_by_price_->first_mkt_order_; order = order->next_order_){
                    bbo_.bid_qty_ += order->qty_;
                }
            }
            else{
                bbo_.bid_price_ = Price_INVALID;
                bbo_.bid_qty_ = Qty_INVALID;
            }
        }
        if(update_ask){
            if(asks_by_price_){
                bbo_.ask_price_ = asks_by_price_->price_;
                bbo_.ask_qty_ = asks_by_price_->first_mkt_order_->qty_;
                for(auto order = asks_by_price_->first_mkt_order_->next_order_; order != asks_by_price_->first_mkt_order_; order = order->next_order_){
                    bbo_.ask_qty_ += order->qty_;
                }
            }
            else{
                bbo_.ask_price_ = Price_INVALID;
                bbo_.ask_qty_ = Qty_INVALID;
            }
        }
    }
}