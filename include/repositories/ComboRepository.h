#pragma once

#include <string>
#include <vector>
#include "data-structures/DLL.h"
#include "utils/StringUtils.h"
#include "utils/FileUtils.h"

/**
 * @brief Model Combo thống nhất cho toàn hệ thống
 * 
 * Được sử dụng bởi cả:
 * - Admin Panel (quản lý combo)
 * - Booking Flow (chọn combo khi đặt vé)
 */
struct Combo {
    std::string id;         // ID combo (VD: "C01")
    std::string name;       // Tên combo
    std::string description;// Mô tả
    int price = 0;          // Giá (VND)
    
    Combo() = default;
    Combo(const std::string& id, const std::string& name, int price, const std::string& desc = "")
        : id(id), name(name), description(desc), price(price) {}
    
    /**
     * @brief Format giá với dấu chấm phân cách
     */
    std::string getPriceFormatted() const {
        std::string priceStr = std::to_string(price);
        std::string result;
        int count = 0;
        for (int i = priceStr.length() - 1; i >= 0; i--) {
            if (count > 0 && count % 3 == 0) {
                result = "." + result;
            }
            result = priceStr[i] + result;
            count++;
        }
        return result + " đ";
    }
};

/**
 * @brief Repository thống nhất quản lý Combo
 * 
 * Tích hợp chức năng của:
 * - UI/components/Admin/ComboRepository (CRUD cho admin)
 * - UI/components/TicketBooking/ComboRepository (load cho booking)
 * 
 * Design: Repository Pattern + Single Responsibility
 * REFACTORED: Sử dụng DLL thay vì std::vector cho internal storage
 */
class ComboRepository {
private:
    std::string filePath;
    DLL<Combo> combos;  // Changed from std::vector<Combo> to DLL<Combo>
    
    /**
     * @brief Tạo ID mới cho combo
     */
    std::string generateNewId() const {
        int maxId = 0;
        for (const auto& combo : combos) {  // DLL iterator
            if (combo.id.length() > 1 && combo.id[0] == 'C') {
                try {
                    int id = std::stoi(combo.id.substr(1));
                    if (id > maxId) maxId = id;
                } catch (...) {}
            }
        }
        maxId++;
        char buffer[10];
        snprintf(buffer, sizeof(buffer), "C%02d", maxId);
        return std::string(buffer);
    }

public:
    explicit ComboRepository(const std::string& path = "../data/combo.txt")
        : filePath(path) {
        loadFromFile();
    }
    
    // ===== LOAD/SAVE =====
    
    /**
     * @brief Load combos từ file
     */
    void loadFromFile() {
        combos.clear();
        auto lines = FileUtils::readLines(filePath);
        
        bool isFirst = true;
        for (const auto& line : lines) {
            if (line.empty()) continue;
            
            auto cols = StringUtils::split(line, '|');
            
            // Skip header
            if (isFirst && StringUtils::isHeaderRow(cols, "combo_id")) {
                isFirst = false;
                continue;
            }
            isFirst = false;
            
            if (cols.size() >= 3) {
                Combo combo;
                combo.id = cols[0];
                combo.name = cols[1];
                combo.price = std::stoi(cols[2]);
                combo.description = (cols.size() > 3) ? cols[3] : "";
                combos.push_back(combo);
            }
        }
    }
    
    /**
     * @brief Lưu combos ra file
     */
    void saveToFile() const {
        std::vector<std::string> lines;
        lines.push_back("combo_id|combo_name|price");
        
        for (const auto& combo : combos) {  // DLL iterator
            lines.push_back(combo.id + "|" + combo.name + "|" + 
                           std::to_string(combo.price));
        }
        
        FileUtils::writeAll(filePath, StringUtils::join(lines, "\n"));
    }
    
    /**
     * @brief Reload data từ file
     */
    void reload() { loadFromFile(); }
    
    // ===== READ OPERATIONS =====
    
    /**
     * @brief Return DLL reference for iteration
     * NOTE: Changed from std::vector to DLL
     */
    const DLL<Combo>& getAll() const { return combos; }
    DLL<Combo>& getAll() { return combos; }
    
    /**
     * @brief Legacy support: Convert to vector when needed
     */
    std::vector<Combo> getAllAsVector() const {
        std::vector<Combo> result;
        result.reserve(combos.size());
        for (const auto& combo : combos) {
            result.push_back(combo);
        }
        return result;
    }
    
    /**
     * @brief Lấy combos dưới dạng DLL (copy)
     */
    DLL<Combo> getAllAsDLL() const {
        return combos;  // Copy constructor
    }
    
    /**
     * @brief Lấy combo theo index
     */
    const Combo* getByIndex(int index) const {
        if (index < 0 || index >= combos.size()) {
            return nullptr;
        }
        return &combos[index];
    }
    
    /**
     * @brief Tìm combo theo ID
     */
    const Combo* findById(const std::string& id) const {
        for (const auto& combo : combos) {
            if (combo.id == id) return &combo;
        }
        return nullptr;
    }
    
    /**
     * @brief Đếm số combos
     */
    int count() const { return combos.size(); }
    
    /**
     * @brief Lấy data dạng vector<vector<string>> cho EditableTable
     * NOTE: Returns std::vector for UI compatibility
     */
    std::vector<std::vector<std::string>> getAllAsTable() const {
        std::vector<std::vector<std::string>> result;
        for (const auto& combo : combos) {
            result.push_back({combo.id, combo.name, std::to_string(combo.price), combo.description});
        }
        return result;
    }
    
    // ===== WRITE OPERATIONS (Admin) =====
    
    /**
     * @brief Thêm combo mới
     * @param name Tên combo
     * @param price Giá
     * @param description Mô tả
     * @return Combo đã thêm (với ID tự sinh)
     */
    Combo add(const std::string& name, int price, const std::string& description = "") {
        Combo combo;
        combo.id = generateNewId();
        combo.name = name;
        combo.price = price;
        combo.description = description;
        combos.push_back(combo);
        saveToFile();
        return combo;
    }
    
    /**
     * @brief Thêm combo từ vector (cho EditableTable)
     */
    void addRecord(const std::vector<std::string>& record) {
        if (record.size() >= 3) {
            add(record[1], std::stoi(record[2]), record.size() > 3 ? record[3] : "");
        }
    }
    
    /**
     * @brief Cập nhật combo theo index
     */
    bool update(int index, const std::string& name, int price, const std::string& description = "") {
        if (index < 0 || index >= combos.size()) {
            return false;
        }
        combos[index].name = name;
        combos[index].price = price;
        combos[index].description = description;
        saveToFile();
        return true;
    }
    
    /**
     * @brief Cập nhật combo từ vector (cho EditableTable)
     */
    void updateRecord(int index, const std::vector<std::string>& record) {
        if (index >= 0 && index < combos.size() && record.size() >= 3) {
            combos[index].id = record[0];
            combos[index].name = record[1];
            combos[index].price = std::stoi(record[2]);
            combos[index].description = record.size() > 3 ? record[3] : "";
            saveToFile();
        }
    }
    
    /**
     * @brief Xóa combo theo index
     */
    bool remove(int index) {
        if (index < 0 || index >= combos.size()) {
            return false;
        }
        combos.removeAt(index);  // DLL method
        saveToFile();
        return true;
    }
    
    /**
     * @brief Xóa combo (alias cho EditableTable)
     */
    void deleteRecord(int index) { remove(index); }
};
