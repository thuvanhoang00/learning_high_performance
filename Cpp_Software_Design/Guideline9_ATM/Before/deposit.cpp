#include "deposit.h"
#include <iostream>

void Deposit::execute(){
    int amount = ui_.requestDepositAmount();
    std::cout << "Deposit amount = " << amount  << "\n";
}