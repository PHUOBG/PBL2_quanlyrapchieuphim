#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <optional>
#include <memory>

using namespace sf;
using namespace std;

class RoomPanel {
private:
    struct RoomRow {
        string roomId;
        string roomName;
        string currentMovie;
        string upcomingMovie;
    };

    struct RoomInfo {
        string id;
        string name;
    };

    struct MovieInfo {
        string title;
        int durationMinutes;
    };

    struct ShowtimeSlot {
        chrono::system_clock::time_point start;
        chrono::system_clock::time_point end;
        string movieTitle;
    };

    Font& font;
    float width;
    float height;
    Vector2f position;

    RectangleShape background;
    RectangleShape tableHeaderBg;
    unique_ptr<Text> titleText;

    Texture reloadTexture;
    unique_ptr<Sprite> reloadSprite;
    RectangleShape reloadButtonBg;
    bool reloadHovered = false;
    bool reloadPressed = false;

    RectangleShape notificationBg;
    unique_ptr<Text> notificationText;
    string notificationMessage;
    bool notificationVisible = false;
    Clock notificationClock;

    vector<RoomRow> rows;
    vector<RoomInfo> rooms;
    unordered_map<string, vector<ShowtimeSlot>> roomSchedules;
    unordered_map<string, vector<ShowtimeSlot>> liveSchedules;
    unordered_map<string, vector<ShowtimeSlot>> cachedSchedules;
    bool cacheDirty = false;
    const string cacheFilePath = "../data/room_schedule_cache.txt";

    int hoveredRow = -1;
    int selectedRow = -1;

    Clock statusRefreshClock;
    const float statusRefreshInterval = 1.0f;

    static vector<string> splitLine(const string& line, char delimiter);
    static string trim(const string& text);
    static optional<chrono::system_clock::time_point> parseDateTime(const string& dateStr, const string& timeStr);
    static optional<chrono::system_clock::time_point> parseCacheDateTime(const string& value);
    static string formatDateTime(chrono::system_clock::time_point tp);

    void setupUI();
    void loadData();
    vector<RoomInfo> loadRooms(const string& path);
    unordered_map<string, MovieInfo> loadMovies(const string& path);
    void loadShowtimes(const string& path, const unordered_map<string, MovieInfo>& movies);
    void loadCache();
    void saveCache();
    bool removeExpiredCachedShows();
    bool ensureUpcomingShowsCached();
    bool hasSlot(const vector<ShowtimeSlot>& slots, const ShowtimeSlot& slot) const;
    static bool slotsEqual(const ShowtimeSlot& a, const ShowtimeSlot& b);
    static bool pruneSchedules(unordered_map<string, vector<ShowtimeSlot>>& schedules,
                               chrono::system_clock::time_point now);
    void updateRoomStatuses();
    void renderTable(RenderWindow& window);

    void showNotification(const string& message);
    void renderNotification(RenderWindow& window);

    Color bgColor = Color(244, 246, 250);
    Color headerColor = Color(20, 118, 172);
    Color rowColor = Color(255, 255, 255);
    Color hoverColor = Color(229, 241, 251);
    Color selectedColor = Color(204, 228, 247);
    Color borderColor = Color(201, 206, 214);
    Color textColor = Color(34, 34, 34);

    static constexpr float TABLE_X = 40.f;
    static constexpr float TABLE_Y = 120.f;
    static constexpr float TABLE_WIDTH = 1100.f;
    static constexpr float HEADER_HEIGHT = 44.f;
    static constexpr float ROW_HEIGHT = 48.f;
    static constexpr float COL_ID_X = TABLE_X + 10.f;
    static constexpr float COL_NAME_X = TABLE_X + 140.f;
    static constexpr float COL_CURRENT_X = TABLE_X + 420.f;
    static constexpr float COL_NEXT_X = TABLE_X + 780.f;

public:
    RoomPanel(Font& font, float width, float height);
    void setPosition(Vector2f pos);
    void handleEvent(const Event& event, const RenderWindow& window);
    void update(Vector2f mousePos, bool mousePressed);
    void render(RenderWindow& window);
};