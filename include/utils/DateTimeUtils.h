#pragma once
#include <string>
#include <ctime>
#include <chrono>
#include <sstream>
#include <iomanip>

using namespace std;

/**
 * @brief Utility class cho các thao tác xử lý ngày giờ
 * 
 * Tập trung các hàm helper liên quan đến date/time:
 * - Parse và format ngày giờ
 * - So sánh ngày
 * - Tính toán khoảng cách thời gian
 * - Chuyển đổi giữa các định dạng
 */
class DateTimeUtils {
public:
    // ===== CONSTANTS =====
    static constexpr const char* DATE_FORMAT_DISPLAY = "%d/%m/%Y";      // dd/mm/yyyy
    static constexpr const char* DATE_FORMAT_ISO = "%Y-%m-%d";          // yyyy-mm-dd
    static constexpr const char* DATE_FORMAT_COMPACT = "%Y%m%d";        // yyyymmdd
    static constexpr const char* TIME_FORMAT_DISPLAY = "%H:%M";         // HH:MM
    static constexpr const char* TIME_FORMAT_FULL = "%H:%M:%S";         // HH:MM:SS
    static constexpr const char* DATETIME_FORMAT = "%Y-%m-%d %H:%M:%S"; // Full datetime
    
    // ===== CURRENT TIME =====
    
    /**
     * @brief Lấy thời gian hiện tại
     * @return time_t của hiện tại
     */
    static time_t now() {
        return time(nullptr);
    }
    
    /**
     * @brief Lấy ngày hôm nay dạng chuỗi (YYYY-MM-DD)
     * @return Chuỗi ngày ISO
     */
    static string getTodayISO() {
        return formatDate(now(), DATE_FORMAT_ISO);
    }
    
    /**
     * @brief Lấy ngày hôm nay dạng số (YYYYMMDD)
     * @return Số nguyên biểu diễn ngày
     */
    static int getTodayAsNumber() {
        time_t now_t = now();
        tm* local = localtime(&now_t);
        return (local->tm_year + 1900) * 10000 + (local->tm_mon + 1) * 100 + local->tm_mday;
    }
    
    /**
     * @brief Lấy giờ hiện tại tính bằng phút từ 00:00
     * @return Số phút từ đầu ngày
     */
    static int getCurrentTimeInMinutes() {
        time_t now_t = now();
        tm* local = localtime(&now_t);
        return local->tm_hour * 60 + local->tm_min;
    }
    
    // ===== FORMATTING =====
    
    /**
     * @brief Format time_t thành chuỗi
     * @param timestamp Thời gian cần format
     * @param format Định dạng (strftime format)
     * @return Chuỗi đã format
     */
    static string formatDate(time_t timestamp, const char* format) {
        tm* local = localtime(&timestamp);
        char buffer[32];
        strftime(buffer, sizeof(buffer), format, local);
        return string(buffer);
    }
    
    /**
     * @brief Format tm struct thành chuỗi
     * @param date tm struct
     * @param format Định dạng
     * @return Chuỗi đã format
     */
    static string formatTm(const tm& date, const char* format) {
        char buffer[32];
        strftime(buffer, sizeof(buffer), format, &date);
        return string(buffer);
    }
    
    /**
     * @brief Chuyển phút thành chuỗi HH:MM
     * @param totalMinutes Tổng số phút
     * @return Chuỗi thời gian
     */
    static string minutesToTimeString(int totalMinutes) {
        int hours = totalMinutes / 60;
        int minutes = totalMinutes % 60;
        
        stringstream ss;
        ss << setfill('0') << setw(2) << hours << ":"
           << setfill('0') << setw(2) << minutes;
        return ss.str();
    }
    
    // ===== PARSING =====
    
    /**
     * @brief Parse chuỗi ngày ISO (YYYY-MM-DD) thành tm
     * @param dateStr Chuỗi ngày
     * @return tm struct (giờ = 0)
     */
    static tm parseISODate(const string& dateStr) {
        tm result = {};
        stringstream ss(dateStr);
        string year, month, day;
        
        getline(ss, year, '-');
        getline(ss, month, '-');
        getline(ss, day, '-');
        
        try {
            result.tm_year = stoi(year) - 1900;
            result.tm_mon = stoi(month) - 1;
            result.tm_mday = stoi(day);
        } catch (...) {
            // Return empty tm on error
        }
        
        return result;
    }
    
    /**
     * @brief Parse chuỗi ngày display (DD/MM/YYYY) thành tm
     * @param dateStr Chuỗi ngày
     * @return tm struct (giờ = 0)
     */
    static tm parseDisplayDate(const string& dateStr) {
        tm result = {};
        stringstream ss(dateStr);
        string day, month, year;
        
        getline(ss, day, '/');
        getline(ss, month, '/');
        getline(ss, year, '/');
        
        try {
            result.tm_mday = stoi(day);
            result.tm_mon = stoi(month) - 1;
            result.tm_year = stoi(year) - 1900;
        } catch (...) {
            // Return empty tm on error
        }
        
        return result;
    }
    
