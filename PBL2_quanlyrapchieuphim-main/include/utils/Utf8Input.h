#pragma once

#include <string>

namespace Utf8Input {
inline void eraseLastCodepoint(std::string& value) {
    if (value.empty()) return;
    std::size_t start = value.size() - 1;
    while (start > 0 && (static_cast<unsigned char>(value[start]) & 0xC0) == 0x80) --start;
    value.erase(start);
}

inline void eraseLastCodepoint(std::wstring& value) {
    if (!value.empty()) value.pop_back();
}

inline void appendCodepoint(std::string& value, char32_t codepoint) {
    if (codepoint > 0x10FFFF || (codepoint >= 0xD800 && codepoint <= 0xDFFF)) return;
    if (codepoint <= 0x7F) value.push_back(static_cast<char>(codepoint));
    else if (codepoint <= 0x7FF) {
        value.push_back(static_cast<char>(0xC0 | (codepoint >> 6)));
        value.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    } else if (codepoint <= 0xFFFF) {
        value.push_back(static_cast<char>(0xE0 | (codepoint >> 12)));
        value.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
        value.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    } else {
        value.push_back(static_cast<char>(0xF0 | (codepoint >> 18)));
        value.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F)));
        value.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
        value.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    }
}
}
