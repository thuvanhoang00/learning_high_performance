#pragma once
#include "common/types.h"
#include "common/memory_pool.h"
#include "common/logging.h"
#include "market_order.h"
#include "exchange/market_data/market_update.h"
namespace Trading{
class TradeEngine;
class MarketOrderBook final{
private:
    const TickerId ticker_id_;
    TradeEngine *trade_engine_ = nullptr;
    OrderHashMap oid_to_order_;
    MemPool<MarketOrdersAtPrice> orders_at_price_pool_;
    MarketOrdersAtPrice *bids_by_price_ = nullptr;
    MarketOrdersAtPrice *asks_by_price_ = nullptr;
    OrdersAtPriceHashMap price_orders_at_price_;
    MemPool<MarketOrder> order_pool_;
    BBO bbo_;
    std::string time_str_;
    Logger *logger_;
public:
    MarketOrderBook(TickerId ticker_id, Logger *logger);
    ~MarketOrderBook();
    auto setTradeEngine(TradeEngine *trade_engine)->void;
    auto onMarketUpdate(const Exchange::MEMarketUpdate *market_update) noexcept ->void;
    auto updateBBO(bool update_bid, bool update_ask) noexcept ->void;
    auto priceToIndex(Price price) const noexcept{
        return (price % ME_MAX_PRICE_LEVELS);
    }
    auto getOrdersAtPrice(Price price) const noexcept->MarketOrdersAtPrice*{
        return price_orders_at_price_.at(priceToIndex(price));
    }
    auto addOrder(MarketOrder *order) noexcept ->void;
    auto removeOrder(MarketOrder *order) noexcept->void;
    auto addOrdersAtPrice(MarketOrdersAtPrice *new_orders_at_price) noexcept {
      price_orders_at_price_.at(priceToIndex(new_orders_at_price->price_)) = new_orders_at_price;

      const auto best_orders_by_price = (new_orders_at_price->side_ == Side::BUY ? bids_by_price_ : asks_by_price_);
      if (UNLIKELY(!best_orders_by_price)) {
        (new_orders_at_price->side_ == Side::BUY ? bids_by_price_ : asks_by_price_) = new_orders_at_price;
        new_orders_at_price->prev_entry_ = new_orders_at_price->next_entry_ = new_orders_at_price;
      } else {
        auto target = best_orders_by_price;
        bool add_after = ((new_orders_at_price->side_ == Side::SELL && new_orders_at_price->price_ > target->price_) ||
                          (new_orders_at_price->side_ == Side::BUY && new_orders_at_price->price_ < target->price_));
        if (add_after) {
          target = target->next_entry_;
          add_after = ((new_orders_at_price->side_ == Side::SELL && new_orders_at_price->price_ > target->price_) ||
                       (new_orders_at_price->side_ == Side::BUY && new_orders_at_price->price_ < target->price_));
        }
        while (add_after && target != best_orders_by_price) {
          add_after = ((new_orders_at_price->side_ == Side::SELL && new_orders_at_price->price_ > target->price_) ||
                       (new_orders_at_price->side_ == Side::BUY && new_orders_at_price->price_ < target->price_));
          if (add_after)
            target = target->next_entry_;
        }

        if (add_after) { // add new_orders_at_price after target.
          if (target == best_orders_by_price) {
            target = best_orders_by_price->prev_entry_;
          }
          new_orders_at_price->prev_entry_ = target;
          target->next_entry_->prev_entry_ = new_orders_at_price;
          new_orders_at_price->next_entry_ = target->next_entry_;
          target->next_entry_ = new_orders_at_price;
        } else { // add new_orders_at_price before target.
          new_orders_at_price->prev_entry_ = target->prev_entry_;
          new_orders_at_price->next_entry_ = target;
          target->prev_entry_->next_entry_ = new_orders_at_price;
          target->prev_entry_ = new_orders_at_price;

          if ((new_orders_at_price->side_ == Side::BUY && new_orders_at_price->price_ > best_orders_by_price->price_) ||
              (new_orders_at_price->side_ == Side::SELL &&
               new_orders_at_price->price_ < best_orders_by_price->price_)) {
            target->next_entry_ = (target->next_entry_ == best_orders_by_price ? new_orders_at_price
                                                                               : target->next_entry_);
            (new_orders_at_price->side_ == Side::BUY ? bids_by_price_ : asks_by_price_) = new_orders_at_price;
          }
        }
      }
    }

    /// Remove the MarketOrdersAtPrice from the containers - the hash map and the doubly linked list of price levels.
    auto removeOrdersAtPrice(Side side, Price price) noexcept {
      const auto best_orders_by_price = (side == Side::BUY ? bids_by_price_ : asks_by_price_);
      auto orders_at_price = getOrdersAtPrice(price);

      if (UNLIKELY(orders_at_price->next_entry_ == orders_at_price)) { // empty side of book.
        (side == Side::BUY ? bids_by_price_ : asks_by_price_) = nullptr;
      } else {
        orders_at_price->prev_entry_->next_entry_ = orders_at_price->next_entry_;
        orders_at_price->next_entry_->prev_entry_ = orders_at_price->prev_entry_;

        if (orders_at_price == best_orders_by_price) {
          (side == Side::BUY ? bids_by_price_ : asks_by_price_) = orders_at_price->next_entry_;
        }

        orders_at_price->prev_entry_ = orders_at_price->next_entry_ = nullptr;
      }

      price_orders_at_price_.at(priceToIndex(price)) = nullptr;

      orders_at_price_pool_.deallocate(orders_at_price);
    }
    
    auto getBBO() const noexcept->const BBO*{
        return &bbo_;
    }
    auto toString(bool detailed, bool validity_check) const ->std::string;
};

typedef std::array<MarketOrderBook *, ME_MAX_TICKERS> MarketOrderBookHashMap;
}// end namespace