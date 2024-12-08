#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "../account.h"
#include "../string_parser.h"

// func declarations
void read_input(const char *filename);
void process_transactions();
void write_output(const char *filename);
void update_balance();

const int max_accounts = 11;
const int max_transactions = 120052;

// global arrays
account accounts[max_accounts];
transaction transactions[max_transactions];

// counters
int num_accounts = 0;
int num_transactions = 0;
pthread_mutex_t transaction_counter_lock;
int transaction_counter = 0;

int main(int argc, char *argv[]) {
    if (argc != 2) { //wrong arg input check
        fprintf(stderr, "Usage: %s <input file>\n", argv[0]);
        return 1;
    }
    // init mutex for transaction counter
    pthread_mutex_init(&transaction_counter_lock, NULL);

    read_input(argv[1]);
    process_transactions();
    update_balance();
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
        sprintf(accounts[i].out_file, "account_%s.txt", accounts[i].account_number);
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

// process all transactions
void process_transactions() {
    int wrong_password_counter = 0; 
    for (int i = 0; i < num_transactions; i++) {
        transaction *t = &transactions[i];

        for (int j = 0; j < num_accounts; j++) {
            account *acc = &accounts[j];

            // find source account
            if (strcmp(acc->account_number, t->src_account) == 0) {
                if (strcmp(acc->password, t->password) != 0) {
                    //printf("invalid password for account %s\n", acc->account_number);
                    wrong_password_counter++;
                    break; // test for correct password
                }

                if (t->type == 'D') {
                    acc->balance += t->amount; // deposit
                    acc->transaction_tracter += t->amount; // update transaction tracker
                } else if (t->type == 'W') {
                    acc->balance -= t->amount; // withdraw
                    acc->transaction_tracter += t->amount; // update transaction tracker
                } else if (t->type == 'T') {
                    // transfer
                    acc->balance -= t->amount; // remove from src account
                    acc->transaction_tracter += t->amount; // update transaction tracker

                    // find destination account
                    for (int k = 0; k < num_accounts; k++) {
                        if (strcmp(accounts[k].account_number, t->dest_account) == 0) {
                            accounts[k].balance += t->amount; // add to dest account
                            break;
                        }
                    }
                } else if (t->type == 'C') { // check balance
                    //printf("Balance for account %s: %.2f\n", acc->account_number, acc->balance); // Check balance
                    break;
                }
                if (t->type != 'C') {
                    pthread_mutex_lock(&transaction_counter_lock);
                    transaction_counter++;
                    pthread_mutex_unlock(&transaction_counter_lock);
                }
                
                break; // exit loop after processing the transaction for this account
            }
        }
    }
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

void update_balance() {
    for (int i = 0; i < num_accounts; i++) {
        pthread_mutex_lock(&accounts[i].ac_lock); // lock thread
        //printf("balance for account %d : %.2f\n", i, accounts[i].balance); // debug
        accounts[i].balance += accounts[i].transaction_tracter * accounts[i].reward_rate;
        //printf("balance for account %d : %.2f\n", i, accounts[i].balance); // debug
        accounts[i].transaction_tracter = 0.0; // reset tracker after updating
        pthread_mutex_unlock(&accounts[i].ac_lock); // unlock thread
    }
}