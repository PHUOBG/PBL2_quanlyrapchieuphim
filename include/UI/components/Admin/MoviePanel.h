#pragma once

#include <SFML/Graphics.hpp>
#include "UI/components/Admin/TextBox.h"
#include "repositories/admin/AdminMovieRepository.h"
#include "UI/components/Button.h"
#include "data-structures/DLL.h"
#include <string>
#include <memory>

using namespace sf;
using namespace std;

class MoviePanel {
private:
    Font& font;
    float width, height;
    Vector2f position;
    
    // Tang truy cap du lieu
    unique_ptr<AdminMovieRepository> repository;
    
    // Thanh phan giao dien
    unique_ptr<Text> titleText;
    RectangleShape background;
    RectangleShape tableHeaderBg;
    DLL<RectangleShape> rowBackgrounds;
    DLL<RectangleShape> rowBorders;
    
    // Cac nut CRUD tuy chinh
    RectangleShape btnAddBg;      // Thêm phim
    RectangleShape btnEditBg;     // Sửa thông tin
    RectangleShape btnDeleteBg;   // Xóa phim
    unique_ptr<Text> btnAddText;
    unique_ptr<Text> btnEditText;
    unique_ptr<Text> btnDeleteText;
    bool btnAddHover = false;
    bool btnEditHover = false;
    bool btnDeleteHover = false;
    bool btnAddPressed = false;
    bool btnEditPressed = false;
    bool btnDeletePressed = false;
    
    // Nut tai lai bang bieu tuong
    Texture reloadTexture;
    Sprite reloadSprite;
    RectangleShape reloadButtonBg;
    bool btnReloadHover = false;
    bool btnReloadPressed = false;
    
    // Trang thai cua hop thoai
    enum PopupType { NONE, ADD, EDIT, DELETE_CONFIRM };
    PopupType currentPopup;
    int selectedRow;
    
    // Thanh phan trong popup
    RectangleShape popupBackground;
    RectangleShape popupOverlay;
    unique_ptr<Text> popupTitle;
    DLL<unique_ptr<TextBox>> inputBoxes;
    unique_ptr<Button> btnPopupSave;
    unique_ptr<Button> btnPopupCancel;
    
    // Thong bao trang thai
    string notificationText;
    Clock notificationClock;
    RectangleShape notificationBg;
    unique_ptr<Text> notificationTextObj;
    bool notificationVisible = false;

    // Popup canh bao khi chua chon dong
    bool selectionWarningVisible = false;
    RectangleShape selectionWarningOverlay;
    RectangleShape selectionWarningBg;
    unique_ptr<Text> selectionWarningText;
    RectangleShape selectionWarningButton;
    unique_ptr<Text> selectionWarningButtonText;
    bool selectionWarningButtonHover = false;
    string selectionWarningMessage;
    
    // Trang thai bang hien thi
    int scrollOffset;
    int hoveredRow;
    
    // Cac hang so bo cuc (toi uu cho panel ~1428px)
    static constexpr float TABLE_X = 40.f;      // Reduced left margin
    static constexpr float TABLE_Y = 120.f;
    static constexpr float TABLE_WIDTH = 1100.f; // Reduced from 1320 to fit in panel
    static constexpr float TABLE_HEIGHT = 680.f;
    static constexpr float HEADER_HEIGHT = 44.f;
    static constexpr float ROW_HEIGHT = 38.f;
    
    static constexpr float COL_ID_X = 60.f;      // Adjusted
    static constexpr float COL_ID_WIDTH = 80.f;
    static constexpr float COL_TITLE_X = 150.f;  // Adjusted
    static constexpr float COL_TITLE_WIDTH = 350.f;
    static constexpr float COL_DURATION_X = 510.f; // Adjusted
    static constexpr float COL_DURATION_WIDTH = 120.f;
    static constexpr float COL_DATE_X = 640.f;   // Adjusted
    static constexpr float COL_DATE_WIDTH = 150.f;
    static constexpr float COL_STATUS_X = 800.f;   // Adjusted
    static constexpr float COL_STATUS_WIDTH = 300.f;
    
    // Bang mau su dung
    Color bgColor = Color(244, 246, 250);      // #F4F6FA
    Color headerColor = Color(20, 118, 172);   // #1476AC
    Color rowColor = Color(255, 255, 255);     // #FFFFFF
    Color hoverColor = Color(229, 241, 251);   // #E5F1FB
    Color selectedColor = Color(204, 228, 247); // #CCE4F7
    Color borderColor = Color(201, 206, 214);  // #C9CED6
    Color textColor = Color(34, 34, 34);       // #222222
    
    // Ham ho tro ve hinh chu nhat bo goc
    
    void setupUI();
    void renderTable(RenderWindow& window);
    void renderPopup(RenderWindow& window);
    void renderNotification(RenderWindow& window);
    void renderSelectionWarning(RenderWindow& window);
    
    void openAddPopup();
    void openEditPopup();
    void openDeleteConfirm();
    void closePopup();
    
    void handleAdd();
    void handleEdit();
    void handleDelete();
    void handleReload();
    
    void showNotification(const string& message);
    void showSelectionWarning(const string& message);
    void updateSelectionWarning(Vector2f mousePos, bool mousePressed);
    void handleSelectionWarningEvent(const Event& event, const RenderWindow& window);
    
    public:
        MoviePanel(Font& font, float width, float height);
    
        void setPosition(Vector2f pos);
        void handleEvent(const Event& event, const RenderWindow& window);
        void update(Vector2f mousePos, bool mousePressed);
        void render(RenderWindow& window);
    };
