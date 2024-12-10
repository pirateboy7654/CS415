#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "../account.h"
#include "../string_parser.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/wait.h"

// func declarations
void read_input(const char *filename);
void* thread_process_transactions(void* arg);
void write_output(const char *filename);
void* update_balance(void* arg);
void auditor_process(int read_fd);

#define max_accounts 11
#define max_transactions 120052

// global arrays
account accounts[max_accounts];
transaction transactions[max_transactions];

// counters
int num_accounts = 0;
int num_transactions = 0;
pthread_mutex_t transaction_counter_lock;
int transaction_counter = 0;
int num_threads = 10;
int pipe_fd[2]; // file descriptors for the pipe

int main(int argc, char *argv[]) {
    if (argc != 2) { //wrong arg input check
        fprintf(stderr, "Usage: %s <input file>\n", argv[0]);
        return 1;
    }

    pthread_t threads[num_threads];
    int thread_ids[num_threads];

    // init mutex for transaction counter
    pthread_mutex_init(&transaction_counter_lock, NULL);

    // Create a pipe for communication with the auditor
    if (pipe(pipe_fd) == -1) {
        perror("Pipe creation failed");
        exit(1);
    }

    // Fork to create the auditor process
    pid_t pid = fork();
    if (pid < 0) {
        perror("Fork failed");
        exit(1);
    } else if (pid == 0) {
        // Child process: Auditor
        close(pipe_fd[1]); // Close unused write end
        auditor_process(pipe_fd[0]); // Auditor reads from the pipe
        exit(0);
    }

    // Parent process: Duck Bank
    close(pipe_fd[0]); // Close unused read end
    read_input(argv[1]);
    printf("after read input num trans : %d\n", num_transactions);

    // create all threads
    printf("creating threads\n");
    for (int i = 0; i < num_threads; i++) {
        thread_ids[i] = i;
        pthread_create(&threads[i], NULL, thread_process_transactions, &thread_ids[i]);
    }
    // join threads
    printf("joining threads\n");
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    printf("calling update balance\n");
    update_balance(NULL);

    printf("calling write output\n");
    write_output("output.txt");

    // Signal auditor process to stop by closing the write end of the pipe
    printf("attemping to close pipe \n");
    close(pipe_fd[1]);
    printf("pipe closed\n");
    // Wait for the auditor process to finish
    wait(NULL);


    // cleanup mutexes
    for (int i = 0; i < num_accounts; i++) {
        pthread_mutex_destroy(&accounts[i].ac_lock);
    }
    pthread_mutex_destroy(&transaction_counter_lock);

    return 0;
}

// read accs and transactions from input file
void read_input(const char *filename) {
    printf("Attempting to open file: %s\n", filename);
    FILE *file = fopen(filename, "r");
    if (!file) { // file open error check
        perror("Failed to open input file");
        exit(1);
    }

    char buffer[10024];
    const char *delim = " "; // delim for transactions

    // read number of accounts (first line)
    if (fgets(buffer, sizeof(buffer), file)) {
        command_line cmd = str_filler(buffer, delim);
        num_accounts = atoi(cmd.command_list[0]);
        free_command_line(&cmd);
    }

    // read account details
    for (int i = 0; i < num_accounts; i++) {
        // skip the "index #" line
        if (fgets(buffer, sizeof(buffer), file)) {
            // nothing
        }

        // read account number & copy to struct object
        if (fgets(buffer, sizeof(buffer), file)) {
            buffer[strcspn(buffer, "\n")] = 0; // strip newline char
            strcpy(accounts[i].account_number, buffer);
        }

        // read password & copy to struct object
        if (fgets(buffer, sizeof(buffer), file)) {
            buffer[strcspn(buffer, "\n")] = 0; // strip newline char
            strcpy(accounts[i].password, buffer);
        }

        // read balance & copy to struct object
        if (fgets(buffer, sizeof(buffer), file)) {
            accounts[i].balance = atof(buffer);
        }

        // read reward rate & copy to struct object
        if (fgets(buffer, sizeof(buffer), file)) {
            accounts[i].reward_rate = atof(buffer);
        }

        // init other fields
        accounts[i].transaction_tracter = 0.0;
        char temp_out_file[10024];
        snprintf(temp_out_file, sizeof(temp_out_file),"account_%s.txt", accounts[i].account_number);
        strncpy(accounts[i].out_file, temp_out_file, sizeof(accounts[i].out_file) - 1);
        accounts[i].out_file[sizeof(accounts[i].out_file) - 1] = '\0'; // null terminate

        pthread_mutex_init(&accounts[i].ac_lock, NULL);
    }

    // read transactions
    while (fgets(buffer, sizeof(buffer), file)) {
        command_line cmd = str_filler(buffer, delim); // get line and tokens
        transaction *t = &transactions[num_transactions]; 

        t->type = cmd.command_list[0][0]; // first char is type (T D W C)
        strcpy(t->src_account, cmd.command_list[1]); // copy account to t
        strcpy(t->password, cmd.command_list[2]); // copy password to t
        // changes based on type, for number or args given 
        if (t->type == 'T') { // transfer
            strcpy(t->dest_account, cmd.command_list[3]);
            t->amount = atof(cmd.command_list[4]);
        } else if (t->type == 'D' || t->type == 'W') { // deposit or withdraw
            t->amount = atof(cmd.command_list[3]);
        }

        num_transactions++;
        free_command_line(&cmd);
    }
    printf("num_transactions : %d\n", num_transactions);    
    fclose(file);
}

