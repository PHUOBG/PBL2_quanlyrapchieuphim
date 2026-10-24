#pragma once

#include <SFML/Graphics.hpp>
#include <array>
#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "UI/components/Admin/DropdownBox.h"

class ShowtimePanel {
private:
    struct MovieInfo {
        std::string title;
        int durationMinutes = 0;
    };

    struct TimelineItem {
        std::string showtimeId;
        std::string movieId;
        std::string roomId;
        std::string date;
        std::string timeStr;
        int price = 0;
        int durationMinutes = 0;
        std::chrono::system_clock::time_point start;
        std::chrono::system_clock::time_point end;
    };

    using TimePoint = std::chrono::system_clock::time_point;

    sf::Font& font;
    float width;
    float height;
    sf::Vector2f position;

    sf::RectangleShape background;
    sf::RectangleShape tableHeaderBg;
    sf::RectangleShape summaryCard;
    sf::RectangleShape tableBorder;
    std::unique_ptr<sf::Text> titleText;
    std::unique_ptr<sf::Text> summaryPrimaryText;
    std::unique_ptr<sf::Text> summaryTitleText;
    std::unique_ptr<sf::Text> summaryMetaDateText;
    std::unique_ptr<sf::Text> summaryMetaRoomText;

    std::unique_ptr<DropdownBox> dateDropdown;
    std::unique_ptr<DropdownBox> roomDropdown;

    sf::Texture reloadTexture;
    std::unique_ptr<sf::Sprite> reloadSprite;
    sf::RectangleShape reloadButtonBg;
    bool reloadHovered = false;
    bool reloadPressed = false;

    sf::RectangleShape notificationBg;
    std::unique_ptr<sf::Text> notificationText;
    std::string notificationMessage;
    bool notificationVisible = false;
    sf::Clock notificationClock;

    std::vector<TimelineItem> allShowtimes;
    std::vector<TimelineItem> filteredShowtimes;
    std::unordered_set<std::string> archivedIds;
    std::unordered_set<std::string> loadedShowtimeIds;
    std::unordered_map<std::string, MovieInfo> movieCatalog;
    std::unordered_map<std::string, std::string> roomNames;

    std::vector<std::string> dateOptions;
    std::vector<std::string> roomOptionLabels;
    std::unordered_map<std::string, std::string> roomLabelToId;
    std::string activeDateFilter;
    std::string activeRoomFilterLabel;

    int hoveredRow = -1;
    int topRowIndex = 0;

    static constexpr float TABLE_X = 40.f;
    static constexpr float TABLE_Y = 200.f;
    static constexpr float TABLE_WIDTH = 1100.f;
    static constexpr float TABLE_HEIGHT = 600.f;
    static constexpr float HEADER_HEIGHT = 50.f;
    static constexpr float ROW_HEIGHT = 52.f;
    static constexpr int MAX_VISIBLE_ROWS = 10;

    const std::string showtimeFile = "../data/showtimes.txt";
    const std::string historyFile = "../data/showtimes_history.txt";
    const std::string roomsFile = "../data/rooms.txt";
    const std::string moviesFile = "../data/movies.txt";

    void setupUI();
    void loadData();
    void loadMovies();
    void loadRooms();
    void loadShowtimes();
    void loadHistory();
    void appendHistory(const std::vector<TimelineItem>& entries);
    void refreshDropdowns();
    void applyFilters();
    void updateSummaryText();
    void updateHoveredRow(sf::Vector2f mousePos);
    void clampScroll();

    void handleScroll(float delta, const sf::RenderWindow& window);
    void renderTable(sf::RenderWindow& window);
    void renderEmptyState(sf::RenderWindow& window);

    void showNotification(const std::string& message);
    void renderNotification(sf::RenderWindow& window);

    static std::vector<std::string> splitLine(const std::string& line, char delimiter);
    static std::string trim(const std::string& text);
    static std::optional<TimePoint> parseDateTime(const std::string& dateStr, const std::string& timeStr);
    static std::string formatTime(const TimePoint& tp);
    static std::string formatTimeRange(const TimelineItem& item);
    static std::string formatPrice(int price);
    static std::string getTodayDateString();
    static bool isDateOnOrBeforeToday(const std::string& dateStr);
    static std::string serializeShowtime(const TimelineItem& item);
    std::optional<TimelineItem> buildShowtime(const std::vector<std::string>& parts);

public:
    ShowtimePanel(sf::Font& font, float width, float height);

    void setPosition(sf::Vector2f pos);
    void handleEvent(const sf::Event& event, const sf::RenderWindow& window);
    void update(sf::Vector2f mousePos, bool mousePressed);
    void render(sf::RenderWindow& window);
};
