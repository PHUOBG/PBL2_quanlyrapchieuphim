#include "repositories/booking/TicketRepository.h"

#include <chrono>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <sstream>
#include <unordered_map>

TicketRepository::TicketRepository(const string& filepath)
    : filename(filepath) {}

namespace {
static std::unordered_map<std::string, std::string> loadComboIdToName() {
    std::unordered_map<std::string, std::string> out;

    std::ifstream file("../data/combo.txt", std::ios::binary);
    if (!file.is_open()) return out;

    std::string data((std::istreambuf_iterator<char>(file)), {});
    file.close();

    if (data.size() >= 3 &&
        (unsigned char)data[0] == 0xEF &&
        (unsigned char)data[1] == 0xBB &&
        (unsigned char)data[2] == 0xBF) {
        data.erase(0, 3);
    }

    std::stringstream ss(data);
    std::string line;
    std::getline(ss, line); // header

    while (std::getline(ss, line)) {
        if (line.empty()) continue;
        std::stringstream ls(line);
        std::string id, name, price;
        std::getline(ls, id, '|');
        std::getline(ls, name, '|');
        std::getline(ls, price, '|');
        if (!id.empty() && !name.empty()) {
            out[id] = name;
        }
    }

    return out;
}

static std::string trimCopy(const std::string& source) {
    size_t start = source.find_first_not_of(" \t\n\r");
    size_t end = source.find_last_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    return source.substr(start, end - start + 1);
}

static std::string formatComboTokenForDisplay(const std::string& raw,
                                              const std::unordered_map<std::string, std::string>& idToName) {
    std::string token = trimCopy(raw);
    if (token.empty()) return token;
    if (token == "Không có") return token;

    // Direction B: CBxx:xN
    size_t pos = token.find(":x");
    if (pos != std::string::npos && pos > 0) {
        std::string id = token.substr(0, pos);
        std::string qtyStr = token.substr(pos + 2);
        int qty = 0;
        try {
            qty = std::stoi(qtyStr);
        } catch (...) {
            qty = 0;
        }

        auto it = idToName.find(id);
        if (it != idToName.end()) {
            if (qty > 0) {
                return it->second + " x" + std::to_string(qty);
            }
            return it->second;
        }
    }

    // Legacy: "Name xN"
    return token;
}
}

string TicketRepository::generateTicketId() {
    ifstream file(filename);
    int maxId = 0;

    if (file.is_open()) {
        string line;
        getline(file, line);
        while (getline(file, line)) {
            if (line.empty()) continue;

            stringstream ss(line);
            string ticketId;
            getline(ss, ticketId, '|');

            if (ticketId.length() > 1 && ticketId[0] == 'T') {
                try {
                    int id = stoi(ticketId.substr(1));
                    if (id > maxId) maxId = id;
                } catch (...) {
                }
            }
        }
        file.close();
    }

    maxId++;
    char buffer[10];
    snprintf(buffer, sizeof(buffer), "T%04d", maxId);
    return string(buffer);
}

string TicketRepository::getCurrentDate() {
    auto now = chrono::system_clock::now();
    time_t now_c = chrono::system_clock::to_time_t(now);
    tm* localTime = localtime(&now_c);
    char buffer[11];
    strftime(buffer, sizeof(buffer), "%d/%m/%Y", localTime);
    return string(buffer);
}

string TicketRepository::getCurrentTime() {
    auto now = chrono::system_clock::now();
    time_t now_c = chrono::system_clock::to_time_t(now);
    tm* localTime = localtime(&now_c);
    char buffer[9];
    strftime(buffer, sizeof(buffer), "%H:%M:%S", localTime);
    return string(buffer);
}

Ticket TicketRepository::createTicket(
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
    Ticket ticket;
    ticket.ticketId = generateTicketId();
    ticket.showtimeId = showtimeId;
    ticket.title = title;
    ticket.date = date;
    ticket.time = time;
    ticket.roomName = roomName;
    ticket.booked = booked;
    ticket.comboName = comboName;
    ticket.price = price;
    ticket.email = email;
    ticket.fullName = fullName;
    ticket.bookedDate = getCurrentDate();
    ticket.bookedTime = getCurrentTime();
    saveTicket(ticket);
    return ticket;
}

void TicketRepository::saveTicket(const Ticket& ticket) {
    // Mở file với binary mode để đảm bảo UTF-8 được lưu đúng
    ofstream file(filename, ios::app | ios::binary);
    if (!file.is_open()) return;

    // Nếu file trống, ghi header với UTF-8 BOM
    file.seekp(0, ios::end);
    if (file.tellp() == 0) {
        const char bom[] = {(char)0xEF, (char)0xBB, (char)0xBF};
        file.write(bom, 3);
        file << "ticket_id|showtime_id|title|date|time|room_name|booked|combo_ids|price|email|fullName|booked_date|booked_time\n";
    }

    file << ticket.ticketId << "|"
         << ticket.showtimeId << "|"
         << ticket.title << "|"
         << ticket.date << "|"
         << ticket.time << "|"
         << ticket.roomName << "|"
         << ticket.booked << "|"
         << ticket.comboName << "|"
         << ticket.price << "|"
         << ticket.email << "|"
         << ticket.fullName << "|"
         << ticket.bookedDate << "|"
         << ticket.bookedTime << "\n";

    file.close();
}

String TicketRepository::getTitleUtf8(const Ticket& t) { return String::fromUtf8(t.title.begin(), t.title.end()); }
String TicketRepository::getRoomUtf8(const Ticket& t) { return String::fromUtf8(t.roomName.begin(), t.roomName.end()); }
String TicketRepository::getFullNameUtf8(const Ticket& t) { return String::fromUtf8(t.fullName.begin(), t.fullName.end()); }
String TicketRepository::getComboUtf8(const Ticket& t) { return String::fromUtf8(t.comboName.begin(), t.comboName.end()); }
String TicketRepository::getDateUtf8(const Ticket& t) { return String::fromUtf8(t.date.begin(), t.date.end()); }
String TicketRepository::getTimeUtf8(const Ticket& t) { return String::fromUtf8(t.time.begin(), t.time.end()); }
String TicketRepository::getBookedUtf8(const Ticket& t) { return String::fromUtf8(t.booked.begin(), t.booked.end()); }

String TicketRepository::getComboForHistoryUtf8(const Ticket& t) {
    if (t.comboName.empty() || t.comboName == "Không có") {
        return String::fromUtf8(t.comboName.begin(), t.comboName.end());
    }

    static const std::unordered_map<std::string, std::string> comboIdToName = loadComboIdToName();

    string comboStr = t.comboName;
    string result;
    int comboCount = 0;
    size_t pos = 0;

    while (pos < comboStr.length()) {
        size_t commaPos = comboStr.find(',', pos);
        if (commaPos == string::npos) commaPos = comboStr.length();

        string item = comboStr.substr(pos, commaPos - pos);

        size_t start = item.find_first_not_of(" \t");
        size_t end = item.find_last_not_of(" \t");
        if (start != string::npos) {
            item = item.substr(start, end - start + 1);
        }

        if (comboCount < 2) {
            if (comboCount > 0) result += ", ";
            result += formatComboTokenForDisplay(item, comboIdToName);
        }

        comboCount++;
        pos = commaPos + 1;
    }

    if (comboCount > 2) {
        result += "...";
    }

    return String::fromUtf8(result.begin(), result.end());
}
