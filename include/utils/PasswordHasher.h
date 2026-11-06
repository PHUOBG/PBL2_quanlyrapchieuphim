#pragma once
#include <string>
#include <sstream>
#include <iomanip>
using namespace std;

class PasswordHasher {
private:
    static unsigned long djb2Hash(const string& str);
    
public:
    // Hash password with salt
    static string hashPassword(const string& password);
    
    // Verify password against hash
    static bool verifyPassword(const string& password, const string& hash);
    
    // Check password strength
    static int getPasswordStrength(const string& password);
    
    static string getStrengthLabel(int score);
};