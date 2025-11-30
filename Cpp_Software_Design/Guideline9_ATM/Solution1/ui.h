#pragma once

class DepositUI{
public:
    virtual int requestDepositAmount() = 0;
};

class TransferUI{
public:
    virtual void requestTransferAmount() = 0;
};

class UI : public DepositUI, public TransferUI
{
public:
    int selectMenu();
    int requestDepositAmount() override;
    void requestWithdrawalAmount();
    void requestTransferAmount() override;
};