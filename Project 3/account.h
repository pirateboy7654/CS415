#ifndef ACCOUNT_H_
#define ACCOUNT_H_

typedef struct
{
	char account_number[17];
	char password[9];
    double balance;
    double reward_rate;
    double savings_balance;
    double savings_reward_rate; // fixed at 2%
    double transaction_tracter;

    char out_file[64];

    pthread_mutex_t ac_lock;
}account;

typedef struct 
{
    char type; // T, D, W, C
    char src_account[17];
    char password[9];
    char dest_account[17]; // only for T
    double amount; // for T, D, W
} transaction;

#endif /* ACCOUNT_H_ */