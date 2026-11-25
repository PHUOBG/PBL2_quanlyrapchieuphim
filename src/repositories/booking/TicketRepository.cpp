#include "repositories/booking/TicketRepository.h"
#include "repositories/booking/ShowtimeSeatRepository.h"
#include "utils/FileUtils.h"

#include <chrono>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <cctype>
#include <vector>
#include <utility>

using std::ifstream;
using std::ofstream;
using std::string;
using std::stringstream;

namespace {
const char* TICKET_HEADER = "ticket_id|showtime_id|title|date|time|room_name|booked|combo_ids|price|email|fullName|booked_date|booked_time|ticket_subtotal|combo_subtotal|discount_amount|final_total";

static std::unordered_map<std::string, std::string> loadComboIdToName() {
    std::unordered_map<std::string, std::string> out;

    std::ifstream file(FileUtils::resolveDataPath("data/combo.txt"), std::ios::binary);
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

TicketRepository::TicketRepository(const string& filepath)
    : filename(filepath) {}

string TicketRepository::trim(const string& value) {
    if (value.empty()) return value;
    size_t start = 0;
    size_t end = value.size();

    if (value.size() >= 3 && static_cast<unsigned char>(value[0]) == 0xEF &&
        static_cast<unsigned char>(value[1]) == 0xBB && static_cast<unsigned char>(value[2]) == 0xBF) {
        start = 3;
    }

    while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start]))) {
        ++start;
    }
    while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1]))) {
        --end;
    }
    return value.substr(start, end - start);
}

DLL<string> TicketRepository::split(const string& line, char delimiter) {
    DLL<string> tokens;
    string token;
    stringstream ss(line);
    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }
    if (!line.empty() && line.back() == delimiter) {
        tokens.push_back(string());
    }
    return tokens;
}

bool TicketRepository::parseLine(const string& rawLine, Ticket& ticket) {
    string line = trim(rawLine);
    if (line.empty()) {
        return false;
    }

    auto columns = split(line, '|');
    if (columns.getSize() < 13) {
        return false;
    }

    try {
        ticket.ticketId = columns[0];
        ticket.showtimeId = columns[1];
        ticket.title = columns[2];
        ticket.date = columns[3];
        ticket.time = columns[4];
        ticket.roomName = columns[5];
        ticket.booked = columns[6];
        ticket.comboName = columns[7];
        ticket.price = std::stoi(columns[8]);
        ticket.email = columns[9];
        ticket.fullName = columns[10];
        ticket.bookedDate = columns[11];
        ticket.bookedTime = columns[12];
        if (columns.getSize() >= 17) {
            ticket.ticketSubtotal = std::stoi(columns[13]);
            ticket.comboSubtotal = std::stoi(columns[14]);
            ticket.discountAmount = std::stoi(columns[15]);
            ticket.finalTotal = std::stoi(columns[16]);
            ticket.hasPriceBreakdown = true;
        } else {
            ticket.ticketSubtotal = ticket.price;
            ticket.comboSubtotal = 0;
            ticket.discountAmount = 0;
            ticket.finalTotal = ticket.price;
            ticket.hasPriceBreakdown = false;
        }
    } catch (...) {
        return false;
    }
    return true;
}

DLL<Ticket> TicketRepository::loadAll() const {
    DLL<Ticket> tickets;
    ifstream file(filename);
    if (!file.is_open()) {
        return tickets;
    }

    string line;
    bool headerSkipped = false;
    while (std::getline(file, line)) {
        string trimmed = trim(line);
        if (trimmed.empty()) continue;

        if (!headerSkipped) {
            headerSkipped = true;
            if (trimmed.rfind("ticket_id|", 0) == 0 || trimmed.rfind("ticketId|", 0) == 0) {
                continue;
            }
        }

        Ticket ticket;
        if (parseLine(trimmed, ticket)) {
            tickets.push_back(ticket);
        }
    }
    return tickets;
}

bool TicketRepository::writeTickets(const DLL<Ticket>& tickets) const {
    string out;
    out += "\xEF\xBB\xBF";
    out += TICKET_HEADER;
    out += '\n';
    for (int i = 0; i < tickets.getSize(); ++i) {
        const Ticket& t = tickets[i];
        int fTot = t.hasPriceBreakdown ? t.finalTotal : t.price;
        int tSub = t.hasPriceBreakdown ? t.ticketSubtotal : t.price;
        int cSub = t.hasPriceBreakdown ? t.comboSubtotal : 0;
        int disc = t.hasPriceBreakdown ? t.discountAmount : 0;
        out += t.ticketId + '|' + t.showtimeId + '|' + t.title + '|' + t.date + '|' + t.time + '|' + t.roomName + '|' +
               t.booked + '|' + t.comboName + '|' + std::to_string(t.price) + '|' + t.email + '|' + t.fullName + '|' +
               t.bookedDate + '|' + t.bookedTime + '|' + std::to_string(tSub) + '|' + std::to_string(cSub) + '|' +
               std::to_string(disc) + '|' + std::to_string(fTot) + '\n';
    }
    return FileUtils::writeAtomic(filename, out);
}

