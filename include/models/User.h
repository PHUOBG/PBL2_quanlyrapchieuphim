#pragma once
#include <string>
#include <ctime>
#include "core/AppRole.h"

using namespace std;

class User {
private:
    string email;
    string passwordHash;
    string fullName;
    string birthDate;
    string phone;
    time_t registeredAt;
    AppRole role;
public:
    User();
    User(const string& email, 
         const string& passwordHash,
         const string& fullName,
         const string& birthDate,
         const string& phone,
         time_t registeredAt,
         AppRole role = AppRole::Customer);

    string getEmail() const;
    string getPasswordHash() const;
    string getFullName() const;
    string getBirthDate() const;
    string getPhone() const;
    time_t getRegisteredAt() const;
    AppRole getRole() const;
    string getUsername() const;

    void setPasswordHash(const string&);
    void setFullName(const string&);
    void setBirthDate(const string&);
    void setPhone(const string&);
    void setRole(AppRole);

    string getRoleString() const;
    static AppRole parseRole(const string& roleStr);
};
