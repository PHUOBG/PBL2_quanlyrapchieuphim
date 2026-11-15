#pragma once

#include <string>
#include <SFML/System/String.hpp>
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
    int price;
    string email;
    string fullName;
    string bookedDate;
    string bookedTime;
};

class TicketRepository {
private:
    string filename;

    string generateTicketId();
    string getCurrentDate();
    string getCurrentTime();

public:
    TicketRepository(const string& filepath = "../data/tickets.txt");

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
    );

    void saveTicket(const Ticket& ticket);

    static String getTitleUtf8(const Ticket& t);
    static String getRoomUtf8(const Ticket& t);
    static String getFullNameUtf8(const Ticket& t);
    static String getComboUtf8(const Ticket& t);
    static String getDateUtf8(const Ticket& t);
    static String getTimeUtf8(const Ticket& t);
    static String getBookedUtf8(const Ticket& t);
    static String getComboForHistoryUtf8(const Ticket& t);
};
