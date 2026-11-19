#pragma once

#include <SFML/Graphics.hpp>

#include <string>

#include "data-structures/DLL.h"

using namespace sf;
using namespace std;

// Cấu trúc lưu trạng thái ghế của 1 suất chiếu
struct ShowtimeSeatStatus {
    string room_id;          // PHONG_001, PHONG_002, ...
    string showtime_id;      // SUATCHIEU_20251106_0001, ...
    DLL<string> bookedSeats; // A1, B2, C3, ...

    ShowtimeSeatStatus();
    ShowtimeSeatStatus(const string& rId, const string& stId);
};

class ShowtimeSeatRepository {
private:
    string filePath;
    DLL<ShowtimeSeatStatus> statusList;
    DLL<string> parseBookedSeats(const string& bookedStr);
    string formatBookedSeats(const DLL<string>& seats);

public:
    ShowtimeSeatRepository(const string& path = "../data/RoomStatusAtShowtime.txt");
    void loadFromFile();
    DLL<string> getBookedSeats(const string& showtimeId, const string& roomId);
    bool hasAnySeatBooked(const string& showtimeId, const string& roomId, const DLL<string>& checkSeats);
    bool addBookedSeats(const string& showtimeId, const string& roomId, const DLL<string>& newSeats);
    // Atomic check-and-reserve: reloads from disk, fails if any seat is taken/duplicated or save fails.
    bool tryReserveSeats(const string& showtimeId, const string& roomId, const DLL<string>& seats);
    bool removeBookedSeats(const string& showtimeId, const string& roomId, const DLL<string>& seatsToRemove);
    bool removeBookedSeatsByShowtime(const string& showtimeId, const string& seatsCsv);
    bool saveToFile();
};
