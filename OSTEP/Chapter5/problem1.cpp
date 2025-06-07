#include <iostream>
#include <unistd.h>

/*
*   Simple using fork() API
*   
*/
int main()
{

    int x=100;
    int rc=fork(); // return pid of child
    if(rc<0){
        fprintf(stderr, "fork failed\n");
        exit(1);
    }
    else if(rc==0){
        std::cout << "Child process: " << getpid() << ", x's value: " << x << std::endl;
    }
    else{
        std::cout << "Parent process of: " << rc << ", x's value: " << x  << std::endl;
    }
    return 0;
}