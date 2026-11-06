#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <chrono>
#include <ctime>
#include "data-structures/DLL.h"
#include "utils/StringUtils.h"
#include "utils/FileUtils.h"

/**
 * @brief Model Voucher thống nhất cho toàn hệ thống
 */
struct Voucher {
    std::string id;             // ID voucher (VD: "V001")
    std::string code;           // Mã voucher (VD: "GIAM50K")
    int discountValue = 0;      // Giá trị giảm (VND hoặc %)
    std::string discountType;   // Loại giảm: "fixed" hoặc "percent"
    int minOrder = 0;           // Đơn hàng tối thiểu
    int maxDiscount = 0;        // Giảm tối đa (cho percent)
    std::string expiryDate;     // Ngày hết hạn (dd/mm/yyyy)
    int quantity = 0;           // Số lượng còn lại
    std::string status;         // active/inactive
    
    Voucher() = default;
    
    bool isActive() const {
        return status == "active" || status.empty();
    }
    
    bool isExpired() const {
        if (expiryDate.empty()) return false;
        
        // Parse dd/mm/yyyy
        auto parts = StringUtils::split(expiryDate, '/');
        if (parts.size() != 3) return false;
        
        try {
            int day = std::stoi(parts[0]);
            int month = std::stoi(parts[1]);
            int year = std::stoi(parts[2]);
            
            auto now = std::chrono::system_clock::now();
            std::time_t now_c = std::chrono::system_clock::to_time_t(now);
            std::tm* localTime = std::localtime(&now_c);
            
            int currentYear = localTime->tm_year + 1900;
            int currentMonth = localTime->tm_mon + 1;
            int currentDay = localTime->tm_mday;
            
            if (year < currentYear) return true;
            if (year > currentYear) return false;
            if (month < currentMonth) return true;
            if (month > currentMonth) return false;
            return day < currentDay;
        } catch (...) {
            return false;
        }
    }
    
    bool isValid() const {
        return isActive() && !isExpired() && quantity > 0;
    }
    
    /**
     * @brief Tính số tiền được giảm
     */
    int calculateDiscount(int orderTotal) const {
        if (!isValid()) return 0;
        if (orderTotal < minOrder) return 0;
        
        if (discountType == "percent") {
            int discount = orderTotal * discountValue / 100;
            if (maxDiscount > 0 && discount > maxDiscount) {
                return maxDiscount;
            }
            return discount;
        }
        
        // Fixed discount
        return discountValue;
    }
    
    std::string getDiscountDisplay() const {
        if (discountType == "percent") {
            return std::to_string(discountValue) + "%";
        }
        // Format fixed amount
        std::string priceStr = std::to_string(discountValue);
        std::string result;
        int count = 0;
        for (int i = priceStr.length() - 1; i >= 0; i--) {
            if (count > 0 && count % 3 == 0) {
                result = "." + result;
            }
            result = priceStr[i] + result;
            count++;
        }
        return result + "đ";
    }
};

/**
 * @brief Voucher của user (wallet)
 */
struct UserVoucher {
    std::string email;          // Email user
    std::string voucherId;      // ID voucher
    bool isUsed = false;        // Đã sử dụng chưa
    
    UserVoucher() = default;
    UserVoucher(const std::string& e, const std::string& v, bool u = false)
        : email(e), voucherId(v), isUsed(u) {}
};

/**
 * @brief Repository thống nhất quản lý Voucher
 * 
 * Tích hợp chức năng của:
 * - UI/components/Admin/VoucherRepository
 * - services/VoucherService (phần data)
 */
class VoucherRepository {
private:
    std::string voucherFilePath;
    std::string userVoucherFilePath;
    std::vector<Voucher> vouchers;
    std::vector<UserVoucher> userVouchers;
    
    /**
     * @brief Tạo ID voucher mới
     */
    std::string generateNewId() const {
        int maxId = 0;
        for (const auto& v : vouchers) {
            if (v.id.length() > 1 && v.id[0] == 'V') {
                try {
                    int id = std::stoi(v.id.substr(1));
                    if (id > maxId) maxId = id;
                } catch (...) {}
            }
        }
        maxId++;
        char buffer[10];
        snprintf(buffer, sizeof(buffer), "V%03d", maxId);
        return std::string(buffer);
    }

public:
    explicit VoucherRepository(
        const std::string& voucherPath = "../data/vouchers.txt",
        const std::string& userVoucherPath = "../data/user_vouchers.txt")
        : voucherFilePath(voucherPath), userVoucherFilePath(userVoucherPath) {
        loadFromFile();
        loadUserVouchers();
    }
    
