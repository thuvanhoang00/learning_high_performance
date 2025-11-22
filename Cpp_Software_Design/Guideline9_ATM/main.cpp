#include "deposit.h"
#include "transfer.h"
#include "withdrawal.h"
#include "ui.h"
int main()
{
    Transaction* t = new Deposit();
    t->execute();
    UI ui;
    ui.selectMenu();
    return 0;
}