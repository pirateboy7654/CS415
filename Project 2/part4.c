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
int get_mem_usage(pid_t pid);
long get_exec_time(pid_t pid);
int get_context_switches(pid_t pid);

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <input file>\n", argv[0]);
        exit(1); // error check
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
        exit(1);
    } // error check

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
        } 
        else if (pid == 0) {
            // Ccild process wait for SIGUSR1 before calling exec
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
    printf("Scheduler called\n");
    // stop current process if still running
    if (waitpid(pid_array[current_process], NULL, WNOHANG) == 0) {
        printf("Stopping process %d\n", pid_array[current_process]);
        kill(pid_array[current_process], SIGSTOP);
    }

    // move to the next process in round robin algorithm, skipping completed processes
    current_process = (current_process + 1) % process_count;
    while (waitpid(pid_array[current_process], NULL, WNOHANG) != 0) 
    {
        current_process = (current_process + 1) % process_count;
    }
    // resume next active process
    printf("Resuming process %d\n", pid_array[current_process]);
    kill(pid_array[current_process], SIGCONT);

    // display resource usage for each process
    // combined tab and character width specifier to make table
    int memory;
    long cpu_time;
    int switches;

    printf("\nResource Usage:\n");
    printf("----------------------------------------------------------------\n");
    printf("PID\tMemory (KB)\tCPU Time (ticks)\tContext Switches\n");
    printf("---\t-----------\t----------------\t----------------\n");
    for (int i = 0; i < process_count; i++) {
        if (waitpid(pid_array[i], NULL, WNOHANG) == 0) {  // check if process is still active
            memory = get_mem_usage(pid_array[i]); // call getter functions 
            cpu_time = get_exec_time(pid_array[i]);
            switches = get_context_switches(pid_array[i]);    
            printf("%-10d %-12d \t%-14ld \t\t%-16ld\n",
                pid_array[i], memory, cpu_time, switches);  
        }
    }
    printf("----------------------------------------------------------------\n");

    // reset alarm for the next time slice
    alarm(time_slice);
}

// getter function to retrieve memory usage from /proc/[pid]/status
int get_mem_usage(pid_t pid) {
    char path[40];
    snprintf(path, sizeof(path), "/proc/%d/status", pid);
    FILE *file = fopen(path, "r");
    if (!file) {
        perror("Could not open /proc/[pid]/status");
        return -1; // error check
    }

    int memory = 0;
    char line[1024];
    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "VmRSS:", 6) == 0) {  // look for VmRSS for resident set size
            sscanf(line, "VmRSS: %d", &memory); // found it!
            break;
        }
    }
    fclose(file);
    return memory;  // return memory in KBs
}

// getter function to retrieve execuction time from /proc/[pid]/stat
long get_exec_time(pid_t pid) {
    char path[40];
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);
    FILE *file = fopen(path, "r");
    if (!file) {
        perror("Could not open /proc/[pid]/stat");
        return -1; // error check
    }

    long utime = 0; // user time
    long stime = 0; // system time

    char temp[1024];
    for (int i = 1; i <= 13; i++) {  // skip the first 13 fields
        fscanf(file, "%s", temp);
        //printf("debug: field %d: %s\n", i, temp);  // debug
    }
    fscanf(file, "%ld %ld", &utime, &stime);  // fields 14 and 15
    fclose(file);
    //printf("debug: CPU time for PID %d: %ld ticks\n", pid, (utime+stime)); // debug

    return utime + stime;  // total CPU time in ticks (combine both)
}

// function to get number of context switches (voluntary)
int get_context_switches(pid_t pid) {
    char path[40];
    snprintf(path, sizeof(path), "/proc/%d/status", pid);
    FILE *file = fopen(path, "r");
    if (!file) {
        perror("Could not open /proc/[pid]/status");
        return -1; // error check
    }
    int switches = 0;
    char line[1024];
    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "voluntary_ctxt_switches:", 24) == 0) { // found line
            sscanf(line, "voluntary_ctxt_switches: %d", &switches);
        }
    }
    fclose(file);
    return switches;
}