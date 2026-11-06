#pragma once

#include <SFML/Graphics.hpp>
#include "data-structures/DLL.h"
using namespace sf;

struct Showtime {
    String showtime_id;
    String movie_id;
    String room_id;
    String date;
    String time;
    int price;
};

class ShowtimeRepository {
public:
    DLL<Showtime> loadFromFile(const String& path);
};
