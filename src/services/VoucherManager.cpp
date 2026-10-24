#include "services/VoucherManager.h"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <sstream>

using namespace std;

VoucherManager::VoucherManager(const string& defPath, const string& walletPathValue)
    : definitionPath(defPath), walletPath(walletPathValue) {
    loadData();
}

void VoucherManager::loadData() {
    voucherDefs.clear();
    voucherLookup.clear();

    ifstream file(definitionPath);
    if (!file.is_open()) return;

    string line;
    while (getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        vector<string> cols = splitString(line, '|');
        if (cols.size() < 5) continue;
        if (isHeaderRow(cols, "code")) continue;

        VoucherDef def;
        def.code = trim(cols[0]);
        def.type = stoi(cols[1]);
        def.value = stod(cols[2]);
        def.minBill = stod(cols[3]);
        def.description = trim(cols[4]);

        voucherDefs.push_back(def);
        voucherLookup[def.code] = def;
    }
}

void VoucherManager::giveVoucher(const string& email, const string& code, int daysToExpire, int quantity) {
    string expiry = buildExpiryDate(daysToExpire);

    bool needHeader = false;
    {
        ifstream check(walletPath);
        if (!check.is_open() || check.peek() == ifstream::traits_type::eof()) {
            needHeader = true;
        }
    }

    ofstream file(walletPath, ios::app);
    if (!file.is_open()) return;
    if (needHeader) {
        file << "email|code|status|expiry_date|quantity\n";
    }
    file << email << '|' << code << "|1|" << expiry << '|' << quantity << '\n';
}

vector<VoucherDisplay> VoucherManager::getVouchersByUser(const string& email) {
    vector<VoucherDisplay> result;
    vector<UserVoucher> wallet = loadWallet();
    int today = todayAsNumber();

    for (const auto& voucher : wallet) {
        if (voucher.email != email) continue;
        
        // Bỏ qua voucher hết hạn hoặc hết số lượng
        if (dateStringToNumber(voucher.expiry) < today || voucher.quantity <= 0) continue;
        
        auto it = voucherLookup.find(voucher.code);
        if (it == voucherLookup.end()) continue;

        VoucherDisplay display;
        display.code = voucher.code;
        display.description = it->second.description;
        display.value = it->second.value;
        display.type = it->second.type;
        display.status = voucher.status;
        display.expiry = voucher.expiry;
        display.quantity = voucher.quantity;
        result.push_back(display);
    }

    return result;
}

double VoucherManager::applyVoucher(const string& email, const string& code, double totalBill, bool consume) {
    vector<UserVoucher> wallet = loadWallet();
    bool updated = false;
    double discount = 0.0;
    int today = todayAsNumber();

    for (auto& voucher : wallet) {
        if (voucher.email != email || voucher.code != code) continue;
        if (voucher.status != 1) break;
        if (dateStringToNumber(voucher.expiry) < today) break;
        if (voucher.quantity <= 0) break;

        auto defIt = voucherLookup.find(code);
        if (defIt == voucherLookup.end()) break;
        const auto& def = defIt->second;
        if (totalBill < def.minBill) break;

        if (def.type == 1) {
            discount = min(def.value, totalBill);
        } else {
            discount = min(totalBill, totalBill * def.value / 100.0);
        }

        if (discount > 0.0 && consume) {
            voucher.quantity--;
            updated = true;
        }
        break;
    }

    if (updated) {
        persistWallet(wallet);
    }

    return discount;
}

vector<string> VoucherManager::splitString(const string& input, char delimiter) const {
    vector<string> tokens;
    string token;
    stringstream ss(input);
    while (getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

string VoucherManager::trim(const string& input) const {
    const char* whitespace = " \t\r\n";
    size_t start = input.find_first_not_of(whitespace);
    if (start == string::npos) return "";
    size_t end = input.find_last_not_of(whitespace);
    return input.substr(start, end - start + 1);
}

string VoucherManager::buildExpiryDate(int daysToExpire) const {
    using namespace chrono;
    auto now = system_clock::now();
    auto future = now + chrono::hours(24 * daysToExpire);
    time_t futureTime = system_clock::to_time_t(future);
    tm tmValue;
#if defined(_WIN32)
    localtime_s(&tmValue, &futureTime);
#else
    localtime_r(&futureTime, &tmValue);
#endif
    char buffer[9];
    strftime(buffer, sizeof(buffer), "%Y%m%d", &tmValue);
    return string(buffer);
}

int VoucherManager::todayAsNumber() const {
    using namespace chrono;
    auto now = system_clock::now();
    time_t nowTime = system_clock::to_time_t(now);
    tm tmValue;
#if defined(_WIN32)
    localtime_s(&tmValue, &nowTime);
#else
    localtime_r(&nowTime, &tmValue);
#endif
    return (tmValue.tm_year + 1900) * 10000 + (tmValue.tm_mon + 1) * 100 + tmValue.tm_mday;
}

int VoucherManager::dateStringToNumber(const string& date) const {
    if (date.length() != 8) return 0;
    return stoi(date);
}

void VoucherManager::persistWallet(const vector<UserVoucher>& wallet) const {
    ofstream file(walletPath, ios::trunc);
    if (!file.is_open()) return;
    file << "email|code|status|expiry_date|quantity\n";
    for (const auto& voucher : wallet) {
        file << voucher.email << '|' << voucher.code << '|' << voucher.status << '|' << voucher.expiry << '|' << voucher.quantity << '\n';
    }
}

vector<UserVoucher> VoucherManager::loadWallet() const {
    vector<UserVoucher> wallet;
    ifstream file(walletPath);
    if (!file.is_open()) return wallet;

    string line;
    while (getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        vector<string> cols = splitString(line, '|');
        if (cols.size() < 4) continue;

        if (isHeaderRow(cols, "email")) continue;

        UserVoucher voucher;
        voucher.email = trim(cols[0]);
        voucher.code = trim(cols[1]);
        voucher.status = stoi(cols[2]);
        voucher.expiry = trim(cols[3]);
        voucher.quantity = (cols.size() >= 5) ? stoi(cols[4]) : 1;
        wallet.push_back(voucher);
    }

    return wallet;
}

bool VoucherManager::isHeaderRow(const vector<string>& cols, const string& expectedFirstCell) const {
    if (cols.empty()) return false;
    string cell = trim(cols[0]);
    transform(cell.begin(), cell.end(), cell.begin(), [](unsigned char ch) {
        return static_cast<char>(tolower(ch));
    });
    string expected = expectedFirstCell;
    transform(expected.begin(), expected.end(), expected.begin(), [](unsigned char ch) {
        return static_cast<char>(tolower(ch));
    });
    return cell == expected;
}

void VoucherManager::cleanupExpiredVouchers() {
    vector<UserVoucher> wallet = loadWallet();
    int today = todayAsNumber();
    vector<UserVoucher> validVouchers;

    for (const auto& voucher : wallet) {
        // Chỉ giữ lại voucher còn hạn và còn số lượng
        if (dateStringToNumber(voucher.expiry) >= today && voucher.quantity > 0) {
            validVouchers.push_back(voucher);
        }
    }

    // Chỉ persist nếu có thay đổi
    if (validVouchers.size() != wallet.size()) {
        persistWallet(validVouchers);
    }
}
