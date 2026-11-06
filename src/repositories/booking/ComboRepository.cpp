#include "repositories/booking/ComboRepository.h"

#include <fstream>
#include <sstream>

DLL<Combo> ComboRepository::loadFromFile(const String& path) {
    DLL<Combo> combos;
    ifstream file(path.toAnsiString());
    if (!file.is_open()) return combos;

    string line;
    getline(file, line); // bỏ header

    while (getline(file, line)) {
        if (line.empty()) continue;
        stringstream ss(line);
        string id, name, priceStr;
        Combo c;

        getline(ss, id, '|');
        getline(ss, name, '|');
        getline(ss, priceStr, '|');

        c.id = id;
        c.name = String::fromUtf8(name.begin(), name.end());
        c.price = stoi(priceStr);
        combos.push_back(c);
    }

    file.close();
    return combos;
}
