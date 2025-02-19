#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>
#include "LineParser.h"
#include <string.h>
#include <sys/wait.h>
#include <signal.h>
#include <signal.h>
#include <linux/limits.h>

#include <sys/syscall.h>
void execute(cmdLine *pCmdLine, int debug);
void changeDir(cmdLine *parseCmd, int debug);
void wake_Term_Stop(cmdLine *parseCmd, char *proc, int debug);
void redirction(cmdLine *parseCmd, int debug);

int main(int argc, char **argv){


  int debug = 0;
  for (int i = 0; i < argc; i++){
  
  
    if (strncmp(argv[i], "-d", 2) == 0){
      debug = 1;
  
    }
  }
  while (1)
  {
  
    char pathname[PATH_MAX];
    if (getcwd(pathname, sizeof(pathname)) != NULL){
  
  
      printf("%s>", pathname);
    }
  
    else{
      perror("getcwd failed");
  
  
      exit(1);
    }
    char input[2048];
    if (fgets(input, 2048, stdin) == NULL && debug == 1){
  
  
      perror("Error reading the line!");
    }
    if (strncmp(input, "quit", 4) == 0){
      exit(1);
    }
  
    cmdLine *parseCmd = parseCmdLines(input);
    if (strcmp(parseCmd->arguments[0], "cd") == 0){

      changeDir(parseCmd, debug);
    }
    else{

      if (strcmp(parseCmd->arguments[0], "wake") == 0 || strcmp(parseCmd->arguments[0], "term") == 0|| strcmp(parseCmd->arguments[0],"stop") == 0){


        wake_Term_Stop(parseCmd, parseCmd->arguments[0], debug);
      }
      else{
        execute(parseCmd, debug);
      }
    }

    freeCmdLines(parseCmd);
  }
  return 0;

}


void changeDir(cmdLine *parseCmd, int debug){

  if (chdir(parseCmd->arguments[1]) == -1 && debug == 1){

    perror("Error in changing dir");


  }
}


void redirction(cmdLine *parseCmd, int debug){
  if (parseCmd->inputRedirect){

    if (!freopen(parseCmd->inputRedirect, "r", stdin) && debug == 1){
      perror("Error in opening the readfile");


    }
  }

  if (parseCmd->outputRedirect){
    if (!freopen(parseCmd->outputRedirect, "w", stdout) && debug == 1){


      perror("Error in opening the writefile");
    }


  }
}

void execute(cmdLine *pCmdLine, int debug){


  pid_t pid = fork();
  if (pid == -1){
    if (debug == 1){
      perror("Fork Bomb!");

    }

    exit(1);
  }
  if (debug == 1){


    fprintf(stderr, "pid = %d\n", pid);
    fprintf(stderr, "Executing command = %s\n\n", pCmdLine->arguments[0]);

  }
  if (pid == 0){


    redirction(pCmdLine, debug);
    if (execvp(pCmdLine->arguments[0], pCmdLine->arguments) == -1){

      if (debug == 1){


        perror("Error in execv");
      }



      _exit(1);
    }
  }

  else{
    if (pCmdLine->blocking){
      if (waitpid(pid, NULL, 0) == -1){


        if (debug == 1){
          perror("child with pid you stated is unavailable!");


        }


        exit(1);
      }

    }
  }
  return;

}

void wake_Term_Stop(cmdLine *parseCmd, char *proc, int debug){
  pid_t pid = atoi(parseCmd->arguments[1]);

  if (strcmp(parseCmd->arguments[0], "wake") == 0){
    if (kill(pid, SIGCONT) == -1 && debug == 1){

      perror("Error in sending the signal -wake-");
    }
  }
  else if (strcmp(parseCmd->arguments[0], "term") == 0){
    if (kill(pid, SIGINT) == -1 && debug == 1){
      perror("Error in sending the signal -term-");
    }
  }
  else if (strcmp(parseCmd->arguments[0], "stop") == 0){

    if (kill(pid, SIGTSTP) == -1 && debug == 1){
      perror("Error in sending the signal -stop-");

    }

  }
}

