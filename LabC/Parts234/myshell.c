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
#define HISTLEN 10
#define TERMINATED  -1
#define RUNNING 1
#define SUSPENDED 0
void execute(cmdLine *pCmdLine, int debug);
void changeDir(cmdLine *parseCmd, int debug);
void wake_Term_Stop(cmdLine *parseCmd, char *proc, int debug);
void redirction(cmdLine *parseCmd, int debug);
void addProcess(process** process_list, cmdLine* cmd, pid_t pid);
void printProcessList(process** process_list);
void freeProcessList(process* process_list);
void updateProcessList(process **process_list);/*/: go over the process list, and for each process check if it is done, 
you can use waitpid with the option WNOHANG. WNOHANG does not block the calling process, the process returns from the call to waitpid immediately. 
If no process with the given process id exists, then waitpid returns -1.*/
void updateProcessStatus(process* process_list, int pid, int status);//: find the process with the given id in the process_list and change its status to the received status.
typedef struct process{
    cmdLine* cmd;                         /* the parsed command line*/
    pid_t pid;                            /* the process id that is running the command*/
    int status;                           /* status of the process: RUNNING/SUSPENDED/TERMINATED */
    struct process *next;                 /* next process in chain */
} process;

process *process_list = NULL; // Global process list
void freeProcessList(process* process_list) {
    process* current = process_list;
    process* temp;

    while (current != NULL) {
        // If cmdLine is dynamically allocated, free it here
        if (current->cmd != NULL) {
            free(current->cmd);
        }
        
        temp = current;
        current = current->next;
        free(temp); // Free process node
    }
}
void updateProcessList(process **process_list) {
    process* current = *process_list;
    int status;

    while (current != NULL) {
        pid_t pid = current->pid;
        pid_t result = waitpid(pid, &status, WNOHANG);

        if (result == -1) {
            // Process does not exist anymore or an error occurred
            current->status = TERMINATED; // Assuming TERMINATED is a constant defined
        } else if (result > 0) {
            if (WIFSTOPPED(status)) {
                // Process has been stopped (SIGTSTOP)
                current->status = SUSPENDED; // Assuming SUSPENDED is a constant defined
            } else if (WIFCONTINUED(status)) {
                // Process has been resumed (SIGCONT)
                current->status = RUNNING; // Assuming RUNNING is a constant defined
            } else if (WIFEXITED(status) || WIFSIGNALED(status)) {
                // Process has been terminated (SIGINT, SIGTERM, etc.)
                current->status = TERMINATED; // Mark as terminated
            }
        }

        current = current->next;
    }
}
void updateProcessStatus(process* process_list, int pid, int status) {
    process* current = process_list;

    while (current != NULL) {
        if (current->pid == pid) {
            current->status = status;
            break;
        }
        current = current->next;
    }
}

void addProcess(process** process_list, cmdLine* cmd, pid_t pid) {
    process *current = *process_list;
    while (current != NULL) {
        if (current->pid == pid) {
            return; // Process already exists
        }
        current = current->next;
    }

    process *new_process = (process *)malloc(sizeof(process));
    if (!new_process) {
        perror("Failed to allocate memory for new process");
        exit(1);
    }

    new_process->cmd = cmd;
    new_process->pid = pid;
    new_process->status = RUNNING;
    new_process->next = *process_list;
    *process_list = new_process;
}

void printProcessList(process** process_list) {
    updateProcessList(process_list);
    process *current = *process_list;
    int index = 0; // Index tracker
    process* prev = NULL;
    printf("INDEX\tPID\tSTATUS\t\tCOMMAND\n");
    printf("========================================\n");
    
    while (current != NULL) {
        // Print the index
        printf("%d\t", index);
        
        // Print PID
        printf("%d\t", current->pid);
        
        // Print status
        switch (current->status) {
            case RUNNING:
                printf("RUNNING\t");
                break;
            case SUSPENDED:
                printf("SUSPENDED\t");
                break;
            case TERMINATED:
                printf("TERMINATED\t");
                 if (prev == NULL) {
                *process_list = current->next; // Remove first element
                 } else {
                prev->next = current->next; // Remove current element
                }
                process* temp = current;
                current = current->next;
                free(temp); // Free terminated process
                break;
            default:
                printf("UNKNOWN\t");
        }
        
        // Print the command
        printf("%s\n", current->cmd->arguments[0]);

        // Move to the next process and increment index
        prev = current ;
        current = current->next;
        index++;
    }
    printf("========================================\n");
}
typedef struct historyNode {
    char *command;  // Store the unparsed command line
    struct historyNode *next;
} historyNode;

