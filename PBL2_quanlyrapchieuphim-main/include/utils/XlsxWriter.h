#pragma once

#include <string>
#include <vector>

class XlsxWriter {
public:
    static bool write(const std::string& path, const std::vector<std::vector<std::string>>& rows);
};
