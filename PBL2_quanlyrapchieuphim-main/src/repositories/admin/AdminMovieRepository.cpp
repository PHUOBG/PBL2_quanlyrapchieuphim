#include "repositories/admin/AdminMovieRepository.h"
#include "utils/FileUtils.h"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <iomanip>
#include <filesystem>

namespace {
string encodeSynopsis(const string& value) {
    string out; out.reserve(value.size());
    for (char c : value) {
        if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else if (c == '|') out += "\\p";
        else if (c != '\r') out.push_back(c);
    }
    return out;
}
string decodeSynopsis(const string& value) {
    string out; out.reserve(value.size());
    for (size_t i = 0; i < value.size(); ++i) {
        if (value[i] == '\\' && i + 1 < value.size()) {
            if (value[i + 1] == 'n') { out.push_back('\n'); ++i; continue; }
            if (value[i + 1] == 'p') { out.push_back('|'); ++i; continue; }
            if (value[i + 1] == '\\') { out.push_back('\\'); ++i; continue; }
        }
        out.push_back(value[i]);
    }
    return out;
}
bool isValidMovieId(const string& id) {
    if (id.size() >= 2 && id[0] == 'F' &&
        all_of(id.begin() + 1, id.end(), [](unsigned char ch){ return std::isdigit(ch); })) return true;
    const string legacyPrefix = "#PBL-";
    return id.rfind(legacyPrefix, 0) == 0 && id.size() > legacyPrefix.size() &&
           all_of(id.begin() + static_cast<ptrdiff_t>(legacyPrefix.size()), id.end(),
                  [](unsigned char ch){ return std::isdigit(ch); });
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
    bool hasLegacyId = false;
    for (const auto& row : data) {
        if (row.empty()) continue;
        const string& candidate = row[0];
        if (!isValidMovieId(candidate)) continue;
        const bool legacy = candidate.rfind("#PBL-", 0) == 0;
        hasLegacyId = hasLegacyId || legacy;
        try {
            int n = stoi(candidate.substr(legacy ? 5 : 1));
            if (n > maxNum) maxNum = n;
        } catch (...) {
            continue;
        }
    }

    int next = maxNum + 1;
    stringstream ss;
    if (hasLegacyId) ss << "#PBL-" << setfill('0') << setw(3) << next;
    else ss << "F" << setfill('0') << setw(4) << next;
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
        // 15 cols: ...|release_date|end_date|...|poster_path|status|screen_format
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

            while (rowData.size() < 15) rowData.push_back("");
            rowData[11] = decodeSynopsis(rowData[11]);

            data.push_back(rowData);
        }
    }

    file.close();
}

bool AdminMovieRepository::saveToFile() {
    const string tempPath = filePath + ".tmp";
    const string backupPath = filePath + ".bak";
    ofstream file(tempPath, ios::trunc);
    if (!file.is_open()) return false;

    file << "movie_id|title|age_rating|country|language|genres|duration_min|release_date|end_date|director|cast|synopsis|poster_path|status|screen_format\n";

    for (const auto& row : data) {
        for (size_t i = 0; i < row.size(); i++) {
            file << (i == 11 ? encodeSynopsis(row[i]) : row[i]);
            if (i < row.size() - 1) file << "|";
        }
        file << "\n";
    }

    file.flush();
    bool written = file.good();
    file.close();
    if (!written) { filesystem::remove(tempPath); return false; }
    error_code ec;
    filesystem::remove(backupPath, ec);
    ec.clear();
    bool hadOriginal = filesystem::exists(filePath, ec) && !ec;
    if (hadOriginal) {
        filesystem::rename(filePath, backupPath, ec);
        if (ec) { filesystem::remove(tempPath); return false; }
    }
    filesystem::rename(tempPath, filePath, ec);
    if (ec) {
        if (hadOriginal) { error_code restoreError; filesystem::rename(backupPath, filePath, restoreError); }
        filesystem::remove(tempPath);
        return false;
    }
    if (hadOriginal) filesystem::remove(backupPath, ec);
    return true;
}

void AdminMovieRepository::addRecord(const vector<string>& record) {
    vector<string> fullRecord = record;
    if (fullRecord.empty()) fullRecord.resize(15);
    if (fullRecord[0].empty() || !isValidMovieId(fullRecord[0])) fullRecord[0] = generateNewId();
    else if (any_of(data.begin(), data.end(), [&](const vector<string>& row) {
        return !row.empty() && row[0] == fullRecord[0];
    })) fullRecord[0] = generateNewId();

    while (fullRecord.size() < 15) {
        fullRecord.push_back("");
    }

    data.push_back(fullRecord);
}

void AdminMovieRepository::updateRecord(int index, const vector<string>& record) {
    if (index < 0 || index >= (int)data.size()) return;

    vector<string> updatedRecord = record;
    if (updatedRecord.size() < 15) {
        updatedRecord.resize(15);
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
