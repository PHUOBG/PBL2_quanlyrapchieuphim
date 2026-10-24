#include "repositories/RoomRepository.h"

#include <fstream>
#include <sstream>

RoomRepository::RoomRepository(const sf::String& path)
    : filePath(path) {
    loadRooms();
}

void RoomRepository::loadRooms() {
    roomNameById.clear();

    std::ifstream file(filePath.toAnsiString());
    if (!file.is_open()) return;

    std::string line;
    std::getline(file, line); // header

    while (std::getline(file, line)) {
        if (line.empty()) continue;

        std::stringstream ss(line);
        std::string id;
        std::string name;

        std::getline(ss, id, '|');
        std::getline(ss, name, '|');

        if (id.empty()) continue;

        roomNameById.emplace(id, sf::String::fromUtf8(name.begin(), name.end()));
    }
}

sf::String RoomRepository::getRoomNameById(const sf::String& roomId) const {
    const std::string key = roomId.toAnsiString();
    const auto it = roomNameById.find(key);
    if (it == roomNameById.end()) {
        return roomId;
    }
    return it->second;
}
