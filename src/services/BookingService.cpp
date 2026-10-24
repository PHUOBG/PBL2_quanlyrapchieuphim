#include "services/BookingService.h"
#include <fstream>
#include <sstream>
#include <algorithm>

using namespace std;

BookingService::BookingService(const string& usersPath,
                               const string& ticketsPath,
                               const string& seatsPath)
    : userRepository(make_unique<UserRepository>(usersPath)),
      ticketRepository(ticketsPath),
      seatRepository(seatsPath) {
}

void BookingService::getUserInfo(const string& email, string& fullName, string& phone) {
    User* user = userRepository->findByEmail(email);
    
    if (user) {
        fullName = user->getFullName();
        phone = user->getPhone();
    } else {
        fullName = "Khách hàng";
        phone = "";
    }
}

double BookingService::applyVoucher(const string& email, const string& voucherCode,
                                     int subtotal, bool commit) {
    return voucherManager.applyVoucher(email, voucherCode, subtotal, commit);
}

vector<VoucherDisplay> BookingService::getUserVouchers(const string& email) {
    return voucherManager.getVouchersByUser(email);
}

Ticket BookingService::createTicket(const BookingInfo& info) {
    string combosStr = formatCombosForStorage(info.selectedCombos);
    
    return ticketRepository.createTicket(
        info.showtimeId,
        info.movieName,
        info.date,
        info.time,
        info.roomName,
        info.seatsDisplay,
        combosStr,
        info.totalPrice,
        info.customerEmail,
        info.customerName
    );
}

void BookingService::saveBookedSeats(const string& showtimeId, const string& roomId,
                                      const vector<string>& seats) {
    // Convert vector to DLL as required by ShowtimeSeatRepository
    DLL<string> seatsDLL;
    for (const auto& seat : seats) {
        seatsDLL.push_back(seat);
    }
    seatRepository.addBookedSeats(showtimeId, roomId, seatsDLL);
}

DLL<string> BookingService::getBookedSeats(const string& showtimeId, const string& roomId) {
    return seatRepository.getBookedSeats(showtimeId, roomId);
}

int BookingService::calculateTotal(int subtotal, int voucherDiscount) {
    return max(0, subtotal - voucherDiscount);
}

string BookingService::formatCombosForStorage(const vector<BookingInfo::ComboItem>& combos) {
    if (combos.empty()) {
        return "Không có";
    }
    
    string result;
    for (size_t i = 0; i < combos.size(); i++) {
        if (i > 0) result += ", ";
        if (!combos[i].comboId.empty()) {
            result += combos[i].comboId + ":x" + to_string(combos[i].quantity);
        } else {
            // Fallback (legacy) if comboId isn't provided
            result += combos[i].comboName + " x" + to_string(combos[i].quantity);
        }
    }
    return result;
}
