#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <signal.h>
#include <sys/wait.h>

#define max_processes 100  // max processes
int max_args = 10;      // max arguments
int time_slice = 1; // time slice for scheduling
int process_count = 0; 
int current_process = 0; 
pid_t pid_array[max_processes]; // array of processes

//function declarations
void execute_commands(const char *filename);
void scheduler(int signum);

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <input file>\n", argv[0]);
        exit(1);  // error check
    }

    // set up the SIGALRM handler
    signal(SIGALRM, scheduler);

    execute_commands(argv[1]);

    // send SIGUSR1 to all children to start them waiting for SIGCONT
    for (int j = 0; j < process_count; j++) {
        kill(pid_array[j], SIGUSR1);
    }

    // start first process
    printf("Starting process %d\n", pid_array[current_process]);
    kill(pid_array[current_process], SIGCONT);

    // start scheduling timer
    alarm(time_slice);

    // wait for all child processes to finish
    for (int j = 0; j < process_count; j++) {
        int status;
        if (waitpid(pid_array[j], &status, 0) == -1) {
            perror("Waitpid failed");
        } // error check
    }

    printf("Exiting now\n");
    return 0;
}

void execute_commands(const char *filename) {
    FILE *file = fopen(filename, "r"); 
    if (file == NULL) {
        perror("Error opening file");
        exit(1); // error check
    }
    
    char line[1024];
    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGUSR1);
    // block SIGUSR1 in the parent to avoid premature handling
    sigprocmask(SIG_BLOCK, &sigset, NULL);

    while (fgets(line, sizeof(line), file)) {
        // strip newline character
        line[strcspn(line, "\n")] = '\0';
        
        // tokenize the line
        char *arguments[max_args];  // adjust size based on max args
        int i = 0;
        arguments[i] = strtok(line, " ");
        while (arguments[i] != NULL && i < (max_args - 1)) {
            arguments[++i] = strtok(NULL, " ");
        }
        arguments[i] = NULL;  // null terminate the argument array

        // fork a new process
        pid_t pid = fork();
        if (pid < 0) {
            perror("Fork failed");
            exit(1); // error check
        } else if (pid == 0) {
            // child process wait for SIGUSR1 before calling exec
            printf("Child %d waiting for SIGUSR1\n", getpid());
            int sig; 
            sigwait(&sigset, &sig); // wait for SIGUSR1

            // debug
            //printf("Child %d received SIGUSR1 and is now executing command\n", getpid());
            // call exec after recieving SIGUSR1
            if (execvp(arguments[0], arguments) == -1) {
                perror("Exec failed");
                exit(1); // error check
            }
        } else {
            // parent process
            pid_array[process_count++] = pid; //store pid of child process
        }
    }
    fclose(file);
}

void scheduler(int signum) {
    // stop current process
    printf("Stopping process %d\n", pid_array[current_process]);
    kill(pid_array[current_process], SIGSTOP);

    // move to next process
    current_process = (current_process + 1) % process_count;

    // resume next process
    printf("Resuming process %d\n", pid_array[current_process]);
    kill(pid_array[current_process], SIGCONT);

    // reset alarm for next time slice
    alarm(time_slice);
}