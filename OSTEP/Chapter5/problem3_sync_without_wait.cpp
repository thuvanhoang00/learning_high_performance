#include <iostream>
#include <unistd.h>
#include <sys/wait.h>

// Using pipe to synchronize between child and parent
int main()
{
    int pipefd[2];
    pipe(pipefd); // Create pipe
    int rc = fork();
    if(rc < 0){
        exit(1);
    }
    else if(rc==0){
        close(pipefd[0]); // close read end
        std::cout << "Child process hello\n";
        write(pipefd[1], "x", 1); // send signal to parent
        close(pipefd[1]);
    }
    else{
        // wait(NULL);

        close(pipefd[1]); // close write end
        char buf;
        read(pipefd[0], &buf, 1); // wait for signal from child
        std::cout << "Parent process goodbye\n";
        close(pipefd[0]);
    }
    return 0;
}