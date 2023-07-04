#include "bank_account.h"
#include "etl/string.h"

uint16_t BankAccount::total_accounts = 0;

BankAccount::BankAccount()
    : account_id(0), account_balance(0.0)
{
    memset(account_name, 0, NAME_PASSWORD_SIZE);
    memset(account_password, 0, NAME_PASSWORD_SIZE);
}

BankAccount::BankAccount(const uint8_t *name, const uint8_t *password)
    : account_id(total_accounts++), account_balance(0.0)
{
    memcpy(account_name, name, NAME_PASSWORD_SIZE);
    memcpy(account_password, password, NAME_PASSWORD_SIZE);
}

uint16_t BankAccount::get_account_id() const
{
    return account_id;
}

bool BankAccount::verify_account_name(uint8_t *name) const
{
    if (memcmp(account_name, name, sizeof(account_name)) != 0)
    {
        return false;
    }
    return true;
}

const uint8_t* BankAccount::get_account_name() const
{
    return account_name;
}

double BankAccount::get_account_balance() const
{
    return account_balance;
}

void BankAccount::deposit(double amount)
{
    account_balance += amount;
}

bool BankAccount::withdraw(double amount)
{
    if (account_balance >= amount)
    {
        account_balance -= amount;
        return true;
    }
    return false;
}

void BankAccount::set_password(const uint8_t *password)
{
    memcpy(account_password, password, NAME_PASSWORD_SIZE);
}

bool BankAccount::verify_password(const uint8_t *password) const
{
    if (memcmp(account_password, password, NAME_PASSWORD_SIZE) == 0)
    {
        return true;
    }
    return false;
}

uint16_t BankAccount::get_total_accounts()
{
    return total_accounts;
}