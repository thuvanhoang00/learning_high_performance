#pragma once
#include "common/types.h"
#include "common/memory_pool.h"
#include "common/logging.h"
#include "order_server/client_response.h"
#include "market_data/market_update.h"
#include "me_order.h"
using namespace thu;
namespace Exchange{
class MatchingEngine;
class MEOrderBook final{
private:
    TickerId ticker_id_ = TickerId_INVALID;
    MatchingEngine *matching_engine_ = nullptr;
    ClientOrderHashMap cid_oid_to_order_;
    MemPool<MEOrdersAtPrice> orders_at_price_pool_;
    MEOrdersAtPrice *bids_by_price_ = nullptr;
    MEOrdersAtPrice *asks_by_price_ = nullptr;
    OrdersAtPriceHashMap price_orders_at_prices_;
    MemPool<MEOrder> order_pool_;
    MEClientResponse client_response_;
    MEMarketUpdate market_update_;
    OrderId next_market_order_id_ = 1;
    std::string time_str_;
    Logger *logger_ = nullptr;
public:
    MEOrderBook(TickerId ticker_id, MatchingEngine *matching_engine, Logger *logger);
    ~MEOrderBook();

    MEOrderBook() = delete;
    MEOrderBook(const MEOrderBook&) = delete;
    MEOrderBook(MEOrderBook&&) = delete;
    MEOrderBook& operator=(const MEOrderBook&) = delete;
    MEOrderBook& operator=(MEOrderBook&&) = delete;
private:
    auto generateNewMarketOrderId() noexcept->OrderId{
        return next_market_order_id_;
    }
    auto priceToIndex(Price price) const noexcept{
        return (price % ME_MAX_PRICE_LEVELS);
    }
    auto getOrdersAtPrice(Price price) const noexcept->MEOrdersAtPrice*{
        return price_orders_at_prices_.at(priceToIndex(price));
    }
public:
    auto add(ClientId client_id, OrderId client_order_id, TickerId ticker_id, Side side, Price price, Qty qty) noexcept -> void;
    auto getNextPriority(Price price) noexcept {
        const auto orders_at_price = getOrdersAtPrice(price);
        if(!orders_at_price){
            return 1lu;
        }
        return orders_at_price->first_me_order_->prev_order_->priority_+1;
    }
    auto addOrder(MEOrder *order) noexcept ->void;
    auto addOrdersAtPrice(MEOrdersAtPrice *new_orders_at_price) noexcept->void;
    auto cancel(ClientId client_id, OrderId order_id, TickerId ticker_id) noexcept -> void;
    auto removeOrder(MEOrder *order) noexcept->void;
    auto removeOrdersAtPrice(Side side, Price price) noexcept -> void;
    auto checkForMatch(ClientId client_id, OrderId client_order_id, TickerId ticker_id, Side side, Price price, Qty qty, Qty new_market_order_id) noexcept -> Qty;
    auto match(TickerId ticker_id, ClientId client_id, Side side, OrderId client_order_id, OrderId new_market_order_id, MEOrder *itr, Qty *leaves_qty) noexcept -> void;
    auto toString(bool detailed, bool validity_check) const -> std::string;
};

typedef std::array<MEOrderBook *, ME_MAX_TICKETS> OrderBookHashMap;
}