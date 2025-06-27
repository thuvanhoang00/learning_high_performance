#include "me_order_book.h"
#include "matching_engine.h"
namespace Exchange{
MEOrderBook::MEOrderBook(TicketId ticker_id, Logger *logger, MatchingEngine *matching_engine)
: ticker_id_(ticker_id), logger_(logger), matching_engine_(matching_engine), orders_at_price_pool_(ME_MAX_PRICE_LEVELS), order_pool_(ME_MAX_ORDER_IDS)
{

}

MEOrderBook::~MEOrderBook()
{
    logger_->log("%:% %() % MEOrderBook\n%\n",
                    __FILE__, __LINE__, __FUNCTION__, thu::getCurrentTimeStr(&time_str_), toString(false, true));
    matching_engine_ = nullptr;
    bids_by_price_ = asks_by_price_ = nullptr;
    for(auto &itr : cid_oid_to_order_){
        itr.fill(nullptr);
    }
}
auto MEOrderBook::add(ClientId client_id, OrderId client_order_id, TicketId ticker_id, Side side, Price price, Qty qty) noexcept -> void
{
    const auto new_market_order_id = generateNewMarketOrderId();
    client_response_ = {ClientResponseType::ACCEPTED, client_id, ticker_id, client_order_id, new_market_order_id, side, price, 0, qty};
    matching_engine_->sendClientResponse(&client_response_);
    const auto leaves_qty = checkForMatch(client_id, client_order_id, ticker_id, side, price, qty, new_market_order_id);
    if(LIKELY(leaves_qty)){
        const auto priority = getNextPriority(price);
        auto order = order_pool_.allocate(ticker_id, client_id, client_order_id, new_market_order_id, side, price, leaves_qty, priority, nullptr, nullptr);
        addOrder(order);
        market_update_ = {MarketUpdateType::ADD, new_market_order_id, ticker_id, side, price, leaves_qty, priority};
        matching_engine_->sendMarketUpdate(&market_update_);
    }
}
auto MEOrderBook::addOrder(MEOrder *order) noexcept -> void
{
    const auto orders_at_price = getOrdersAtPrice(order->price_);
    if(!orders_at_price){
        order->next_order_ = order->prev_order_ = order;
        auto new_orders_at_price = orders_at_price_pool_.allocate(order->side_, order->price_, order, nullptr, nullptr);
        addOrdersAtPrice(new_orders_at_price);
    }
    else{
        auto first_order = (orders_at_price ? orders_at_price->first_me_order_ : nullptr);
        first_order->prev_order_->next_order_ = order;
        order->prev_order_ = first_order->prev_order_;
        order->next_order_ = first_order;
        first_order->prev_order_ = order;
    }
    cid_oid_to_order_.at(order->client_id_).at(order->client_order_id_) = order;
}
auto MEOrderBook::addOrdersAtPrice(MEOrdersAtPrice *new_orders_at_price) noexcept -> void
{
    price_orders_at_prices_.at(priceToIndex(new_orders_at_price->price_)) = new_orders_at_price;
    const auto best_orders_by_price = (new_orders_at_price->side_ == Side::BUY ? bids_by_price_ : asks_by_price_);
    if(UNLIKELY(!best_orders_by_price)){
        (new_orders_at_price->side_ == Side::BUY ? bids_by_price_ : asks_by_price_) = new_orders_at_price;
        new_orders_at_price->prev_entry_ = new_orders_at_price->next_entry_ = new_orders_at_price;
    }
    else{
        auto target = best_orders_by_price;
        bool add_after = ((new_orders_at_price->side_ == Side::SELL && new_orders_at_price->price_ > target->price_) || (new_orders_at_price->price_ < target->price_));
        if(add_after){
            target = target->next_entry_;
            add_after = ((new_orders_at_price->side_ == Side::SELL && new_orders_at_price->price_ > target->price_) || (new_orders_at_price->side_ == Side::BUY && new_orders_at_price->price_ < target->price_));
        }
        while (add_after && target != best_orders_by_price)
        {
            add_after = ((new_orders_at_price->side_ == Side::SELL && new_orders_at_price->price_ > target->price_) || (new_orders_at_price->side_ == Side::BUY && new_orders_at_price->price_ < target->price_));
            if(add_after){
                target = target->next_entry_;
            }
        }
        if(add_after){ // add new_orders_at_price after target
            if(target == best_orders_by_price){
                target = best_orders_by_price->prev_entry_;
            }
            new_orders_at_price->prev_entry_ = target;
            target->next_entry_->prev_entry_ = new_orders_at_price;
            new_orders_at_price->next_entry_ = target->next_entry_;
            target->next_entry_ = new_orders_at_price;
        }
        else{ // add new orders at price before target
            new_orders_at_price->prev_entry_ = target->prev_entry_;
            new_orders_at_price->next_entry_ = target;
            target->prev_entry_->next_entry_ = new_orders_at_price;
            target->prev_entry_ = new_orders_at_price;
            if ((new_orders_at_price->side_ == Side::BUY && new_orders_at_price->price_ > best_orders_by_price->price_) || (new_orders_at_price->side_ == Side::SELL && new_orders_at_price->price_ < best_orders_by_price->price_))
            {
                target->next_entry_ = (target->next_entry_ == best_orders_by_price ? new_orders_at_price : target->next_entry_);
                (new_orders_at_price->side_ == Side::BUY ? bids_by_price_ : asks_by_price_) = new_orders_at_price;
            }
        }
    }
}
auto MEOrderBook::cancel(ClientId client_id, OrderId order_id, TicketId ticker_id) noexcept -> void
{
    auto is_cancelable = (client_id < cid_oid_to_order_.size());
    MEOrder *exchange_order = nullptr;
    if(LIKELY(is_cancelable)){
        auto &co_itr = cid_oid_to_order_.at(client_id);
        exchange_order = co_itr.at(order_id);
        is_cancelable = (exchange_order != nullptr);
    }
    if(UNLIKELY(!is_cancelable)){
        client_response_ = {ClientResponseType::CANCEL_REJECTED, client_id, ticker_id, order_id, OrderId_INVALID, Side::INVALID, Price_INVALID, Qty_INVALID, Qty_INVALID};
    }
    else{
        client_response_ = {ClientResponseType::CANCELED, client_id, ticker_id, order_id, exchange_order->market_order_id_, exchange_order->side_, exchange_order->price_, Qty_INVALID, exchange_order->qty_};
        market_update_ = {MarketUpdateType::CANCEL, exchange_order->market_order_id_, ticker_id, exchange_order->side_, exchange_order->price_, 0, exchange_order->priority_};
        removeOrder(exchange_order);
        matching_engine_->sendMarketUpdate(&market_update_);
    }
    matching_engine_->sendClientResponse(&client_response_);
}
auto MEOrderBook::removeOrder(MEOrder *order) noexcept -> void
{
    auto orders_at_price = getOrdersAtPrice(order->price_);
    if(order->prev_order_ == order){ // only one element
        removeOrdersAtPrice(order->side_, order->price_);
    }
    else{ // remove the link
        const auto order_before = order->prev_order_;
        const auto order_after = order->next_order_;
        order_before->next_order_ = order_after;
        order_after->prev_order_ = order_before;
        if(orders_at_price->first_me_order_ == order){
            orders_at_price->first_me_order_ = order_after;
        }
        order->prev_order_ = order->next_order_ = nullptr;
    }
    cid_oid_to_order_.at(order->client_id_).at(order->client_order_id_) = nullptr;
    order_pool_.deallocate(order);
}
auto MEOrderBook::removeOrdersAtPrice(Side side, Price price) noexcept -> void
{
    const auto best_orders_by_price = (side == Side::BUY ? bids_by_price_ :asks_by_price_);
    auto orders_at_price = getOrdersAtPrice(price);
    if(UNLIKELY(orders_at_price->next_entry_ == orders_at_price)){ // empty side of book
        (side == Side::BUY ? bids_by_price_ : asks_by_price_) = nullptr;
    }
    else{
        orders_at_price->prev_entry_->next_entry_ = orders_at_price->next_entry_;
        orders_at_price->next_entry_->prev_entry_ = orders_at_price->prev_entry_;
        if(orders_at_price == best_orders_by_price){
            (side == Side::BUY ? bids_by_price_ : asks_by_price_) = orders_at_price->next_entry_;
        }
        orders_at_price->prev_entry_ = orders_at_price->next_entry_ = nullptr;
    }
    price_orders_at_prices_.at(priceToIndex(price)) = nullptr;
    orders_at_price_pool_.deallocate(orders_at_price);
}
auto MEOrderBook::checkForMatch(ClientId client_id, OrderId client_order_id, TicketId ticker_id, Side side, Price price, Qty qty, Qty new_market_order_id) noexcept -> Qty
{
    auto leaves_qty = qty;
    if(side == Side::BUY){
        while(leaves_qty && asks_by_price_){
            const auto ask_itr = asks_by_price_->first_me_order_;
            if(LIKELY(price < ask_itr->price_)){
                break;
            }
            match(ticker_id, client_id, side, client_order_id, new_market_order_id, ask_itr, &leaves_qty);
        }
    }
    if (side == Side::SELL)
    {
        while (leaves_qty && bids_by_price_){
            const auto bid_itr = bids_by_price_->first_me_order_;
            if (LIKELY(price > bid_itr->price_))
            {
                break;
            }
            match(ticker_id, client_id, side, client_order_id, new_market_order_id, bid_itr, &leaves_qty);
        }
    }
    return leaves_qty;
}
auto MEOrderBook::match(TicketId ticker_id, ClientId client_id, Side side, OrderId client_order_id, OrderId new_market_order_id, MEOrder *itr, Qty *leaves_qty) noexcept -> void
{
    const auto order = itr;
    const auto order_qty = order->qty_;
    const auto fill_qty = std::min(*leaves_qty, order_qty);
    *leaves_qty -= fill_qty;
    order->qty_ -= fill_qty;

    client_response_ = {ClientResponseType::FILLED, client_id, ticker_id, client_order_id, new_market_order_id, side, itr->price_, fill_qty, *leaves_qty};
    matching_engine_->sendClientResponse(&client_response_);
    client_response_ = {ClientResponseType::FILLED, order->client_id_, ticker_id, order->client_order_id_, order->market_order_id_, order->side_, itr->price_, fill_qty, order->qty_};
    matching_engine_->sendClientResponse(&client_response_);
    market_update_ = {MarketUpdateType::TRADE, OrderId_INVALID, ticker_id, side, itr->price_, fill_qty, Priority_INVALID};
    matching_engine_->sendMarketUpdate(&market_update_);

    if(!order->qty_){
        market_update_ = {MarketUpdateType::CANCEL, order->market_order_id_, ticker_id, order->side_, order->price_, order_qty, Priority_INVALID};
        matching_engine_->sendMarketUpdate(&market_update_);
        removeOrder(order);
    }
    else{
        market_update_ = {MarketUpdateType::MODIFY, order->market_order_id_, ticker_id, order->side_, order->price_, order->qty_, order->priority_};
        matching_engine_->sendMarketUpdate(&market_update_);
    }
}
auto MEOrderBook::toString(bool detailed, bool validity_check) const -> std::string
{
    return std::string();
}
}