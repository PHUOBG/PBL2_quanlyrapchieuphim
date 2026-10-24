#include "repositories/admin/AdminVoucherRepository.h"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <sstream>

using namespace std;

VoucherDefinition::VoucherDefinition(const std::string& c, int t, double v, double m, const std::string& d)
    : code(c), type(t), value(v), minBill(m), description(d) {}

UserVoucherEntry::UserVoucherEntry(const std::string& e, const std::string& c, int s, const std::string& exp, int qty)
    : email(e), code(c), status(s), expiryDate(exp), quantity(qty) {}

AdminVoucherRepository::AdminVoucherRepository(
    const string& defPath,
    const string& walletPathValue,
    const string& usersPathValue
) : definitionPath(defPath), walletPath(walletPathValue), usersPath(usersPathValue) {
    reload();
}

void AdminVoucherRepository::reload() {
    loadDefinitions();
    loadUserVouchers();
}

void AdminVoucherRepository::loadDefinitions() {
    definitions.clear();
    definitionLookup.clear();

    ifstream file(definitionPath);
    if (!file.is_open()) return;

    string line;
    while (getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        vector<string> cols = splitString(line, '|');
        if (cols.size() < 5) continue;
        if (isHeaderRow(cols, "code")) continue;

        VoucherDefinition def;
        def.code = toUpperCase(trim(cols[0]));
        def.type = stoi(cols[1]);
        def.value = stod(cols[2]);
        def.minBill = stod(cols[3]);
        def.description = trim(cols[4]);

        definitions.push_back(def);
        definitionLookup[def.code] = def;
    }
}

void AdminVoucherRepository::saveDefinitions() {
    ofstream file(definitionPath, ios::trunc);
    if (!file.is_open()) return;

    file << "code|type|value|min_bill|description\n";
    for (const auto& def : definitions) {
        file << def.code << "|" << def.type << "|" << def.value << "|" << def.minBill << "|" << def.description << "\n";
    }
}

vector<VoucherDefinition> AdminVoucherRepository::getAllDefinitions() const {
    return definitions;
}

const VoucherDefinition* AdminVoucherRepository::findDefinition(const string& code) const {
    string upperCode = toUpperCase(trim(code));
    auto it = definitionLookup.find(upperCode);
    if (it != definitionLookup.end()) {
        return &(it->second);
    }
    return nullptr;
}

bool AdminVoucherRepository::addDefinition(const VoucherDefinition& def) {
    string upperCode = toUpperCase(trim(def.code));

    if (definitionLookup.find(upperCode) != definitionLookup.end()) {
        return false;
    }

    VoucherDefinition newDef = def;
    newDef.code = upperCode;

    definitions.push_back(newDef);
    definitionLookup[upperCode] = newDef;
    saveDefinitions();
    return true;
}

bool AdminVoucherRepository::updateDefinition(const VoucherDefinition& def) {
    string upperCode = toUpperCase(trim(def.code));

    auto it = definitionLookup.find(upperCode);
    if (it == definitionLookup.end()) {
        return false;
    }

    it->second = def;
    it->second.code = upperCode;

    for (auto& d : definitions) {
        if (d.code == upperCode) {
            d = def;
            d.code = upperCode;
            break;
        }
    }

    saveDefinitions();
    return true;
}

bool AdminVoucherRepository::deleteDefinition(const string& code) {
    string upperCode = toUpperCase(trim(code));

    auto it = definitionLookup.find(upperCode);
    if (it == definitionLookup.end()) {
        return false;
    }

    definitionLookup.erase(it);
    definitions.erase(
        remove_if(definitions.begin(), definitions.end(),
            [&upperCode](const VoucherDefinition& d) { return d.code == upperCode; }),
        definitions.end()
    );

    saveDefinitions();
    return true;
}

void AdminVoucherRepository::loadUserVouchers() {
    userVouchers.clear();

    ifstream file(walletPath);
    if (!file.is_open()) return;

    string line;
    while (getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        vector<string> cols = splitString(line, '|');
        if (cols.size() < 4) continue;
        if (isHeaderRow(cols, "email")) continue;

        UserVoucherEntry entry;
        entry.email = trim(cols[0]);
        entry.code = toUpperCase(trim(cols[1]));
        entry.status = stoi(cols[2]);
        entry.expiryDate = trim(cols[3]);
        entry.quantity = (cols.size() >= 5) ? stoi(cols[4]) : 1;

        userVouchers.push_back(entry);
    }
}

void AdminVoucherRepository::saveUserVouchers() {
    ofstream file(walletPath, ios::trunc);
    if (!file.is_open()) return;

    file << "email|code|status|expiry_date|quantity\n";
    for (const auto& entry : userVouchers) {
        file << entry.email << "|" << entry.code << "|" << entry.status << "|" << entry.expiryDate << "|" << entry.quantity << "\n";
    }
}

void AdminVoucherRepository::cleanupExpiredVouchers() {
    int today = todayAsNumber();

    size_t before = userVouchers.size();
    userVouchers.erase(
        remove_if(userVouchers.begin(), userVouchers.end(),
            [&](const UserVoucherEntry& entry) {
                int expiry = dateStringToNumber(entry.expiryDate);
                return entry.status == 1 && expiry < today;
            }),
        userVouchers.end()
    );

    if (userVouchers.size() != before) {
        saveUserVouchers();
    }
}

