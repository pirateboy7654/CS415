#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "../account.h"
#include "../string_parser.h"

// func declarations
void read_input(const char *filename);
void* thread_process_transactions(void* arg);
void write_output(const char *filename);
void* update_balance(void* arg);

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

int main(int argc, char *argv[]) {
    if (argc != 2) { //wrong arg input check
        fprintf(stderr, "Usage: %s <input file>\n", argv[0]);
        return 1;
    }

    pthread_t threads[num_threads];
    int thread_ids[num_threads];

    // init mutex for transaction counter
    pthread_mutex_init(&transaction_counter_lock, NULL);

    read_input(argv[1]);

    // create all threads
    for (int i = 0; i < num_threads; i++) {
        thread_ids[i] = i;
        pthread_create(&threads[i], NULL, thread_process_transactions, &thread_ids[i]);
    }
    // join threads
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    update_balance(NULL);
    write_output("output.txt");

    // cleanup mutexes
    for (int i = 0; i < num_accounts; i++) {
        pthread_mutex_destroy(&accounts[i].ac_lock);
    }
    pthread_mutex_destroy(&transaction_counter_lock);

    return 0;
}

// read accs and transactions from input file
void read_input(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) { // file open error check
        perror("Failed to open input file");
        exit(1);
    }

    char buffer[256];
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
        snprintf(accounts[i].out_file, sizeof(accounts[i].out_file),"account_%s.txt", accounts[i].account_number);
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
    fclose(file);
}

void* thread_process_transactions(void* arg) {
    int thread_id = *(int*)arg;

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

    int found_account = 0; 
    for (int i = start; i < end; i++) {
        transaction *t = &transactions[i];
        found_account = 0; 
        for (int j = 0; j < num_accounts; j++) {
            account *acc = &accounts[j];

            if (strcmp(acc->account_number, t->src_account) == 0) {
                found_account = 1;
                if (strcmp(acc->password, t->password) != 0) {
                    printf("Invalid password for account %s\n", acc->account_number);
                    break; }

                pthread_mutex_lock(&acc->ac_lock); // Lock the account before modifying it
                if (t->type == 'D') {
                    acc->balance += t->amount;
                    acc->transaction_tracter += t->amount;
                } else if (t->type == 'W') {
                    acc->balance -= t->amount;
                    acc->transaction_tracter += t->amount;
                } else if (t->type == 'T') {
                    acc->balance -= t->amount;
                    acc->transaction_tracter += t->amount;

                    // Update the destination account
                    for (int k = 0; k < num_accounts; k++) {
                        if (strcmp(accounts[k].account_number, t->dest_account) == 0) {
                            pthread_mutex_lock(&accounts[k].ac_lock);
                            accounts[k].balance += t->amount;
                            pthread_mutex_unlock(&accounts[k].ac_lock);
                            break;
                        }}}
                pthread_mutex_unlock(&acc->ac_lock); // Unlock the account
                break;
    }   
    if (found_account == 0) {
        //printf("src acc %s not found\n", t->src_account);
    }
    }}

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
        pthread_mutex_unlock(&accounts[i].ac_lock); // unlock thread
    }
    return NULL;
}


