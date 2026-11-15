#include "services/VoucherService.h"

#include <fstream>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <map>

using namespace std;

VoucherService::VoucherService(const string& defPath, const string& walletPath, const string& usersPath)
    : definitionPath(defPath), walletPath(walletPath), usersPath(usersPath),
      voucherManager(defPath, walletPath) {
    loadDefinitions();
    loadAutoProvisionConfigs();
}

VoucherService::~VoucherService() {
    // Never call shared_from_this() in a destructor: when the last shared_ptr
    // releases the object, shared_from_this() would throw std::bad_weak_ptr.
    // If we are still owned by a shared_ptr, we can safely unsubscribe.
    if (auto self = weak_from_this().lock()) {
        AppEventSystem::getInstance().unsubscribe(self);
    }
}

void VoucherService::initialize() {
    AppEventSystem::getInstance().subscribe(shared_from_this());
}

// ===== Compatibility APIs (Customer UI) =====

vector<VoucherDisplay> VoucherService::getVouchersByUser(const string& email) {
    voucherManager.loadData();
    return voucherManager.getVouchersByUser(email);
}

double VoucherService::applyVoucher(const string& email, const string& voucherCode,
                                   int subtotal, bool commit) {
    voucherManager.loadData();
    return voucherManager.applyVoucher(email, voucherCode, static_cast<double>(subtotal), commit);
}

const VoucherDef* VoucherService::getDefinition(const string& code) const {
    auto it = voucherManager.voucherLookup.find(code);
    if (it == voucherManager.voucherLookup.end()) return nullptr;
    return &it->second;
}

// ===== Helper methods =====

string VoucherService::trim(const string& input) const {
    size_t start = input.find_first_not_of(" \t\r\n");
    if (start == string::npos) return "";
    size_t end = input.find_last_not_of(" \t\r\n");
    return input.substr(start, end - start + 1);
}

vector<string> VoucherService::splitString(const string& input, char delimiter) const {
    vector<string> result;
    stringstream ss(input);
    string item;
    while (getline(ss, item, delimiter)) {
        result.push_back(trim(item));
    }
    return result;
}

string VoucherService::buildExpiryDate(int daysToExpire) const {
    auto now = chrono::system_clock::now();
    auto future = now + chrono::hours(24 * daysToExpire);
    auto tt = chrono::system_clock::to_time_t(future);
    
    tm local_tm;
#ifdef _WIN32
    localtime_s(&local_tm, &tt);
#else
    localtime_r(&tt, &local_tm);
#endif
    
    ostringstream oss;
    oss << put_time(&local_tm, "%Y%m%d");
    return oss.str();
}

int VoucherService::todayAsNumber() const {
    auto now = chrono::system_clock::now();
    auto tt = chrono::system_clock::to_time_t(now);
    
    tm local_tm;
#ifdef _WIN32
    localtime_s(&local_tm, &tt);
#else
    localtime_r(&tt, &local_tm);
#endif
    
    return (local_tm.tm_year + 1900) * 10000 + (local_tm.tm_mon + 1) * 100 + local_tm.tm_mday;
}

int VoucherService::dateStringToNumber(const string& date) const {
    if (date.size() != 8) return 0;
    try {
        return stoi(date);
    } catch (...) {
        return 0;
    }
}

// ===== Data Loading/Saving =====

void VoucherService::loadDefinitions() {
    definitions.clear();
    
    ifstream file(definitionPath);
    if (!file.is_open()) return;
    
    string line;
    bool isFirstLine = true;
    while (getline(file, line)) {
        if (line.empty()) continue;
        
        auto cols = splitString(line, '|');
        if (cols.size() < 5) continue;
        
        // Skip header
        if (isFirstLine && (cols[0] == "code" || cols[0] == "Code")) {
            isFirstLine = false;
            continue;
        }
        isFirstLine = false;
        
        VoucherDef def;
        def.code = cols[0];
        def.type = stoi(cols[1]);
        def.value = stod(cols[2]);
        def.minBill = stod(cols[3]);
        def.description = cols.size() > 4 ? cols[4] : "";
        
        definitions.push_back(def);
    }
}

void VoucherService::saveDefinitions() {
    ofstream file(definitionPath);
    if (!file.is_open()) return;
    
    // Header
    file << "code|type|value|minBill|description\n";
    
    for (const auto& def : definitions) {
        file << def.code << "|" << def.type << "|" << def.value << "|" 
             << def.minBill << "|" << def.description << "\n";
    }
}

void VoucherService::loadAutoProvisionConfigs() {
    autoProvisionConfigs.clear();
    
    string configPath = "../data/voucher_auto_provision.txt";
    ifstream file(configPath);
    if (!file.is_open()) return;
    
    string line;
    while (getline(file, line)) {
        if (line.empty()) continue;
        
        auto cols = splitString(line, '|');
        if (cols.size() < 4) continue;
        
        // Skip header
        if (cols[0] == "code" || cols[0] == "voucherCode") continue;
        
        AutoProvisionConfig config;
        config.voucherCode = cols[0];
        config.daysToExpire = stoi(cols[1]);
        config.quantity = stoi(cols[2]);
        config.enabled = (cols[3] == "1" || cols[3] == "true");
        
        autoProvisionConfigs.push_back(config);
    }
}

