#pragma once

class TransferUI{
public:
    virtual ~TransferUI() =default;
    virtual void requestTransferAmount() = 0;
};