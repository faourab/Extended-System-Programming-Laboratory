#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
int main(){


    int pipfd[2];
    pid_t cpid;
    char message[] = "Hello my dad!";
    
    
    if (pipe(pipfd) == -1){
        perror("Pipe Bomb!");
        exit(1);
    
    }
    cpid = fork();
    if (cpid== -1){
        perror("Fork Bomb!");
    
    
        exit(1);
    }
    if (cpid == 0){
    
    
        close(pipfd[0]);
        if (write(pipfd[1], message, strlen(message)) != strlen(message)){
    
            perror("Error in writing!");
            exit(1);
        }
    
    
        close(pipfd[1]);
    }
    else{
        close(pipfd[1]);
    
    
        char buffer[strlen(message)];
        if(read(pipfd[0],buffer,sizeof(buffer))==-1){
    
            perror("Error in reading!");
            exit(1);
    
    
            }
        close(pipfd[0]);
        buffer[strlen(message)]='\0';
    
        printf("Son tells dad: %s\n",buffer);
    
    }
    return 0;
}