#include "matching_engine.h"
namespace Exchange{
MatchingEngine::MatchingEngine(ClientRequestLFQueue *client_request, ClientResponseLFQueue *client_responses, MEMarketUpdateLFQueue *market_updates)
:   incoming_requests_ (client_request)
    , outgoing_ogw_responses_(client_responses)
    , outgoing_md_updates_(market_updates)
    , logger_("exchange_matching_engine.log")
{
    for(size_t i = 0; i < ticker_order_book_.size(); ++i){
        ticker_order_book_[i] = new MEOrderBook(i, &logger_, this);
    }
}

MatchingEngine::~MatchingEngine()
{
    run_ = false;
    using namespace std::literals::chrono_literals;
    std::this_thread::sleep_for(1s);
    incoming_requests_ = nullptr;
    outgoing_ogw_responses_ = nullptr;
    outgoing_md_updates_ = nullptr;
    for(auto& order_book : ticker_order_book_){
        delete order_book;
        order_book = nullptr;
    }
}
auto MatchingEngine::start() -> void
{
    run_ = true;
    ASSERT(createAndStartThread(-1, "MatchingEngine", [this](){run();}) != nullptr, "Failed to start MatchingEngine thread.");
}
auto MatchingEngine::stop() -> void
{
    run_ = false;
}
auto MatchingEngine::run() noexcept
{
    logger_.log("%:% %() %\n",
                 __FILE__, __LINE__, __FUNCTION__, thu::getCurrentTimeStr(&time_str_));
    while (run_)
    {
        const auto me_client_request = incoming_requests_->getNextToRead();
        if(LIKELY(me_client_request)){
            logger_.log("%:% %() % Processing %\n",
                        __FILE__, __LINE__, __FUNCTION__, thu::getCurrentTimeStr(&time_str_), me_client_request->toString());
            processClientRequest(me_client_request);
            incoming_requests_->updateReadIndex();
        }
    }
    
}
auto MatchingEngine::processClientRequest(const MEClientRequest *client_request) noexcept -> void
{
    auto order_book = ticker_order_book_[client_request->client_id_];
    switch (client_request->type_)
    {
    case ClientRequestType::NEW:
    {
        order_book->add(client_request->client_id_,
                        client_request->order_id_,
                        client_request->ticket_id_,
                        client_request->side_,
                        client_request->price_,
                        client_request->qty_);
    }
    break;
    case ClientRequestType::CANCEL:
    {
        order_book->cancel(client_request->client_id_,
                           client_request->order_id_,
                           client_request->ticket_id_);
    }   
    break;
    default:
    {
        FATAL("Received invalid client-request-type:"+clientRequestTypeToString(client_request->type_));
    }
        break;
    } 
}
auto MatchingEngine::sendClientResponse(const MEClientResponse *client_response) noexcept -> void
{
    logger_.log("%:% %() % Sending %\n",
                __FILE__, __LINE__, __FUNCTION__, thu::getCurrentTimeStr(&time_str_), client_response->toString());
    auto next_write = outgoing_ogw_responses_->getNextToWriteTo();
    *next_write = std::move(*client_response);
    outgoing_ogw_responses_->updateWriteIndex();
}
auto MatchingEngine::sendMarketUpdate(const MEMarketUpdate *market_update) noexcept -> void
{
    logger_.log("%:% %() % Sending %\n",
                __FILE__, __LINE__, __FUNCTION__, thu::getCurrentTimeStr(&time_str_), market_update->toString());
    auto next_write = outgoing_md_updates_->getNextToWriteTo();
    *next_write = *market_update;
    outgoing_md_updates_->updateWriteIndex();
}
}