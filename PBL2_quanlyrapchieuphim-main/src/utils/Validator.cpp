#include "utils/Validator.h"

bool Validator::isValidEmail(const string& email) {
    if (email.length() < 5) return false;

    size_t at = email.find('@');
    size_t dot = email.rfind('.');

    // Check format: local@domain.tld
    if (at == string::npos || dot == string::npos) return false;
    if (at == 0 || at == email.length() - 1) return false;
    if (dot <= at + 1 || dot == email.length() - 1) return false;

    // Check for valid characters
    for (size_t i = 0; i < email.length(); i++) {
        char c = email[i];
        if (i == at || i == dot) continue;
        if (!isalnum((unsigned char)c) && c != '.' && c != '_' && c != '-' && c != '+') {
            return false;
        }
    }

    return true;
}

bool Validator::isValidPhone(const string& phone) {
    if (phone.length() != 10) return false;
    if (phone[0] != '0') return false;

    for (char c : phone) {
        if (!isdigit((unsigned char)c)) return false;
    }

    // Check valid prefixes (Vietnam operators)
    string prefix = phone.substr(0, 3);
    const string validPrefixes[] = {
        "090", "091", "093", "094", "096", "097", "098", "099",  // Mobifone
        "032", "033", "034", "035", "036", "037", "038", "039",  // Viettel
        "070", "076", "077", "078", "079",                        // Viettel
        "081", "082", "083", "084", "085", "088", "089"          // Vinaphone
    };

    for (const string& valid : validPrefixes) {
        if (prefix == valid) return true;
    }

    return false;
}

bool Validator::isValidDate(const string& date) {
    if (date.length() != 10) return false;
    if (date[2] != '/' || date[5] != '/') return false;

    // Check all characters except slashes are digits
    for (int i = 0; i < 10; i++) {
        if (i == 2 || i == 5) continue; // Skip slash positions
        if (!isdigit((unsigned char)date[i])) return false;
    }

    try {
        int day = stoi(date.substr(0, 2));
        int month = stoi(date.substr(3, 2));
        int year = stoi(date.substr(6, 4));

        if (month < 1 || month > 12) return false;
        if (day < 1 || day > 31) return false;
        if (year < 1900 || year > 2100) return false;

        // Check days in month
        int daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

        // Leap year
        bool isLeap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
        if (isLeap) daysInMonth[1] = 29;

        if (day > daysInMonth[month - 1]) return false;

        // Kiểm tra ngày sinh phải là quá khứ (không được là tương lai)
        time_t now = time(nullptr);
        tm* localTime = localtime(&now);
        int currentYear = localTime->tm_year + 1900;
        int currentMonth = localTime->tm_mon + 1;
        int currentDay = localTime->tm_mday;

        // So sánh năm
        if (year > currentYear) return false;
        if (year == currentYear) {
            // Cùng năm, kiểm tra tháng
            if (month > currentMonth) return false;
            if (month == currentMonth) {
                // Cùng tháng, kiểm tra ngày
                if (day > currentDay) return false;
            }
        }

        return true;
    } catch (...) {
        return false;
    }
}

bool Validator::isStrongPassword(const string& password) {
    if (password.length() < 8) return false;

    bool hasUpper = false, hasLower = false, hasDigit = false;
    for (char c : password) {
        if (isupper((unsigned char)c)) hasUpper = true;
        if (islower((unsigned char)c)) hasLower = true;
        if (isdigit((unsigned char)c)) hasDigit = true;
    }

    return hasUpper && hasLower && hasDigit;
}

bool Validator::isValidUsername(const string& username) {
    if (username.length() < 3 || username.length() > 20) return false;

    for (char c : username) {
        if (!isalnum((unsigned char)c) && c != '_') return false;
    }

    return true;
}

bool Validator::isValidFullName(const string& fullName) {
    if (fullName.empty()) return false;
    if (fullName.length() > 100) return false;

    // Kiểm tra không chứa số
    for (char c : fullName) {
        if (isdigit((unsigned char)c)) return false;
    }

    // Cho phép chữ cái, khoảng trắng, và các ký tự Unicode (tiếng Việt)
    return true;
}
