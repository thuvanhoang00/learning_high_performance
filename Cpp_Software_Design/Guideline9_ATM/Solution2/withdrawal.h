#pragma once
#include "transaction.h"
class Withdrawal : public Transaction{
public:
    virtual void execute() override;
};