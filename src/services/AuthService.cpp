#include "services/AuthService.h"

using namespace std;

AuthService::AuthService(const string& filePath) 
    : currentUserEmail(""), loggedIn(false) {
    repository = make_unique<UserRepository>(filePath);
    voucherManager = make_unique<VoucherManager>();
    ensureDefaultAdmin();
}

AuthService::~AuthService() {
}

bool AuthService::registerUser(const string& email, 
                               const string& password, 
                               const string& fullName, 
                               const string& birthDate, 
                               const string& phone,
                               AppRole role) {
    // Validate inputs
    if (!Validator::isValidEmail(email)) {
        return false;
    }
    
    if (!Validator::isStrongPassword(password)) {
        return false;
    }
    
    // Check if email already exists
    if (repository->exists(email)) {
        return false;
    }
    
    // Hash password
    string passwordHash = PasswordHasher::hashPassword(password);
    
    // Create new user
    User newUser(
        email,
        passwordHash,
        fullName.empty() ? email : fullName,
        birthDate,
        phone,
        time(nullptr),
        role
    );
    
    bool success = repository->addUser(newUser);
    
    // Tự động tặng voucher WELCOME cho user mới (chỉ áp dụng cho Customer)
    if (success && role == AppRole::Customer && voucherManager) {
        voucherManager->giveVoucher(email, "WELCOME", 365, 1); // Hạn 1 năm, 1 lượt dùng
        
        // Publish UserRegisteredEvent for auto-provisioning additional vouchers
        AppEventSystem::getInstance().publish(UserRegisteredEvent(email, newUser.getFullName()));
    }
    
    return success;
}

bool AuthService::verify(const string& email, const string& password) {
    User* user = repository->findByEmail(email);
    
    if (!user) {
        return false;
    }
    
    // Verify password
    return PasswordHasher::verifyPassword(password, user->getPasswordHash());
}

bool AuthService::login(const string& email, const string& password) {
    User* user = repository->findByEmail(email);
    
    if (!user) {
        return false; // User not found
    }
    
    // Verify password
    if (!PasswordHasher::verifyPassword(password, user->getPasswordHash())) {
        return false; // Wrong password
    }
    
    // Login successful
    currentUserEmail = email;
    loggedIn = true;
    
    return true;
}

void AuthService::logout() {
    currentUserEmail = "";
    loggedIn = false;
}

bool AuthService::isLoggedIn() const {
    return loggedIn && !currentUserEmail.empty();
}

string AuthService::getCurrentUserEmail() const {
    return currentUserEmail;
}

User* AuthService::getCurrentUser() {
    if (!isLoggedIn()) {
        return nullptr;
    }
    
    return repository->findByEmail(currentUserEmail);
}

bool AuthService::hasRole(AppRole role) const {
    if (!isLoggedIn()) {
        return false;
    }
    
    User* user = const_cast<AuthService*>(this)->getCurrentUser();
    if (!user) {
        return false;
    }
    
    return user->getRole() == role;
}

bool AuthService::isAdmin() const {
    return hasRole(AppRole::Admin);
}

User* AuthService::getUser(const string& email) {
    return repository->findByEmail(email);
}

bool AuthService::emailExists(const string& email) {
    return repository->exists(email);
}

bool AuthService::changeUserRole(const string& email, AppRole newRole) {
    User* user = repository->findByEmail(email);
    if (!user) {
        return false;
    }
    
    user->setRole(newRole);
    return repository->updateUser(*user);
}

UserRepository* AuthService::getRepository() {
    return repository.get();
}

void AuthService::ensureDefaultAdmin() {
    // Create default admin if not exists
    if (!repository->exists("admin@cinexine.vn")) {
        registerUser(
            "admin@cinexine.vn",
            "admin123",
            "Nguyễn Văn Quản Trị",
            "01/01/1990",
            "0900000000",
            AppRole::Admin
        );
    }
}
