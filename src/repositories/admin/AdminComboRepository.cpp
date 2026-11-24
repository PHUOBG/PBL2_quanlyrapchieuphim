#include "repositories/admin/AdminComboRepository.h"
#include "utils/FileUtils.h"

#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cctype>

namespace {
bool isValidComboId(const string& id) {
    if (id.size() < 3) return false;
    if (id[0] != 'C' || id[1] != 'B') return false;
    return all_of(id.begin() + 2, id.end(), [](unsigned char ch){ return std::isdigit(ch); });
}
}

AdminComboRepository::AdminComboRepository(const string& path) : filePath(path) {
    loadFromFile();
}

vector<vector<string>> AdminComboRepository::getAllData() const {
    return data;
}

int AdminComboRepository::getRecordCount() const {
    return static_cast<int>(data.size());
}

vector<string> AdminComboRepository::splitString(const string& str, char delimiter) {
    vector<string> tokens;
    stringstream ss(str);
    string token;

    while (getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }

    return tokens;
}

string AdminComboRepository::generateNewId() const {
    int maxNum = 0;
    for (const auto& row : data) {
        if (row.empty()) continue;
        const string& candidate = row[0];
        if (!isValidComboId(candidate)) continue;
        try {
            int num = stoi(candidate.substr(2));
            maxNum = max(maxNum, num);
        } catch (...) {
            continue;
        }
    }

    stringstream ss;
    ss << "CB" << setfill('0') << setw(2) << (maxNum + 1);
    return ss.str();
}

void AdminComboRepository::loadFromFile() {
    data.clear();

    ifstream file(filePath);
    if (!file.is_open()) {
        return;
    }

    string line;
    getline(file, line);

    while (getline(file, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;
        if (line.size() >= 3 &&
            static_cast<unsigned char>(line[0]) == 0xEF &&
            static_cast<unsigned char>(line[1]) == 0xBB &&
            static_cast<unsigned char>(line[2]) == 0xBF) {
            line = line.substr(3);
        }

        vector<string> tokens = splitString(line, '|');
        if (tokens.size() >= 3) {
            data.push_back(tokens);
        }
    }

    file.close();
}

bool AdminComboRepository::saveToFile() const {
    string out = "combo_id|combo_name|price\n";
    for (const auto& row : data) {
        for (size_t i = 0; i < row.size(); ++i) {
            string cell = row[i];
            for (char& ch : cell) {
                if (ch == '|' || ch == '\n' || ch == '\r') ch = ' ';
            }
            out += cell;
            if (i < row.size() - 1) out += "|";
        }
        out += "\n";
    }
    return FileUtils::writeAtomic(filePath, out);
}

void AdminComboRepository::addRecord(const vector<string>& record) {
    vector<string> newRecord = record;
    if (newRecord.empty()) {
        newRecord.push_back(generateNewId());
    } else if (newRecord[0].empty() || !isValidComboId(newRecord[0])) {
        if (newRecord[0].empty()) {
            newRecord[0] = generateNewId();
        } else {
            newRecord.insert(newRecord.begin(), generateNewId());
        }
    } else {
        for (const auto& existing : data) {
            if (!existing.empty() && existing[0] == newRecord[0]) {
                newRecord[0] = generateNewId();
                break;
            }
        }
    }

    while (newRecord.size() < 3) {
        newRecord.push_back("");
    }
    if (newRecord.size() > 3) {
        newRecord.resize(3);
    }

    data.push_back(newRecord);
}

void AdminComboRepository::updateRecord(int index, const vector<string>& record) {
    if (index < 0 || index >= (int)data.size()) return;

    vector<string> updated = record;
    while (updated.size() < 3) {
        updated.push_back("");
    }
    if (updated.size() > 3) {
        updated.resize(3);
    }

    if (!data[index].empty()) {
        updated[0] = data[index][0];
    }

    data[index] = updated;
}

void AdminComboRepository::deleteRecord(int index) {
    if (index < 0 || index >= (int)data.size()) return;
    data.erase(data.begin() + index);
}

vector<string> AdminComboRepository::getRecord(int index) const {
    if (index < 0 || index >= (int)data.size()) return {};
    return data[index];
}

DLL<Combo> AdminComboRepository::getCombos() const {
    DLL<Combo> list;
    for (const auto& row : data) {
        if (row.size() < 3) continue;
        Combo c;
        c.id = row[0];
        c.name = sf::String::fromUtf8(row[1].begin(), row[1].end());
        try {
            c.price = std::stoi(row[2]);
        } catch (...) {
            c.price = 0;
        }
        list.push_back(c);
    }
    return list;
}

