#pragma once
#include "common/macros.h"
#include "common/logging.h"
#include "exchange/order_server/client_response.h"
#include "exchange/order_server/client_request.h"
#include "om_order.h"
#include "risk_manager.h"
#include "trade_engine.h"
using namespace thu;
namespace Trading{
class TradeEngine;

class OrderManager{
private:
    TradeEngine *trade_engine_ = nullptr;
    const RiskManager &risk_manager_;
    std::string time_str_;
    Logger *logger_ = nullptr;
    OMOrderTickerSideHashMap ticker_side_order_;
    OrderId next_order_id_ = 1;
public:
    OrderManager(Logger *logger, TradeEngine *trade_engine, RiskManager &risk_manager)
    : logger_(logger), trade_engine_(trade_engine), risk_manager_(risk_manager)
    {}
    auto getOMOrderSideHashMap(TickerId ticker_id) const {
        return &(ticker_side_order_.at(ticker_id));
    }
    auto newOrder(OMOrder *order, TickerId ticker_id, Price price, Side side, Qty qty) noexcept->void{
        const Exchange::MEClientRequest new_request{Exchange::ClientRequestType::NEW
            , trade_engine_->clientId()
            , ticker_id
            , next_order_id_
            , side
            , price
            , qty};
        
        *order = {ticker_id, next_order_id_, side, price, qty, OMOrderState::PENDING_NEW};
        ++next_order_id_;
        logger_->log("%:% %() % Sent new order % for %\n", __FILE__, __LINE__, __FUNCTION__,
                 thu::getCurrentTimeStr(&time_str_), new_request.toString().c_str(), order->toString().c_str());
    }
    auto cancelOrder(OMOrder *order) noexcept->void{
        const Exchange::MEClientRequest cancel_request{Exchange::ClientRequestType::CANCEL, trade_engine_->clientId()
            , order->ticker_id_
            , order->side_
            , order->price_
            , order->qty_};
        trade_engine_->sendClientRequest(&cancel_request);
        order->order_state_ = OMOrderState::PENDING_CANCEL;
        logger_->log("%:% %() % Sent cancel % for %\n", __FILE__, __LINE__, __FUNCTION__,
                 thu::getCurrentTimeStr(&time_str_), cancel_request.toString().c_str(), order->toString().c_str());
    }
    auto moveOrder(OMOrder *order, TickerId ticker_id, Price price, Side side, Qty qty) noexcept{
        switch (order->order_state_)
        {
        case OMOrderState::LIVE:
        {
            if (order->price_ != price || order->qty_ != qty)
                cancelOrder(order);
        }
        break;
        case OMOrderState::INVALID:
        case OMOrderState::DEAD:
        {
            if (LIKELY(price != Price_INVALID))
            {
                const auto risk_result = risk_manager_.checkPreTradeRisk(ticker_id, side, qty);
                if (LIKELY(risk_result == RiskCheckResult::ALLOWED))
                {
                    newOrder(order, ticker_id, price, side, qty);
                }
                else
                {
                    logger_->log("%:% %() % Ticker:% Side:% Qty:% RiskCheckResult : %\n ", __FILE__, __LINE__, __FUNCTION__,
                                 thu::getCurrentTimeStr(&time_str_),
                                 tickerIdToString(ticker_id),
                                 sideToString(side),
                                 qtyToString(qty),
                                 riskCheckResultToString(risk_result));
                }
            }
        }
        break;
        case OMOrderState::PENDING_NEW:
        case OMOrderState::PENDING_CANCEL:
        default:
            break;
        }
    }

    auto moveOrders(TickerId ticker_id, Price bid_price, Price ask_price, Qty clip) noexcept{
        auto bid_order = &(ticker_side_order_.at(ticker_id)).at(sideToIndex(Side::BUY));
        moveOrder(bid_order, ticker_id, bid_price, Side::BUY, clip);
        auto ask_order = &(ticker_side_order_.at(ticker_id)).at(sideToIndex(Side::SELL));
        moveOrder(ask_order, ticker_id, ask_price, Side::SELL, clip);
    }

    auto onOrderUpdate(const Exchange::MEClientResponse *client_response) noexcept->void{
        logger_->log("%:% %() % %\n", __FILE__, __LINE__,
                     __FUNCTION__, thu::getCurrentTimeStr(&time_str_),
                     client_response->toString().c_str());
        auto order = &(ticker_side_order_.at(ticker_id)).at(sideToIndex(client_response->side_));
        logger_->log("%:% %() % %\n", __FILE__, __LINE__, __FUNCTION__, thu::getCurrentTimeStr(&time_str_), order->toString().c_str());
        switch (client_response->type_)
        {
        case Exchange::ClientResponseType::ACCEPTED:
            order->order_state_ = OMOrderState::LIVE;
            break;
        case Exchange::ClientResponseType::CANCELED:
            order->order_state_ = OMOrderState::DEAD;
            break;
        case Exchange::ClientResponseType::FILLED:{
            order->qty_ = client_response->leaves_qty_;
            if(!order->qty_)
                order->order_state_ = OMOrderState::DEAD;
        }
            break;
        case EXchange::ClientResponseType::CANCEL_REJECTED:
        case EXchange::ClientResponseType::INVALID:
        default:
            break;
        }
    }
};
}// end namespace