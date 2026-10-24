#include "repositories/admin/AdminComboRepository.h"

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

void AdminComboRepository::saveToFile() const {
    ofstream file(filePath);
    if (!file.is_open()) {
        return;
    }

    file << "combo_id|combo_name|price\n";
    for (const auto& row : data) {
        for (size_t i = 0; i < row.size(); ++i) {
            file << row[i];
            if (i < row.size() - 1) file << "|";
        }
        file << "\n";
    }

    file.close();
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
