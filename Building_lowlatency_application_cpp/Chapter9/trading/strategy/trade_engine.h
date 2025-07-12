#pragma once
#include "common/types.h"
namespace Trading{
class TradeEngine{
private:
    ClientId client_id_;
public:
    TradeEngine(){}
    auto clientId() const{
        return client_id_;
    }
};
}// end namespace