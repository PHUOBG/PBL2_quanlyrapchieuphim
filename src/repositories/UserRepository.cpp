#include "repositories/UserRepository.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <array>
#include <vector>

namespace {
    std::string resolveUserFilePath(const std::string& rawPath) {
        namespace fs = std::filesystem;
        fs::path input(rawPath);
        if (fs::exists(input)) {
            return input.string();
        }

        std::vector<fs::path> tails;
        tails.push_back(input);

        // If path contains a "data" segment, try resolving from that segment.
        {
            auto it = std::find(input.begin(), input.end(), fs::path("data"));
            if (it != input.end()) {
                fs::path tail;
                for (auto jt = it; jt != input.end(); ++jt) {
                    tail /= *jt;
                }
                if (!tail.empty()) {
                    tails.push_back(tail);
                }
            }
        }

        // Common fallbacks for this repository.
        if (!input.filename().empty()) {
            tails.push_back(input.filename());
            if (input.filename() == fs::path("users.txt")) {
                tails.push_back(fs::path("data") / "users.txt");
            }
        }

        const std::array<std::string, 5> prefixes = {"./", "../", "../../", "../../../", ""};
        for (const auto& tail : tails) {
            for (const auto& prefix : prefixes) {
                fs::path candidate = fs::path(prefix) / tail;
                if (fs::exists(candidate)) {
                    return candidate.string();
                }
            }
        }

        return rawPath;
    }
}

UserRepository::UserRepository(const string& filepath) 
    : filePath(resolveUserFilePath(filepath)) {
    namespace fs = filesystem;
    if (!fs::exists(filePath)) {
        ofstream file(filePath);
        file << "email|passwordHash|fullName|birthDate|phone|registeredAt|role\n";
        file.close();
    }
    loadFromFile();
}

UserRepository::~UserRepository() {
    saveToFile();
}

string UserRepository::normalizeEmail(const string& email) const {
    string normalized = email;
    transform(normalized.begin(), normalized.end(), normalized.begin(), ::tolower);
    return normalized;
}

User UserRepository::parseLine(const string& line) const {
    stringstream ss(line);
    string email, passwordHash, fullName, birthDate, phone, registeredAtStr, roleStr;
    
    getline(ss, email, '|');
    getline(ss, passwordHash, '|');
    getline(ss, fullName, '|');
    getline(ss, birthDate, '|');
    getline(ss, phone, '|');
    getline(ss, registeredAtStr, '|');
    getline(ss, roleStr, '|');
    // Legacy compatibility: old files may include a trailing "status" column.
    // We ignore it because account locking has been removed.
    
    time_t registeredAt = 0;
    try {
        registeredAt = stoll(registeredAtStr);
    } catch (...) {
        registeredAt = time(nullptr);
    }
    
    AppRole role = User::parseRole(roleStr);
    
    return User(email, passwordHash, fullName, birthDate, phone, registeredAt, role);
}

string UserRepository::serializeLine(const User& user) const {
    stringstream ss;
    ss << user.getEmail() << "|"
       << user.getPasswordHash() << "|"
       << user.getFullName() << "|"
       << user.getBirthDate() << "|"
       << user.getPhone() << "|"
       << user.getRegisteredAt() << "|"
       << user.getRoleString();
    return ss.str();
}

void UserRepository::loadFromFile() {
    users.clear();

    ifstream file(filePath);
    if (!file.is_open()) return;
    
    string line;
    getline(file, line); // Skip header
    
    while (getline(file, line)) {
        if (line.empty()) continue;
        
        try {
            User user = parseLine(line);
            string key = normalizeEmail(user.getEmail());
            users.insert(key, user);
        } catch (...) {
            // Skip invalid lines
            continue;
        }
    }
    
    file.close();
}

void UserRepository::saveToFile() {
    ofstream file(filePath);
    if (!file.is_open()) return;
    
    // Write header
    file << "email|passwordHash|fullName|birthDate|phone|registeredAt|role\n";
    
    // Write all users
    vector<User> allUsers = getAllUsers();
    for (const auto& user : allUsers) {
        file << serializeLine(user) << "\n";
    }
    
    file.close();
}

bool UserRepository::exists(const string& email) const {
    string key = normalizeEmail(email);
    return users.contains(key);
}

User* UserRepository::findByEmail(const string& email) {
    string key = normalizeEmail(email);
    return users.get(key);
}

bool UserRepository::addUser(const User& user) {
    string key = normalizeEmail(user.getEmail());
    
    if (exists(key)) {
        return false; // Email already exists
    }
    
    users.insert(key, user);
    saveToFile();
    return true;
}

bool UserRepository::updateUser(const User& user) {
    string key = normalizeEmail(user.getEmail());
    
    if (!exists(key)) {
        return false; // User not found
    }
    
    // Update existing user (email remains the same - PRIMARY KEY)
    users.insert(key, user);
    saveToFile();
    return true;
}

bool UserRepository::deleteUser(const string& email) {
    string key = normalizeEmail(email);
    
    if (!exists(key)) {
        return false;
    }
    
    users.remove(key);
    saveToFile();
    return true;
}

vector<User> UserRepository::getAllUsers() const {
    vector<User> result;

    auto& mutableUsers = const_cast<HashTable<string, User>&>(users);

    mutableUsers.forEach([&result](const string&, User& user) {
        result.push_back(user);
    });

    return result;
}

vector<User> UserRepository::getUsersByRole(AppRole role) const {
    vector<User> allUsers = getAllUsers();
    vector<User> filtered;
    
    for (const auto& user : allUsers) {
        if (user.getRole() == role) {
            filtered.push_back(user);
        }
    }
    
    return filtered;
}

int UserRepository::countByRole(AppRole role) const {
    return getUsersByRole(role).size();
}

int UserRepository::getTotalUsers() const {
    return getAllUsers().size();
}
