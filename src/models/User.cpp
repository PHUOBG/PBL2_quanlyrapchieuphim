#include "models/User.h"
#include <algorithm>
#include <cctype>

User::User() 
    : email(""),
      passwordHash(""),
      fullName(""),
      birthDate(""),
      phone(""),
      registeredAt(0),
    role(AppRole::Guest)
{}

User::User(const string& email, 
           const string& passwordHash,
           const string& fullName,
           const string& birthDate,
           const string& phone,
           time_t registeredAt,
         AppRole role)
    : email(email),
      passwordHash(passwordHash),
      fullName(fullName),
      birthDate(birthDate),
      phone(phone),
      registeredAt(registeredAt),
    role(role)
{}

string User::getRoleString() const {
    switch (role) {
        case AppRole::Guest: return "guest";
        case AppRole::Customer: return "customer";
        case AppRole::Admin: return "admin";
        default: return "guest";
    }
}

AppRole User::parseRole(const string& roleStr) {
    string lower = roleStr;
    transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    
    if (lower == "admin") return AppRole::Admin;
    // Legacy: Staff role removed. Treat as Customer.
    if (lower == "staff") return AppRole::Customer;
    if (lower == "customer") return AppRole::Customer;
    return AppRole::Guest;
}

string User::getEmail() const { return email; }
string User::getPasswordHash() const { return passwordHash; }
string User::getFullName() const { return fullName; }
string User::getBirthDate() const { return birthDate; }
string User::getPhone() const { return phone; }
time_t User::getRegisteredAt() const { return registeredAt; }
AppRole User::getRole() const { return role; }

string User::getUsername() const { return fullName; }

void User::setPasswordHash(const string& hash) { passwordHash = hash; }
void User::setFullName(const string& name) { fullName = name; }
void User::setBirthDate(const string& date) { birthDate = date; }
void User::setPhone(const string& phoneNum) { phone = phoneNum; }
void User::setRole(AppRole r) { role = r; }
