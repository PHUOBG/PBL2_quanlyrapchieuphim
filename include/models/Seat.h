#pragma once
#include "repositories/booking/ShowtimeSeatRepository.h"

struct Seat {
    std::string seatId;
    std::string row;
    int number = 0;
    bool isVip = false;
    bool isBooked = false;
};

