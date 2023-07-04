#ifndef BANK_ACCOUNT_H
#define BANK_ACCOUNT_H

#include "main.h"

class BankAccount {
private:
    static uint16_t total_accounts; // Class variable to track the total number of accounts
    uint16_t account_id;
    uint8_t account_name[NAME_PASSWORD_SIZE];
    uint8_t account_password[NAME_PASSWORD_SIZE];
    double account_balance;
    
public:
    BankAccount();
    BankAccount(const uint8_t* name, const uint8_t* password);
    uint16_t get_account_id() const;
    bool verify_account_name(uint8_t* name) const;
    const uint8_t* get_account_name() const;
    double get_account_balance() const;
    void deposit(double amount);
    bool withdraw(double amount);
    void set_password(const uint8_t* password);
    bool verify_password(const uint8_t* password) const;
    static uint16_t get_total_accounts();
};

#endif // BANK_ACCOUNT_H