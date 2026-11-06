#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <utility>
#include <functional>

struct VoucherDefinition {
    std::string code;           // Mã voucher (unique key)
    int type = 1;          // 1: fixed amount, 2: percentage
    double value = 0.0;    // Giá trị giảm (VND hoặc %)
    double minBill = 0.0;  // Hóa đơn tối thiểu để áp dụng
    std::string description;    // Mô tả voucher

    VoucherDefinition() = default;
    VoucherDefinition(const std::string& c, int t, double v, double m, const std::string& d);
};

struct UserVoucherEntry {
    std::string email;          // Email người nhận
    std::string code;           // Mã voucher
    int status = 1;        // 1: usable, 0: used/expired
    std::string expiryDate;     // Ngày hết hạn (YYYYMMDD)
    int quantity = 1;      // Số lượng còn lại

    UserVoucherEntry() = default;
    UserVoucherEntry(const std::string& e, const std::string& c, int s, const std::string& exp, int qty);
};

struct VoucherUserInfo {
    std::string email;
    std::string fullName;
    int quantity;
    std::string expiryDate;
    int status;
};

class AdminVoucherRepository {
public:
    AdminVoucherRepository(
        const std::string& defPath = "../data/voucher_defs.txt",
        const std::string& walletPath = "../data/user_wallets.txt",
        const std::string& usersPath = "../data/users.txt"
    );

    void reload();

    // ===== VOUCHER DEFINITION CRUD =====
    void loadDefinitions();
    void saveDefinitions();
    std::vector<VoucherDefinition> getAllDefinitions() const;
    const VoucherDefinition* findDefinition(const std::string& code) const;
    bool addDefinition(const VoucherDefinition& def);
    bool updateDefinition(const VoucherDefinition& def);
    bool deleteDefinition(const std::string& code);

    // ===== USER VOUCHER MANAGEMENT =====
    void loadUserVouchers();
    void saveUserVouchers();
    void cleanupExpiredVouchers();
    bool giveVoucherToUser(const std::string& email, const std::string& code, int daysToExpire, int quantity = 1);
    int giveVoucherToAllActiveUsers(const std::string& code, int daysToExpire, int quantity = 1);
    bool removeVoucherFromUser(const std::string& email, const std::string& code);
    std::vector<VoucherUserInfo> getUsersWithVoucher(const std::string& code) const;
    int countUsersWithVoucher(const std::string& code) const;

    // ===== USER QUERY =====
    std::vector<std::pair<std::string, std::string>> getActiveUsers() const;

private:
    std::string definitionPath;
    std::string walletPath;
    std::string usersPath;

    std::vector<VoucherDefinition> definitions;
    std::unordered_map<std::string, VoucherDefinition> definitionLookup;
    std::vector<UserVoucherEntry> userVouchers;

    static std::string trim(const std::string& s);
    static std::string toUpperCase(const std::string& s);
    static bool isHeaderRow(const std::vector<std::string>& cols, const std::string& firstColName);
    static std::vector<std::string> splitString(const std::string& s, char delimiter);

    static int todayAsNumber();
    static int dateStringToNumber(const std::string& yyyymmdd);
    static std::string buildExpiryDate(int daysToExpire);
};
