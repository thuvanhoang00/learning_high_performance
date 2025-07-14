#pragma once
#include "common/macros.h"
#include "common/logging.h"
#include "position_keeper.h"
#include "om_order.h"
#include "common/types.h"
namespace Trading{
class OrderManager;

enum class RiskCheckResult : int8_t{
    INVALID = 0,
    ORDER_TOO_LARGE = 1,
    POSITION_TOO_LARGE = 2,
    LOSS_TOO_LARGE = 3,
    ALLOWED = 4
};

inline auto riskCheckResultToString(RiskCheckResult result) {
    switch (result)
    {
    case RiskCheckResult::INVALID:
        return "INVALID";
    case RiskCheckResult::ORDER_TOO_LARGE:
        return "ORDER_TOO_LARGE";
    case RiskCheckResult::POSITION_TOO_LARGE:
        return "POSITION_TOO_LARGE";
    case RiskCheckResult::LOSS_TOO_LARGE:
        return "LOSS_TOO_LARGE";
    case RiskCheckResult::ALLOWED:
        return "ALLOWED";
    default:
        return "";
    }
}

struct RiskInfo{
    const PositionInfo *position_info_ = nullptr;
    RiskCfg risk_cfg_;
    auto toString() const
    {
        std::stringstream ss;
        ss << "RiskInfo" << "["
           << "pos:" << position_info_->toString() << " "
           << risk_cfg_.toString()
           << "]";
        return ss.str();
    }
    auto checkPreTradeRisk(Side side, Qty qty) const noexcept{
        if(UNLIKELY(qty > risk_cfg_.max_order_size_))
            return RiskCheckResult::ORDER_TOO_LARGE;
        if(UNLIKELY(std::abs(position_info_->position_ + sideToValue(side)*static_cast<int32_t>(qty)) 
                            > static_cast<int32_t>(risk_cfg_.max_position_))){
            return RiskCheckResult::POSITION_TOO_LARGE;                        
        }
        if(UNLIKELY(position_info_->total_pnl_ < risk_cfg_.max_loss_)){
            return RiskCheckResult::LOSS_TOO_LARGE;
        }
        return RiskCheckResult::ALLOWED;
    }
};

typedef std::array<RiskInfo, ME_MAX_TICKERS> TickerRiskInfoHashMap;

class RiskManager{
private:
    std::string time_str_;
    Logger *logger_ = nullptr;
    TickerRiskInfoHashMap ticker_risk_;
public:
    RiskManager(Logger *logger, const PositionKeeper *position_keeper, const TradeEngineCfgHashMap &ticker_cfg)
        : logger_(logger)
    {
        for(TickerId i = 0; i< ME_MAX_TICKERS; ++i){
            ticker_risk_.at(i).position_info_ = position_keeper->getPositionInfo(i);
            ticker_risk_.at(i).risk_cfg_ = ticker_cfg[i].risk_cfg_;
        }
    }
    auto checkPreTradeRisk(TickerId ticker_id, Side side, Qty qty) const noexcept{
        return ticker_risk_.at(ticker_id).checkPreTradeRisk(side, qty);
    }
};
} // end namespace