void* thread_process_transactions(void* arg) {

    int thread_id = *(int*)arg;
    printf("Thread %d started.\n", thread_id);
    // divide transactions among threads
    int transactions_per_thread = num_transactions / num_threads;
    int start = thread_id * transactions_per_thread;
    int end;
    if (thread_id == num_threads - 1) {
        end = num_transactions; // last thread processes all remaining transactions
    } else {
        end = start + transactions_per_thread; // first 9 split
    }
    printf("Thread %d processing transactions from %d to %d\n", thread_id, start, end);

    //int found_account = 0; 
    for (int i = start; i < end; i++) {
        transaction *t = &transactions[i];
        //found_account = 0; 
        for (int j = 0; j < num_accounts; j++) {
            account *acc = &accounts[j];

            if (strcmp(acc->account_number, t->src_account) == 0) {
                //found_account = 1;
                if (strcmp(acc->password, t->password) != 0) {
                    //printf("Invalid password for account %s\n", acc->account_number);
                    break; }

                // Successful transaction processing
                
                pthread_mutex_lock(&acc->ac_lock); // Lock the account before modifying it
                

                printf("Thread %d locked account %s\n", thread_id, acc->account_number);

                if (t->type == 'D') {
                    acc->balance += t->amount;
                    acc->transaction_tracter += t->amount;
                } else if (t->type == 'W') {
                    acc->balance -= t->amount;
                    acc->transaction_tracter += t->amount;
                } else if (t->type == 'T') {
                    

                    // Update the destination account
                    for (int k = 0; k < num_accounts; k++) {
                        if (strcmp(accounts[k].account_number, t->dest_account) == 0) {
                            //pthread_mutex_lock(&accounts[k].ac_lock);
                            if (strcmp(acc->account_number, accounts[k].account_number) < 0) {
                                pthread_mutex_lock(&acc->ac_lock);        // Lock the smaller account first
                                pthread_mutex_lock(&accounts[k].ac_lock); // Lock the larger account second
                            } else {
                                pthread_mutex_lock(&accounts[k].ac_lock); // Lock the smaller account first
                                pthread_mutex_lock(&acc->ac_lock);        // Lock the larger account second
                            }
                            printf("Thread %d locked account %s\n", thread_id, acc->account_number);
                            acc->balance -= t->amount;
                            acc->transaction_tracter += t->amount;
                            accounts[k].balance += t->amount;
                            pthread_mutex_unlock(&accounts[k].ac_lock);
                            pthread_mutex_unlock(&acc->ac_lock);

                            printf("Thread %d unlocked account %s\n", thread_id, acc->account_number);
                            break;
                        }}
                } else if (t->type == 'C') {
                    pthread_mutex_lock(&transaction_counter_lock);
                    transaction_counter++;
                    if (transaction_counter % 500 == 0) {
                        char log_entry[10024];
                        time_t now = time(NULL);
                        snprintf(log_entry, sizeof(log_entry),
                                "Worker checked balance of Account %s. Balance is %.2f. Check ocurred at %s",
                                acc->account_number, acc->balance, ctime(&now)); // 
                        log_entry[sizeof(log_entry) - 1] = '\0'; // null terminate
                        if (write(pipe_fd[1], log_entry, strlen(log_entry) + 1) == -1) {
                            perror("Write to pipe failed");
                        }
                    }
                    pthread_mutex_unlock(&transaction_counter_lock);
                    printf("Thread %d unlocked trans counter %s\n", thread_id, acc->account_number);

                }
                pthread_mutex_unlock(&acc->ac_lock); // Unlock the account
                printf("Thread %d unlocked account %s\n", thread_id, acc->account_number);

                break;
    }   
    /*if (found_account == 0) {
        //printf("src acc %s not found\n", t->src_account);
    }*/
    }}
    printf("thread %d finished\n", thread_id);
    return NULL;
}

// write final balances to output file
void write_output(const char *filename) {
    FILE *file = fopen(filename, "w");
    if (!file) { // error check for opening output file
        perror("Failed to open output file");
        exit(1);
    }

    for (int i = 0; i < num_accounts; i++) {
        // write the index and balance in the required format
        fprintf(file, "%d balance: \t%.2f\n", i, accounts[i].balance);
    }

    fclose(file); // close output file 
}

void* update_balance(void* arg) {
    for (int i = 0; i < num_accounts; i++) {
        pthread_mutex_lock(&accounts[i].ac_lock); // lock thread
        //printf("balance for account %d : %.2f\n", i, accounts[i].balance); // debug
        accounts[i].balance += accounts[i].transaction_tracter * accounts[i].reward_rate;
        //printf("balance for account %d : %.2f\n", i, accounts[i].balance); // debug
        accounts[i].transaction_tracter = 0.0; // reset tracker after updating

        char message[10024];
        time_t now = time(NULL);
        snprintf(message, sizeof(message), 
                 "Applied Interest to Account %s. New Balance is %.2f. Time of Update: %s", 
                 accounts[i].account_number, accounts[i].balance, ctime(&now));
        //write(pipe_fd[1], message, strlen(message));
        message[sizeof(message)-1] = '\0'; // null terminate
        if (write(pipe_fd[1], message, strlen(message)) == -1) {
            perror("Write to pipe failed");
        }

        pthread_mutex_unlock(&accounts[i].ac_lock); // unlock thread
    }
    return NULL;
}

void auditor_process(int read_fd) {
    FILE *ledger = fopen("ledger.txt", "w");
    if (!ledger) {
        perror("Failed to open ledger.txt");
        exit(1);
    }

    char buffer[10024];
    ssize_t bytes_read;
    while ((bytes_read = read(read_fd, buffer, sizeof(buffer))) > 0) {
        buffer[bytes_read] = '\0'; // null-terminate the string
        fprintf(ledger, "%s\n", buffer);
    }

    fclose(ledger);
    close(read_fd); // Close the read end of the pipe
}