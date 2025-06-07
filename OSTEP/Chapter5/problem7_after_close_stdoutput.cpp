#include <iostream>
#include <unistd.h>

/*
*   Simple code to gain idea of STDOUT_FILENO
*   After close STDOUT_FILENO then iostream does nothing
*/

int main()
{
    int rc = fork();
    if(rc<0){
        exit(1);
    }
    else if(rc==0){
        close(STDOUT_FILENO);
        std::cout << "Try to print sth after close STDOUT_FILENO\n";
    }
    else{
        std::cout << "Parent process\n";
    }

    return 0;
}