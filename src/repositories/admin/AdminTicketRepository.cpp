#include "repositories/admin/AdminTicketRepository.h"

#include <fstream>
#include <sstream>
#include <cctype>

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
    ofstream file(filePath, std::ios::trunc);
    if (!file.is_open()) {
        return false;
    }
    file << TICKET_HEADER << '\n';
    for (int i = 0; i < tickets.getSize(); ++i) {
        const Ticket& t = tickets[i];
        file << t.ticketId << '|' << t.showtimeId << '|' << t.title << '|' << t.date << '|' << t.time
             << '|' << t.roomName << '|' << t.booked << '|' << t.comboName << '|' << t.price << '|' << t.email
             << '|' << t.fullName << '|' << t.bookedDate << '|' << t.bookedTime << '\n';
    }
    return true;
}

bool AdminTicketRepository::deleteTicket(const string& ticketId) {
    DLL<Ticket> tickets = loadAll();
    bool removed = false;
    for (int i = 0; i < tickets.getSize(); ) {
        if (tickets[i].ticketId == ticketId) {
            tickets.removeAt(i);
            removed = true;
        } else {
            ++i;
        }
    }
    if (!removed) {
        return false;
    }
    return writeTickets(tickets);
}
