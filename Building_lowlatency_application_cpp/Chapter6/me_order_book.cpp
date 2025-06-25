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
}