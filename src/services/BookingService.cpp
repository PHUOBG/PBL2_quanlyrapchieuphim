#include "services/BookingService.h"
#include "utils/FileUtils.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <unordered_set>

using namespace std;

BookingService::BookingService(const string& usersPath,
                               const string& ticketsPath,
                               const string& seatsPath)
    : userRepository(make_unique<UserRepository>(FileUtils::resolveDataPath(usersPath))),
      ticketRepository(FileUtils::resolveDataPath(ticketsPath)),
      seatRepository(FileUtils::resolveDataPath(seatsPath)) {
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
    DLL<string> seatsDLL;
    unordered_set<string> uniqueCheck;
    for (const auto& s : info.selectedSeats) {
        if (s.empty()) continue;
        if (!uniqueCheck.insert(s).second) {
            // Duplicate seat within the same transaction -> reject
            return Ticket{};
        }
        seatsDLL.push_back(s);
    }
    if (seatsDLL.getSize() == 0 && !info.seatsDisplay.empty()) {
        stringstream ss(info.seatsDisplay);
        string tok;
        while (getline(ss, tok, ',')) {
            size_t a = tok.find_first_not_of(" \t\r\n");
            size_t b = tok.find_last_not_of(" \t\r\n");
            if (a == string::npos) continue;
            string clean = tok.substr(a, b - a + 1);
            if (!uniqueCheck.insert(clean).second) {
                return Ticket{};
            }
            seatsDLL.push_back(clean);
        }
    }

    if (info.showtimeId.empty() || seatsDLL.getSize() == 0 || info.totalPrice < 0) {
        return Ticket{};
    }

    // Step 1: atomically check + reserve seats (reloads from disk, persists).
    const string targetRoom = info.roomId.empty() ? info.roomName : info.roomId;
    if (!seatRepository.tryReserveSeats(info.showtimeId, targetRoom, seatsDLL)) {
        return Ticket{};
    }

    // Step 2: persist ticket. If that fails, release the seats (rollback).
    string combosStr = formatCombosForStorage(info.selectedCombos);
    string canonicalSeats;  // always derived from the reserved seats so deletion releases exactly them
    for (int i = 0; i < seatsDLL.getSize(); ++i) canonicalSeats += (i ? ", " : "") + seatsDLL[i];
    Ticket created = ticketRepository.createTicket(
        info.showtimeId, info.movieName, info.date, info.time, info.roomName,
        canonicalSeats, combosStr, info.totalPrice, info.customerEmail, info.customerName);

    if (created.ticketId.empty()) {
        seatRepository.removeBookedSeats(info.showtimeId, targetRoom, seatsDLL);
        return Ticket{};
    }
    return created;
}

bool BookingService::saveBookedSeats(const string& showtimeId, const string& roomId,
                                      const vector<string>& seats) {
    // Convert vector to DLL as required by ShowtimeSeatRepository
    DLL<string> seatsDLL;
    for (const auto& seat : seats) {
        seatsDLL.push_back(seat);
    }
    return seatRepository.addBookedSeats(showtimeId, roomId, seatsDLL);
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
