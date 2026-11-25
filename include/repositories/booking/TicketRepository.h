#pragma once

#include <string>
#include <SFML/System/String.hpp>
#include "data-structures/DLL.h"

using namespace std;
using namespace sf;

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
    int ticketSubtotal = 0;
    int comboSubtotal = 0;
    int discountAmount = 0;
    int finalTotal = 0;
    bool hasPriceBreakdown = false;
};

class TicketRepository {
protected:
    string filename;

    string generateTicketId();
    string getCurrentDate();
    string getCurrentTime();

    static string trim(const string& value);
    static bool parseLine(const string& line, Ticket& ticket);
    static DLL<string> split(const string& line, char delimiter);

public:
    explicit TicketRepository(const string& filepath = "../data/tickets.txt");

    // Unified Query & Read
    DLL<Ticket> loadAll() const;

    // Creation & Save
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
        const string& fullName,
        int ticketSubtotal = -1,
        int comboSubtotal = -1,
        int discountAmount = 0,
        int finalTotal = -1
    );

    bool saveTicket(const Ticket& ticket);
    bool writeTickets(const DLL<Ticket>& tickets) const;

    // Deletion & Seat Release
    bool deleteTicket(const string& ticketId);

    // UTF-8 Display Helpers
    static String getTitleUtf8(const Ticket& t);
    static String getRoomUtf8(const Ticket& t);
    static String getFullNameUtf8(const Ticket& t);
    static String getComboUtf8(const Ticket& t);
    static String getDateUtf8(const Ticket& t);
    static String getTimeUtf8(const Ticket& t);
    static String getBookedUtf8(const Ticket& t);
    static String getComboForHistoryUtf8(const Ticket& t);
};
