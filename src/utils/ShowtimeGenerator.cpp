#include "utils/ShowtimeGenerator.h"
#include "utils/FileUtils.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>

// Static members initialization
deque<MovieInfo> ShowtimeGenerator::movieQueue;
vector<GeneratedShowtime> ShowtimeGenerator::generatedShowtimes;

// ==================== DATA LOADING ====================

vector<MovieInfo> ShowtimeGenerator::loadMovies(const string& moviesPath) {
    vector<MovieInfo> movies;
    ifstream file(moviesPath);
    if (!file.is_open()) {
        return movies;
    }
    
    string line;
    getline(file, line); // Skip header
    
    while (getline(file, line)) {
        stringstream ss(line);
        string token;
        MovieInfo movie;

        getline(ss, movie.id, '|');        // 0: movie_id
        getline(ss, movie.title, '|');     // 1: title
        getline(ss, token, '|');           // 2: age_rating (skip)
        getline(ss, token, '|');           // 3: country (skip)
        getline(ss, token, '|');           // 4: language (skip)
        getline(ss, token, '|');           // 5: genres (skip)
        getline(ss, token, '|');           // 6: duration_min
        try {
            movie.duration = stoi(token);
        } catch (...) {
            movie.duration = 120; // Default 2 hours
        }
        getline(ss, token, '|');           // 7: release_date (skip)
        getline(ss, token, '|');           // 8: (end_date OR director) (skip)
        getline(ss, token, '|');           // 9: (director OR cast) (skip)
        getline(ss, token, '|');           // 10: (cast OR synopsis) (skip)
        getline(ss, token, '|');           // 11: (synopsis OR poster_path) (skip)
        getline(ss, token, '|');           // 12: (poster_path OR status) (skip)

        // Status removed: every movie in movies.txt is eligible for showtime distribution.
        if (!movie.id.empty()) {
            movies.push_back(movie);
        }
    }
    
    file.close();
    return movies;
}

vector<string> ShowtimeGenerator::loadRooms(const string& roomsPath) {
    vector<string> rooms;
    ifstream file(roomsPath);
    if (!file.is_open()) {
        return rooms;
    }
    
    string line;
    getline(file, line); // Skip header
    
    while (getline(file, line)) {
        stringstream ss(line);
        string roomId;
        getline(ss, roomId, '|');
        if (!roomId.empty()) {
            rooms.push_back(roomId);
        }
    }
    
    file.close();
    return rooms;
}

// ==================== SCHEDULING ALGORITHM ====================

bool ShowtimeGenerator::hasConflict(const string& movieId, int startMinute, int endMinute,
                                     const string& excludeRoom, const string& date) {
    for (const auto& st : generatedShowtimes) {
        // Chỉ check cùng ngày và cùng phim
        if (st.date == date && st.movieId == movieId && st.roomId != excludeRoom) {
            int stStart = timeToMinutes(st.time);
            int stEnd = stStart + 120; // Assume ~2h for simplicity, actual duration varies
            
            // Check overlap: NOT (endA <= startB OR startA >= endB)
            if (!(endMinute <= stStart || startMinute >= stEnd)) {
                return true; // Có conflict
            }
        }
    }
    return false;
}

MovieInfo ShowtimeGenerator::selectNextMovie(int startMinute, int endMinute, const string& roomId) {
    (void)endMinute;
    if (movieQueue.empty()) {
        return {"", "", 120};
    }
    
    size_t queueSize = movieQueue.size();
    string currentDate = generatedShowtimes.empty() ? "" : generatedShowtimes.back().date;
    
    // Try each movie in queue to find one without conflict
    for (size_t attempt = 0; attempt < queueSize; attempt++) {
        MovieInfo movie = movieQueue.front();
        movieQueue.pop_front();
        
        int movieEndMinute = startMinute + movie.duration;
        
        if (!hasConflict(movie.id, startMinute, movieEndMinute, roomId, currentDate)) {
            movieQueue.push_back(movie); // Đưa về cuối queue (Round-Robin)
            return movie;
        }
        
        // Có conflict, đưa về cuối và thử phim khác
        movieQueue.push_back(movie);
    }
    
    // Fallback: trả về phim đầu tiên nếu tất cả đều conflict
    MovieInfo movie = movieQueue.front();
    movieQueue.pop_front();
    movieQueue.push_back(movie);
    return movie;
}

int ShowtimeGenerator::getPriceByTime(int hour) {
    // Giá vé theo khung giờ:
    // - Sáng (9-12h): 65,000
    // - Trưa/Chiều (12-17h): 70,000  
    // - Tối sớm (17-20h): 75,000
    // - Tối muộn (20-24h): 80,000
    if (hour < 12) return 65000;
    if (hour < 17) return 70000;
    if (hour < 20) return 75000;
    return 80000;
}

