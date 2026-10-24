#include "repositories/admin/TextTableRepository.h"

#include <fstream>

std::vector<std::string> TextTableRepository::readAllLines(const std::string& path) const {
    std::vector<std::string> lines;

    std::ifstream file(path);
    if (!file.is_open()) return lines;

    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty()) {
            lines.push_back(line);
        }
    }

    return lines;
}

std::vector<std::string> TextTableRepository::readDataLinesSkipHeader(const std::string& path) const {
    std::vector<std::string> lines;

    std::ifstream file(path);
    if (!file.is_open()) return lines;

    std::string line;
    if (!std::getline(file, line)) {
        return lines;
    }

    while (std::getline(file, line)) {
        if (!line.empty()) {
            lines.push_back(line);
        }
    }

    return lines;
}

void TextTableRepository::writeDataLines(const std::string& path,
                                        const std::string& headerLine,
                                        const std::vector<std::string>& dataLines) const {
    std::ofstream file(path, std::ios::trunc);
    if (!file.is_open()) return;

    if (!headerLine.empty()) {
        file << headerLine;
        if (headerLine.back() != '\n') file << "\n";
    }

    for (const auto& line : dataLines) {
        file << line;
        if (!line.empty() && line.back() != '\n') file << "\n";
    }
}

void TextTableRepository::appendDataLines(const std::string& path,
                                         const std::string& headerLine,
                                         const std::vector<std::string>& dataLines) const {
    if (dataLines.empty()) return;

    bool needsHeader = false;
    {
        std::ifstream check(path, std::ios::binary);
        if (!check.is_open()) {
            needsHeader = !headerLine.empty();
        } else {
            check.seekg(0, std::ios::end);
            needsHeader = (check.tellg() == 0) && !headerLine.empty();
        }
    }

    std::ofstream file(path, std::ios::app | std::ios::binary);
    if (!file.is_open()) return;

    if (needsHeader) {
        file << headerLine;
        if (headerLine.back() != '\n') file << "\n";
    }

    for (const auto& line : dataLines) {
        file << line;
        if (!line.empty() && line.back() != '\n') file << "\n";
    }
}
