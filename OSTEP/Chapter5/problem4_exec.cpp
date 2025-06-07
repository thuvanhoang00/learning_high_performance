#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>

/*
*   Simple use of fork() API
*   Also testing on wait() in child process -> will return -1 if not waiting process
*/

int main()
{
    int rc = fork();
    if(rc<0){
        exit(1);
    }
    else if(rc==0){
        std::cout << "Child process\n";
        char *args[2];
        args[0]=".";
        args[1]=NULL;        
        execvp("/bin/ls", args);
        int status = wait(nullptr);
        if(status==-1){
            std::cerr << "Child wait() failed: " << strerror(errno) << "\n";
        }
    }
    else{
        std::cout << "Parent process\n";
    }
    return 0;
}