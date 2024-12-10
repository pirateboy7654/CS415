#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include "../account.h"
#include "../string_parser.h"
#include <sys/stat.h>
#include <sys/types.h>

// function declarations
void read_input(const char *filename);
void* thread_process_transactions(void* arg);
void write_output(const char *filename);
void* update_balance(void* arg);
void* update_savings(void* arg);
void initialize_shared_memory();
void finalize_shared_memory();

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

// synchronization variables 
pthread_barrier_t start_barrier;
pthread_mutex_t threshold_mutex;
pthread_cond_t threshold_cond;
int processed_transactions = 0;
const int TRANSACTION_THRESHOLD = 5000;
int bank_ready = 0;

// shared memory variables
void* shared_mem = NULL;
size_t shared_mem_size = 0;
int shm_fd;

int main(int argc, char *argv[]) {
    if (argc != 2) { // wrong argument input check
        fprintf(stderr, "Usage: %s <input file>\n", argv[0]);
        return 1;
    }

    pthread_t threads[num_threads];
    pthread_t bank_thread, savings_thread;
    int thread_ids[num_threads];

    // initialize synchronization variables
    pthread_barrier_init(&start_barrier, NULL, num_threads + 1);
    pthread_mutex_init(&threshold_mutex, NULL);
    pthread_cond_init(&threshold_cond, NULL);
    pthread_mutex_init(&transaction_counter_lock, NULL);

    read_input(argv[1]);

    // initialize shared memory for communication with puddles Bank
    initialize_shared_memory();

    // create worker threads
    for (int i = 0; i < num_threads; i++) {
        thread_ids[i] = i;
        pthread_create(&threads[i], NULL, thread_process_transactions, &thread_ids[i]);
    }

    // create bank thread
    pthread_create(&bank_thread, NULL, update_balance, NULL);

    // create savings thread
    pthread_create(&savings_thread, NULL, update_savings, NULL);

    // wait for all threads to be ready
    //printf("Main thread reached the barrier.\n");
    pthread_barrier_wait(&start_barrier);
    //printf("Main thread passed the barrier.\n");

    // join worker threads
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    // notify bank thread to finalize
    pthread_mutex_lock(&threshold_mutex);
    if (processed_transactions < num_transactions) {
        //printf("Main thread: Final notification sent to bank thread.\n");
        bank_ready = 1;
        pthread_cond_signal(&threshold_cond);
    }
    pthread_mutex_unlock(&threshold_mutex);

    // join banker and savings threads
    pthread_join(bank_thread, NULL);
    pthread_join(savings_thread, NULL);

    write_output("output.txt");

    // cleanup shared memory
    finalize_shared_memory();

    // cleanup synchronization variables
    pthread_barrier_destroy(&start_barrier);
    pthread_mutex_destroy(&threshold_mutex);
    pthread_cond_destroy(&threshold_cond);
    for (int i = 0; i < num_accounts; i++) {
        pthread_mutex_destroy(&accounts[i].ac_lock);
    }
    pthread_mutex_destroy(&transaction_counter_lock);

    return 0;
}

// read accounts and transactions from input file
void read_input(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) { // file open error check
        perror("Failed to open input file");
        exit(1);
    }

    char buffer[256];
    const char *delim = " "; // delimiter for transactions

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

        // initialize other fields
        accounts[i].transaction_tracter = 0.0;
        accounts[i].savings_balance = accounts[i].balance * 0.2; // 20% initial savings
        accounts[i].savings_reward_rate = 0.02; // fixed savings reward rate
        char temp_out_file[64];
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
    fclose(file);
}

