#include "deposit.h"
#include "transfer.h"
#include "withdrawal.h"
#include "ui.h"
#include <memory>

void process(std::unique_ptr<Transaction> p_transac, int menu){
    switch (menu)
    {
    case 1:
        if(dynamic_cast<Deposit*>(p_transac.get())){
            p_transac->execute();
        }
        break;
    case 2:
        if(dynamic_cast<Transfer*>(p_transac.get())){
            p_transac->execute();
        }
        break;
    case 3:
        if(dynamic_cast<Withdrawal*>(p_transac.get())){
            p_transac->execute();
        }
        break;
    case 4:
    default:
        break;
    }
    
}

int main()
{
    UI ui;
    std::unique_ptr<Transaction> ptr = std::make_unique<Deposit>(ui);
    int menu = ui.selectMenu();
    process(std::move(ptr), menu);
    return 0;
}