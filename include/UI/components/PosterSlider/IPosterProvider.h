#pragma once
#include "data-structures/DLL.h"
#include <SFML/System/String.hpp>
using namespace sf;

class IPosterProvider {
public:
    virtual DLL<String> getPosterPaths() = 0;
    virtual ~IPosterProvider() = default;
};