// worker thread function
void* thread_process_transactions(void* arg) {
    int thread_id = *(int*)arg;
    //printf("Thread %d waiting at the barrier.\n", thread_id);
    pthread_barrier_wait(&start_barrier); // wait until all threads are ready
    //printf("Thread %d passed the barrier.\n", thread_id);

    int transactions_per_thread = (num_transactions + num_threads - 1) / num_threads; // round up
    int start = thread_id * transactions_per_thread;
    int end = (start + transactions_per_thread > num_transactions) ? num_transactions : start + transactions_per_thread;
    //printf("Thread %d assigned range: %d to %d\n", thread_id, start, end);

    for (int i = start; i < end; i++) {
        transaction *t = &transactions[i];

        for (int j = 0; j < num_accounts; j++) {
            account *acc = &accounts[j];

            if (strcmp(acc->account_number, t->src_account) == 0) {
                if (strcmp(acc->password, t->password) != 0) {
                    //printf("invalid password for account %s. skipping transaction.\n", acc->account_number);
                    pthread_mutex_lock(&threshold_mutex);
                    processed_transactions++; // increment for skipped transactions
                    pthread_mutex_unlock(&threshold_mutex);
                    break;
                }

                pthread_mutex_lock(&acc->ac_lock); // lock the account before modifying it
                if (t->type == 'D') {
                    acc->balance += t->amount;
                    acc->transaction_tracter += t->amount;
                } else if (t->type == 'W') {
                    acc->balance -= t->amount;
                    acc->transaction_tracter += t->amount;
                } else if (t->type == 'T') {
                    acc->balance -= t->amount;
                    acc->transaction_tracter += t->amount;

                    // update the destination account
                    for (int k = 0; k < num_accounts; k++) {
                        if (strcmp(accounts[k].account_number, t->dest_account) == 0) {
                            pthread_mutex_lock(&accounts[k].ac_lock);
                            accounts[k].balance += t->amount;
                            pthread_mutex_unlock(&accounts[k].ac_lock);
                            break;
                        }
                    }
                }
                pthread_mutex_unlock(&acc->ac_lock); // unlock the account

                // check if transaction threshold is reached
                pthread_mutex_lock(&threshold_mutex);
                processed_transactions++;
                //printf("Thread %d: Processed transaction %d/%d\n", thread_id, processed_transactions, num_transactions);
                if (processed_transactions % TRANSACTION_THRESHOLD == 0 || processed_transactions == num_transactions) {
                    bank_ready = 1;
                    pthread_cond_signal(&threshold_cond);
                    //printf("Thread %d: Notified bank thread.\n", thread_id);
                }
                pthread_mutex_unlock(&threshold_mutex);

                break;
            }
        }
    }

    pthread_mutex_lock(&threshold_mutex);
    if (processed_transactions >= num_transactions) {
        bank_ready = 1;
        pthread_cond_signal(&threshold_cond);
        //printf("Thread %d: All transactions processed. Final notification sent.\n", thread_id);
    } else {
        //printf("Thread %d: Remaining transactions to process: %d.\n", thread_id, num_transactions - processed_transactions);
    }
    pthread_mutex_unlock(&threshold_mutex);

    return NULL;
}
// bank thread function
void* update_balance(void* arg) {
    int total_updates = 0;

    while (1) {
        pthread_mutex_lock(&threshold_mutex);
        while (!bank_ready) {
            if (processed_transactions >= num_transactions) {
                pthread_mutex_unlock(&threshold_mutex);
                return NULL; // exit the thread
            }
            pthread_cond_wait(&threshold_cond, &threshold_mutex);
        }
        bank_ready = 0; // reset the flag for the next round
        pthread_mutex_unlock(&threshold_mutex);

        // update Duck Bank balances
        for (int i = 0; i < num_accounts; i++) {
            pthread_mutex_lock(&accounts[i].ac_lock);
            accounts[i].balance += accounts[i].transaction_tracter * accounts[i].reward_rate;
            accounts[i].transaction_tracter = 0.0;
            pthread_mutex_unlock(&accounts[i].ac_lock);
        }

        // write individual account updates to files in "output/"
        for (int i = 0; i < num_accounts; i++) {
            char file_path[256];
            snprintf(file_path, sizeof(file_path), "output/account_%s.txt", accounts[i].account_number);

            FILE *file = fopen(file_path, "a");
            if (file) {
                fprintf(file, "Current Balance: \t%.2f\n", accounts[i].balance);
                fclose(file);
            }
        }

        // notify Puddles Bank to update savings balances
        pthread_mutex_lock(&threshold_mutex);
        //printf("Bank thread: Completed update %d. Notifying Puddles Bank.\n", total_updates);
        pthread_mutex_unlock(&threshold_mutex);

        total_updates++;
    }
}

