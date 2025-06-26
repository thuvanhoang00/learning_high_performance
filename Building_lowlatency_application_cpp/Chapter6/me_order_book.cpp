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
}