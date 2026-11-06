#pragma once
#include <string>
#include <vector>
#include <deque>
#include <ctime>
#include <unordered_set>

using namespace std;

struct ShowtimeData {
    string showtime_id;
    string movie_id;
    string room_id;
    string date;      // Format: YYYY-MM-DD
    string time;      // Format: HH:MM
    int price;
};

// Thông tin phim cho scheduling
struct MovieData {
    string id;
    string title;
    int duration;  // phút
    string release_date; // start_date (dd/mm/yyyy)
    string end_date;     // dd/mm/yyyy
    string status;
};

class ShowtimeCleanupService {
public:
    static void maintainShowtimes(const string& showtimesPath, int daysToGenerate = 7);
    static void removeExpiredShowtimes(const string& showtimesPath);
    static void addNewShowtimes(const string& showtimesPath, int daysToGenerate);
    static void forceRegenerate(const string& showtimesPath, int daysToGenerate = 30);
    
private:
    // ===== CONSTANTS for Round-Robin algorithm =====
    static const int START_HOUR = 9;      // 9:00 AM
    static const int END_HOUR = 24;       // 24:00 (midnight)
    static const int BUFFER_MINUTES = 15; // Thời gian dọn dẹp giữa các suất
    static const int ROOM_OFFSET = 20;    // Mỗi phòng bắt đầu lệch 20 phút
    
    // ===== DATA LOADING =====
    static vector<MovieData> loadMovies(const string& moviesPath);
    static vector<string> loadRooms(const string& roomsPath);

    // Remove movies that are past end_date (today > end_date). Keeps IDs stable; does not touch showtimes.
    static void purgeExpiredMoviesFromFile(const string& moviesPath);
    
    // Load all showtimes from file
    static vector<ShowtimeData> loadShowtimes(const string& filepath);
    
    // Save showtimes back to file
    static void saveShowtimes(const string& filepath, const vector<ShowtimeData>& showtimes);
    
    // ===== ROUND-ROBIN SCHEDULING =====
    static deque<MovieData> movieQueue;
    static vector<ShowtimeData> tempShowtimes; // For conflict checking
    
    // Chọn phim tiếp theo, tránh conflict
    static MovieData selectNextMovie(int startMinute, const string& roomId, const string& date);
    
    // Kiểm tra conflict
    static bool hasConflict(const string& movieId, int startMinute, int endMinute, 
                            const string& excludeRoom, const string& date);
    
    // Giá vé theo khung giờ
    static int getPriceByTime(int hour);
    
    // ===== UTILITIES =====
    static bool isExpired(const ShowtimeData& showtime);
    static tm parseDate(const string& dateStr);
    static void addTime(tm& datetime, const string& timeStr);
    static string getLatestDate(const vector<ShowtimeData>& showtimes);
    static string formatDate(const tm* date);
    static int getNextCounter(const vector<ShowtimeData>& showtimes, const string& date);
    static int timeToMinutes(const string& timeStr);
    static string minutesToTime(int minutes);
    
    // Làm tròn phút lên bội số của 10 (09:03 -> 09:10, 09:51 -> 10:00)
    static int roundUpToNext10(int minutes);
    
    // Lấy giờ phút hiện tại tính bằng phút từ 00:00
    static int getCurrentTimeInMinutes();

    // ===== SAFETY / STATUS HELPERS =====
    static bool isReleaseOnOrBeforeShowDate(const string& releaseDateDdMmYyyy, const string& showDateYyyyMmDd);
    static int statusPriority(const string& status);
    static unordered_set<string> loadLockedShowtimeIdsFromTickets(const string& ticketsPath, const string& todayStr);
};
