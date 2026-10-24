#pragma once
#include <SFML/Graphics.hpp>
#include <array>
#include <memory>
#include <string>
#include <vector>
#include "data-structures/DLL.h"
#include "repositories/admin/AdminTicketRepository.h"
#include "UI/components/Admin/AdminSidebar.h"
#include "UI/components/Admin/RoundRectButton.h"

class TicketPanel {
private:
    struct ColumnInfo {
        const char* label;
        float width;
    };

    Font& font;
    float width;
    float height;
    Vector2f position;

    AdminTicketRepository repository;
    DLL<Ticket> tickets;

    RectangleShape background;
    RectangleShape tableHeaderBg;
    std::unique_ptr<Text> titleText;

    std::unique_ptr<SidebarRoundRectButton> btnView;
    std::unique_ptr<SidebarRoundRectButton> btnDelete;
    std::unique_ptr<SidebarRoundRectButton> btnReload;
    Texture reloadTexture;
    Sprite reloadSprite;

    bool detailCloseHover = false;

    int selectedRow = -1;
    int hoveredRow = -1;
    int scrollOffset = 0;

    // Popup hien chi tiet
    bool detailVisible = false;
    RectangleShape overlay;
    RectangleShape detailPanel;
    std::unique_ptr<Text> detailTitle;
    struct DetailEntry {
        Text label;
        Text value;

        explicit DetailEntry(Font& font) : label(font), value(font) {}
        DetailEntry(const DetailEntry&) = default;
        DetailEntry(DetailEntry&&) noexcept = default;
        DetailEntry& operator=(const DetailEntry&) = default;
        DetailEntry& operator=(DetailEntry&&) noexcept = default;
    };
    std::vector<DetailEntry> detailEntries;
    RectangleShape detailCloseBg;
    std::unique_ptr<Text> detailCloseText;

    // Thong bao nho tren man hinh
    bool notificationVisible = false;
    Clock notificationClock;
    RectangleShape notificationBg;
    std::unique_ptr<Text> notificationText;

    static constexpr float TABLE_X = 40.f;
    static constexpr float TABLE_Y = 120.f;
    static constexpr float TABLE_WIDTH = 1100.f;
    static constexpr float TABLE_HEIGHT = 720.f;
    static constexpr float HEADER_HEIGHT = 46.f;
    static constexpr float ROW_HEIGHT = 40.f;

    inline static const std::array<ColumnInfo, 5> COLUMNS = {
        ColumnInfo{"ID vé", 120.f},
        ColumnInfo{"ID suất chiếu", 300.f},
        ColumnInfo{"Email đặt vé", 320.f},
        ColumnInfo{"Ngày đặt vé", 180.f},
        ColumnInfo{"Giờ đặt vé", 180.f}
    };

    void setupUI();
    void refreshTickets();
    void handleReload();
    void handleDelete();
    void openDetailPopup();
    void closeDetailPopup();
    void rebuildDetailTexts(const Ticket& ticket);

    void renderTable(RenderWindow& window);
    void renderButtons(RenderWindow& window);
    void renderNotification(RenderWindow& window);
    void renderDetailPopup(RenderWindow& window);

    void updateButtonStates();
    void showNotification(const std::string& message);
    int hitTestRow(Vector2f mousePos) const;
    FloatRect getTableBounds() const;

    static String utf8(const std::string& text);

public:
    TicketPanel(Font& font, float width, float height);

    void setPosition(Vector2f pos);
    void handleEvent(const Event& event, const RenderWindow& window);
    void update(Vector2f mousePos, bool mousePressed);
    void render(RenderWindow& window);
};
