#pragma once

#include <string>
#include <vector>
#include "data-structures/DLL.h"
#include "utils/StringUtils.h"
#include "utils/FileUtils.h"

/**
 * @brief Model Showtime thống nhất cho toàn hệ thống
 */
struct Showtime {
    std::string showtimeId;     // ID suất chiếu
    std::string movieId;        // ID phim
    std::string roomId;         // ID phòng
    std::string date;           // Ngày chiếu (YYYY-MM-DD)
    std::string time;           // Giờ chiếu (HH:MM)
    int price = 0;              // Giá vé
    
    Showtime() = default;
    
    /**
     * @brief Lấy display string cho UI
     */
    std::string getDisplayTime() const {
        return time;
    }
    
    /**
     * @brief Format giá với dấu chấm
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
 * @brief Repository thống nhất quản lý Showtime
 * 
 * Design: Repository Pattern
 * REFACTORED: Sử dụng DLL thay vì std::vector cho internal storage
 */
class ShowtimeRepository {
private:
    std::string filePath;
    DLL<Showtime> showtimes;  // Changed from std::vector<Showtime> to DLL<Showtime>
    
    /**
     * @brief Tạo ID mới
     */
    std::string generateNewId() const {
        int maxId = 0;
        for (const auto& st : showtimes) {  // DLL iterator
            if (st.showtimeId.length() > 2 && st.showtimeId.substr(0, 2) == "ST") {
                try {
                    int id = std::stoi(st.showtimeId.substr(2));
                    if (id > maxId) maxId = id;
                } catch (...) {}
            }
        }
        maxId++;
        char buffer[10];
        snprintf(buffer, sizeof(buffer), "ST%04d", maxId);
        return std::string(buffer);
    }

public:
    explicit ShowtimeRepository(const std::string& path = "../data/showtimes.txt")
        : filePath(path) {
        loadFromFile();
    }
    
    // ===== LOAD/SAVE =====
    
    void loadFromFile() {
        showtimes.clear();
        auto lines = FileUtils::readLines(filePath);
        
        bool isFirst = true;
        for (const auto& line : lines) {
            if (line.empty()) continue;
            
            auto cols = StringUtils::split(line, '|');
            
            if (isFirst && StringUtils::isHeaderRow(cols, "showtime_id")) {
                isFirst = false;
                continue;
            }
            isFirst = false;
            
            if (cols.size() >= 6) {
                Showtime st;
                st.showtimeId = cols[0];
                st.movieId = cols[1];
                st.roomId = cols[2];
                st.date = cols[3];
                st.time = cols[4];
                st.price = std::stoi(cols[5]);
                showtimes.push_back(st);
            }
        }
    }
    
    void saveToFile() const {
        std::vector<std::string> lines;
        lines.push_back("showtime_id|movie_id|room_id|date|time|price");
        
        for (const auto& st : showtimes) {  // DLL iterator
            lines.push_back(st.showtimeId + "|" + st.movieId + "|" + st.roomId + "|" +
                           st.date + "|" + st.time + "|" + std::to_string(st.price));
        }
        
        FileUtils::writeAll(filePath, StringUtils::join(lines, "\n"));
    }
    
    void reload() { loadFromFile(); }
    
    // ===== READ OPERATIONS =====
    
    /**
     * @brief Return DLL reference for iteration
     * NOTE: Changed from std::vector to DLL
     */
    const DLL<Showtime>& getAll() const { return showtimes; }
    DLL<Showtime>& getAll() { return showtimes; }

    /**
     * @brief Legacy support: Convert to vector when needed (e.g., for UI components)
     */
    std::vector<Showtime> getAllAsVector() const {
        std::vector<Showtime> result;
        result.reserve(showtimes.size());
        for (const auto& st : showtimes) {
            result.push_back(st);
        }
        return result;
    }
    
    DLL<Showtime> getAllAsDLL() const {
        return showtimes;  // Copy constructor
    }
    
    /**
     * @brief Lấy showtimes theo movie ID
     */
    DLL<Showtime> getByMovieId(const std::string& movieId) const {
        DLL<Showtime> result;
        for (const auto& st : showtimes) {
            if (st.movieId == movieId) {
                result.push_back(st);
            }
        }
        return result;
    }
    
    /**
     * @brief Lấy showtimes theo ngày
     */
    DLL<Showtime> getByDate(const std::string& date) const {
        DLL<Showtime> result;
        for (const auto& st : showtimes) {
            if (st.date == date) {
                result.push_back(st);
            }
        }
        return result;
    }
    
    /**
     * @brief Tìm theo ID
     */
    const Showtime* findById(const std::string& id) const {
        for (const auto& st : showtimes) {
            if (st.showtimeId == id) return &st;
        }
        return nullptr;
    }
    
    int count() const { return showtimes.size(); }
    
    /**
     * @brief Lấy dữ liệu dạng table cho UI
     * NOTE: Returns std::vector for UI compatibility
     */
    std::vector<std::vector<std::string>> getAllAsTable() const {
        std::vector<std::vector<std::string>> result;
        for (const auto& st : showtimes) {
            result.push_back({st.showtimeId, st.movieId, st.roomId, 
                             st.date, st.time, std::to_string(st.price)});
        }
        return result;
    }
    
    // ===== WRITE OPERATIONS =====
    
    Showtime add(const std::string& movieId, const std::string& roomId,
                 const std::string& date, const std::string& time, int price) {
        Showtime st;
        st.showtimeId = generateNewId();
        st.movieId = movieId;
        st.roomId = roomId;
        st.date = date;
        st.time = time;
        st.price = price;
        showtimes.push_back(st);
        saveToFile();
        return st;
    }
    
    void addRecord(const std::vector<std::string>& record) {
        if (record.size() >= 5) {
            add(record[1], record[2], record[3], record[4], 
                record.size() > 5 ? std::stoi(record[5]) : 0);
        }
    }
    
    bool update(int index, const std::string& movieId, const std::string& roomId,
                const std::string& date, const std::string& time, int price) {
        if (index < 0 || index >= showtimes.size()) return false;
        showtimes[index].movieId = movieId;
        showtimes[index].roomId = roomId;
        showtimes[index].date = date;
        showtimes[index].time = time;
        showtimes[index].price = price;
        saveToFile();
        return true;
    }
    
    void updateRecord(int index, const std::vector<std::string>& record) {
        if (index >= 0 && index < showtimes.size() && record.size() >= 6) {
            showtimes[index].showtimeId = record[0];
            showtimes[index].movieId = record[1];
            showtimes[index].roomId = record[2];
            showtimes[index].date = record[3];
            showtimes[index].time = record[4];
            showtimes[index].price = std::stoi(record[5]);
            saveToFile();
        }
    }
    
    bool remove(int index) {
        if (index < 0 || index >= showtimes.size()) return false;
        showtimes.removeAt(index);  // DLL method
        saveToFile();
        return true;
    }
    
    void deleteRecord(int index) { remove(index); }
    
    /**
     * @brief Xóa theo showtime ID
     */
    bool removeById(const std::string& id) {
        int index = 0;
        for (const auto& st : showtimes) {
            if (st.showtimeId == id) {
                showtimes.removeAt(index);
                saveToFile();
                return true;
            }
            index++;
        }
        return false;
    }
};
