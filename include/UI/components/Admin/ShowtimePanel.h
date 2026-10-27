#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <optional>

using namespace sf;
using namespace std;

struct ShowtimeItem {
    string id;
    string movieId;
    wstring movieTitle;
    string roomId;
    wstring roomName;
    string date;        // yyyy-mm-dd
    string startTime;   // hh:mm
    string endTime;     // hh:mm
    int price = 65000;
    int bookedSeats = 28;
    int totalSeats = 36;
};

class ShowtimePanel {
private:
    Font& font;
    Font boldFont;
    float width;
    float height;
    Vector2f position;

    // Data
    vector<ShowtimeItem> allShowtimes;
    vector<ShowtimeItem> filteredShowtimes;

    // Filters
    string selectedDate = "2026-10-27";
    wstring selectedRoom = L"T\u1ea5t c\u1ea3 ph\u00f2ng";
    wstring selectedStatus = L"T\u1ea5t c\u1ea3 tr\u1ea1ng th\u00e1i";

    // Dropdowns
    bool roomDropdownOpen = false;
    bool statusDropdownOpen = false;
    vector<wstring> roomList;
    vector<wstring> statusList;

    // Pagination
    int currentPage = 1;
    const int pageSize = 10;
    int totalPages = 1;

    // Hover states
    int hoveredRow = -1;
    int hoveredAction = -1; // 0: Edit, 1: Seats, 2: Delete

    // Toast notification
    string notificationMessage;
    Clock notificationClock;
    bool notificationVisible = false;

    // Popup Add/Edit
    bool showAddPopup = false;
    int editingIndex = -1;
    wstring popupMovieTitle;
    wstring popupRoomName;
    string popupStartTime;
    string popupPrice;
    int popupActiveField = 0;

    void initSampleShowtimes();
    void loadShowtimesFromFile();
    void saveShowtimesToFile();
    void applyFilter();
    void autoScheduleShowtimes();

    void drawRoundedCard(RenderWindow& win, Vector2f pos, Vector2f sz, float r, Color fill, float outline = 0.f, Color outCol = Color::Transparent);

public:
    ShowtimePanel(Font& fontRef, float w, float h);

    void setPosition(Vector2f pos);
    void handleEvent(const Event& event, const RenderWindow& window);
    void update(Vector2f mousePos, bool mousePressed);
    void render(RenderWindow& window);
};