    // ===== LOAD/SAVE VOUCHERS =====
    
    void loadFromFile() {
        vouchers.clear();
        auto lines = FileUtils::readLines(voucherFilePath);
        
        bool isFirst = true;
        for (const auto& line : lines) {
            if (line.empty()) continue;
            
            auto cols = StringUtils::split(line, '|');
            
            if (isFirst && StringUtils::isHeaderRow(cols, "id")) {
                isFirst = false;
                continue;
            }
            isFirst = false;
            
            if (cols.size() >= 5) {
                Voucher v;
                v.id = cols[0];
                v.code = cols[1];
                v.discountValue = std::stoi(cols[2]);
                v.discountType = cols[3];
                v.minOrder = cols.size() > 4 ? std::stoi(cols[4]) : 0;
                v.maxDiscount = cols.size() > 5 ? std::stoi(cols[5]) : 0;
                v.expiryDate = cols.size() > 6 ? cols[6] : "";
                v.quantity = cols.size() > 7 ? std::stoi(cols[7]) : 0;
                v.status = cols.size() > 8 ? cols[8] : "active";
                vouchers.push_back(v);
            }
        }
    }
    
    void saveToFile() const {
        std::ofstream file(voucherFilePath, std::ios::trunc | std::ios::binary);
        if (!file.is_open()) return;
        
        const char bom[] = { (char)0xEF, (char)0xBB, (char)0xBF };
        file.write(bom, 3);
        
        file << "id|code|discountValue|discountType|minOrder|maxDiscount|expiryDate|quantity|status\n";
        
        for (const auto& v : vouchers) {
            file << v.id << "|" << v.code << "|" << v.discountValue << "|"
                 << v.discountType << "|" << v.minOrder << "|" << v.maxDiscount << "|"
                 << v.expiryDate << "|" << v.quantity << "|" << v.status << "\n";
        }
        file.close();
    }
    
    // ===== LOAD/SAVE USER VOUCHERS =====
    
    void loadUserVouchers() {
        userVouchers.clear();
        auto lines = FileUtils::readLines(userVoucherFilePath);
        
        bool isFirst = true;
        for (const auto& line : lines) {
            if (line.empty()) continue;
            
            auto cols = StringUtils::split(line, '|');
            
            if (isFirst && StringUtils::isHeaderRow(cols, "email")) {
                isFirst = false;
                continue;
            }
            isFirst = false;
            
            if (cols.size() >= 3) {
                UserVoucher uv;
                uv.email = cols[0];
                uv.voucherId = cols[1];
                uv.isUsed = (cols[2] == "1" || cols[2] == "true");
                userVouchers.push_back(uv);
            }
        }
    }
    
    void saveUserVouchers() const {
        std::ofstream file(userVoucherFilePath, std::ios::trunc | std::ios::binary);
        if (!file.is_open()) return;
        
        const char bom[] = { (char)0xEF, (char)0xBB, (char)0xBF };
        file.write(bom, 3);
        
        file << "email|voucherId|isUsed\n";
        
        for (const auto& uv : userVouchers) {
            file << uv.email << "|" << uv.voucherId << "|" << (uv.isUsed ? "1" : "0") << "\n";
        }
        file.close();
    }
    
    void reload() {
        loadFromFile();
        loadUserVouchers();
    }
    
    // ===== READ VOUCHER OPERATIONS =====
    
    const std::vector<Voucher>& getAll() const { return vouchers; }
    
    DLL<Voucher> getAllAsDLL() const {
        DLL<Voucher> result;
        for (const auto& v : vouchers) {
            result.push_back(v);
        }
        return result;
    }
    
    std::vector<Voucher> getActiveVouchers() const {
        std::vector<Voucher> result;
        for (const auto& v : vouchers) {
            if (v.isValid()) {
                result.push_back(v);
            }
        }
        return result;
    }
    
    const Voucher* findById(const std::string& id) const {
        for (const auto& v : vouchers) {
            if (v.id == id) return &v;
        }
        return nullptr;
    }
    
    const Voucher* findByCode(const std::string& code) const {
        for (const auto& v : vouchers) {
            if (v.code == code) return &v;
        }
        return nullptr;
    }
    
    int count() const { return static_cast<int>(vouchers.size()); }
    
    std::vector<std::vector<std::string>> getAllAsTable() const {
        std::vector<std::vector<std::string>> result;
        for (const auto& v : vouchers) {
            result.push_back({v.id, v.code, std::to_string(v.discountValue),
                             v.discountType, std::to_string(v.minOrder),
                             std::to_string(v.maxDiscount), v.expiryDate,
                             std::to_string(v.quantity), v.status});
        }
        return result;
    }
    
