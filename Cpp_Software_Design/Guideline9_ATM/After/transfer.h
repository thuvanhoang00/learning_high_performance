#pragma once
#include "transaction.h"
class Transfer : public Transaction{
public:
    virtual void execute() override;
};