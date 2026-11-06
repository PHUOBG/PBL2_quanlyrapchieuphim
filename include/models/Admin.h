#pragma once
#include "models/User.h"
#include <vector>

using namespace std;

class Admin : public User {
public:
    Admin();
    explicit Admin(const User& user);
    Admin(const string& email, 
          const string& passwordHash,
          const string& fullName,
          const string& birthDate,
          const string& phone,
          time_t registeredAt);
    bool canManageMovies() const;
    bool canManageShowtimes() const;
    bool canManageRooms() const;
    bool canManageCombos() const;
    bool canManageVouchers() const;
    bool canManageUsers() const;
    bool canViewRevenue() const;
    bool canViewOverview() const;
    string getAdminDisplayName() const;
    bool isValidAdmin() const;
    bool hasAdminAccess() const;
};
