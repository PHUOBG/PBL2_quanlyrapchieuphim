#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <ctime>
#include <filesystem>
#include <cstdint>
#include <iomanip>
#include <SFML/System.hpp>
#include "utils/FileUtils.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shellapi.h>
#endif

/**
 * @brief Kết quả trả về sau khi thực hiện xuất file CSV.
 */
struct ExportResult {
    bool success = false;       ///< Ghi file thành công hay không
    bool fileOpened = false;     ///< Mở file bằng ứng dụng mặc định (Excel/WPS...) thành công hay không
    std::string filePath;        ///< Đường dẫn thực tế của file đã xuất
    std::string errorMessage;    ///< Thông báo lỗi nếu thất bại
};

/**
 * @brief Lớp trừu tượng cơ sở áp dụng Template Method Pattern cho các tác vụ xuất file CSV.
 * 
 * Lớp cha CsvExporter chịu trách nhiệm toàn bộ quy trình cố định:
 * 1. Định vị đường dẫn xuất file (có fallback an toàn)
 * 2. Mở file stream nhị phân (binary mode để kiểm soát byte chính xác)
 * 3. Ghi UTF-8 BOM (0xEF, 0xBB, 0xBF) để Excel tự nhận diện encoding tiếng Việt
 * 4. Gọi writeReportHeader() (phương thức thuần ảo - do lớp con định nghĩa)
 * 5. Gọi writeData() (phương thức thuần ảo - do lớp con định nghĩa)
 * 6. Gọi writeSummary() (hook tùy chọn - lớp con có thể override)
 * 7. Kiểm tra tính toàn vẹn của dữ liệu đã ghi (out.good()) và đóng file
 * 8. Mở file bằng ứng dụng mặc định của hệ điều hành và kiểm tra kết quả thực tế
 * 
 * Các lớp con chỉ cần tập trung vào nghiệp vụ định dạng nội dung của từng loại báo cáo.
 */
class CsvExporter {
public:
    virtual ~CsvExporter() = default;

    /**
     * @brief Định vị đường dẫn file xuất trong thư mục exports/ và tự động tạo thư mục nếu cần.
     */
    static std::string resolveExportPath(const std::string& relative) {
        namespace fs = std::filesystem;
        fs::path relPath(relative);
        std::string parentDir = relPath.has_parent_path() ? relPath.parent_path().string() : "exports";
        const std::array<std::string, 4> prefixes = {"", "../", "./", "../../"};

        // 1. Kiểm tra xem thư mục cha (ví dụ "exports/") đã tồn tại ở prefix nào
        for (const auto& prefix : prefixes) {
            fs::path candDir = fs::path(prefix) / parentDir;
            if (fs::exists(candDir) && fs::is_directory(candDir)) {
                return (candDir / relPath.filename()).string();
            }
        }

        // 2. Tìm thư mục gốc dựa theo thư mục data/ (exports/ nằm cạnh data/)
        for (const auto& prefix : prefixes) {
            fs::path dataDir = fs::path(prefix) / "data";
            if (fs::exists(dataDir) && fs::is_directory(dataDir)) {
                fs::path targetDir = fs::path(prefix) / parentDir;
                std::error_code ec;
                fs::create_directories(targetDir, ec);
                return (targetDir / relPath.filename()).string();
            }
        }

        // 3. Fallback dùng FileUtils::resolveDataPath
        return FileUtils::resolveDataPath(relative);
    }

    /**
     * @brief Template Method định nghĩa khung thuật toán xuất báo cáo CSV.
     * @param defaultRelativePath Đường dẫn tương đối mong muốn (ví dụ "exports/doanh_thu_report.csv")
     * @param autoOpenFile Có tự động mở file bằng Excel / ứng dụng mặc định không
     * @return ExportResult Chứa trạng thái thành công, trạng thái mở file và đường dẫn
     */
    ExportResult exportToCsv(const std::string& defaultRelativePath, bool autoOpenFile = true) {
        ExportResult result;
        result.filePath = resolveExportPath(defaultRelativePath);

        // Đảm bảo thư mục cha (ví dụ "exports/") tồn tại
        std::filesystem::path p(result.filePath);
        if (p.has_parent_path()) {
            std::error_code ec;
            std::filesystem::create_directories(p.parent_path(), ec);
        }

        std::ofstream out(result.filePath, std::ios::binary | std::ios::trunc);
        if (!out.is_open()) {
            // Fallback: nếu thư mục đích không tồn tại hoặc bị khóa quyền ghi, thử lưu tại thư mục hiện hành
            result.filePath = p.filename().string();
            out.open(result.filePath, std::ios::binary | std::ios::trunc);
            if (!out.is_open()) {
                result.success = false;
                result.fileOpened = false;
                result.errorMessage = "Không thể mở hoặc tạo file CSV tại: " + result.filePath;
                return result;
            }
        }

        // Bước 1: Ghi UTF-8 BOM
        writeBom(out);

        // Bước 2: Ghi tiêu đề & metadata báo cáo (Abstract primitive method)
        writeReportHeader(out);

        // Bước 3: Ghi dữ liệu chính (Abstract primitive method)
        writeData(out);

        // Bước 4: Ghi tóm tắt/chân trang nếu có (Optional hook)
        writeSummary(out);

        // Bước 5: Kiểm tra lỗi stream trước khi đóng
        out.flush();
        const bool writeOk = out.good();
        out.close();

        if (!writeOk) {
            result.success = false;
            result.fileOpened = false;
            result.errorMessage = "Lỗi trong quá trình ghi dữ liệu ra đĩa!";
            return result;
        }

        result.success = true;

        // Bước 6: Mở file và kiểm tra kết quả thực tế từ HĐH
        if (autoOpenFile) {
            result.fileOpened = openExportedFile(result.filePath);
        }

        return result;
    }

