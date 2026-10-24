#pragma once
#include <SFML/Graphics.hpp>
#include <memory>
#include <vector>
#include <string>
#include "repositories/admin/AdminComboRepository.h"
#include "UI/components/Admin/TextBox.h"
#include "UI/components/Button.h"

using namespace sf;
using namespace std;

class ComboPanel {
private:
    Font& font;
    float width;
    float height;
    Vector2f position;

    unique_ptr<AdminComboRepository> repository;

    RectangleShape background;
    RectangleShape tableHeaderBg;
    unique_ptr<Text> titleText;

    RectangleShape btnAddBg;
    RectangleShape btnEditBg;
    RectangleShape btnDeleteBg;
    RectangleShape reloadButtonBg;
    unique_ptr<Text> btnAddText;
    unique_ptr<Text> btnEditText;
    unique_ptr<Text> btnDeleteText;

    Texture reloadTexture;
    Sprite reloadSprite;

    bool btnAddHover = false;
    bool btnEditHover = false;
    bool btnDeleteHover = false;
    bool btnReloadHover = false;
    bool btnAddPressed = false;
    bool btnEditPressed = false;
    bool btnDeletePressed = false;
    bool btnReloadPressed = false;

    enum PopupType { NONE, ADD, EDIT, DELETE_CONFIRM };
    PopupType currentPopup = NONE;

    int selectedRow = -1;
    int hoveredRow = -1;

    RectangleShape popupOverlay;
    RectangleShape popupBackground;
    unique_ptr<Text> popupTitle;
    vector<unique_ptr<TextBox>> inputBoxes;
    unique_ptr<Button> btnPopupSave;
    unique_ptr<Button> btnPopupCancel;

    string notificationText;
    Clock notificationClock;
    RectangleShape notificationBg;
    unique_ptr<Text> notificationTextObj;

    static constexpr float TABLE_X = 40.f;
    static constexpr float TABLE_Y = 120.f;
    static constexpr float TABLE_WIDTH = 900.f;
    static constexpr float ROW_HEIGHT = 44.f;
    static constexpr float HEADER_HEIGHT = 48.f;
    static constexpr float COL_ID_X = 60.f;
    static constexpr float COL_NAME_X = 180.f;
    static constexpr float COL_PRICE_X = 720.f;

    Color bgColor = Color(244, 246, 250);
    Color headerColor = Color(20, 118, 172);
    Color rowColor = Color::White;
    Color hoverColor = Color(229, 241, 251);
    Color selectedColor = Color(204, 228, 247);
    Color borderColor = Color(201, 206, 214);
    Color textColor = Color(34, 34, 34);

    void setupUI();
    void renderTable(RenderWindow& window);
    void renderPopup(RenderWindow& window);
    void renderNotification(RenderWindow& window);

    void openAddPopup();
    void openEditPopup();
    void openDeletePopup();
    void closePopup();

    void handleAdd();
    void handleEdit();
    void handleDelete();
    void handleReload();

    void showNotification(const string& message);
    static bool isDigits(const string& value);

public:
    ComboPanel(Font& font, float width, float height);

    void setPosition(Vector2f pos);
    void handleEvent(const Event& event, const RenderWindow& window);
    void update(Vector2f mousePos, bool mousePressed);
    void render(RenderWindow& window);
};