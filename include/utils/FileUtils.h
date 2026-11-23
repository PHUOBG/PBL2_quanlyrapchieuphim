#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <array>
#include <filesystem>
#include "utils/StringUtils.h"

using namespace std;
class FileUtils {
public:
    static bool exists(const string& path) {
        return filesystem::exists(path);
    }

    // Resolve a relative data path across common working directories.
    // Example: resolveDataPath("data/movies.txt") will try
    // "data/movies.txt", "../data/movies.txt", "./data/movies.txt", "../../data/movies.txt".
    static string resolveDataPath(const string& relative) {
        namespace fs = std::filesystem;
        const std::array<std::string, 4> prefixes = {"", "../", "./", "../../"};
        for (const auto& prefix : prefixes) {
            fs::path candidate = fs::path(prefix) / relative;
            if (fs::exists(candidate)) {
                return candidate.string();
            }
        }
        // Callers sometimes pass "../data/x.txt" (valid only when cwd is build/). Strip leading
        // "../" / "./" and retry so the same path works from the project root as well.
        {
            string stripped = relative;
            while (stripped.rfind("../", 0) == 0 || stripped.rfind("./", 0) == 0) {
                stripped.erase(0, stripped[1] == '/' ? 2 : 3);
            }
            if (stripped != relative && !stripped.empty()) {
                for (const auto& prefix : prefixes) {
                    fs::path candidate = fs::path(prefix) / stripped;
                    if (fs::exists(candidate)) {
                        return candidate.string();
                    }
                }
            }
        }
        // Fallback: if .txt was requested, try .csv (and vice versa)
        fs::path relPath(relative);
        if (relPath.extension() == ".txt" || relPath.extension() == ".csv") {
            fs::path altRel = relPath;
            altRel.replace_extension(relPath.extension() == ".txt" ? ".csv" : ".txt");
            for (const auto& prefix : prefixes) {
                fs::path candidate = fs::path(prefix) / altRel;
                if (fs::exists(candidate)) {
                    return candidate.string();
                }
            }
        }
        return relative;
    }

    template <typename T>
    static bool safeLoadFromFile(T& target, const string& path) {
        namespace fs = std::filesystem;
        try {
            string resolved = resolveDataPath(path);
            if (!resolved.empty() && fs::exists(resolved)) {
                return target.loadFromFile(resolved);
            }
            if (fs::exists(path)) {
                return target.loadFromFile(path);
            }
        } catch (...) {
            return false;
        }
        return false;
    }

    template <typename T>
    static bool safeLoadFromFile(T& target, const wstring& wpath) {
        namespace fs = std::filesystem;
        try {
            fs::path p(wpath);
            if (fs::exists(p)) {
                return target.loadFromFile(p.string());
            }
            string s(wpath.begin(), wpath.end());
            return safeLoadFromFile(target, s);
        } catch (...) {
            return false;
        }
        return false;
    }

    static bool ensureDirectoryExists(const string& dirPath) {
        if (filesystem::exists(dirPath)) return true;
        return filesystem::create_directories(dirPath);
    }
    
    static string readAll(const string& path) {
        ifstream file(path);
        if (!file.is_open()) return "";
        
        stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

    static vector<string> readLines(const string& path, bool skipHeader = false) {
        vector<string> lines;
        ifstream file(path);
        if (!file.is_open()) return lines;
        
        string line;
        if (skipHeader) getline(file, line);
        
        while (getline(file, line)) {
            if (!line.empty())
                lines.push_back(line);
        }
        
        return lines;
    }

    static vector<vector<string>> readCSV(const string& path, char delimiter = '|', bool skipHeader = true) {
        vector<vector<string>> data;
        ifstream file(path);
        if (!file.is_open()) return data;
        
        string line;
        if (skipHeader) getline(file, line);
        
        while (getline(file, line)) {
            if (line.empty()) continue;
            
            vector<string> row = StringUtils::split(line, delimiter, true);
            if (!row.empty()) {
                data.push_back(row);
            }
        }
        
        return data;
    }

    static bool writeAll(const string& path, const string& content) {
        ofstream file(path);
        if (!file.is_open()) return false;
        
        file << content;
        return true;
    }

    // Crash-safe write: write to "<path>.tmp", flush, then replace the target.
    // Returns false (and leaves the original file untouched) on any failure.
    static bool writeAtomic(const string& path, const string& content) {
        namespace fs = std::filesystem;
        const string tmp = path + ".tmp";
        {
            ofstream out(tmp, ios::binary | ios::trunc);
            if (!out.is_open()) return false;
            out.write(content.data(), static_cast<std::streamsize>(content.size()));
            out.flush();
            if (!out.good()) {
                out.close();
                std::error_code ignore;
                fs::remove(tmp, ignore);
                return false;
            }
        }
        std::error_code ec;
        fs::rename(tmp, path, ec);
        if (ec) {
            // Windows: rename over existing may fail -> fall back to remove + rename
            std::error_code ec2;
            fs::remove(path, ec2);
            ec.clear();
            fs::rename(tmp, path, ec);
            if (ec) {
                std::error_code ignore;
                fs::remove(tmp, ignore);
                return false;
            }
        }
        return true;
    }

    static bool writeLines(const string& path, const vector<string>& lines, const string& header = "") {
        ofstream file(path);
        if (!file.is_open()) return false;
        
        if (!header.empty()) 
            file << header << "\n";
        
        for (const auto& line : lines) 
            file << line << "\n";
        
        return true;
    }

    static bool writeCSV(const string& path, const vector<vector<string>>& data, 
                         char delimiter = '|', const string& header = "") {
        ofstream file(path);
        if (!file.is_open()) return false;
        
        if (!header.empty()) 
            file << header << "\n";
        
        for (const auto& row : data) {
            for (size_t i = 0; i < row.size(); i++) {
                if (i > 0) file << delimiter;
                file << row[i];
            }
            file << "\n";
        }
        
        return true;
    }

    static bool append(const string& path, const string& content) {
        ofstream file(path, ios::app);
        if (!file.is_open()) return false;
        
        file << content;
        return true;
    }

    static bool appendCSVRow(const string& path, const vector<string>& row, char delimiter = '|') {
        ofstream file(path, ios::app);
        if (!file.is_open()) return false;
        
        for (size_t i = 0; i < row.size(); i++) {
            if (i > 0) file << delimiter;
            file << row[i];
        }
        file << "\n";
        
        return true;
    }

    static bool ensureCSVExists(const string& path, const string& header) {
        if (exists(path)) return true;
        
        ofstream file(path);
        if (!file.is_open()) return false;
        
        file << header << "\n";
        return true;
    }

    static bool remove(const string& path) {
        if (!exists(path)) return true;
        return filesystem::remove(path);
    }

    static string getFileName(const string& path) {
        return filesystem::path(path).filename().string();
    }

    static string getExtension(const string& path) {
        return filesystem::path(path).extension().string();
    }
};
