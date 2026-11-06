#pragma once

#include <string>
#include "data-structures/DLL.h"
#include "repositories/booking/TicketRepository.h"

class AdminTicketRepository {
private:
    std::string filePath;

    static std::string trim(const std::string& value);
    static bool parseLine(const std::string& line, Ticket& ticket);
    static DLL<std::string> split(const std::string& line, char delimiter);
    bool writeTickets(const DLL<Ticket>& tickets) const;

public:
    explicit AdminTicketRepository(const std::string& path);

    DLL<Ticket> loadAll() const;
    bool deleteTicket(const std::string& ticketId);
};
