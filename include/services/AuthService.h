#pragma once
#include <string>
#include <ctime>
#include <memory>
#include "models/User.h"
#include "repositories/UserRepository.h"
#include "services/VoucherManager.h"
#include "services/EventSystem.h"
#include "utils/PasswordHasher.h"
#include "utils/Validator.h"
#include "core/AppRole.h"
using namespace std;

class AuthService {
public:
    explicit AuthService(const string&);
    ~AuthService();
    bool registerUser(const string& email, 
                     const string& password, 
                     const string& fullName = "", 
                     const string& birthDate = "", 
                     const string& phone = "",
                     AppRole role = AppRole::Customer);
    bool verify(const string& email, const string& password);
    bool login(const string& email, const string& password);
    void logout();
    bool isLoggedIn() const;
    string getCurrentUserEmail() const;
    User* getCurrentUser();

    bool hasRole(AppRole) const;
    bool isAdmin() const;

    User* getUser(const string&);
    bool emailExists(const string&);
    bool changeUserRole(const string&, AppRole);
    UserRepository* getRepository();
    void ensureDefaultAdmin();

private:
    unique_ptr<UserRepository> repository;
    unique_ptr<VoucherManager> voucherManager;
    
    string currentUserEmail;
    bool loggedIn;
};
