#pragma once
#include "transaction.h"
class Deposit : public Transaction{
public:
    virtual void execute() override;
};