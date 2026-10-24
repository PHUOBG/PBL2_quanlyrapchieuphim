#include "repositories/admin/AdminMovieRepository.h"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <iomanip>

namespace {
bool isValidMovieId(const string& id) {
    return id.size() >= 2 && id[0] == 'F' &&
           all_of(id.begin() + 1, id.end(), [](unsigned char ch){ return std::isdigit(ch); });
}
}

AdminMovieRepository::AdminMovieRepository(const string& path) : filePath(path) {
    loadFromFile();
}

vector<vector<string>> AdminMovieRepository::getAllData() const {
    return data;
}

int AdminMovieRepository::getRecordCount() const {
    return static_cast<int>(data.size());
}

vector<string> AdminMovieRepository::splitString(const string& str, char delimiter) {
    vector<string> tokens;
    stringstream ss(str);
    string token;

    while (getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }

    return tokens;
}

string AdminMovieRepository::generateNewId() {
    int maxNum = 0;
    for (const auto& row : data) {
        if (row.empty()) continue;
        const string& candidate = row[0];
        if (!isValidMovieId(candidate)) continue;
        try {
            int n = stoi(candidate.substr(1));
            if (n > maxNum) maxNum = n;
        } catch (...) {
            continue;
        }
    }

    int next = maxNum + 1;
    stringstream ss;
    ss << "F" << setfill('0') << setw(4) << next;
    return ss.str();
}

void AdminMovieRepository::loadFromFile() {
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
            (unsigned char)line[0] == 0xEF &&
            (unsigned char)line[1] == 0xBB &&
            (unsigned char)line[2] == 0xBF) {
            line = line.substr(3);
        }

        vector<string> rowData = splitString(line, '|');
        // Accept legacy:
        // 12 cols: ...|poster_path
        // 13 cols: ...|poster_path|status
        // New:
        // 14 cols: ...|release_date|end_date|...|poster_path|status
        if (rowData.size() >= 12) {
            if (rowData.size() == 12) {
                // Insert end_date at index 8, and append status
                rowData.insert(rowData.begin() + 8, "");
                rowData.push_back("");
            } else if (rowData.size() == 13) {
                // Insert end_date at index 8
                string status = rowData.back();
                rowData.pop_back();
                rowData.insert(rowData.begin() + 8, "");
                rowData.push_back(status);
            }

            while (rowData.size() < 14) rowData.push_back("");
            if (rowData.size() > 14) rowData.resize(14);

            data.push_back(rowData);
        }
    }

    file.close();
}

void AdminMovieRepository::saveToFile() {
    ofstream file(filePath);
    if (!file.is_open()) {
        return;
    }

    file << "movie_id|title|age_rating|country|language|genres|duration_min|release_date|end_date|director|cast|synopsis|poster_path|status\n";

    for (const auto& row : data) {
        for (size_t i = 0; i < row.size(); i++) {
            file << row[i];
            if (i < row.size() - 1) file << "|";
        }
        file << "\n";
    }

    file.close();
}

void AdminMovieRepository::addRecord(const vector<string>& record) {
    vector<string> fullRecord = record;

    if (fullRecord.empty()) {
        fullRecord.push_back(generateNewId());
    } else if (fullRecord[0].empty()) {
        fullRecord[0] = generateNewId();
    } else if (!isValidMovieId(fullRecord[0])) {
        fullRecord.insert(fullRecord.begin(), generateNewId());
    }

    while (fullRecord.size() < 14) {
        fullRecord.push_back("");
    }
    if (fullRecord.size() > 14) {
        fullRecord.resize(14);
    }

    data.push_back(fullRecord);
}

void AdminMovieRepository::updateRecord(int index, const vector<string>& record) {
    if (index < 0 || index >= (int)data.size()) return;

    vector<string> updatedRecord = record;
    if (updatedRecord.size() < 14) {
        updatedRecord.resize(14);
        updatedRecord[13] = "";
    }
    if (!data[index].empty()) {
        updatedRecord[0] = data[index][0];
    }

    data[index] = updatedRecord;
}

void AdminMovieRepository::deleteRecord(int index) {
    if (index < 0 || index >= (int)data.size()) return;
    data.erase(data.begin() + index);
}

vector<string> AdminMovieRepository::getRecord(int index) const {
    if (index < 0 || index >= (int)data.size()) return {};
    return data[index];
}
