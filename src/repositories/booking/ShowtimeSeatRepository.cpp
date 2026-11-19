#include "repositories/booking/ShowtimeSeatRepository.h"
#include "utils/FileUtils.h"

#include <algorithm>
#include <fstream>
#include <sstream>

ShowtimeSeatStatus::ShowtimeSeatStatus() = default;

ShowtimeSeatStatus::ShowtimeSeatStatus(const string& rId, const string& stId)
    : room_id(rId), showtime_id(stId) {}

ShowtimeSeatRepository::ShowtimeSeatRepository(const string& path)
    : filePath(FileUtils::resolveDataPath(path)) {
    loadFromFile();
}

DLL<string> ShowtimeSeatRepository::parseBookedSeats(const string& bookedStr) {
    DLL<string> seats;
    if (bookedStr.empty()) return seats;

    stringstream ss(bookedStr);
    string seat;

    while (getline(ss, seat, ',')) {
        seat.erase(0, seat.find_first_not_of(" \t\r\n"));
        seat.erase(seat.find_last_not_of(" \t\r\n") + 1);

        if (!seat.empty()) {
            seats.push_back(seat);
        }
    }

    return seats;
}

string ShowtimeSeatRepository::formatBookedSeats(const DLL<string>& seats) {
    if (seats.getSize() == 0) return "";

    stringstream ss;
    for (int i = 0; i < seats.getSize(); ++i) {
        ss << seats[i];
        if (i < seats.getSize() - 1) {
            ss << ", ";
        }
    }
    return ss.str();
}

void ShowtimeSeatRepository::loadFromFile() {
    statusList.clear();

    ifstream file(filePath);
    if (!file.is_open()) {
        ofstream newFile(filePath);
        if (newFile.is_open()) {
            newFile << "room_id|showtime_id|booked\n";
            newFile.close();
        }
        return;
    }

    string line;
    getline(file, line);

    while (getline(file, line)) {
        if (line.empty()) continue;

        stringstream ss(line);
        string roomId, showtimeId, bookedStr;

        getline(ss, roomId, '|');
        getline(ss, showtimeId, '|');
        getline(ss, bookedStr, '|');

        ShowtimeSeatStatus status(roomId, showtimeId);
        status.bookedSeats = parseBookedSeats(bookedStr);
        statusList.push_back(status);
    }

    file.close();
}

DLL<string> ShowtimeSeatRepository::getBookedSeats(const string& showtimeId, const string& roomId) {
    for (int i = 0; i < statusList.getSize(); ++i) {
        if (statusList[i].showtime_id == showtimeId &&
            (roomId.empty() || statusList[i].room_id == roomId)) {
            return statusList[i].bookedSeats;
        }
    }

    return DLL<string>();
}

bool ShowtimeSeatRepository::hasAnySeatBooked(const string& showtimeId, const string& roomId, const DLL<string>& checkSeats) {
    if (checkSeats.getSize() == 0) return false;
    for (int i = 0; i < statusList.getSize(); ++i) {
        if (statusList[i].showtime_id == showtimeId &&
            (roomId.empty() || statusList[i].room_id == roomId)) {
            for (int j = 0; j < checkSeats.getSize(); ++j) {
                for (int k = 0; k < statusList[i].bookedSeats.getSize(); ++k) {
                    if (statusList[i].bookedSeats[k] == checkSeats[j]) {
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

bool ShowtimeSeatRepository::addBookedSeats(const string& showtimeId, const string& roomId, const DLL<string>& newSeats) {
    if (newSeats.getSize() == 0) return true;
    if (showtimeId.empty()) return false;

    // Work on a snapshot so a failed save leaves memory == disk.
    DLL<ShowtimeSeatStatus> backup = statusList;

    int target = -1;
    for (int i = 0; i < statusList.getSize(); ++i) {
        if (statusList[i].showtime_id == showtimeId && statusList[i].room_id == roomId) {
            target = i;
            break;
        }
    }
    if (target < 0) {
        statusList.push_back(ShowtimeSeatStatus(roomId, showtimeId));
        target = statusList.getSize() - 1;
    }
    for (int j = 0; j < newSeats.getSize(); ++j) {
        bool dup = false;
        for (int k = 0; k < statusList[target].bookedSeats.getSize(); ++k) {
            if (statusList[target].bookedSeats[k] == newSeats[j]) { dup = true; break; }
        }
        if (!dup) statusList[target].bookedSeats.push_back(newSeats[j]);
    }

    if (!saveToFile()) {
        statusList = backup;
        return false;
    }
    return true;
}

bool ShowtimeSeatRepository::tryReserveSeats(const string& showtimeId, const string& roomId, const DLL<string>& seats) {
    if (showtimeId.empty() || seats.getSize() == 0) return false;
    // Reject duplicates inside one request
    for (int a = 0; a < seats.getSize(); ++a)
        for (int b = a + 1; b < seats.getSize(); ++b)
            if (seats[a] == seats[b]) return false;
    // Re-read the persisted state so we never decide on stale data
    loadFromFile();
    if (hasAnySeatBooked(showtimeId, roomId, seats)) return false;
    return addBookedSeats(showtimeId, roomId, seats);
}
bool ShowtimeSeatRepository::removeBookedSeats(const string& showtimeId, const string& roomId, const DLL<string>& seatsToRemove) {
    if (seatsToRemove.getSize() == 0) return false;
    bool changed = false;
    for (int i = 0; i < statusList.getSize(); ++i) {
        if (statusList[i].showtime_id == showtimeId &&
            (roomId.empty() || statusList[i].room_id == roomId)) {
            for (int r = 0; r < seatsToRemove.getSize(); ++r) {
                for (int k = 0; k < statusList[i].bookedSeats.getSize(); ) {
                    if (statusList[i].bookedSeats[k] == seatsToRemove[r]) {
                        statusList[i].bookedSeats.removeAt(k);
                        changed = true;
                    } else {
                        ++k;
                    }
                }
            }
        }
    }
    if (changed && !saveToFile()) {
        loadFromFile();
        return false;
    }
    return changed;
}

bool ShowtimeSeatRepository::removeBookedSeatsByShowtime(const string& showtimeId, const string& seatsCsv) {
    DLL<string> parsed = parseBookedSeats(seatsCsv);
    return removeBookedSeats(showtimeId, "", parsed);
}

bool ShowtimeSeatRepository::saveToFile() {
    string out = "room_id|showtime_id|booked\n";
    for (int i = 0; i < statusList.getSize(); ++i) {
        const ShowtimeSeatStatus& status = statusList[i];
        out += status.room_id + "|" + status.showtime_id + "|" + formatBookedSeats(status.bookedSeats) + "\n";
    }
    return FileUtils::writeAtomic(filePath, out);
}