// Global history list pointers
historyNode *history_head = NULL;  // Pointer to the start of the history list
historyNode *history_tail = NULL;  // Pointer to the end of the history list
int history_size = 0 ;
// Function to add a command to the history list
void addHistory(const char *cmd) {
    if (history_size == HISTLEN) {
        // History is full, remove the oldest entry (head of the list)
        historyNode* temp = history_head;
        history_head = history_head->next;
        free(temp->command);
        free(temp);
        history_size--;  // Decrease the history size
    }

    // Allocate new history node
    historyNode* new_history = (historyNode*)malloc(sizeof(historyNode));
    if (new_history == NULL) {
        perror("Failed to allocate memory for history");
        exit(1);
    }
    
    new_history->command = strdup(cmd);  // Copy the command into history
    new_history->next = NULL;

    // Add new history to the end of the list
    if (history_tail == NULL) {
        history_head = history_tail = new_history;  // If list is empty, set both head and tail
    } else {
        history_tail->next = new_history;
        history_tail = new_history;  // Move tail to the new node
    }

    history_size++;
}


// Function to print the history list
void printHistory() {
    historyNode *current = history_head;
    int index = 1;
    while (current != NULL) {
        printf("%d: %s\n", index++, current->command);
        current = current->next;
    }
}

// Function to execute the last command (!!)
void executeLastHistory() {
    if (history_head == NULL) {
        printf("No history available\n");
        return;
    }

    // Execute the last command by parsing and running it again
    char *last_command = history_tail->command;
    printf("Executing last command: %s\n", last_command);

    // Parse and execute the last command as you would any other command
    cmdLine *parsed_cmd = parseCmdLines(last_command);
    execute(parsed_cmd, 0);  // The execute function you already implemented
    freeCmdLines(parsed_cmd);
}

// Function to execute the nth history command (!n)
void executeNthHistory(int n) {
    if (n < 1 || n > historyHeadCount()) {
        printf("Invalid history index\n");
        return;
    }

    // Traverse the list to find the nth command
    historyNode *current = history_head;
    for (int i = 1; i < n; i++) {
        current = current->next;
    }

    printf("Executing command %d: %s\n", n, current->command);

    // Parse and execute the nth command
    cmdLine *parsed_cmd = parseCmdLines(current->command);
    execute(parsed_cmd, 0);  // The execute function you already implemented
    freeCmdLines(parsed_cmd);
}
int main(int argc, char **argv) {
    int debug = 0;
    for (int i = 0; i < argc; i++) {
        if (strncmp(argv[i], "-d", 2) == 0) {
            debug = 1;
        }
    }

    while (1) {
        char pathname[PATH_MAX];
        if (getcwd(pathname, sizeof(pathname)) != NULL) {
            printf("%s>", pathname);
        } else {
            perror("getcwd failed");
            exit(1);
        }

        char input[2048];
        if (fgets(input, 2048, stdin) == NULL && debug == 1) {
            perror("Error reading the line!");
        }

        if (strncmp(input, "quit", 4) == 0) {
            exit(1);
        }

        if (strncmp(input, "procs", 5) == 0) {
            printProcessList(&process_list);
            continue;
        }
         if (strncmp(input, "history", 7) == 0) {
            printHistory();  // Print the command history
            continue;        // Skip the rest of the loop
        }

        // Handle '!!' (last command) and '!n' (nth command)
        if (strcmp(input, "!!") == 0) {
            executeLastHistory();  // Execute the last command
            continue;              // Skip the rest of the loop
        } else if (input[0] == '!' && strlen(input) > 1 && isdigit(input[1])) {
            int n = atoi(&input[1]);
            if (n < 1 || n > history_size) {
            printf("Error: No such history command!\n");
            } else {
                executeNthHistory(n);  // Execute the nth command from history
            }
    continue;  // Skip the rest of the loop
}

        // Add the command to history (if it's not a history command)
        addHistory(input);

        cmdLine *parseCmd = parseCmdLines(input);
        if (strcmp(parseCmd->arguments[0], "cd") == 0) {
            changeDir(parseCmd, debug);
        } else {
            if (strcmp(parseCmd->arguments[0], "wake") == 0 || strcmp(parseCmd->arguments[0], "term") == 0 || strcmp(parseCmd->arguments[0], "stop") == 0) {
                wake_Term_Stop(parseCmd, parseCmd->arguments[0], debug);
            } else {
                execute(parseCmd, debug);
            }
        }

        freeCmdLines(parseCmd);
        freeHistory();
    }
    return 0;
}
void freeHistory() {
    historyNode* current = history_head;
    historyNode* next;

    while (current != NULL) {
        next = current->next;
        free(current->command);  // Free the command string
        free(current);           // Free the history node
        current = next;
    }

    history_head = history_tail = NULL;  // Reset history list to NULL
    history_size = 0;  // Reset history size
}
void execute(cmdLine *pCmdLine, int debug) {
    pid_t pid = fork();
    if (pid == -1) {
        if (debug == 1) {
            perror("Fork Bomb!");
        }
        exit(1);
    }
    if (debug == 1) {
        fprintf(stderr, "pid = %d\n", pid);
        fprintf(stderr, "Executing command = %s\n\n", pCmdLine->arguments[0]);
    }

    if (pid == 0) {
        redirction(pCmdLine, debug);
        if (execvp(pCmdLine->arguments[0], pCmdLine->arguments) == -1) {
            if (debug == 1) {
                perror("Error in execvp");
            }
            _exit(1);
        }
    } else {
        addProcess(&process_list, pCmdLine, pid);
        if (pCmdLine->blocking) {
            if (waitpid(pid, NULL, 0) == -1) {
                if (debug == 1) {
                    perror("Child with specified pid is unavailable!");
                }
                exit(1);
            }
        }
    }
}

