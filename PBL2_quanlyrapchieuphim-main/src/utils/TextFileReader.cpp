#include "utils/TextFileReader.h"

#include <fstream>
#include <sstream>

bool TextFileReader::loadFile(const String& path, char delimiter) {
    ifstream file(path.toAnsiString(), ios::binary);
    if (!file.is_open()) return false;
    string content((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
    file.close();

    istringstream iss(content);
    string line;
    bool first = true;
    headers.clear();
    rows.clear();

    while (getline(iss, line)) {
        DLL<String> cols;
        stringstream ss(line);
        string token;
        while (getline(ss, token, delimiter))
            cols.push_back(String::fromUtf8(token.begin(), token.end()));
        if (first) {
            headers = cols;
            first = false;
        }
        else rows.push_back(cols);
    }
    return true;
}

int TextFileReader::getRowCount() const { return rows.getSize(); }
int TextFileReader::getColumnCount() const { return headers.getSize(); }
const DLL<String>& TextFileReader::getHeaders() const { return headers; }
const DLL<String>& TextFileReader::getRow(int i) const { return rows[i]; }

String TextFileReader::getValue(int r, int c) const {
    if (r < 0 || r >= rows.getSize()) return L"";
    const DLL<String>& row = rows[r];
    if (c < 0 || c >= row.getSize()) return L"";
    return row[c];
}
