#pragma once

#include <string>
#include <vector>
#include <memory>
#include "repositories/booking/TicketRepository.h"
#include "repositories/booking/ShowtimeSeatRepository.h"
#include "services/VoucherManager.h"
#include "repositories/UserRepository.h"

struct BookingInfo {
    // Customer info
    string customerEmail;
    string customerName;
    string customerPhone;
    
    // Showtime info
    string showtimeId;
    string movieId;
    string movieName;
    string roomId;
    string roomName;
    string date;
    string time;
    int ticketPrice = 0;
    
    // Seats
    vector<string> selectedSeats;
    string seatsDisplay;
    int totalSeats = 0;
    
    // Combos
    struct ComboItem {
        string comboId;
        string comboName;
        int price = 0;
        int quantity = 0;
    };
    vector<ComboItem> selectedCombos;
    int comboTotalPrice = 0;
    
    // Voucher & Total
    string voucherCode;
    int voucherDiscount = 0;
    int totalPrice = 0;
};

class BookingService {
private:
    unique_ptr<UserRepository> userRepository;
    VoucherManager voucherManager;
    TicketRepository ticketRepository;
    ShowtimeSeatRepository seatRepository;

public:
    BookingService(const string& usersPath = "../data/users.txt",
                   const string& ticketsPath = "../data/tickets.txt",
                   const string& seatsPath = "../data/RoomStatusAtShowtime.txt");
    
    void getUserInfo(const string& email, string& fullName, string& phone);
    double applyVoucher(const string& email, const string& voucherCode, 
                        int subtotal, bool commit = false);
    vector<VoucherDisplay> getUserVouchers(const string& email);
    Ticket createTicket(const BookingInfo& info);
    void saveBookedSeats(const string& showtimeId, const string& roomId,
                         const vector<string>& seats);
    DLL<string> getBookedSeats(const string& showtimeId, const string& roomId);
    int calculateTotal(int subtotal, int voucherDiscount);
    static string formatCombosForStorage(const vector<BookingInfo::ComboItem>& combos);
};
