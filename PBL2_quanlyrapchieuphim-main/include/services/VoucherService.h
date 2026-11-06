#pragma once

#include <string>
#include <vector>
#include <memory>
#include <set>
#include "services/EventSystem.h"
#include "services/VoucherManager.h"

struct AutoProvisionConfig {
    string voucherCode;
    int daysToExpire = 30;
    int quantity = 1;
    bool enabled = true;
};

class VoucherService : public IAppEventObserver, public enable_shared_from_this<VoucherService> {
public:
    VoucherService(const string& defPath = "../data/voucher_defs.txt",
                   const string& walletPath = "../data/user_wallets.txt",
                   const string& usersPath = "../data/users.txt");
    
    ~VoucherService() override;
    void initialize();

    // Compatibility APIs used by customer booking/account UI
    vector<VoucherDisplay> getVouchersByUser(const string& email);
    double applyVoucher(const string& email, const string& voucherCode,
                        int subtotal, bool commit = false);
    const VoucherDef* getDefinition(const string& code) const;

    vector<VoucherDef> getAllDefinitions() const;
    bool addVoucherDefinition(const VoucherDef& def);
    bool updateVoucherDefinition(const VoucherDef& def);
    bool deleteVoucherDefinition(const string& code);
    bool giveVoucherToUser(const string& email, const string& voucherCode,
                           int daysToExpire, int quantity = 1);
    int giveVoucherToUsers(const set<string>& emails, const string& voucherCode,
                          int daysToExpire, int quantity = 1);
    int distributeVoucherToAllActiveUsers(const string& voucherCode,
                                          int daysToExpire, int quantity = 1);
    bool removeVoucherFromUser(const string& email, const string& voucherCode);
    void setAutoProvision(const AutoProvisionConfig& config);
    void removeAutoProvision(const string& voucherCode);
    vector<AutoProvisionConfig> getAutoProvisionConfigs() const;
    bool isAutoProvisionEnabled(const string& voucherCode) const;
    vector<pair<string, string>> getActiveUsers() const;
    struct UserVoucherInfo {
        string email;
        string fullName;
        int quantity;
        string expiryDate;
    };
    vector<UserVoucherInfo> getUsersWithVoucher(const string& voucherCode) const;
    int countUsersWithVoucher(const string& voucherCode) const;
    void cleanupExpiredVouchers();
    void reload();
    void onAppEvent(const AppEvent& event) override;
    vector<string> getSubscribedEvents() const override;
    
private:
    string definitionPath;
    string walletPath;
    string usersPath;

    VoucherManager voucherManager;
    
    vector<VoucherDef> definitions;
    vector<AutoProvisionConfig> autoProvisionConfigs;
    
    // Helper methods
    void loadDefinitions();
    void saveDefinitions();
    void loadAutoProvisionConfigs();
    void saveAutoProvisionConfigs();
    
    string buildExpiryDate(int daysToExpire) const;
    int todayAsNumber() const;
    int dateStringToNumber(const string& date) const;
    vector<string> splitString(const string& input, char delimiter) const;
    string trim(const string& input) const;
    
    void handleUserRegistered(const UserRegisteredEvent& event);
};