void VoucherService::saveAutoProvisionConfigs() {
    string configPath = "../data/voucher_auto_provision.txt";
    ofstream file(configPath);
    if (!file.is_open()) return;
    
    file << "code|daysToExpire|quantity|enabled\n";
    
    for (const auto& config : autoProvisionConfigs) {
        file << config.voucherCode << "|" << config.daysToExpire << "|" 
             << config.quantity << "|" << (config.enabled ? "1" : "0") << "\n";
    }
}

void VoucherService::reload() {
    loadDefinitions();
    loadAutoProvisionConfigs();
}

// ===== Voucher Definition Operations =====

vector<VoucherDef> VoucherService::getAllDefinitions() const {
    return definitions;
}

bool VoucherService::addVoucherDefinition(const VoucherDef& def) {
    // Check for duplicate code
    for (const auto& existing : definitions) {
        if (existing.code == def.code) {
            return false;
        }
    }
    
    definitions.push_back(def);
    saveDefinitions();
    
    // Publish event
    AppEventSystem::getInstance().publish(VoucherCreatedEvent(def.code));
    
    return true;
}

bool VoucherService::updateVoucherDefinition(const VoucherDef& def) {
    for (auto& existing : definitions) {
        if (existing.code == def.code) {
            existing.type = def.type;
            existing.value = def.value;
            existing.minBill = def.minBill;
            existing.description = def.description;
            saveDefinitions();
            return true;
        }
    }
    return false;
}

bool VoucherService::deleteVoucherDefinition(const string& code) {
    auto it = find_if(definitions.begin(), definitions.end(),
        [&code](const VoucherDef& def) { return def.code == code; });
    
    if (it == definitions.end()) return false;
    
    definitions.erase(it);
    saveDefinitions();
    
    // Also remove from user wallets
    vector<string> lines;
    ifstream file(walletPath);
    string line;
    while (getline(file, line)) {
        auto cols = splitString(line, '|');
        if (cols.size() >= 2 && cols[1] != code) {
            lines.push_back(line);
        }
    }
    file.close();
    
    ofstream out(walletPath);
    for (const auto& l : lines) {
        out << l << "\n";
    }
    
    // Remove from auto-provision
    removeAutoProvision(code);
    
    return true;
}

// ===== Distribution Operations =====

bool VoucherService::giveVoucherToUser(const string& email, const string& voucherCode,
                                        int daysToExpire, int quantity) {
    // Check if voucher exists
    bool found = false;
    for (const auto& def : definitions) {
        if (def.code == voucherCode) {
            found = true;
            break;
        }
    }
    if (!found) return false;
    
    string expiry = buildExpiryDate(daysToExpire);
    
    // Check if user already has this voucher
    vector<string> lines;
    ifstream file(walletPath);
    string line;
    bool updated = false;
    bool isFirst = true;
    
    while (getline(file, line)) {
        if (line.empty()) continue;
        
        auto cols = splitString(line, '|');
        if (isFirst && cols.size() > 0 && (cols[0] == "email" || cols[0] == "Email")) {
            lines.push_back(line);
            isFirst = false;
            continue;
        }
        isFirst = false;
        
        if (cols.size() >= 5 && cols[0] == email && cols[1] == voucherCode && cols[2] == "1") {
            // Update existing entry
            int existingQty = stoi(cols[4]);
            cols[4] = to_string(existingQty + quantity);
            cols[3] = expiry; // Update expiry
            
            string newLine = cols[0];
            for (size_t i = 1; i < cols.size(); i++) {
                newLine += "|" + cols[i];
            }
            lines.push_back(newLine);
            updated = true;
        } else {
            lines.push_back(line);
        }
    }
    file.close();
    
    // Add new entry if not updated
    if (!updated) {
        string newLine = email + "|" + voucherCode + "|1|" + expiry + "|" + to_string(quantity);
        lines.push_back(newLine);
    }
    
    ofstream out(walletPath);
    for (const auto& l : lines) {
        out << l << "\n";
    }
    
    return true;
}

int VoucherService::giveVoucherToUsers(const set<string>& emails, const string& voucherCode,
                                       int daysToExpire, int quantity) {
    int count = 0;
    for (const auto& email : emails) {
        if (giveVoucherToUser(email, voucherCode, daysToExpire, quantity)) {
            count++;
        }
    }
    return count;
}

int VoucherService::distributeVoucherToAllActiveUsers(const string& voucherCode,
                                                       int daysToExpire, int quantity) {
    auto users = getActiveUsers();
    int count = 0;
    for (const auto& user : users) {
        if (giveVoucherToUser(user.first, voucherCode, daysToExpire, quantity)) {
            count++;
        }
    }
    return count;
}

