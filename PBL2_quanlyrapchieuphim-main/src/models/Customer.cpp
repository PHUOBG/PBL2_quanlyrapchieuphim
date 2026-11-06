#include "models/Customer.h"

using namespace std;

Customer::Customer() 
    : User()
{
    setRole(AppRole::Customer);
}

Customer::Customer(const User& user)
    : User(user)
{

    if (getRole() != AppRole::Customer) {
        setRole(AppRole::Customer);
    }
}

Customer::Customer(const string& email, 
                   const string& passwordHash,
                   const string& fullName,
                   const string& birthDate,
                   const string& phone,
                   time_t registeredAt)
    : User(email, passwordHash, fullName, birthDate, phone, registeredAt, AppRole::Customer)
{
}

bool Customer::canBookTicket() const {
    return getRole() == AppRole::Customer;
}

bool Customer::canUseVoucher() const {
    return getRole() == AppRole::Customer;
}

bool Customer::canViewPersonalInfo() const {
    return getRole() == AppRole::Customer;
}

bool Customer::canViewBookingHistory() const {
    return getRole() == AppRole::Customer;
}

string Customer::getDisplayName() const {
    return getFullName();
}

bool Customer::isValidCustomer() const {
    return getRole() == AppRole::Customer;
}
