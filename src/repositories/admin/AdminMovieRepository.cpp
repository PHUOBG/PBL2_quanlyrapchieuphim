#include "repositories/admin/AdminMovieRepository.h"
#include "utils/FileUtils.h"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <iomanip>

namespace {
bool isValidMovieId(const string& id) {
    if (id.size() > 5 && id.rfind("#PBL-", 0) == 0 &&
        all_of(id.begin() + 5, id.end(), [](unsigned char ch){ return std::isdigit(ch); })) {
        return true;
    }
    return id.size() >= 2 && id[0] == 'F' &&
           all_of(id.begin() + 1, id.end(), [](unsigned char ch){ return std::isdigit(ch); });
}
}

AdminMovieRepository::AdminMovieRepository(const string& path) {
    filePath = FileUtils::resolveDataPath(path);
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
            int n = (candidate.rfind("#PBL-", 0) == 0)
                ? stoi(candidate.substr(5))
                : stoi(candidate.substr(1));
            if (n > maxNum) maxNum = n;
        } catch (...) {
            continue;
        }
    }

    int next = maxNum + 1;
    stringstream ss;
    ss << "#PBL-" << setfill('0') << setw(3) << next;
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
        if (!line.empty() && line.back() == '\r') line.pop_back();
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

bool AdminMovieRepository::saveToFile() {
    string out = "movie_id|title|age_rating|country|language|genres|duration_min|release_date|end_date|director|cast|synopsis|poster_path|status\n";
    for (const auto& row : data) {
        for (size_t i = 0; i < row.size(); i++) {
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

void AdminMovieRepository::addRecord(const vector<string>& record) {
    vector<string> fullRecord = record;

    if (fullRecord.empty()) {
        fullRecord.push_back(generateNewId());
    } else if (fullRecord[0].empty()) {
        fullRecord[0] = generateNewId();
    } else if (!isValidMovieId(fullRecord[0])) {
        fullRecord.insert(fullRecord.begin(), generateNewId());
    } else {
        // Ensure uniqueness if caller passed an existing movie ID
        for (const auto& existing : data) {
            if (!existing.empty() && existing[0] == fullRecord[0]) {
                fullRecord[0] = generateNewId();
                break;
            }
        }
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
