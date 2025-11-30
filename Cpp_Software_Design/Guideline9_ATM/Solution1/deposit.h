#pragma once
#include "transaction.h"
#include "ui.h"
class Deposit : public Transaction{
public:
    Deposit(DepositUI& ui) : ui_(ui) {}
    virtual ~Deposit(){}
    virtual void execute() override;
private:
    DepositUI& ui_;
};