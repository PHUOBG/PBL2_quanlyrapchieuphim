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
    if (!birthDate.empty() && !Validator::isValidDate(birthDate)) return false;
    if (!phone.empty() && !Validator::isValidPhone(phone)) return false;
    if (!fullName.empty() && !Validator::isValidFullName(fullName)) return false;
    
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
    // Seed the default admin ONLY on a fresh install (empty user store). Re-seeding whenever the
    // default account is missing would resurrect a deleted account with a well-known password.
    // Note: Bypass isStrongPassword here because the documented default password is "admin123".
    if (repository->getTotalUsers() == 0) {
        const string defaultHash = PasswordHasher::hashPassword("admin123");
        repository->addUser(User(
            "admin@cinexine.vn",
            defaultHash,
            "Nguyễn Văn Quản Trị",
            "01/01/1990",
            "0900000000",
            time(nullptr),
            AppRole::Admin
        ));
        repository->addUser(User(
            "admin@cinema.com",
            defaultHash,
            "Quản Trị Viên",
            "01/01/1990",
            "0900000001",
            time(nullptr),
            AppRole::Admin
        ));
    }
}

AuthService::ResetResult AuthService::resetPassword(const string& email, const string& phone, const string& newPassword) {
    if (email.empty() || phone.empty() || newPassword.empty()) return ResetResult::MissingInput;
    if (!Validator::isStrongPassword(newPassword)) return ResetResult::WeakPassword;

    User* user = repository->findByEmail(email);
    // Same result for unknown account / wrong phone, so account existence is not disclosed.
    if (!user || user->getPhone().empty() || user->getPhone() != phone) return ResetResult::VerificationFailed;

    string oldHash = user->getPasswordHash();
    user->setPasswordHash(PasswordHasher::hashPassword(newPassword));
    if (!repository->saveToFile()) {
        user->setPasswordHash(oldHash);
        return ResetResult::SaveFailed;
    }
    return ResetResult::Ok;
}

bool AuthService::changePassword(const string& email, const string& oldPassword, const string& newPassword) {
    User* user = repository->findByEmail(email);
    if (!user) return false;
    if (!PasswordHasher::verifyPassword(oldPassword, user->getPasswordHash())) return false;
    if (!Validator::isStrongPassword(newPassword)) return false;
    string oldHash = user->getPasswordHash();
    user->setPasswordHash(PasswordHasher::hashPassword(newPassword));
    if (!repository->saveToFile()) {
        user->setPasswordHash(oldHash);
        return false;
    }
    return true;
}

bool AuthService::deleteAccount(const string& email) {
    if (!repository->deleteUser(email)) return false;
    if (loggedIn && currentUserEmail == email) logout();
    return true;
}