// ==================== MAIN GENERATION ====================

void ShowtimeGenerator::generateShowtimesFile(const string& outputPath, int numDays) {
    // Load data
    vector<MovieInfo> movies = loadMovies(FileUtils::resolveDataPath("data/movies.txt"));
    vector<string> rooms = loadRooms("../data/rooms.txt");
    
    if (movies.empty()) {
        return;
    }
    if (rooms.empty()) {
        return;
    }
    
    // Initialize Round-Robin queue
    movieQueue.clear();
    for (const auto& movie : movies) {
        movieQueue.push_back(movie);
    }
    
    // Clear previous results
    generatedShowtimes.clear();
    
    ofstream file(outputPath);
    if (!file.is_open()) {
        return;
    }
    
    // Write header
    file << "showtime_id|movie_id|room_id|date|time|price\n";
    
    time_t now = time(nullptr);
    int globalCounter = 0;
    
    // Generate for each day
    for (int day = 0; day < numDays; day++) {
        time_t futureTime = now + (day * 24 * 60 * 60);
        tm* futureDate = localtime(&futureTime);
        string dateStr = formatDate(futureDate);
        
        int dailyCounter = 0;
        
        // Reset daily showtime list for conflict checking
        vector<GeneratedShowtime> dailyShowtimes;
        
        // Generate showtimes for each room
        for (size_t r = 0; r < rooms.size(); r++) {
            string roomId = rooms[r];
            
            // Offset mỗi phòng để phim khởi chiếu lệch nhau
            // Room 1: 9:00, Room 2: 9:20, Room 3: 9:40, etc.
            int currentMinute = START_HOUR * 60 + (static_cast<int>(r) * ROOM_OFFSET);
            int endDayMinute = END_HOUR * 60;
            
            // Fill room with showtimes until end of day
            while (currentMinute < endDayMinute) {
                // Temporarily set current date for conflict checking
                generatedShowtimes = dailyShowtimes;
                
                // Select next movie using Round-Robin
                MovieInfo movie = selectNextMovie(currentMinute, currentMinute + 180, roomId);
                if (movie.id.empty()) break;
                
                int showtimeEndMinute = currentMinute + movie.duration;
                
                // Nếu vượt quá 24:00, dừng
                if (showtimeEndMinute > endDayMinute) break;
                
                // Create showtime
                globalCounter++;
                dailyCounter++;
                
                GeneratedShowtime st;
                st.showtimeId = generateShowtimeId(dateStr, dailyCounter);
                st.movieId = movie.id;
                st.roomId = roomId;
                st.date = dateStr;
                st.time = minutesToTime(currentMinute);
                st.price = getPriceByTime(currentMinute / 60);
                
                dailyShowtimes.push_back(st);
                
                // Write to file
                file << st.showtimeId << "|"
                     << st.movieId << "|"
                     << st.roomId << "|"
                     << st.date << "|"
                     << st.time << "|"
                     << st.price << "\n";
                
                // Move to next slot (movie duration + buffer)
                currentMinute = showtimeEndMinute + BUFFER_MINUTES;
            }
        }
        
        // Save daily showtimes for reference
        generatedShowtimes = dailyShowtimes;
    }
    
    file.close();
}

// ==================== UTILITIES ====================

string ShowtimeGenerator::formatDate(tm* date) {
    stringstream ss;
    ss << (date->tm_year + 1900) << "-"
       << setfill('0') << setw(2) << (date->tm_mon + 1) << "-"
       << setfill('0') << setw(2) << date->tm_mday;
    return ss.str();
}

string ShowtimeGenerator::generateShowtimeId(const string& date, int counter) {
    // Remove dashes from date: 2025-11-26 -> 20251126
    string dateCompact = date;
    dateCompact.erase(remove(dateCompact.begin(), dateCompact.end(), '-'), dateCompact.end());
    
    stringstream ss;
    ss << "SUATCHIEU_" << dateCompact << "_" 
       << setfill('0') << setw(4) << counter;
    return ss.str();
}

int ShowtimeGenerator::timeToMinutes(const string& timeStr) {
    // Parse "HH:MM" to minutes since midnight
    int hour = 0, minute = 0;
    size_t colonPos = timeStr.find(':');
    if (colonPos != string::npos) {
        hour = stoi(timeStr.substr(0, colonPos));
        minute = stoi(timeStr.substr(colonPos + 1));
    }
    return hour * 60 + minute;
}

string ShowtimeGenerator::minutesToTime(int minutes) {
    // Convert minutes since midnight to "HH:MM"
    int hour = minutes / 60;
    int minute = minutes % 60;
    
    stringstream ss;
    ss << setfill('0') << setw(2) << hour << ":"
       << setfill('0') << setw(2) << minute;
    return ss.str();
}
