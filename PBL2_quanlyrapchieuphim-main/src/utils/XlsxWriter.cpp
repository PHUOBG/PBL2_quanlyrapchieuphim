#include "utils/XlsxWriter.h"

#include <cstdint>
#include <cstdlib>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace {
struct ZipEntry { std::string name; std::string data; uint32_t crc = 0; uint32_t offset = 0; };

void put16(std::ostream& out, uint16_t value) { out.put(static_cast<char>(value & 0xff)); out.put(static_cast<char>((value >> 8) & 0xff)); }
void put32(std::ostream& out, uint32_t value) { put16(out, static_cast<uint16_t>(value)); put16(out, static_cast<uint16_t>(value >> 16)); }

uint32_t crc32(const std::string& data) {
    uint32_t crc = 0xffffffffu;
    for (unsigned char byte : data) {
        crc ^= byte;
        for (int bit = 0; bit < 8; ++bit) crc = (crc >> 1) ^ (0xedb88320u & (0u - (crc & 1u)));
    }
    return ~crc;
}

std::string xmlEscape(const std::string& value) {
    std::string out;
    for (char c : value) {
        switch (c) {
            case '&': out += "&amp;"; break;
            case '<': out += "&lt;"; break;
            case '>': out += "&gt;"; break;
            case '"': out += "&quot;"; break;
            case '\'': out += "&apos;"; break;
            default: out += c;
        }
    }
    return out;
}

std::string columnName(size_t index) {
    std::string result;
    do { result.insert(result.begin(), static_cast<char>('A' + (index % 26))); index = index / 26; if (index) --index; } while (index);
    return result;
}

std::string makeSheet(const std::vector<std::vector<std::string>>& rows) {
    std::ostringstream xml;
    xml << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        << "<worksheet xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\"><sheetData>";
    for (size_t r = 0; r < rows.size(); ++r) {
        xml << "<row r=\"" << (r + 1) << "\">";
        for (size_t c = 0; c < rows[r].size(); ++c) {
            const std::string ref = columnName(c) + std::to_string(r + 1);
            const std::string& value = rows[r][c];
            char* end = nullptr;
            std::strtod(value.c_str(), &end);
            const bool numeric = !value.empty() && end == value.c_str() + value.size() && std::isfinite(std::strtod(value.c_str(), nullptr));
            if (numeric) xml << "<c r=\"" << ref << "\"><v>" << value << "</v></c>";
            else xml << "<c r=\"" << ref << "\" t=\"inlineStr\"><is><t xml:space=\"preserve\">" << xmlEscape(value) << "</t></is></c>";
        }
        xml << "</row>";
    }
    xml << "</sheetData></worksheet>";
    return xml.str();
}
}

bool XlsxWriter::write(const std::string& path, const std::vector<std::vector<std::string>>& rows) {
    namespace fs = std::filesystem;
    const fs::path target(path);
    std::error_code ec;
    if (!target.parent_path().empty()) fs::create_directories(target.parent_path(), ec);
    if (ec) return false;

    std::vector<ZipEntry> entries = {
        {"[Content_Types].xml", "<?xml version=\"1.0\" encoding=\"UTF-8\"?><Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\"><Default Extension=\"rels\" ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/><Default Extension=\"xml\" ContentType=\"application/xml\"/><Override PartName=\"/xl/workbook.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.sheet.main+xml\"/><Override PartName=\"/xl/worksheets/sheet1.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.worksheet+xml\"/></Types>"},
        {"_rels/.rels", "<?xml version=\"1.0\" encoding=\"UTF-8\"?><Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\"><Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument\" Target=\"xl/workbook.xml\"/></Relationships>"},
        {"xl/workbook.xml", "<?xml version=\"1.0\" encoding=\"UTF-8\"?><workbook xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\" xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\"><sheets><sheet name=\"Doanh thu\" sheetId=\"1\" r:id=\"rId1\"/></sheets></workbook>"},
        {"xl/_rels/workbook.xml.rels", "<?xml version=\"1.0\" encoding=\"UTF-8\"?><Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\"><Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/worksheet\" Target=\"worksheets/sheet1.xml\"/></Relationships>"},
        {"xl/worksheets/sheet1.xml", makeSheet(rows)}
    };

    for (auto& entry : entries) entry.crc = crc32(entry.data);
    const fs::path tempPath = target.string() + ".tmp";
    std::ofstream out(tempPath, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) return false;
    for (auto& entry : entries) {
        entry.offset = static_cast<uint32_t>(out.tellp());
        put32(out, 0x04034b50); put16(out, 20); put16(out, 0); put16(out, 0); put16(out, 0); put16(out, 0);
        put32(out, entry.crc); put32(out, static_cast<uint32_t>(entry.data.size())); put32(out, static_cast<uint32_t>(entry.data.size()));
        put16(out, static_cast<uint16_t>(entry.name.size())); put16(out, 0);
        out.write(entry.name.data(), static_cast<std::streamsize>(entry.name.size()));
        out.write(entry.data.data(), static_cast<std::streamsize>(entry.data.size()));
    }
    const uint32_t directoryOffset = static_cast<uint32_t>(out.tellp());
    for (const auto& entry : entries) {
        put32(out, 0x02014b50); put16(out, 20); put16(out, 20); put16(out, 0); put16(out, 0); put16(out, 0); put16(out, 0);
        put32(out, entry.crc); put32(out, static_cast<uint32_t>(entry.data.size())); put32(out, static_cast<uint32_t>(entry.data.size()));
        put16(out, static_cast<uint16_t>(entry.name.size())); put16(out, 0); put16(out, 0); put16(out, 0); put16(out, 0); put32(out, 0); put32(out, entry.offset);
        out.write(entry.name.data(), static_cast<std::streamsize>(entry.name.size()));
    }
    const uint32_t directorySize = static_cast<uint32_t>(out.tellp()) - directoryOffset;
    put32(out, 0x06054b50); put16(out, 0); put16(out, 0); put16(out, static_cast<uint16_t>(entries.size()));
    put16(out, static_cast<uint16_t>(entries.size())); put32(out, directorySize); put32(out, directoryOffset); put16(out, 0);
    out.flush(); const bool success = out.good(); out.close();
    if (!success) { fs::remove(tempPath, ec); return false; }
    const fs::path backupPath = target.string() + ".bak";
    fs::remove(backupPath, ec); ec.clear();
    const bool hadOriginal = fs::exists(target, ec) && !ec;
    if (hadOriginal) {
        fs::rename(target, backupPath, ec);
        if (ec) { fs::remove(tempPath); return false; }
    }
    fs::rename(tempPath, target, ec);
    if (ec) {
        if (hadOriginal) { std::error_code restore; fs::rename(backupPath, target, restore); }
        fs::remove(tempPath);
        return false;
    }
    if (hadOriginal) fs::remove(backupPath, ec);
    return true;
}
