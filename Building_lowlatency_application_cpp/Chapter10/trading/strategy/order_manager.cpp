#include "order_manager.h"
namespace Trading
{
    OrderManager::OrderManager(Logger *logger, TradeEngine *trade_engine, RiskManager &risk_manager)
        : trade_engine_(trade_engine), risk_manager_(risk_manager), logger_(logger)
    {
    }
}