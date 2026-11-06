#pragma once

#include <string>
#include <vector>
#include <unordered_map>

struct VoucherDef {
    std::string code;
    int type = 0;          // 1: fixed amount, 2: percentage
    double value = 0.0;
    double minBill = 0.0;
    std::string description;
};

struct UserVoucher {
    std::string email;
    std::string code;
    int status = 0;        // 1: usable, 0: used/expired
    std::string expiry;    // YYYYMMDD
    int quantity = 1;      // số lượng còn lại
};

struct VoucherDisplay {
    std::string code;
    std::string description;
    double value = 0.0;
    int type = 0;
    int status = 0;
    std::string expiry;
    int quantity = 1;
};

class VoucherManager {
public:
    VoucherManager(const std::string& defPath = "../data/voucher_defs.txt",
                   const std::string& walletPath = "../data/user_wallets.txt");

    void loadData();
    void giveVoucher(const std::string& email, const std::string& code, int daysToExpire, int quantity = 1);
    std::vector<VoucherDisplay> getVouchersByUser(const std::string& email);
    double applyVoucher(const std::string& email, const std::string& code, double totalBill, bool consume = true);
    void cleanupExpiredVouchers();

    // Public để VoucherListView có thể truy cập thông tin definition
    std::unordered_map<std::string, VoucherDef> voucherLookup;

private:
    std::string definitionPath;
    std::string walletPath;
    std::vector<VoucherDef> voucherDefs;

    std::vector<std::string> splitString(const std::string& input, char delimiter) const;
    std::string trim(const std::string& input) const;
    std::string buildExpiryDate(int daysToExpire) const;
    int todayAsNumber() const;
    int dateStringToNumber(const std::string& date) const;
    void persistWallet(const std::vector<UserVoucher>& wallet) const;
    std::vector<UserVoucher> loadWallet() const;
    bool isHeaderRow(const std::vector<std::string>& cols, const std::string& expectedFirstCell) const;
};
