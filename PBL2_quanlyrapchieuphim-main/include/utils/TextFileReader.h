#pragma once
#include <SFML/System/String.hpp>
#include <fstream>
#include <sstream>
#include "data-structures/DLL.h"
using namespace std;
using namespace sf;

class TextFileReader {
private:
    DLL<String> headers;
    DLL<DLL<String>> rows;
public:
    bool loadFile(const String& path, char delimiter = '|');
    int getRowCount() const;
    int getColumnCount() const;
    const DLL<String>& getHeaders() const;
    const DLL<String>& getRow(int i) const;
    String getValue(int r, int c) const;
};
