#include "market_order_book.h"
#include "trade_engine.h"
namespace Trading
{
    MarketOrderBook::MarketOrderBook(TickerId ticker_id, Logger *logger)
        : ticker_id_(ticker_id), orders_at_price_pool_(ME_MAX_PRICE_LEVELS), order_pool_(ME_MAX_ORDER_IDS), logger_(logger)
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
    auto MarketOrderBook::setTradeEngine(TradeEngine *trade_engine) -> void
    {
        trade_engine_ = trade_engine;
    }
    auto MarketOrderBook::onMarketUpdate(const Exchange::MEMarketUpdate *market_update) noexcept -> void
    {
        const auto bid_updated = (bids_by_price_ && market_update->side_ == Side::BUY && market_update->price_ >= bids_by_price_->price_);
        const auto ask_updated = (asks_by_price_ && market_update->side_ == Side::SELL && market_update->price_ <= asks_by_price_->price_);
        switch (market_update->type_)
        {
        case Exchange::MarketUpdateType::ADD:
        {
            auto order = order_pool_.allocate(market_update->order_id_, market_update->side_, market_update->price_, market_update->qty_, market_update->priority_, nullptr, nullptr);
            addOrder(order);
        }
        break;
        case Exchange::MarketUpdateType::MODIFY:
        {
            auto order = oid_to_order_.at(market_update->order_id_);
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
            for (auto &order : oid_to_order_)
            {
                if (order)
                    order_pool_.deallocate(order);
            }
            oid_to_order_.fill(nullptr);
            if (bids_by_price_)
            {
                for (auto bid = bids_by_price_->next_entry_; bid != bids_by_price_; bid = bid->next_entry_)
                {
                    orders_at_price_pool_.deallocate(bid);
                }
                orders_at_price_pool_.deallocate(bids_by_price_);
            }
            if (asks_by_price_)
            {
                for (auto ask = asks_by_price_->next_entry_; ask != asks_by_price_; ask = ask->next_entry_)
                {
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
        trade_engine_->onOrderBookUpdate(market_update->ticker_id_, market_update->price_, market_update->side_, this);
        logger_->log("%:% %() % OrderBook\n%\n", __FILE__, __LINE__, __FUNCTION__,
                     thu::getCurrentTimeStr(&time_str_), toString(false, true));
    }
    auto MarketOrderBook::updateBBO(bool update_bid, bool update_ask) noexcept -> void
    {
        if (update_bid)
        {
            if (bids_by_price_)
            {
                bbo_.bid_price_ = bids_by_price_->price_;
                bbo_.bid_qty_ = bids_by_price_->first_mkt_order_->qty_;
                for (auto order = bids_by_price_->first_mkt_order_->next_order_; order != bids_by_price_->first_mkt_order_; order = order->next_order_)
                {
                    bbo_.bid_qty_ += order->qty_;
                }
            }
            else
            {
                bbo_.bid_price_ = Price_INVALID;
                bbo_.bid_qty_ = Qty_INVALID;
            }
        }
        if (update_ask)
        {
            if (asks_by_price_)
            {
                bbo_.ask_price_ = asks_by_price_->price_;
                bbo_.ask_qty_ = asks_by_price_->first_mkt_order_->qty_;
                for (auto order = asks_by_price_->first_mkt_order_->next_order_; order != asks_by_price_->first_mkt_order_; order = order->next_order_)
                {
                    bbo_.ask_qty_ += order->qty_;
                }
            }
            else
            {
                bbo_.ask_price_ = Price_INVALID;
                bbo_.ask_qty_ = Qty_INVALID;
            }
        }
    }

    auto MarketOrderBook::addOrder(MarketOrder *order) noexcept -> void
    {
        const auto orders_at_price = getOrdersAtPrice(order->price_);
        if (!orders_at_price)
        {
            order->next_order_ = order->prev_order_ = order;
            auto new_orders_at_price = orders_at_price_pool_.allocate(order->side_, order->price_, order, nullptr, nullptr);
            addOrdersAtPrice(new_orders_at_price);
        }
        else
        {
            auto first_order = (orders_at_price ? orders_at_price->first_mkt_order_ : nullptr); // copied from git
            first_order->prev_order_->next_order_ = order;
            order->prev_order_ = first_order->prev_order_;
            order->next_order_ = first_order;
            first_order->prev_order_ = order;
        }
        oid_to_order_.at(order->order_id_) = order;
    }

    auto MarketOrderBook::removeOrder(MarketOrder *order) noexcept -> void
    {
        auto orders_at_price = getOrdersAtPrice(order->price_);
        if (order->prev_order_ == order)
        { // only one element
            removeOrdersAtPrice(order->side_, order->price_);
        }
        else
        { // remove the link
            const auto order_before = order->prev_order_;
            const auto order_after = order->next_order_;
            order_before->next_order_ = order_after;
            order_after->prev_order_ = order_before;
            if (orders_at_price->first_mkt_order_ == order)
            {
                orders_at_price->first_mkt_order_ = order_after;
            }
            order->prev_order_ = order->next_order_ = nullptr;
        }
        oid_to_order_.at(order->order_id_) = nullptr;
        order_pool_.deallocate(order);
    }

    auto MarketOrderBook::toString(bool detailed, bool validity_check) const -> std::string
    {
        std::stringstream ss;
        std::string time_str;

        auto printer = [&](std::stringstream &ss, MarketOrdersAtPrice *itr, Side side, Price &last_price,
                           bool sanity_check)
        {
            char buf[4096];
            Qty qty = 0;
            size_t num_orders = 0;

            for (auto o_itr = itr->first_mkt_order_;; o_itr = o_itr->next_order_)
            {
                qty += o_itr->qty_;
                ++num_orders;
                if (o_itr->next_order_ == itr->first_mkt_order_)
                    break;
            }
            sprintf(buf, " <px:%3s p:%3s n:%3s> %-3s @ %-5s(%-4s)",
                    priceToString(itr->price_).c_str(), priceToString(itr->prev_entry_->price_).c_str(),
                    priceToString(itr->next_entry_->price_).c_str(),
                    priceToString(itr->price_).c_str(), qtyToString(qty).c_str(), std::to_string(num_orders).c_str());
            ss << buf;
            for (auto o_itr = itr->first_mkt_order_;; o_itr = o_itr->next_order_)
            {
                if (detailed)
                {
                    sprintf(buf, "[oid:%s q:%s p:%s n:%s] ",
                            orderIdToString(o_itr->order_id_).c_str(), qtyToString(o_itr->qty_).c_str(),
                            orderIdToString(o_itr->prev_order_ ? o_itr->prev_order_->order_id_ : OrderId_INVALID).c_str(),
                            orderIdToString(o_itr->next_order_ ? o_itr->next_order_->order_id_ : OrderId_INVALID).c_str());
                    ss << buf;
                }
                if (o_itr->next_order_ == itr->first_mkt_order_)
                    break;
            }

            ss << std::endl;

            if (sanity_check)
            {
                if ((side == Side::SELL && last_price >= itr->price_) || (side == Side::BUY && last_price <= itr->price_))
                {
                    FATAL("Bids/Asks not sorted by ascending/descending prices last:" + priceToString(last_price) + " itr:" +
                          itr->toString());
                }
                last_price = itr->price_;
            }
        };

        ss << "Ticker:" << TickerIdToString(ticker_id_) << std::endl;
        {
            auto ask_itr = asks_by_price_;
            auto last_ask_price = std::numeric_limits<Price>::min();
            for (size_t count = 0; ask_itr; ++count)
            {
                ss << "ASKS L:" << count << " => ";
                auto next_ask_itr = (ask_itr->next_entry_ == asks_by_price_ ? nullptr : ask_itr->next_entry_);
                printer(ss, ask_itr, Side::SELL, last_ask_price, validity_check);
                ask_itr = next_ask_itr;
            }
        }

        ss << std::endl
           << "                          X" << std::endl
           << std::endl;

        {
            auto bid_itr = bids_by_price_;
            auto last_bid_price = std::numeric_limits<Price>::max();
            for (size_t count = 0; bid_itr; ++count)
            {
                ss << "BIDS L:" << count << " => ";
                auto next_bid_itr = (bid_itr->next_entry_ == bids_by_price_ ? nullptr : bid_itr->next_entry_);
                printer(ss, bid_itr, Side::BUY, last_bid_price, validity_check);
                bid_itr = next_bid_itr;
            }
        }

        return ss.str();
    }
}