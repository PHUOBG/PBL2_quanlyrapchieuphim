#pragma once
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <cctype>

using namespace std;

/**
 * @brief Utility class cho các thao tác xử lý chuỗi
 * 
 * Tập trung các hàm helper phổ biến để tránh duplicate code:
 * - trim: Xóa khoảng trắng đầu/cuối
 * - split: Tách chuỗi theo delimiter
 * - toLower/toUpper: Chuyển đổi case
 * - normalize: Chuẩn hóa chuỗi cho so sánh
 */
class StringUtils {
public:
    /**
     * @brief Xóa khoảng trắng đầu và cuối chuỗi
     * @param str Chuỗi cần trim
     * @return Chuỗi đã trim
     */
    static string trim(const string& str) {
        const char* whitespace = " \t\r\n";
        size_t start = str.find_first_not_of(whitespace);
        if (start == string::npos) return "";
        size_t end = str.find_last_not_of(whitespace);
        return str.substr(start, end - start + 1);
    }
    
    /**
     * @brief Tách chuỗi thành vector theo delimiter
     * @param str Chuỗi cần tách
     * @param delimiter Ký tự phân cách
     * @param trimResult Có trim kết quả không (default: true)
     * @return Vector các chuỗi con
     */
    static vector<string> split(const string& str, char delimiter, bool trimResult = true) {
        vector<string> tokens;
        string token;
        stringstream ss(str);
        
        while (getline(ss, token, delimiter)) {
            if (trimResult) {
                token = trim(token);
            }
            tokens.push_back(token);
        }
        
        return tokens;
    }
    
    /**
     * @brief Chuyển chuỗi thành chữ thường
     * @param str Chuỗi gốc
     * @return Chuỗi chữ thường
     */
    static string toLower(const string& str) {
        string result = str;
        transform(result.begin(), result.end(), result.begin(), 
                  [](unsigned char c) { return static_cast<char>(tolower(c)); });
        return result;
    }
    
    /**
     * @brief Chuyển chuỗi thành chữ hoa
     * @param str Chuỗi gốc
     * @return Chuỗi chữ hoa
     */
    static string toUpper(const string& str) {
        string result = str;
        transform(result.begin(), result.end(), result.begin(),
                  [](unsigned char c) { return static_cast<char>(toupper(c)); });
        return result;
    }
    
    /**
     * @brief Chuẩn hóa chuỗi (lowercase + trim) cho so sánh
     * @param str Chuỗi gốc
     * @return Chuỗi đã chuẩn hóa
     */
    static string normalize(const string& str) {
        return toLower(trim(str));
    }
    
    /**
     * @brief Kiểm tra chuỗi có rỗng hoặc chỉ chứa khoảng trắng
     * @param str Chuỗi cần kiểm tra
     * @return true nếu rỗng hoặc chỉ chứa khoảng trắng
     */
    static bool isBlank(const string& str) {
        return trim(str).empty();
    }
    
    /**
     * @brief Join vector thành chuỗi với delimiter
     * @param parts Các phần cần nối
     * @param delimiter Ký tự phân cách
     * @return Chuỗi đã nối
     */
    static string join(const vector<string>& parts, const string& delimiter) {
        if (parts.empty()) return "";
        
        string result = parts[0];
        for (size_t i = 1; i < parts.size(); i++) {
            result += delimiter + parts[i];
        }
        return result;
    }
    
    /**
     * @brief Kiểm tra chuỗi có bắt đầu bằng prefix không
     * @param str Chuỗi cần kiểm tra
     * @param prefix Prefix cần tìm
     * @return true nếu bắt đầu bằng prefix
     */
    static bool startsWith(const string& str, const string& prefix) {
        if (prefix.length() > str.length()) return false;
        return str.compare(0, prefix.length(), prefix) == 0;
    }
    
    /**
     * @brief Kiểm tra chuỗi có kết thúc bằng suffix không
     * @param str Chuỗi cần kiểm tra
     * @param suffix Suffix cần tìm
     * @return true nếu kết thúc bằng suffix
     */
    static bool endsWith(const string& str, const string& suffix) {
        if (suffix.length() > str.length()) return false;
        return str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0;
    }
    
    /**
     * @brief Thay thế tất cả occurrences của một chuỗi
     * @param str Chuỗi gốc
     * @param from Chuỗi cần thay
     * @param to Chuỗi thay thế
     * @return Chuỗi đã thay thế
     */
    static string replaceAll(const string& str, const string& from, const string& to) {
        if (from.empty()) return str;
        
        string result = str;
        size_t pos = 0;
        while ((pos = result.find(from, pos)) != string::npos) {
            result.replace(pos, from.length(), to);
            pos += to.length();
        }
        return result;
    }
    
    /**
     * @brief Kiểm tra có phải header row trong CSV không
     * @param columns Các cột đã parse
     * @param expectedFirstCell Giá trị expected của cell đầu tiên
     * @return true nếu là header row
     */
    static bool isHeaderRow(const vector<string>& columns, const string& expectedFirstCell) {
        if (columns.empty()) return false;
        return normalize(columns[0]) == normalize(expectedFirstCell);
    }
};