bool VoucherService::removeVoucherFromUser(const string& email, const string& voucherCode) {
    vector<string> lines;
    ifstream file(walletPath);
    string line;
    bool removed = false;
    
    while (getline(file, line)) {
        if (line.empty()) continue;
        
        auto cols = splitString(line, '|');
        if (cols.size() >= 2 && cols[0] == email && cols[1] == voucherCode) {
            removed = true;
            continue; // Skip this line
        }
        lines.push_back(line);
    }
    file.close();
    
    ofstream out(walletPath);
    for (const auto& l : lines) {
        out << l << "\n";
    }
    
    return removed;
}

// ===== Auto-Provision Configuration =====

void VoucherService::setAutoProvision(const AutoProvisionConfig& config) {
    // Remove existing config for this voucher
    removeAutoProvision(config.voucherCode);
    
    autoProvisionConfigs.push_back(config);
    saveAutoProvisionConfigs();
}

void VoucherService::removeAutoProvision(const string& voucherCode) {
    auto it = find_if(autoProvisionConfigs.begin(), autoProvisionConfigs.end(),
        [&voucherCode](const AutoProvisionConfig& c) { return c.voucherCode == voucherCode; });
    
    if (it != autoProvisionConfigs.end()) {
        autoProvisionConfigs.erase(it);
        saveAutoProvisionConfigs();
    }
}

vector<AutoProvisionConfig> VoucherService::getAutoProvisionConfigs() const {
    return autoProvisionConfigs;
}

bool VoucherService::isAutoProvisionEnabled(const string& voucherCode) const {
    for (const auto& config : autoProvisionConfigs) {
        if (config.voucherCode == voucherCode && config.enabled) {
            return true;
        }
    }
    return false;
}

// ===== Query Operations =====

vector<pair<string, string>> VoucherService::getActiveUsers() const {
    vector<pair<string, string>> users;
    
    ifstream file(usersPath);
    if (!file.is_open()) return users;
    
    string line;
    while (getline(file, line)) {
        if (line.empty()) continue;
        
        auto cols = splitString(line, '|');
        // Skip header
        if (cols.size() >= 1 && (cols[0] == "email" || cols[0] == "Email")) continue;
        
        // Format: email|password|fullName|phone|role|status
        if (cols.size() >= 6) {
            string status = cols[5];
            // Only include active users (status == "1" or "active")
            if (status == "1" || status == "active") {
                string email = cols[0];
                string fullName = cols.size() > 2 ? cols[2] : "";
                users.push_back({email, fullName});
            }
        }
    }
    
    return users;
}

vector<VoucherService::UserVoucherInfo> VoucherService::getUsersWithVoucher(const string& voucherCode) const {
    vector<UserVoucherInfo> result;
    
    // Load user names
    map<string, string> userNames;
    ifstream usersFile(usersPath);
    string line;
    while (getline(usersFile, line)) {
        auto cols = splitString(line, '|');
        if (cols.size() >= 3 && cols[0] != "email") {
            userNames[cols[0]] = cols[2];
        }
    }
    usersFile.close();
    
    // Load wallet entries
    ifstream walletFile(walletPath);
    while (getline(walletFile, line)) {
        auto cols = splitString(line, '|');
        // email|code|status|expiry|quantity
        if (cols.size() >= 5 && cols[1] == voucherCode && cols[2] == "1") {
            UserVoucherInfo info;
            info.email = cols[0];
            info.fullName = userNames.count(cols[0]) ? userNames[cols[0]] : "";
            info.quantity = stoi(cols[4]);
            info.expiryDate = cols[3];
            result.push_back(info);
        }
    }
    
    return result;
}

int VoucherService::countUsersWithVoucher(const string& voucherCode) const {
    return static_cast<int>(getUsersWithVoucher(voucherCode).size());
}

// ===== Maintenance =====

void VoucherService::cleanupExpiredVouchers() {
    int today = todayAsNumber();
    
    vector<string> lines;
    ifstream file(walletPath);
    string line;
    
    while (getline(file, line)) {
        if (line.empty()) continue;
        
        auto cols = splitString(line, '|');
        // Keep header
        if (cols.size() > 0 && (cols[0] == "email" || cols[0] == "Email")) {
            lines.push_back(line);
            continue;
        }
        
        // email|code|status|expiry|quantity
        if (cols.size() >= 4) {
            int expiry = dateStringToNumber(cols[3]);
            if (expiry >= today || cols[2] == "0") {
                lines.push_back(line);
            }
            // Expired and status=1 entries are removed
        }
    }
    file.close();
    
    ofstream out(walletPath);
    for (const auto& l : lines) {
        out << l << "\n";
    }
}

// ===== Observer Interface =====

void VoucherService::onAppEvent(const AppEvent& event) {
    if (event.getType() == "UserRegistered") {
        handleUserRegistered(static_cast<const UserRegisteredEvent&>(event));
    }
}

vector<string> VoucherService::getSubscribedEvents() const {
    return {"UserRegistered"};
}

void VoucherService::handleUserRegistered(const UserRegisteredEvent& event) {
    // Auto-provision vouchers to new user
    for (const auto& config : autoProvisionConfigs) {
        if (config.enabled) {
            giveVoucherToUser(event.email, config.voucherCode, config.daysToExpire, config.quantity);
        }
    }
}
