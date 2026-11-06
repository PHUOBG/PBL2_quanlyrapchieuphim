#pragma once

#include <string>
#include <vector>

using namespace std;

class AdminComboRepository {
private:
    string filePath;
    vector<vector<string>> data;

    vector<string> splitString(const string& str, char delimiter);
    string generateNewId() const;

public:
    explicit AdminComboRepository(const string& path);

    void loadFromFile();
    void saveToFile() const;

    void addRecord(const vector<string>& record);
    void updateRecord(int index, const vector<string>& record);
    void deleteRecord(int index);

    vector<vector<string>> getAllData() const;
    vector<string> getRecord(int index) const;
    int getRecordCount() const;
};
