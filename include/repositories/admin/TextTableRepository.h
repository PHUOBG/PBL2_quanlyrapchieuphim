#pragma once

#include <string>
#include <vector>
using namespace std;

class TextTableRepository {
public:
    vector<string> readAllLines(const string& path) const;
    vector<string> readDataLinesSkipHeader(const string& path) const;
    void writeDataLines(const string& path,
                        const string& headerLine,
                        const vector<string>& dataLines) const;
    void appendDataLines(const string& path,
                         const string& headerLine,
                         const vector<string>& dataLines) const;
};
