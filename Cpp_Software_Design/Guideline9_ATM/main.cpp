#include "deposit.h"
#include "transfer.h"
#include "withdrawal.h"

int main()
{
    Transaction* t = new Deposit();
    t->execute();
    return 0;
}