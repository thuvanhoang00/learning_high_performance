#pragma once
#include "depositUI.h"
#include "transferUI.h"

class UI : public DepositUI, public TransferUI
{
public:
    int selectMenu();
    int requestDepositAmount() override;
    void requestWithdrawalAmount();
    void requestTransferAmount() override;
};