#pragma once

class Transaction{
public:
    virtual ~Transaction() = default;
    virtual void execute() = 0;
};