    // ===== WRITE VOUCHER OPERATIONS =====
    
    bool add(const Voucher& voucher) {
        Voucher v = voucher;
        if (v.id.empty()) {
            v.id = generateNewId();
        }
        vouchers.push_back(v);
        saveToFile();
        return true;
    }
    
    bool addFromRow(const std::vector<std::string>& row) {
        if (row.size() < 4) return false;
        Voucher v;
        v.id = row[0].empty() ? generateNewId() : row[0];
        v.code = row[1];
        try {
            v.discountValue = std::stoi(row[2]);
        } catch (...) {
            return false;
        }
        v.discountType = row[3];
        v.minOrder = row.size() > 4 ? std::stoi(row[4]) : 0;
        v.maxDiscount = row.size() > 5 ? std::stoi(row[5]) : 0;
        v.expiryDate = row.size() > 6 ? row[6] : "";
        v.quantity = row.size() > 7 ? std::stoi(row[7]) : 0;
        v.status = row.size() > 8 ? row[8] : "active";
        return add(v);
    }
    
    bool update(int index, const Voucher& voucher) {
        if (index < 0 || index >= static_cast<int>(vouchers.size())) return false;
        vouchers[index] = voucher;
        saveToFile();
        return true;
    }
    
    bool updateFromRow(int index, const std::vector<std::string>& row) {
        if (index < 0 || index >= static_cast<int>(vouchers.size())) return false;
        if (row.size() < 4) return false;
        
        Voucher& v = vouchers[index];
        v.id = row[0];
        v.code = row[1];
        v.discountValue = std::stoi(row[2]);
        v.discountType = row[3];
        v.minOrder = row.size() > 4 ? std::stoi(row[4]) : v.minOrder;
        v.maxDiscount = row.size() > 5 ? std::stoi(row[5]) : v.maxDiscount;
        v.expiryDate = row.size() > 6 ? row[6] : v.expiryDate;
        v.quantity = row.size() > 7 ? std::stoi(row[7]) : v.quantity;
        v.status = row.size() > 8 ? row[8] : v.status;
        
        saveToFile();
        return true;
    }
    
    bool remove(int index) {
        if (index < 0 || index >= static_cast<int>(vouchers.size())) return false;
        vouchers.erase(vouchers.begin() + index);
        saveToFile();
        return true;
    }
    
    void deleteRecord(int index) { remove(index); }
    
    /**
     * @brief Sử dụng voucher (giảm quantity)
     */
    bool useVoucher(const std::string& id) {
        for (auto& v : vouchers) {
            if (v.id == id && v.isValid()) {
                v.quantity--;
                saveToFile();
                return true;
            }
        }
        return false;
    }
    
    // ===== USER VOUCHER OPERATIONS =====
    
    /**
     * @brief Lấy vouchers của user
     */
    std::vector<Voucher> getUserVouchers(const std::string& email, bool includeUsed = false) const {
        std::vector<Voucher> result;
        for (const auto& uv : userVouchers) {
            if (uv.email == email && (includeUsed || !uv.isUsed)) {
                auto* v = findById(uv.voucherId);
                if (v) {
                    result.push_back(*v);
                }
            }
        }
        return result;
    }
    
    /**
     * @brief Thêm voucher cho user
     */
    bool addVoucherToUser(const std::string& email, const std::string& voucherId) {
        // Check if already has
        for (const auto& uv : userVouchers) {
            if (uv.email == email && uv.voucherId == voucherId && !uv.isUsed) {
                return false; // Already has unused voucher
            }
        }
        
        userVouchers.push_back(UserVoucher(email, voucherId, false));
        saveUserVouchers();
        return true;
    }
    
    /**
     * @brief Đánh dấu voucher của user đã sử dụng
     */
    bool markUserVoucherUsed(const std::string& email, const std::string& voucherId) {
        for (auto& uv : userVouchers) {
            if (uv.email == email && uv.voucherId == voucherId && !uv.isUsed) {
                uv.isUsed = true;
                saveUserVouchers();
                return true;
            }
        }
        return false;
    }
    
    /**
     * @brief Kiểm tra user có voucher không
     */
    bool userHasVoucher(const std::string& email, const std::string& voucherId) const {
        for (const auto& uv : userVouchers) {
            if (uv.email == email && uv.voucherId == voucherId && !uv.isUsed) {
                return true;
            }
        }
        return false;
    }
};
