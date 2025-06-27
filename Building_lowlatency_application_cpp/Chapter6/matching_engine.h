#pragma once
#include "thread_utils.h"
#include "lockfree_queue.h"
#include "macros.h"
#include "client_request.h"
#include "client_response.h"
#include "market_update.h"
#include "me_order_book.h"
#include "logging.h"
using namespace thu;
namespace Exchange{
class MatchingEngine final{
public:
    MatchingEngine(ClientRequestLFQueue *client_request, ClientResponseLFQueue *client_responses, MEMarketUpdateLFQueue *market_updates);
    ~MatchingEngine();
    auto start()->void;
    auto stop()->void;

    MatchingEngine() = delete;
    MatchingEngine(const MatchingEngine &) = delete;
    MatchingEngine(MatchingEngine&&) = delete;
    MatchingEngine& operator=(const MatchingEngine&) = delete;
    MatchingEngine& operator=(MatchingEngine&&) = delete;

    auto sendClientResponse(const MEClientResponse *client_response) noexcept -> void;
    auto sendMarketUpdate(const MEMarketUpdate *market_update) noexcept -> void;
private:
    auto run() noexcept->void;
    auto processClientRequest(const MEClientRequest *client_request) noexcept -> void;

private:
    OrderBookHashMap ticker_order_book_;
    ClientRequestLFQueue *incoming_requests_ = nullptr;
    ClientResponseLFQueue *outgoing_ogw_responses_ = nullptr;
    MEMarketUpdateLFQueue *outgoing_md_updates_ = nullptr;
    volatile bool run_ = false;
    std::string time_str_;
    Logger logger_;
};
}