bool TicketRepository::deleteTicket(const string& ticketId) {
    DLL<Ticket> tickets = loadAll();
    bool removed = false;
    std::vector<std::pair<string, string>> releasedSeats;
    for (int i = 0; i < tickets.getSize(); ) {
        if (tickets[i].ticketId == ticketId) {
            releasedSeats.push_back({tickets[i].showtimeId, tickets[i].booked});
            tickets.removeAt(i);
            removed = true;
        } else {
            ++i;
        }
    }
    if (!removed) {
        return false;
    }
    if (!writeTickets(tickets)) {
        return false;
    }
    // Release seats; ticket is already gone so an orphan reservation would be a bug -> report failure.
    ShowtimeSeatRepository seatRepo(FileUtils::resolveDataPath("data/RoomStatusAtShowtime.txt"));
    bool ok = true;
    for (const auto& item : releasedSeats) {
        seatRepo.removeBookedSeatsByShowtime(item.first, item.second);
    }
    seatRepo.loadFromFile();
    for (const auto& item : releasedSeats) {
        DLL<string> seats = seatRepo.getBookedSeats(item.first, "");
        for (int s = 0; s < seats.getSize(); ++s) {
            if (item.second.find(seats[s]) != string::npos) {
                // check exact token match
                std::stringstream ss(item.second); string tok;
                while (std::getline(ss, tok, ',')) {
                    size_t a = tok.find_first_not_of(" \t"), b = tok.find_last_not_of(" \t");
                    if (a != string::npos && tok.substr(a, b - a + 1) == seats[s]) ok = false;
                }
            }
        }
    }
    return ok;
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
    const string& fullName,
    int ticketSubtotal,
    int comboSubtotal,
    int discountAmount,
    int finalTotal
) {
    auto clean = [](string s) {
        for (char& ch : s) if (ch == '|' || ch == '\n' || ch == '\r') ch = ' ';
        return s;
    };
    Ticket ticket;
    ticket.ticketId = generateTicketId();
    ticket.showtimeId = showtimeId;
    ticket.title = clean(title);
    ticket.date = date;
    ticket.time = time;
    ticket.roomName = clean(roomName);
    ticket.booked = clean(booked);
    ticket.comboName = clean(comboName);
    ticket.price = price;
    ticket.email = clean(email);
    ticket.fullName = clean(fullName);
    ticket.bookedDate = getCurrentDate();
    ticket.bookedTime = getCurrentTime();
    ticket.comboSubtotal = (comboSubtotal >= 0) ? comboSubtotal : 0;
    ticket.discountAmount = (discountAmount >= 0) ? discountAmount : 0;
    ticket.finalTotal = (finalTotal >= 0) ? finalTotal : price;
    ticket.ticketSubtotal = (ticketSubtotal >= 0) ? ticketSubtotal : std::max(0, ticket.finalTotal + ticket.discountAmount - ticket.comboSubtotal);
    ticket.hasPriceBreakdown = true;
    if (!saveTicket(ticket)) return Ticket{};
    return ticket;
}

bool TicketRepository::saveTicket(const Ticket& ticket) {
    // Mở file với binary mode để đảm bảo UTF-8 được lưu đúng
    ofstream file(filename, ios::app | ios::binary);
    if (!file.is_open()) return false;

    // Nếu file trống, ghi header với UTF-8 BOM
    file.seekp(0, ios::end);
    if (file.tellp() == 0) {
        const char bom[] = {(char)0xEF, (char)0xBB, (char)0xBF};
        file.write(bom, 3);
        file << TICKET_HEADER << "\n";
    }

    int fTot = ticket.hasPriceBreakdown ? ticket.finalTotal : ticket.price;
    int tSub = ticket.hasPriceBreakdown ? ticket.ticketSubtotal : ticket.price;
    int cSub = ticket.hasPriceBreakdown ? ticket.comboSubtotal : 0;
    int disc = ticket.hasPriceBreakdown ? ticket.discountAmount : 0;

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
         << ticket.bookedTime << "|"
         << tSub << "|"
         << cSub << "|"
         << disc << "|"
         << fTot << "\n";

    file.flush();
    const bool ok = file.good();
    file.close();
    return ok;
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
