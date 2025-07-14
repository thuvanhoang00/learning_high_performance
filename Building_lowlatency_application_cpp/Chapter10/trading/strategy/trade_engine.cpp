#include "trade_engine.h"
namespace Trading{
    TradeEngine::TradeEngine(ClientId client_id, AlgoType type, const TradeEngineCfgHashMap &ticker_cfg, Exchange::ClientRequestLFQueue *client_requests, Exchange::ClientResponseLFQueue *client_responses, Exchange::MEMarketUpdateLFQueue *market_updates)
    {
    }

    TradeEngine::~TradeEngine()
    {
    }

    auto TradeEngine::run() noexcept->void{
        
    }
}