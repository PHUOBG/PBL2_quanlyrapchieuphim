#pragma once

#include <SFML/Graphics.hpp>
#include <memory>
#include <vector>
#include <string>
#include <functional>
#include <set>
#include "repositories/admin/AdminVoucherRepository.h"
#include "UI/components/Admin/TextBox.h"
#include "UI/components/Admin/UserSelectPopup.h"
#include "UI/components/Button.h"

using namespace sf;
using namespace std;

class VoucherPanel {
public:
    VoucherPanel(Font& font, float width, float height);
    
    void setPosition(Vector2f pos);
    void handleEvent(const Event& event, const RenderWindow& window);
    void update(Vector2f mousePos, bool mousePressed);
    void render(RenderWindow& window);
    
private:
    Font& font;
    float width, height;
    Vector2f position;
    
    // Tang du lieu
    unique_ptr<AdminVoucherRepository> repository;
    
    // Trang thai hien tai
    enum class ViewMode { DEFINITIONS, USER_VOUCHERS };
    ViewMode currentMode = ViewMode::DEFINITIONS;
    int selectedVoucherIndex = -1;
    int hoveredVoucherIndex = -1;
    string selectedVoucherCode = "";
    
    // Thong tin trang thai man hinh user voucher
    int selectedUserIndex = -1;
    int hoveredUserIndex = -1;
    vector<VoucherUserInfo> currentVoucherUsers;
    
    // Trang thai cuon
    int defScrollOffset = 0;
    int userScrollOffset = 0;
    
    // Thanh phan giao dien
    RectangleShape background;
    RectangleShape leftPanelBg;
    RectangleShape rightPanelBg;
    unique_ptr<Text> titleText;
    unique_ptr<Text> leftPanelTitle;
    unique_ptr<Text> rightPanelTitle;
    
    // Header bang
    RectangleShape defTableHeader;
    RectangleShape userTableHeader;
    
    // Nut thao tac o che do document voucher
    struct ActionButton {
        RectangleShape box;
        unique_ptr<Text> label;
        Color baseColor;
        Color hoverColor;
        bool hovered = false;
    };
    
    ActionButton btnAddVoucher;
    ActionButton btnEditVoucher;
    ActionButton btnDeleteVoucher;
    ActionButton btnDistribute;  // Cấp voucher đồng loạt
    ActionButton btnViewUsers;   // Xem users có voucher
    ActionButton btnRefresh;
    
    // Nut thao tac o che do user voucher
    ActionButton btnAddToUser;
    ActionButton btnRemoveFromUser;
    ActionButton btnBackToList;
    
    Texture reloadTexture;
    optional<Sprite> reloadSprite;
    
    // Thiet lap popup
    enum class PopupType { NONE, ADD_VOUCHER, EDIT_VOUCHER, DELETE_VOUCHER, 
                          DISTRIBUTE_VOUCHER, ADD_TO_USER };
    PopupType currentPopup = PopupType::NONE;
    
    RectangleShape popupOverlay;
    RectangleShape popupBackground;
    unique_ptr<Text> popupTitle;
    vector<unique_ptr<TextBox>> inputBoxes;
    vector<unique_ptr<Text>> inputLabels;
    unique_ptr<Button> btnPopupSave;
    unique_ptr<Button> btnPopupCancel;
    
    // Popup chon nguoi dung
    unique_ptr<UserSelectPopup> userSelectPopup_;
    
    // Cach xu ly chon nguoi dung cu (se bo sau khi di chuyen sang popup moi)
    vector<pair<string, string>> availableUsers;  // Danh sach email + ten
    vector<int> filteredAvailableUsers;           // Chi so tuong ung sau khi loc
    set<int> selectedUserSet;                     // Cac chi so dang duoc chon
    int selectedAvailableUserIndex = -1;
    int hoveredAvailableUserIndex = -1;
    int availableUserScrollOffset = 0;
    string userSearchFilter;                      // Tu khoa tim kiem nguoi dung
    RectangleShape userListBg;
    unique_ptr<Button> btnSelectAll;              // Chon tat ca / huy chon
    unique_ptr<TextBox> searchBox;                // Hop tim kiem nguoi dung
    
    // Ham loc nguoi dung theo tu khoa
    void filterUsers(const string& query);
    
    // Thong bao
    string notificationMessage;
    Clock notificationClock;
    bool notificationVisible = false;
    Color notificationColor;
    RectangleShape notificationBg;
    unique_ptr<Text> notificationText;
    
    // Hang so bo cuc
    static constexpr float LEFT_PANEL_WIDTH = 580.f;
    static constexpr float RIGHT_PANEL_WIDTH = 540.f;
    static constexpr float PANEL_MARGIN = 20.f;
    static constexpr float HEADER_HEIGHT = 48.f;
    static constexpr float ROW_HEIGHT = 52.f;
    static constexpr float BUTTON_HEIGHT = 44.f;
    static constexpr float BUTTON_RADIUS = 8.f;
    
    // Bang mau
    Color bgColor = Color(244, 246, 250);
    Color panelBgColor = Color::White;
    Color headerColor = Color(20, 118, 172);
    Color tableHeaderColor = Color(27, 38, 59);
    Color rowColor = Color::White;
    Color rowAltColor = Color(248, 250, 252);
    Color hoverColor = Color(235, 244, 252);
    Color selectedColor = Color(214, 232, 248);
    Color borderColor = Color(210, 216, 224);
    Color textColor = Color(33, 37, 41);
    Color accentColor = Color(20, 118, 172);
    Color successColor = Color(40, 167, 69);
    Color warningColor = Color(255, 193, 7);
    Color dangerColor = Color(220, 53, 69);
    
    // Ham khoi tao UI
    void setupUI();
    void setupButton(ActionButton& button, const string& labelUtf8, 
                     const Color& base, const Color& hover, Vector2f size);
    void layoutElements();
    
    // Ham ve cac phan
    void renderLeftPanel(RenderWindow& window);
    void renderRightPanel(RenderWindow& window);
    void renderVoucherDefinitions(RenderWindow& window);
    void renderUserVouchers(RenderWindow& window);
    void renderButtons(RenderWindow& window);
    void renderPopup(RenderWindow& window);
    void renderNotification(RenderWindow& window);
    
    // Ham cap nhat trang thai
    void updateButton(ActionButton& button, Vector2f mousePos);
    void refreshData();
    void loadUsersForVoucher(const string& code);
    
    // Ham xu ly popup
    void openAddVoucherPopup();
    void openEditVoucherPopup();
    void openDeleteVoucherPopup();
    void openDistributePopup();
    void openAddToUserPopup();
    void closePopup();
    
    // Ham thuc hien hanh dong
    void handleAddVoucher();
    void handleEditVoucher();
    void handleDeleteVoucher();
    void handleDistribute();
    void handleAddToUser();
    void handleRemoveFromUser();
    
    // Ham ho tro chung
    void showNotification(const string& message, const Color& color = Color(20, 118, 172));
    string formatCurrency(double amount) const;
    string formatDate(const string& raw) const;
    string getTypeString(int type) const;
};