bool AdminVoucherRepository::giveVoucherToUser(const string& email, const string& code, int daysToExpire, int quantity) {
    string upperCode = toUpperCase(trim(code));
    string lowerEmail = trim(email);

    if (findDefinition(upperCode) == nullptr) {
        return false;
    }

    string expiry = buildExpiryDate(daysToExpire);

    for (auto& entry : userVouchers) {
        if (entry.email == lowerEmail && entry.code == upperCode) {
            entry.quantity += quantity;
            entry.status = 1;
            entry.expiryDate = expiry;
            saveUserVouchers();
            return true;
        }
    }

    UserVoucherEntry newEntry(lowerEmail, upperCode, 1, expiry, quantity);
    userVouchers.push_back(newEntry);
    saveUserVouchers();
    return true;
}

int AdminVoucherRepository::giveVoucherToAllActiveUsers(const string& code, int daysToExpire, int quantity) {
    vector<pair<string, string>> activeUsers = getActiveUsers();
    int count = 0;

    for (const auto& user : activeUsers) {
        if (giveVoucherToUser(user.first, code, daysToExpire, quantity)) {
            count++;
        }
    }

    return count;
}

bool AdminVoucherRepository::removeVoucherFromUser(const string& email, const string& code) {
    string upperCode = toUpperCase(trim(code));
    string lowerEmail = trim(email);

    auto it = remove_if(userVouchers.begin(), userVouchers.end(),
        [&](const UserVoucherEntry& entry) {
            return entry.email == lowerEmail && entry.code == upperCode;
        });

    if (it == userVouchers.end()) {
        return false;
    }

    userVouchers.erase(it, userVouchers.end());
    saveUserVouchers();
    return true;
}

vector<VoucherUserInfo> AdminVoucherRepository::getUsersWithVoucher(const string& code) const {
    vector<VoucherUserInfo> result;
    string upperCode = toUpperCase(trim(code));

    unordered_map<string, string> userNames;
    ifstream usersFile(usersPath);
    if (usersFile.is_open()) {
        string line;
        while (getline(usersFile, line)) {
            if (line.empty()) continue;

            vector<string> cols = splitString(line, '|');
            if (cols.size() >= 3 && !isHeaderRow(cols, "email")) {
                string email = trim(cols[0]);
                string name = trim(cols[2]);
                userNames[email] = name;
            }
        }
    }

    int today = todayAsNumber();

    for (const auto& entry : userVouchers) {
        if (entry.code == upperCode && entry.status == 1 && entry.quantity > 0 && dateStringToNumber(entry.expiryDate) >= today) {
            VoucherUserInfo info;
            info.email = entry.email;
            info.fullName = userNames.count(entry.email) > 0 ? userNames[entry.email] : entry.email;
            info.quantity = entry.quantity;
            info.expiryDate = entry.expiryDate;
            info.status = entry.status;
            result.push_back(info);
        }
    }

    return result;
}

int AdminVoucherRepository::countUsersWithVoucher(const string& code) const {
    return static_cast<int>(getUsersWithVoucher(code).size());
}

vector<pair<string, string>> AdminVoucherRepository::getActiveUsers() const {
    vector<pair<string, string>> result;

    ifstream file(usersPath);
    if (!file.is_open()) return result;

    string line;
    while (getline(file, line)) {
        if (line.empty()) continue;

        vector<string> cols = splitString(line, '|');
        if (cols.size() < 8) continue;
        if (isHeaderRow(cols, "email")) continue;

        string email = trim(cols[0]);
        string fullName = trim(cols[2]);
        string status = trim(cols[7]);

        if (status == "active") {
            result.push_back({email, fullName});
        }
    }

    return result;
}

string AdminVoucherRepository::trim(const string& s) {
    size_t start = 0;
    size_t end = s.size();

    if (s.size() >= 3 && (unsigned char)s[0] == 0xEF && (unsigned char)s[1] == 0xBB && (unsigned char)s[2] == 0xBF) {
        start = 3;
    }
    while (start < s.size() && isspace((unsigned char)s[start])) start++;
    while (end > start && isspace((unsigned char)s[end - 1])) end--;
    return s.substr(start, end - start);
}

string AdminVoucherRepository::toUpperCase(const string& s) {
    string out = s;
    transform(out.begin(), out.end(), out.begin(), [](unsigned char c){ return (char)toupper(c); });
    return out;
}

bool AdminVoucherRepository::isHeaderRow(const vector<string>& cols, const string& firstColName) {
    if (cols.empty()) return false;
    return toUpperCase(trim(cols[0])) == toUpperCase(firstColName);
}

vector<string> AdminVoucherRepository::splitString(const string& s, char delimiter) {
    vector<string> cols;
    string token;
    stringstream ss(s);
    while (getline(ss, token, delimiter)) {
        cols.push_back(token);
    }
    return cols;
}

int AdminVoucherRepository::todayAsNumber() {
    auto now = chrono::system_clock::now();
    time_t t = chrono::system_clock::to_time_t(now);
    tm local{};
    localtime_s(&local, &t);
    return (local.tm_year + 1900) * 10000 + (local.tm_mon + 1) * 100 + local.tm_mday;
}

int AdminVoucherRepository::dateStringToNumber(const string& yyyymmdd) {
    if (yyyymmdd.size() < 8) return 0;
    try {
        return stoi(yyyymmdd.substr(0, 8));
    } catch (...) {
        return 0;
    }
}

string AdminVoucherRepository::buildExpiryDate(int daysToExpire) {
    using namespace chrono;
    auto now = system_clock::now();
    auto expiry = now + hours(24 * daysToExpire);
    time_t t = system_clock::to_time_t(expiry);
    tm local{};
    localtime_s(&local, &t);

    stringstream ss;
    ss << setfill('0') << setw(4) << (local.tm_year + 1900)
       << setw(2) << (local.tm_mon + 1)
       << setw(2) << local.tm_mday;
    return ss.str();
}
