#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/wait.h>

//function declaration
void execute_commands(const char *filename);

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <input file>\n", argv[0]);
        exit(1);
    }
    execute_commands(argv[1]);
    return 0;
}

void execute_commands(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("Error opening file");
        exit(1);
    }

    int max_args = 10;      // max arguments
    int max_processes = 100;  // max processes
    char line[1024];
    pid_t pid_array[max_processes];
    int process_count = 0; 

    while (fgets(line, sizeof(line), file)) {
        // Remove newline character
        line[strcspn(line, "\n")] = '\0';
        
        // Tokenize the line
        char *arguments[max_args];  // Adjust size based on expected arguments
        int i = 0;
        arguments[i] = strtok(line, " ");
        while (arguments[i] != NULL && i < (max_args - 1)) {
            arguments[++i] = strtok(NULL, " ");
        }
        arguments[i] = NULL;  // Null-terminate the argument array

        // Fork a new process
        pid_t pid = fork();
        if (pid < 0) {
            perror("Fork failed");
            exit(1);
        } else if (pid == 0) {
            // Child process
            if (execvp(arguments[0], arguments) == -1) {
                perror("Exec failed");
                exit(1);
            }
        } else {
            // Parent process
            pid_array[process_count++] = pid; //store pid of child process
        }
    }
    fclose(file);

    //parents waits for each process to finish
    for (int j = 0; j < process_count; j++) {
        int status;
        if (waitpid(pid_array[j], &status, 0) == -1) {
            perror("waitpid failed");
        }
    }
    // exit after all child processes are completed
    exit(0);
}


