#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <memory>
#include <functional>

using namespace sf;
using namespace std;

enum class SeatType {
    STANDARD,      // Ghế Thường (100.000đ)
    VIP,           // Ghế VIP Gold (140.000đ)
    SWEETBOX,      // Ghế Đôi Sweetbox (220.000đ/cặp)
    MAINTENANCE    // Ghế bảo trì
};

struct RoomData {
    string id;
    wstring name;
    wstring roomType;     // "Laser 4K Atmos", "IMAX 3D", "Standard 2D"
    wstring status;       // "Bình thường", "Bảo trì"
    int rows = 6;
    int cols = 6;
    vector<vector<SeatType>> seatMatrix;
};

class RoomPanel {
private:
    Font& font;
    Font boldFont;
    float width;
    float height;
    Vector2f position;

    // Data
    vector<RoomData> rooms;
    int selectedRoomIndex = 0;
    SeatType currentAssignType = SeatType::VIP;
    int activeTab = 0; // 0: Sơ đồ trực quan, 1: Danh sách phòng
    float zoomFactor = 1.0f;

    // Form inputs state
    wstring inputRoomName;
    string inputRows;
    string inputCols;
    wstring inputStatus;
    int activeInputBox = -1; // 0: Name, 1: Rows, 2: Cols

    // Dropdown states
    bool roomDropdownOpen = false;
    bool statusDropdownOpen = false;
    bool seatTypeDropdownOpen = false;

    // Notification toast
    string notificationMessage;
    Clock notificationClock;
    bool notificationVisible = false;

    // Hover states
    int hoveredSeatRow = -1;
    int hoveredSeatCol = -1;
    int hoveredButton = -1; // 1: Refresh, 2: Add, 3: Delete, 4: Save, 5: Cancel, 6: Zoom-, 7: Zoom+

    void initDefaultRooms();
    void loadRoomsFromFile();
    void saveRoomsToFile();
    void updateSeatStatistics(int& standard, int& vip, int& sweetbox, int& maintenance) const;

    // Drawing helpers
    void drawSeat(RenderWindow& win, float x, float y, float w, float h, SeatType type, const string& label, bool hovered);
    void drawRoundedCard(RenderWindow& win, Vector2f pos, Vector2f sz, float r, Color fill, float outline = 0.f, Color outCol = Color::Transparent);
    void drawIcon(RenderWindow& win, const string& iconName, Vector2f pos, Color col);

public:
    RoomPanel(Font& fontRef, float w, float h);

    void setPosition(Vector2f pos);
    void handleEvent(const Event& event, const RenderWindow& window);
    void update(Vector2f mousePos, bool mousePressed);
    void render(RenderWindow& window);
};