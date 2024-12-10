#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include "../account.h"
#include "../string_parser.h"

// Function declarations
void read_input(const char *filename);
void* thread_process_transactions(void* arg);
void write_output(const char *filename);
void* update_balance(void* arg);
void initialize_shared_memory();
void finalize_shared_memory();

#define max_accounts 11
#define max_transactions 120052

// Global arrays
account accounts[max_accounts];
transaction transactions[max_transactions];

// Counters
int num_accounts = 0;
int num_transactions = 0;
pthread_mutex_t transaction_counter_lock;
int transaction_counter = 0;
int num_threads = 10;

// Synchronization variables for Part 3
pthread_barrier_t start_barrier;
pthread_mutex_t threshold_mutex;
pthread_cond_t threshold_cond;
int processed_transactions = 0;
const int TRANSACTION_THRESHOLD = 5000;
int bank_ready = 0;

// Shared memory variables
void* shared_mem = NULL;
size_t shared_mem_size = 0;
int shm_fd;

int main(int argc, char *argv[]) {
    if (argc != 2) { // Wrong argument input check
        fprintf(stderr, "Usage: %s <input file>\n", argv[0]);
        return 1;
    }

    pthread_t threads[num_threads];
    pthread_t bank_thread;
    int thread_ids[num_threads];

    // Initialize synchronization variables
    pthread_barrier_init(&start_barrier, NULL, num_threads + 1);
    pthread_mutex_init(&threshold_mutex, NULL);
    pthread_cond_init(&threshold_cond, NULL);
    pthread_mutex_init(&transaction_counter_lock, NULL);

    read_input(argv[1]);

    // Initialize shared memory for communication with Puddles Bank
    initialize_shared_memory();

    // Create worker threads
    for (int i = 0; i < num_threads; i++) {
        thread_ids[i] = i;
        pthread_create(&threads[i], NULL, thread_process_transactions, &thread_ids[i]);
    }

    // Create bank thread
    pthread_create(&bank_thread, NULL, update_balance, NULL);

    // Wait for all threads to be ready
    printf("Main thread reached the barrier.\n");
    pthread_barrier_wait(&start_barrier);
    printf("Main thread passed the barrier.\n");

    // Join worker threads
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    // Notify bank thread to finalize
    pthread_mutex_lock(&threshold_mutex);
    if (processed_transactions < num_transactions) {
        printf("Main thread: Final notification sent to bank thread.\n");
        bank_ready = 1;
        pthread_cond_signal(&threshold_cond);
    }
    pthread_mutex_unlock(&threshold_mutex);

    pthread_join(bank_thread, NULL);

    write_output("output.txt");

    // Cleanup shared memory
    finalize_shared_memory();

    // Cleanup synchronization variables
    pthread_barrier_destroy(&start_barrier);
    pthread_mutex_destroy(&threshold_mutex);
    pthread_cond_destroy(&threshold_cond);
    for (int i = 0; i < num_accounts; i++) {
        pthread_mutex_destroy(&accounts[i].ac_lock);
    }
    pthread_mutex_destroy(&transaction_counter_lock);

    return 0;
}

// Read accounts and transactions from input file
void read_input(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) { // File open error check
        perror("Failed to open input file");
        exit(1);
    }

    char buffer[256];
    const char *delim = " "; // Delimiter for transactions

    // Read number of accounts (first line)
    if (fgets(buffer, sizeof(buffer), file)) {
        command_line cmd = str_filler(buffer, delim);
        num_accounts = atoi(cmd.command_list[0]);
        free_command_line(&cmd);
    }

    // Read account details
    for (int i = 0; i < num_accounts; i++) {
        // Skip the "index #" line
        if (fgets(buffer, sizeof(buffer), file)) {
            // Nothing
        }

        // Read account number & copy to struct object
        if (fgets(buffer, sizeof(buffer), file)) {
            buffer[strcspn(buffer, "\n")] = 0; // Strip newline char
            strcpy(accounts[i].account_number, buffer);
        }

        // Read password & copy to struct object
        if (fgets(buffer, sizeof(buffer), file)) {
            buffer[strcspn(buffer, "\n")] = 0; // Strip newline char
            strcpy(accounts[i].password, buffer);
        }

        // Read balance & copy to struct object
        if (fgets(buffer, sizeof(buffer), file)) {
            accounts[i].balance = atof(buffer);
        }

        // Read reward rate & copy to struct object
        if (fgets(buffer, sizeof(buffer), file)) {
            accounts[i].reward_rate = atof(buffer);
        }

        // Initialize other fields
        accounts[i].transaction_tracter = 0.0;
        char temp_out_file[64];
        snprintf(temp_out_file, sizeof(temp_out_file),"account_%s.txt", accounts[i].account_number);
        strncpy(accounts[i].out_file, temp_out_file, sizeof(accounts[i].out_file) - 1);
        accounts[i].out_file[sizeof(accounts[i].out_file) - 1] = '\0'; // Null terminate
        pthread_mutex_init(&accounts[i].ac_lock, NULL);
    }

    // Read transactions
    while (fgets(buffer, sizeof(buffer), file)) {
        command_line cmd = str_filler(buffer, delim); // Get line and tokens
        transaction *t = &transactions[num_transactions]; 

        t->type = cmd.command_list[0][0]; // First char is type (T D W C)
        strcpy(t->src_account, cmd.command_list[1]); // Copy account to t
        strcpy(t->password, cmd.command_list[2]); // Copy password to t
        // Changes based on type, for number or args given 
        if (t->type == 'T') { // Transfer
            strcpy(t->dest_account, cmd.command_list[3]);
            t->amount = atof(cmd.command_list[4]);
        } else if (t->type == 'D' || t->type == 'W') { // Deposit or withdraw
            t->amount = atof(cmd.command_list[3]);
        }

        num_transactions++;
        free_command_line(&cmd);
    }
    fclose(file);
}

