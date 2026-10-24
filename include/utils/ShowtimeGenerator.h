#pragma once
#include <string>
#include <vector>
#include <ctime>
#include <deque>

using namespace std;

// Thông tin phim cho scheduling
struct MovieInfo {
    string id;
    string title;
    int duration;  // phút
};

// Suất chiếu được tạo
struct GeneratedShowtime {
    string showtimeId;
    string movieId;
    string roomId;
    string date;     // YYYY-MM-DD
    string time;     // HH:MM
    int price;
};

class ShowtimeGenerator {
public:
    static void generateShowtimesFile(const string& outputPath, int numDays = 30);
    
private:
    // ===== CONSTANTS =====
    static const int START_HOUR = 9;      // 9:00 AM
    static const int END_HOUR = 24;       // 24:00 (midnight)
    static const int BUFFER_MINUTES = 15; // Thời gian dọn dẹp giữa các suất
    static const int ROOM_OFFSET = 20;    // Mỗi phòng bắt đầu lệch 20 phút
    
    // ===== DATA LOADING =====
    static vector<MovieInfo> loadMovies(const string& moviesPath);
    static vector<string> loadRooms(const string& roomsPath);
    
    // ===== SCHEDULING ALGORITHM =====
    // Round-Robin queue để phân bổ phim đều
    static deque<MovieInfo> movieQueue;
    static vector<GeneratedShowtime> generatedShowtimes;
    
    // Chọn phim tiếp theo, tránh conflict (cùng phim chiếu ở 2 phòng cùng lúc)
    static MovieInfo selectNextMovie(int startMinute, int endMinute, const string& roomId);
    
    // Kiểm tra xem phim có đang chiếu ở phòng khác cùng thời điểm không
    static bool hasConflict(const string& movieId, int startMinute, int endMinute, 
                            const string& excludeRoom, const string& date);
    
    // ===== PRICING =====
    // Giá vé theo khung giờ
    static int getPriceByTime(int hour);
    
    // ===== UTILITIES =====
    static string formatDate(tm* date);
    static string generateShowtimeId(const string& date, int counter);
    static int timeToMinutes(const string& timeStr);
    static string minutesToTime(int minutes);
};
