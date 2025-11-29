#include "ui.h"
#include <iostream>
int UI::selectMenu(){
    std::cout << "Select Menu\n";
    std::cout << "1. Deposit\n";
    std::cout << "2. Transfer\n";
    std::cout << "3. Withdrawal\n";
    std::cout << "4. Exit\n";

    int n;
    while(true){
        std::cin >> n;
        if(n<=4 && n>=1) break;
        else {
            std::cout << "Invalid input. Try again!\n";
        }
    }
    return n;

    // switch (n)
    // {
    // case 1:
    //     requestDepositAmount();
    //     break;
    // case 2:
    //     requestTransferAmount();
    //     break;
    // case 3:
    //     requestWithdrawalAmount();
    //     break;
    // case 4:
    // default:
    //     break;
    // }
}

int UI::requestDepositAmount(){
    std::cout << "Enter amount: ";
    int amount;
    std::cin >> amount;
    if(amount <0 ) {
        std::cout << "Invalid amount\n";
        return 0;
    }
    return amount;
}

void UI::requestTransferAmount(){
    std::cout << "requestTransferAmount\n";
}

void UI::requestWithdrawalAmount(){
    std::cout << "requestWithdrawalAmount\n";
}