#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <ctime>
#include <fstream>
#include "data-structures/DLL.h"
#include "utils/StringUtils.h"
#include "utils/FileUtils.h"

struct Ticket {
    string ticketId;
    string showtimeId;
    string title;
    string date;
    string time;
    string roomName;
    string booked;
    string comboName;
    int price = 0;
    string email;
    string fullName;
    string bookedDate;
    string bookedTime;
    
    Ticket() = default;
    
    int getSeatCount() const {
        if (booked.empty()) return 0;
        int count = 1;
        for (char c : booked) {
            if (c == ',') count++;
        }
        return count;
    }

    string getPriceFormatted() const {
        string priceStr = to_string(price);
        string result;
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
 * @brief Repository thống nhất quản lý Ticket
 * REFACTORED: Sử dụng DLL thay vì std::vector cho internal storage
 */
class TicketRepository {
private:
    string filePath;
    DLL<Ticket> tickets;  // Changed from vector<Ticket> to DLL<Ticket>

    string generateNewId() const {
        int maxId = 0;
        for (const auto& t : tickets) {  // DLL iterator
            if (t.ticketId.length() > 1 && t.ticketId[0] == 'T') {
                try {
                    int id = stoi(t.ticketId.substr(1));
                    if (id > maxId) maxId = id;
                } catch (...) {}
            }
        }
        maxId++;
        char buffer[10];
        snprintf(buffer, sizeof(buffer), "T%04d", maxId);
        return string(buffer);
    }
    
    string getCurrentDate() const {
        auto now = chrono::system_clock::now();
        time_t now_c = chrono::system_clock::to_time_t(now);
        tm* localTime = localtime(&now_c);
        char buffer[11];
        strftime(buffer, sizeof(buffer), "%d/%m/%Y", localTime);
        return string(buffer);
    }

    string getCurrentTime() const {
        auto now = chrono::system_clock::now();
        time_t now_c = chrono::system_clock::to_time_t(now);
        tm* localTime = localtime(&now_c);
        char buffer[9];
        strftime(buffer, sizeof(buffer), "%H:%M:%S", localTime);
        return string(buffer);
    }

public:
    explicit TicketRepository(const string& path = "../data/tickets.txt")
        : filePath(path) {
        loadFromFile();
    }
    
    void loadFromFile() {
        tickets.clear();
        auto lines = FileUtils::readLines(filePath);
        
        bool isFirst = true;
        for (const auto& line : lines) {
            if (line.empty()) continue;
            
            auto cols = StringUtils::split(line, '|');
            
            if (isFirst && StringUtils::isHeaderRow(cols, "ticket_id")) {
                isFirst = false;
                continue;
            }
            isFirst = false;
            
            if (cols.size() >= 11) {
                Ticket t;
                t.ticketId = cols[0];
                t.showtimeId = cols[1];
                t.title = cols[2];
                t.date = cols[3];
                t.time = cols[4];
                t.roomName = cols[5];
                t.booked = cols[6];
                t.comboName = cols[7];
                t.price = stoi(cols[8]);
                t.email = cols[9];
                t.fullName = cols[10];
                t.bookedDate = cols.size() > 11 ? cols[11] : "";
                t.bookedTime = cols.size() > 12 ? cols[12] : "";
                tickets.push_back(t);
            }
        }
    }
    
    void saveToFile() const {
        ofstream file(filePath, ios::trunc | ios::binary);
        if (!file.is_open()) return;
        
        const char bom[] = { (char)0xEF, (char)0xBB, (char)0xBF };
        file.write(bom, 3);
        
        file << "ticket_id|showtime_id|title|date|time|room_name|booked|combo_name|price|email|fullName|booked_date|booked_time\n";
        
        for (const auto& t : tickets) {  // DLL iterator
            file << t.ticketId << "|" << t.showtimeId << "|" << t.title << "|"
                 << t.date << "|" << t.time << "|" << t.roomName << "|"
                 << t.booked << "|" << t.comboName << "|" << t.price << "|"
                 << t.email << "|" << t.fullName << "|"
                 << t.bookedDate << "|" << t.bookedTime << "\n";
        }
        file.close();
    }
    
    void reload() { loadFromFile(); }
    
    /**
     * @brief Return DLL reference for iteration
     * NOTE: Changed from std::vector to DLL
     */
    const DLL<Ticket>& getAll() const { return tickets; }
    DLL<Ticket>& getAll() { return tickets; }
    
    /**
     * @brief Legacy support: Convert to vector when needed
     */
    vector<Ticket> getAllAsVector() const {
        vector<Ticket> result;
        result.reserve(tickets.size());
        for (const auto& t : tickets) {
            result.push_back(t);
        }
        return result;
    }
    
    DLL<Ticket> getAllAsDLL() const {
        return tickets;  // Copy constructor
    }

    DLL<Ticket> getByEmail(const string& email) const {
        DLL<Ticket> result;
        for (const auto& t : tickets) {
            if (t.email == email) {
                result.push_back(t);
            }
        }
        return result;
    }

    DLL<Ticket> getByDate(const string& date) const {
        DLL<Ticket> result;
        for (const auto& t : tickets) {
            if (t.date == date) {
                result.push_back(t);
            }
        }
        return result;
    }
    
    const Ticket* findById(const string& id) const {
        for (const auto& t : tickets) {
            if (t.ticketId == id) return &t;
        }
        return nullptr;
    }
    
    int count() const { return tickets.size(); }
    
    /**
     * @brief Lấy dữ liệu dạng table cho UI
     * NOTE: Returns std::vector for UI compatibility
     */
    vector<vector<string>> getAllAsTable() const {
        vector<vector<string>> result;
        for (const auto& t : tickets) {
            result.push_back({t.ticketId, t.showtimeId, t.title, t.date, t.time,
                             t.roomName, t.booked, t.comboName, to_string(t.price),
                             t.email, t.fullName, t.bookedDate, t.bookedTime});
        }
        return result;
    }

    Ticket createTicket(
        const string& showtimeId,
        const string& title,
        const string& date,
        const string& time,
        const string& roomName,
        const string& booked,
        const string& comboName,
        int price,
        const string& email,
        const string& fullName
    ) {
        Ticket t;
        t.ticketId = generateNewId();
        t.showtimeId = showtimeId;
        t.title = title;
        t.date = date;
        t.time = time;
        t.roomName = roomName;
        t.booked = booked;
        t.comboName = comboName;
        t.price = price;
        t.email = email;
        t.fullName = fullName;
        t.bookedDate = getCurrentDate();
        t.bookedTime = getCurrentTime();
        
        tickets.push_back(t);
        
        appendTicket(t);
        
        return t;
    }

    void appendTicket(const Ticket& t) {
        ofstream file(filePath, ios::app | ios::binary);
        if (!file.is_open()) return;
        
        file.seekp(0, ios::end);
        if (file.tellp() == 0) {
            const char bom[] = { (char)0xEF, (char)0xBB, (char)0xBF };
            file.write(bom, 3);
            file << "ticketId|showtimeId|title|date|time|roomName|booked|comboName|price|email|fullName|bookedDate|bookedTime\n";
        }
        
        file << t.ticketId << "|" << t.showtimeId << "|" << t.title << "|"
             << t.date << "|" << t.time << "|" << t.roomName << "|"
             << t.booked << "|" << t.comboName << "|" << t.price << "|"
             << t.email << "|" << t.fullName << "|"
             << t.bookedDate << "|" << t.bookedTime << "\n";
        file.close();
    }

    bool deleteTicket(const string& ticketId) {
        int index = 0;
        for (const auto& t : tickets) {
            if (t.ticketId == ticketId) {
                tickets.removeAt(index);  // DLL method
                saveToFile();
                return true;
            }
            index++;
        }
        return false;
    }
    
    bool remove(int index) {
        if (index < 0 || index >= tickets.size()) return false;
        tickets.removeAt(index);  // DLL method
        saveToFile();
        return true;
    }
    
    void deleteRecord(int index) { remove(index); }

    long long getTotalRevenue() const {
        long long total = 0;
        for (const auto& t : tickets) {
            total += t.price;
        }
        return total;
    }

    long long getRevenueByDate(const string& date) const {
        long long total = 0;
        for (const auto& t : tickets) {
            if (t.bookedDate == date) {
                total += t.price;
            }
        }
        return total;
    }
};
