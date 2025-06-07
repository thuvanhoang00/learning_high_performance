#include <unistd.h>
#include <iostream>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>

/*
*   Parent and child can write to the same fd
*   They both can interfere each other on writing
*   
*/
int main(){
    
    int fd = open("test.txt", O_CREAT|O_WRONLY|O_TRUNC|O_SYNC, S_IRWXU);
    if(fd == -1){
        fprintf(stderr, "Open failed\n");
        exit(1);
    }


    int rc = fork();
    if(rc<0){
        fprintf(stderr, "Fork failed\n");
        exit(1);
    }
    else if(rc==0){
        std::cout << "Child process here! fd = " << fd << "\n";
        const char *str = "Child process here!\n";
        for(int i=0; i<1000; i++){
            write(fd, str, strlen(str));
        }
    }
    else {
        std::cout << "Parent process here! fd = " << fd << "\n";
        const char *str = "Parent process here!\n";
        for(int i=0; i<1000; i++){
            write(fd, str, strlen(str));
        }
    }
    return 0;
}