// savings thread function
void* update_savings(void* arg) {
    mkdir("savings", 0777); // ensure savings directory exists

    while (1) {
        pthread_mutex_lock(&threshold_mutex);
        while (!bank_ready) {
            if (processed_transactions >= num_transactions) {
                pthread_mutex_unlock(&threshold_mutex);
                return NULL; // exit thread
            }
            pthread_cond_wait(&threshold_cond, &threshold_mutex);
        }
        bank_ready = 0; // reset the flag
        pthread_mutex_unlock(&threshold_mutex);

        // apply interest to savings accounts
        for (int i = 0; i < num_accounts; i++) {
            pthread_mutex_lock(&accounts[i].ac_lock);
            accounts[i].savings_balance += accounts[i].savings_balance * accounts[i].savings_reward_rate;
            pthread_mutex_unlock(&accounts[i].ac_lock);
        }

        // write individual savings updates to files in "savings/"
        for (int i = 0; i < num_accounts; i++) {
            char file_path[256];
            snprintf(file_path, sizeof(file_path), "savings/account_%s_savings.txt", accounts[i].account_number);

            FILE *file = fopen(file_path, "a");
            if (file) {
                fprintf(file, "Current Savings Balance \t%.2f\n", accounts[i].savings_balance);
                fclose(file);
            }
        }

        //printf("Savings thread: Interest applied to all accounts.\n");
    }
}
// write final balances to output file
void write_output(const char *filename) {
    mkdir("output", 0777); // ensure output directory exists

    // write final balances to output/output.txt
    char output_file[256];
    snprintf(output_file, sizeof(output_file), "output/%s", filename);
    FILE *file = fopen(output_file, "w");
    if (!file) {
        perror("Failed to open output file");
        exit(1);
    }

    for (int i = 0; i < num_accounts; i++) {
        fprintf(file, "%d balance: \t%.2f\n", i + 1, accounts[i].balance);
    }
    fclose(file);

    // write final savings balances to savings/savings_output.txt
    mkdir("savings", 0777); // ensure savings directory exists
    char savings_file[64];
    snprintf(savings_file, sizeof(savings_file), "savings/savings_output.txt");
    FILE *savings_output = fopen(savings_file, "w");
    if (!savings_output) {
        perror("Failed to open savings output file");
        exit(1);
    }

    for (int i = 0; i < num_accounts; i++) {
        fprintf(savings_output, "Account %d: Savings Final Balance: %.2f\n", i + 1, accounts[i].savings_balance);
    }
    fclose(savings_output);
}



// initialize shared memory
void initialize_shared_memory() {
    shm_fd = shm_open("/duckbank_shared", O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("Failed to create shared memory");
        exit(1);
    }

    shared_mem_size = num_accounts * sizeof(account);

    if (ftruncate(shm_fd, shared_mem_size) == -1) {
        perror("Failed to set size of shared memory");
        exit(1);
    }

    shared_mem = mmap(NULL, shared_mem_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_mem == MAP_FAILED) {
        perror("Failed to map shared memory");
        exit(1);
    }

    // copy account data to shared memory
    memcpy(shared_mem, accounts, shared_mem_size);
    //printf("Shared memory initialized and account data written.\n");
}

// finalize shared memory
void finalize_shared_memory() {
    if (munmap(shared_mem, shared_mem_size) == -1) {
        perror("Failed to unmap shared memory");
    }
    if (shm_unlink("/duckbank_shared") == -1) {
        perror("Failed to unlink shared memory");
    }
    //printf("Shared memory finalized and cleaned up.\n");
}