void changeDir(cmdLine *parseCmd, int debug) {
    if (chdir(parseCmd->arguments[1]) == -1 && debug == 1) {
        perror("Error in changing directory");
    }
}

void redirction(cmdLine *parseCmd, int debug) {
    if (parseCmd->inputRedirect) {
        if (!freopen(parseCmd->inputRedirect, "r", stdin) && debug == 1) {
            perror("Error in opening the read file");
        }
    }

    if (parseCmd->outputRedirect) {
        if (!freopen(parseCmd->outputRedirect, "w", stdout) && debug == 1) {
            perror("Error in opening the write file");
        }
    }
}

void wake_Term_Stop(cmdLine *parseCmd, char *proc, int debug) {
    pid_t pid = atoi(parseCmd->arguments[1]);

    if (strcmp(parseCmd->arguments[0], "wake") == 0) {
        if (kill(pid, SIGCONT) == -1 && debug == 1) {
            perror("Error in sending the signal -wake-");
        }else {
            
            updateProcessStatus(process_list, pid, RUNNING);
            printf("Sent SIGCONT to process %d\n", pid);
        }
    } else if (strcmp(parseCmd->arguments[0], "term") == 0) {
        if (kill(pid, SIGINT) == -1 && debug == 1) {
            perror("Error in sending the signal -term-");
        }else {
            
            updateProcessStatus(process_list, pid, TERMINATED);
            printf("Sent SIGINT to process %d\n", pid);
        }
    } else if (strcmp(parseCmd->arguments[0], "stop") == 0) {
        if (kill(pid, SIGTSTP) == -1 && debug == 1) {
            perror("Error in sending the signal -stop-");
        }else {
            updateProcessStatus(process_list, pid, SUSPENDED);
            printf("Sent SIGTSTP to process %d\n", pid);
        }
    }
}

void execute_pipeline(cmdLine *pipeline) {
    if (pipeline == NULL || pipeline->next == NULL) {
        fprintf(stderr, "Error: Invalid pipeline structure.\n");
        return;
    }

    int pipe_fd[2];
    if (pipe(pipe_fd) == -1) {
        perror("Failed to create pipe");
        exit(EXIT_FAILURE);
    }

    // Create the first child process
    pid_t child1 = fork();
    if (child1 == -1) {
        perror("Fork failed for child1");
        exit(EXIT_FAILURE);
    }

    if (child1 == 0) {
        // In child1: write end of the pipeline
        close(pipe_fd[0]); // Close unused read end
        dup2(pipe_fd[1], STDOUT_FILENO); // Redirect stdout to pipe
        close(pipe_fd[1]); // Close write end after redirection
        execvp(pipeline->arguments[0], pipeline->arguments);
        perror("Execvp failed in child1");
        exit(EXIT_FAILURE);
    }

    // Create the second child process
    pid_t child2 = fork();
    if (child2 == -1) {
        perror("Fork failed for child2");
        exit(EXIT_FAILURE);
    }

    if (child2 == 0) {
        // In child2: read end of the pipeline
        close(pipe_fd[1]); // Close unused write end
        dup2(pipe_fd[0], STDIN_FILENO); // Redirect stdin from pipe
        close(pipe_fd[0]); // Close read end after redirection
        execvp(pipeline->next->arguments[0], pipeline->next->arguments);
        perror("Execvp failed in child2");
        exit(EXIT_FAILURE);
    }

    // In parent: Close both ends of the pipe and wait for children
    close(pipe_fd[0]);
    close(pipe_fd[1]);

    // Wait for both children to finish
    waitpid(child1, NULL, 0);
    waitpid(child2, NULL, 0);
}
