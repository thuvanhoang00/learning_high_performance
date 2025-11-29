#pragma once
#include "transaction.h"
#include "ui.h"
class Deposit : public Transaction{
public:
    Deposit(UI& ui) : ui_(ui) {}
    virtual ~Deposit(){}
    virtual void execute() override;
private:
    UI& ui_;
};