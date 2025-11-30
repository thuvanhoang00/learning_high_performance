#pragma once

class DepositUI{
public:
    virtual ~DepositUI() = default;
    virtual int requestDepositAmount() = 0;
};