    /**
     * @brief Parse chuỗi thời gian (HH:MM) thành phút
     * @param timeStr Chuỗi thời gian
     * @return Tổng số phút từ 00:00
     */
    static int parseTimeToMinutes(const string& timeStr) {
        int hour = 0, minute = 0;
        size_t colonPos = timeStr.find(':');
        if (colonPos != string::npos) {
            try {
                hour = stoi(timeStr.substr(0, colonPos));
                minute = stoi(timeStr.substr(colonPos + 1));
            } catch (...) {}
        }
        return hour * 60 + minute;
    }
    
    /**
     * @brief Chuyển chuỗi ngày compact (YYYYMMDD) thành số
     * @param dateStr Chuỗi ngày
     * @return Số nguyên
     */
    static int dateStringToNumber(const string& dateStr) {
        if (dateStr.length() != 8) return 0;
        try {
            return stoi(dateStr);
        } catch (...) {
            return 0;
        }
    }
    
    // ===== CALCULATIONS =====
    
    /**
     * @brief Tính ngày hết hạn từ số ngày
     * @param daysFromNow Số ngày từ hôm nay
     * @return Chuỗi ngày compact (YYYYMMDD)
     */
    static string buildExpiryDate(int daysFromNow) {
        using namespace chrono;
        auto now_point = system_clock::now();
        auto future = now_point + hours(24 * daysFromNow);
        time_t future_t = system_clock::to_time_t(future);
        
        tm local;
#ifdef _WIN32
        localtime_s(&local, &future_t);
#else
        localtime_r(&future_t, &local);
#endif
        
        char buffer[9];
        strftime(buffer, sizeof(buffer), DATE_FORMAT_COMPACT, &local);
        return string(buffer);
    }
    
    /**
     * @brief Làm tròn phút lên bội số của 10
     * @param minutes Số phút
     * @return Số phút đã làm tròn
     */
    static int roundUpToNext10Minutes(int minutes) {
        int remainder = minutes % 10;
        if (remainder == 0) return minutes;
        return minutes + (10 - remainder);
    }
    
    /**
     * @brief Kiểm tra ngày có phải hôm nay không
     * @param dateStr Chuỗi ngày ISO (YYYY-MM-DD)
     * @return true nếu là hôm nay
     */
    static bool isToday(const string& dateStr) {
        return dateStr == getTodayISO();
    }
    
    /**
     * @brief Kiểm tra ngày có phải quá khứ không
     * @param dateStr Chuỗi ngày ISO (YYYY-MM-DD)
     * @return true nếu là quá khứ
     */
    static bool isPastDate(const string& dateStr) {
        return dateStr < getTodayISO();
    }
    
    /**
     * @brief Kiểm tra ngày có phải tương lai không
     * @param dateStr Chuỗi ngày ISO (YYYY-MM-DD)
     * @return true nếu là tương lai
     */
    static bool isFutureDate(const string& dateStr) {
        return dateStr > getTodayISO();
    }
    
    /**
     * @brief Tính số ngày giữa 2 ngày
     * @param date1 Ngày 1 (ISO format)
     * @param date2 Ngày 2 (ISO format)
     * @return Số ngày chênh lệch (có thể âm)
     */
    static int daysBetween(const string& date1, const string& date2) {
        tm tm1 = parseISODate(date1);
        tm tm2 = parseISODate(date2);
        
        time_t t1 = mktime(&tm1);
        time_t t2 = mktime(&tm2);
        
        double diff = difftime(t2, t1);
        return static_cast<int>(diff / (24 * 60 * 60));
    }
    
    /**
     * @brief Lấy ngày tiếp theo
     * @param dateStr Ngày hiện tại (ISO format)
     * @return Ngày tiếp theo (ISO format)
     */
    static string getNextDay(const string& dateStr) {
        tm date = parseISODate(dateStr);
        date.tm_mday += 1;
        mktime(&date); // Normalize
        return formatTm(date, DATE_FORMAT_ISO);
    }
    
    /**
     * @brief Chuyển đổi từ display format sang ISO format
     * @param displayDate DD/MM/YYYY
     * @return YYYY-MM-DD
     */
    static string displayToISO(const string& displayDate) {
        if (displayDate.length() != 10) return "";
        return displayDate.substr(6, 4) + "-" + displayDate.substr(3, 2) + "-" + displayDate.substr(0, 2);
    }
    
    /**
     * @brief Chuyển đổi từ ISO format sang display format
     * @param isoDate YYYY-MM-DD
     * @return DD/MM/YYYY
     */
    static string isoToDisplay(const string& isoDate) {
        if (isoDate.length() != 10) return "";
        return isoDate.substr(8, 2) + "/" + isoDate.substr(5, 2) + "/" + isoDate.substr(0, 4);
    }
};