    /**
     * @brief Chuẩn hóa chuỗi theo định dạng CSV Escaping chuẩn RFC 4180.
     * Tự động bao dấu ngoặc kép và nhân đôi dấu ngoặc kép ("") nếu trường chứa dấu phẩy,
     * dấu nháy kép hoặc dấu ngắt dòng.
     */
    static std::string escapeCsv(const std::string& field) {
        bool needsQuotes = false;
        for (char c : field) {
            if (c == ',' || c == '"' || c == '\n' || c == '\r') {
                needsQuotes = true;
                break;
            }
        }

        if (!needsQuotes) {
            return field;
        }

        std::string escaped = "\"";
        for (char c : field) {
            if (c == '"') {
                escaped += "\"\""; // RFC 4180: double-quote escaping
            } else if (c != '\r') {
                escaped += c;
            }
        }
        escaped += "\"";
        return escaped;
    }

    /**
     * @brief Helper cho wstring: chuyển sang UTF-8 rồi escape CSV.
     */
    static std::string escapeCsv(const std::wstring& field) {
        return escapeCsv(wstrToUtf8(field));
    }

    /**
     * @brief Chuyển đổi std::wstring sang std::string UTF-8 bằng SFML String.
     */
    static std::string wstrToUtf8(const std::wstring& ws) {
        if (ws.empty()) return "";
        auto u8 = sf::String(ws).toUtf8();
        return std::string(u8.begin(), u8.end());
    }

    /**
     * @brief Mở file đã xuất bằng ứng dụng mặc định của hệ thống và kiểm tra thành công thực tế.
     * @return true nếu ứng dụng mở thành công, false nếu có lỗi hoặc không tìm thấy ứng dụng liên kết.
     */
    static bool openExportedFile(const std::string& path) {
#ifdef _WIN32
        std::filesystem::path absPath = std::filesystem::absolute(path);
        // ShellExecuteA trả về HINSTANCE > 32 nếu thành công (SE_ERR_* <= 32 khi thất bại)
        HINSTANCE hInst = ShellExecuteA(NULL, "open", absPath.string().c_str(), NULL, NULL, SW_SHOWNORMAL);
        return reinterpret_cast<intptr_t>(hInst) > 32;
#else
        int ret = system(("start \"\" \"" + path + "\"").c_str());
        return ret == 0;
#endif
    }

protected:
    // --- Primitive Operations (Các phương thức thuần ảo bắt buộc lớp con cài đặt) ---
    virtual void writeReportHeader(std::ostream& out) = 0;
    virtual void writeData(std::ostream& out) = 0;

    // --- Hook Operation (Lớp con có thể override nếu cần tóm tắt chân trang) ---
    virtual void writeSummary(std::ostream& /*out*/) {}

    /**
     * @brief Ghi 3 byte UTF-8 BOM (0xEF, 0xBB, 0xBF).
     */
    void writeBom(std::ostream& out) const {
        const unsigned char bom[] = {0xEF, 0xBB, 0xBF};
        out.write(reinterpret_cast<const char*>(bom), sizeof(bom));
    }

    /**
     * @brief Lấy thời gian hiện tại định dạng dd/mm/yyyy hh:mm.
     */
    std::string getCurrentTimestamp() const {
        time_t now = time(nullptr);
        tm ltm{};
#if defined(_WIN32)
        localtime_s(&ltm, &now);
#else
        localtime_r(&now, &ltm);
#endif
        char buf[32];
        strftime(buf, sizeof(buf), "%d/%m/%Y %H:%M", &ltm);
        return std::string(buf);
    }
};

