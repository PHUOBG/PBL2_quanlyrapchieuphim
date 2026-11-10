#pragma once

#include <SFML/Graphics.hpp>
#include <memory>
#include <vector>
#include <string>

using namespace sf;
using namespace std;

struct VoucherItem {
    string code;            // "WELCOME"
    wstring subtitle;       // L"Chào mừng thành viên CineCore"
    int type = 2;           // 1: Tiền, 2: %
    int value = 20;         // 20 (%) hoặc 30000 (đ)
    int minBill = 0;        // 0 đ
    int recipientCount = 145;
    wstring tag = L"";      // "Mới", "VIP", "HSSV"
    bool isChecked = false;
};

struct RecipientItem {
    string email;
    wstring fullName;
    string receivedDate;
    int quantity = 1;
    bool isUsed = false;
};

class VoucherPanel {
private:
    Font& font;
    Font boldFont;
    float width, height;
    Vector2f position;

    // Data
    vector<VoucherItem> allVouchers;
    vector<VoucherItem> filteredVouchers;
    int selectedVoucherIndex = 0; // Mặc định WELCOME

    vector<RecipientItem> currentRecipients;
    vector<RecipientItem> filteredRecipients;

    // Filter & Search
    wstring searchVoucherQuery;
    bool searchVoucherActive = false;
    int voucherFilterType = 0; // 0: Tất cả, 1: Giảm %, 2: Giảm tiền

    wstring searchRecipientQuery;
    bool searchRecipientActive = false;

    // Pagination
    int currentVoucherPage = 1;
    const int pageSize = 6;
    int totalVoucherPages = 2;

    int currentRecipientPage = 1;
    const int recipientPageSize = 6;
    int totalRecipientPages = 1;

    // Hover states
    int hoveredVoucherRow = -1;
    int hoveredEditBtnRow = -1;
    int hoveredRecipientRow = -1;
    bool hoveredBackBtn = false;
    bool hoveredAddUserBtn = false;
    bool hoveredRevokeBtn = false;

    // Toast
    string notificationMessage;
    Clock notificationClock;
    bool notificationVisible = false;

    // Popup Add / Edit Voucher
    bool showAddPopup = false;
    int editingIndex = -1;
    int popupActiveField = 0;
    string popupCode;
    wstring popupSub;
    int popupType = 2;
    string popupValue;
    string popupMinBill;

    // Popup Thêm người nhận
    bool showAddRecipientPopup = false;
    int recipientPopupActiveField = 0;
    string popupRecipientEmail;
    wstring popupRecipientName;
    string popupRecipientQty = "1";

    void initDefaultData();
    void loadVouchersFromFile();
    void saveVouchersToFile();
    void applyVoucherFilter();
    void applyRecipientFilter();

    void drawRoundedCard(RenderWindow& win, Vector2f pos, Vector2f sz, float r, Color fill, float outline = 0.f, Color outCol = Color::Transparent);

public:
    VoucherPanel(Font& fontRef, float w, float h);

    void setPosition(Vector2f pos);
    void handleEvent(const Event& event, const RenderWindow& window);
    void update(Vector2f mousePos, bool mousePressed);
    void render(RenderWindow& window);
};
