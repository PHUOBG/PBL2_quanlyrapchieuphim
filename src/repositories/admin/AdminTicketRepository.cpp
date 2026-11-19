#include "repositories/admin/AdminTicketRepository.h"
#include "repositories/booking/ShowtimeSeatRepository.h"
#include "utils/FileUtils.h"

#include <fstream>
#include <sstream>
#include <cctype>
#include <vector>
#include <utility>

using std::ifstream;
using std::ofstream;
using std::string;
using std::stringstream;

namespace {
const char* TICKET_HEADER = "ticket_id|showtime_id|title|date|time|room_name|booked|combo_ids|price|email|fullName|booked_date|booked_time";
}

AdminTicketRepository::AdminTicketRepository(const string& path)
    : filePath(path) {}

std::string AdminTicketRepository::trim(const string& value) {
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

DLL<string> AdminTicketRepository::split(const string& line, char delimiter) {
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

bool AdminTicketRepository::parseLine(const string& rawLine, Ticket& ticket) {
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
    } catch (...) {
        return false;
    }
    return true;
}

DLL<Ticket> AdminTicketRepository::loadAll() const {
    DLL<Ticket> tickets;
    ifstream file(filePath);
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

bool AdminTicketRepository::writeTickets(const DLL<Ticket>& tickets) const {
    string out;
    out += "\xEF\xBB\xBF";
    out += TICKET_HEADER;
    out += '\n';
    for (int i = 0; i < tickets.getSize(); ++i) {
        const Ticket& t = tickets[i];
        out += t.ticketId + '|' + t.showtimeId + '|' + t.title + '|' + t.date + '|' + t.time + '|' + t.roomName + '|' +
               t.booked + '|' + t.comboName + '|' + std::to_string(t.price) + '|' + t.email + '|' + t.fullName + '|' +
               t.bookedDate + '|' + t.bookedTime + '\n';
    }
    return FileUtils::writeAtomic(filePath, out);
}

bool AdminTicketRepository::deleteTicket(const string& ticketId) {
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
        DLL<string> probe;
        // removeBookedSeatsByShowtime returns false when nothing to remove (already free) - that's fine,
        // but a save failure must not be hidden: re-check state afterwards.
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