// Worker thread function
void* thread_process_transactions(void* arg) {
    int thread_id = *(int*)arg;
    printf("Thread %d waiting at the barrier.\n", thread_id);
    pthread_barrier_wait(&start_barrier); // Wait until all threads are ready
    printf("Thread %d passed the barrier.\n", thread_id);

    int transactions_per_thread = (num_transactions + num_threads - 1) / num_threads; // Round up
    int start = thread_id * transactions_per_thread;
    int end = (start + transactions_per_thread > num_transactions) ? num_transactions : start + transactions_per_thread;
    printf("Thread %d assigned range: %d to %d\n", thread_id, start, end);

    for (int i = start; i < end; i++) {
        transaction *t = &transactions[i];

        for (int j = 0; j < num_accounts; j++) {
            account *acc = &accounts[j];

            if (strcmp(acc->account_number, t->src_account) == 0) {
                if (strcmp(acc->password, t->password) != 0) {
                    printf("Invalid password for account %s. Skipping transaction.\n", acc->account_number);
                    pthread_mutex_lock(&threshold_mutex);
                    processed_transactions++; // Increment for skipped transactions
                    pthread_mutex_unlock(&threshold_mutex);
                    break;
                }

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
                        }
                    }
                }
                pthread_mutex_unlock(&acc->ac_lock); // Unlock the account

                // Check if transaction threshold is reached
                pthread_mutex_lock(&threshold_mutex);
                processed_transactions++;
                printf("Thread %d: Processed transaction %d/%d\n", thread_id, processed_transactions, num_transactions);
                if (processed_transactions % TRANSACTION_THRESHOLD == 0 || processed_transactions == num_transactions) {
                    bank_ready = 1;
                    pthread_cond_signal(&threshold_cond);
                    printf("Thread %d: Notified bank thread.\n", thread_id);
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
        printf("Thread %d: All transactions processed. Final notification sent.\n", thread_id);
    } else {
        printf("Thread %d: Remaining transactions to process: %d.\n", thread_id, num_transactions - processed_transactions);
    }
    pthread_mutex_unlock(&threshold_mutex);

    return NULL;
}

// Bank thread function
void* update_balance(void* arg) {
    int total_updates = 0;

    while (1) {
        pthread_mutex_lock(&threshold_mutex);
        while (!bank_ready) {
            if (processed_transactions >= num_transactions) {
                printf("Bank thread: All transactions processed (%d/%d). Exiting.\n", processed_transactions, num_transactions);
                pthread_mutex_unlock(&threshold_mutex);
                return NULL; // Exit the thread
            }
            pthread_cond_wait(&threshold_cond, &threshold_mutex);
        }
        bank_ready = 0; // Reset the flag for the next round
        printf("Bank thread: Processing transactions. Processed: %d/%d\n", processed_transactions, num_transactions);
        pthread_mutex_unlock(&threshold_mutex);

        // Update balances
        for (int i = 0; i < num_accounts; i++) {
            pthread_mutex_lock(&accounts[i].ac_lock);
            accounts[i].balance += accounts[i].transaction_tracter * accounts[i].reward_rate;
            accounts[i].transaction_tracter = 0.0;
            pthread_mutex_unlock(&accounts[i].ac_lock);
        }

        // Write individual account updates to files
        for (int i = 0; i < num_accounts; i++) {
            FILE *file = fopen(accounts[i].out_file, "a");
            if (file) {
                fprintf(file, "Current Balance: \t%.2f\n", accounts[i].balance);
                fclose(file);
            }
        }
        total_updates++;
        printf("Bank thread: Completed update %d\n", total_updates);
    }
}

// Write final balances to output file
void write_output(const char *filename) {
    FILE *file = fopen(filename, "w");
    if (!file) { // Error check for opening output file
        perror("Failed to open output file");
        exit(1);
    }

    for (int i = 0; i < num_accounts; i++) {
        // Write the index and balance in the required format
        fprintf(file, "%d balance: \t%.2f\n", i, accounts[i].balance);
    }

    fclose(file); // Close output file 
}

// Initialize shared memory
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

    // Copy account data to shared memory
    memcpy(shared_mem, accounts, shared_mem_size);
    printf("Shared memory initialized and account data written.\n");
}

// Finalize shared memory
void finalize_shared_memory() {
    if (munmap(shared_mem, shared_mem_size) == -1) {
        perror("Failed to unmap shared memory");
    }
    if (shm_unlink("/duckbank_shared") == -1) {
        perror("Failed to unlink shared memory");
    }
    printf("Shared memory finalized and cleaned up.\n");
}
