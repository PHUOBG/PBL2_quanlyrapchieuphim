#pragma once

#include <SFML/System/String.hpp>
#include <unordered_map>
using namespace sf;
using namespace std;

class RoomRepository {
private:
    String filePath;
    unordered_map<string, String> roomNameById;
    void loadRooms();

public:
    explicit RoomRepository(const String& path = "../data/rooms.txt");
    String getRoomNameById(const String& roomId) const;
};
