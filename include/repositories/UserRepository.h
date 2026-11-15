#pragma once

#include "models/User.h"
#include "data-structures/HashTable.h"
#include <string>
#include <vector>

using namespace std;

class UserRepository {
private:
    string filePath;
    HashTable<string, User> users;

    string normalizeEmail(const string& email) const;
    User parseLine(const string& line) const;
    string serializeLine(const User& user) const;

public:
    UserRepository(const string& filepath = "../data/users.txt");
    ~UserRepository();

    void loadFromFile();
    void saveToFile();

    bool exists(const string& email) const;
    User* findByEmail(const string& email);
    bool addUser(const User& user);
    bool updateUser(const User& user);
    bool deleteUser(const string& email);

    vector<User> getAllUsers() const;
    vector<User> getUsersByRole(AppRole role) const;
    int countByRole(AppRole role) const;
    int getTotalUsers() const;
};
