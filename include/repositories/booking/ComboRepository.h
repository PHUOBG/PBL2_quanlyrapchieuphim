#pragma once
#include <SFML/System/String.hpp>
#include "data-structures/DLL.h"
using namespace sf;
using namespace std;

struct Combo {
    String id;
    String name;
    int price;
};

class ComboRepository {
public:
    DLL<Combo> loadFromFile(const String& path);
};
