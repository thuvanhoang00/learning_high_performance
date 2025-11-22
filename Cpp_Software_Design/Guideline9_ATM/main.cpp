#include "deposit.h"
#include "transfer.h"
#include "withdrawal.h"
#include "ui.h"

void process(Transaction* t, int menu){
    switch (menu)
    {
    case 1:
        if(dynamic_cast<Deposit*>(t)){
            t->execute();
        }
        break;
    case 2:
        if(dynamic_cast<Transfer*>(t)){
            t->execute();
        }
        break;
    case 3:
        if(dynamic_cast<Withdrawal*>(t)){
            t->execute();
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
    Transaction* t = new Deposit(ui);
    int menu = ui.selectMenu();
    process(t, menu);
    return 0;
}