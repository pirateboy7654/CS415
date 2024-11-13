#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <signal.h>
#include <sys/wait.h>

#define max_processes 100  // max processes
int max_args = 10;      // max arguments
int time_slice = 2; // time slice for scheduling
int process_count = 0; 
int current_process = 0;
pid_t pid_array[max_processes];

//function declaration
void execute_commands(const char *filename);
void scheduler(int signum);
int get_mem_usage(pid_t pid);
long get_exec_time(pid_t pid);
long get_io_bytes_read(pid_t pid);



int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <input file>\n", argv[0]);
        exit(1);
    }

    // Set up the SIGALRM handler
    signal(SIGALRM, scheduler);

    execute_commands(argv[1]);

    // Send SIGUSR1 to all children to start them waiting for SIGCONT
    for (int j = 0; j < process_count; j++) {
        kill(pid_array[j], SIGUSR1);
    }

    // Start the first process
    printf("Starting process %d\n", pid_array[current_process]);
    kill(pid_array[current_process], SIGCONT);

    // Start the scheduling timer
    alarm(time_slice);

    // Wait for all child processes to finish
    for (int j = 0; j < process_count; j++) {
        int status;
        if (waitpid(pid_array[j], &status, 0) == -1) {
            perror("Waitpid failed");
        }
    }

    printf("Exiting now\n");
    return 0;
}


void execute_commands(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("Error opening file");
        exit(1);
    }

    
    char line[1024];
    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGUSR1);
    // Block SIGUSR1 in the parent to avoid premature handling
    sigprocmask(SIG_BLOCK, &sigset, NULL);

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
            // Child process wait for SIGUSR1 before calling exec
            printf("Child %d waiting for SIGUSR1\n", getpid());
            int sig; 
            sigwait(&sigset, &sig); // wait for SIGUSR1

            // debug
            printf("Child %d received SIGUSR1 and is now executing command\n", getpid());
            // call exec after recieving SIGUSR1
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
}

// SIGALRM handler for scheduling and displaying resource usage
void scheduler(int signum) {
    // Stop the current process if it's still running
    if (waitpid(pid_array[current_process], NULL, WNOHANG) == 0) {
        printf("Stopping process %d\n", pid_array[current_process]);
        kill(pid_array[current_process], SIGSTOP);
    }

    // Move to the next process in a round-robin fashion, skipping completed processes
    do {
        current_process = (current_process + 1) % process_count;
    } while (waitpid(pid_array[current_process], NULL, WNOHANG) != 0);  // Skip completed processes

    // Resume the next active process
    printf("Resuming process %d\n", pid_array[current_process]);
    kill(pid_array[current_process], SIGCONT);

    // Display resource usage for each process
    int memory;
    long cpu_time;
    long io_read;

    printf("\nResource Usage:\n");
    printf("-------------------------------------------------------------\n");
    printf("PID        | Memory (KB)   | CPU Time (ticks)   | I/O Read (bytes)\n");
    printf("-------------------------------------------------------------\n");
    for (int i = 0; i < process_count; i++) {
        if (waitpid(pid_array[i], NULL, WNOHANG) == 0) {  // Check if process is still active
            memory = get_mem_usage(pid_array[i]);
            cpu_time = get_exec_time(pid_array[i]);
            io_read = get_io_bytes_read(pid_array[i]);    
            printf("%-10d | %-12d | %-16ld | %-14ld\n", 
                pid_array[i], memory, cpu_time, io_read);  
        }
    }
    printf("-------------------------------------------------------------\n");

    // Reset the alarm for the next time slice
    alarm(time_slice);
}

// Function to retrieve memory usage from /proc/[pid]/status
int get_mem_usage(pid_t pid) {
    char path[40];
    snprintf(path, sizeof(path), "/proc/%d/status", pid);
    FILE *file = fopen(path, "r");
    if (!file) {
        perror("Could not open /proc/[pid]/status");
        return -1;
    }

    int memory = 0;
    char line[256];
    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "VmRSS:", 6) == 0) {  // Look for VmRSS for resident set size
            sscanf(line, "VmRSS: %d", &memory);
            break;
        }
    }
    fclose(file);
    return memory;  // Memory in kilobytes
}

// Function to retrieve execuction time from /proc/[pid]/stat
long get_exec_time(pid_t pid) {
    char path[40];
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);
    FILE *file = fopen(path, "r");
    if (!file) {
        perror("Could not open /proc/[pid]/stat");
        return -1;
    }

    long utime, stime;
    for (int i = 1; i <= 13; i++) {  // Skip the first 13 fields
        fscanf(file, "%*s");
    }
    fscanf(file, "%ld %ld", &utime, &stime);  // Fields 14 and 15
    fclose(file);
    printf("Debug: CPU time for PID %d: %ld ticks\n", pid, (utime+stime));  // Add this debug line

    return utime + stime;  // Total CPU time in clock ticks
}

// Function to retrieve I/O read bytes from /proc/[pid]/io
long get_io_bytes_read(pid_t pid) {
    char path[40];
    snprintf(path, sizeof(path), "/proc/%d/io", pid);
    FILE *file = fopen(path, "r");
    if (!file) {
        perror("Could not open /proc/[pid]/io");
        return -1;
    }

    long bytes_read = 0;
    char line[256];
    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "read_bytes:", 11) == 0) {
            sscanf(line, "read_bytes: %ld", &bytes_read);
            break;
        }
    }
    fclose(file);
    return bytes_read;  // Total bytes read by the process
}
