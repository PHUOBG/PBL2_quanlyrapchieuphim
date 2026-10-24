#pragma once

#include <string>
#include <cctype>
#include <ctime>
using namespace std;

class Validator {
public:
    // Email validation
    static bool isValidEmail(const string& email);
    
    // Phone validation (Vietnam format)
    static bool isValidPhone(const string& phone);
    
    // Date validation (dd/mm/yyyy)
    static bool isValidDate(const string& date);
    
    // Password strength check
    static bool isStrongPassword(const string& password);
    
    // Username validation
    static bool isValidUsername(const string& username);
    
    // Full name validation - không chứa số
    static bool isValidFullName(const string& fullName);
};
