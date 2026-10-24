#pragma once

#include <SFML/Graphics.hpp>
#include "repositories/UserRepository.h"
#include <array>
#include <string>
#include <vector>
#include <memory>
#include <optional>

using namespace sf;
using namespace std;

class UserPanel {
private:
    Font& font;
    float width, height;
    Vector2f position;
    
    // Tang du lieu
    unique_ptr<UserRepository> repository;
    vector<User> userList; // Bo nho dem phuc vu hien thi
    
    // Thanh phan giao dien
    unique_ptr<Text> titleText;
    RectangleShape background;
    RectangleShape tableHeaderBg;
    RectangleShape tableBodyBg;

    struct ActionButton {
        RectangleShape box;
        unique_ptr<Text> label;
        Color baseColor;
        Color hoverColor;
        bool hovered = false;
    };

    ActionButton btnViewInfo;
    ActionButton btnDelete;
    ActionButton btnRefresh;
    Texture reloadTexture;
    std::optional<Sprite> reloadSprite;

    // Thong bao trang thai
    RectangleShape notificationBg;
    unique_ptr<Text> notificationText;
    string notificationMessage;
    Clock notificationClock;
    bool notificationVisible = false;
    Color notificationColor = Color(20, 118, 172);

    // Trang thai cua bang
    int selectedRow = -1;
    int hoveredRow = -1;
    int scrollOffset = 0;

    // Hop thoai
    enum class PopupType { None, ViewInfo, DeleteConfirm };
    PopupType activePopup = PopupType::None;
    int popupUserIndex = -1;

    RectangleShape popupOverlay;
    RectangleShape popupPanel;
    std::unique_ptr<Text> popupTitle;

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

    ActionButton popupBtnPrimary;
    ActionButton popupBtnSecondary;
    
    // Hang so bo cuc
    static constexpr float TABLE_X = 40.f;
    static constexpr float TABLE_Y = 120.f;
    static constexpr float TABLE_WIDTH = 1100.f;
    static constexpr float TABLE_HEIGHT = 720.f;
    static constexpr float HEADER_HEIGHT = 46.f;
    static constexpr float ROW_HEIGHT = 40.f;

    // Bang mau dong nhat voi giao dien Admin
    Color bgColor = Color(244, 246, 250);
    Color headerColor = Color(20, 118, 172);
    Color tableHeaderColor = Color(27, 38, 59);
    Color rowColor = Color::White;
    Color hoverColor = Color(235, 244, 252);
    Color selectedColor = Color(214, 232, 248);
    Color borderColor = Color(210, 216, 224);
    Color textColor = Color(33, 37, 41);

    void setupUI();
    void setupButton(ActionButton& button, const string& labelUtf8, const Color& base, const Color& hover, Vector2f size);
    void updateButton(ActionButton& button, Vector2f mousePos);

    void refreshData();
    void renderTable(RenderWindow& window);
    void renderButtons(RenderWindow& window);
    void renderNotification(RenderWindow& window);
    void renderPopup(RenderWindow& window);
    void showNotification(const string& message, const Color& color = Color(20, 118, 172));
    string formatDate(time_t timestamp) const;
    void layoutElements();

    void closePopup();
    void openViewPopup();
    void openDeletePopup();
    void rebuildDetailTexts(const User& user);
    const User* getPopupUser() const;
    
public:
    UserPanel(Font& font, float width, float height);
    
    void setPosition(Vector2f pos);
    void handleEvent(const Event& event, const RenderWindow& window);
    void update(Vector2f mousePos, bool mousePressed);
    void render(RenderWindow& window);
};
