#pragma once
#include "models/User.h"
#include <vector>

using namespace std;

class Customer : public User {
public:
    Customer();
    explicit Customer(const User& user);
    Customer(const string& email, 
             const string& passwordHash,
             const string& fullName,
             const string& birthDate,
             const string& phone,
             time_t registeredAt);
    bool canBookTicket() const;
    bool canUseVoucher() const;
    bool canViewPersonalInfo() const;
    bool canViewBookingHistory() const;
    string getDisplayName() const;
    bool isValidCustomer() const;
};
