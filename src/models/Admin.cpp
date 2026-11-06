#include "models/Admin.h"

using namespace std;

Admin::Admin() 
    : User()
{
    setRole(AppRole::Admin);
}

Admin::Admin(const User& user)
    : User(user)
{
    // Đảm bảo role là Admin
    if (getRole() != AppRole::Admin) {
        setRole(AppRole::Admin);
    }
}

Admin::Admin(const string& email, 
             const string& passwordHash,
             const string& fullName,
             const string& birthDate,
             const string& phone,
             time_t registeredAt)
    : User(email, passwordHash, fullName, birthDate, phone, registeredAt, AppRole::Admin)
{
}

bool Admin::canManageMovies() const {
    return getRole() == AppRole::Admin;
}

bool Admin::canManageShowtimes() const {
    return getRole() == AppRole::Admin;
}

bool Admin::canManageRooms() const {
    return getRole() == AppRole::Admin;
}

bool Admin::canManageCombos() const {
    return getRole() == AppRole::Admin;
}

bool Admin::canManageVouchers() const {
    return getRole() == AppRole::Admin;
}

bool Admin::canManageUsers() const {
    return getRole() == AppRole::Admin;
}

bool Admin::canViewRevenue() const {
    return getRole() == AppRole::Admin;
}

bool Admin::canViewOverview() const {
    return getRole() == AppRole::Admin;
}

string Admin::getAdminDisplayName() const {
    return "Admin: " + getFullName();
}

bool Admin::isValidAdmin() const {
    return getRole() == AppRole::Admin;
}

bool Admin::hasAdminAccess() const {
    return isValidAdmin();
}
