#include "repositories/booking/ShowtimeSeatRepository.h"

#include <algorithm>
#include <fstream>
#include <sstream>

ShowtimeSeatStatus::ShowtimeSeatStatus() = default;

ShowtimeSeatStatus::ShowtimeSeatStatus(const string& rId, const string& stId)
    : room_id(rId), showtime_id(stId) {}

ShowtimeSeatRepository::ShowtimeSeatRepository(const string& path)
    : filePath(path) {
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
        if (statusList[i].showtime_id == showtimeId && statusList[i].room_id == roomId) {
            return statusList[i].bookedSeats;
        }
    }

    return DLL<string>();
}

void ShowtimeSeatRepository::addBookedSeats(const string& showtimeId, const string& roomId, const DLL<string>& newSeats) {
    if (newSeats.getSize() == 0) return;

    bool found = false;
    for (int i = 0; i < statusList.getSize(); ++i) {
        if (statusList[i].showtime_id == showtimeId && statusList[i].room_id == roomId) {
            for (int j = 0; j < newSeats.getSize(); ++j) {
                bool isDuplicate = false;
                for (int k = 0; k < statusList[i].bookedSeats.getSize(); ++k) {
                    if (statusList[i].bookedSeats[k] == newSeats[j]) {
                        isDuplicate = true;
                        break;
                    }
                }
                if (!isDuplicate) {
                    statusList[i].bookedSeats.push_back(newSeats[j]);
                }
            }

            found = true;
            break;
        }
    }

    if (!found) {
        ShowtimeSeatStatus newStatus(roomId, showtimeId);
        newStatus.bookedSeats = newSeats;
        statusList.push_back(newStatus);
    }

    saveToFile();
}

void ShowtimeSeatRepository::saveToFile() {
    ofstream file(filePath);
    if (!file.is_open()) return;

    file << "room_id|showtime_id|booked\n";

    for (int i = 0; i < statusList.getSize(); ++i) {
        const ShowtimeSeatStatus& status = statusList[i];
        file << status.room_id << "|" << status.showtime_id << "|" << formatBookedSeats(status.bookedSeats) << "\n";
    }

    file.close();
}
