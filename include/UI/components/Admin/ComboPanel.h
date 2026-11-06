#pragma once
#include <SFML/Graphics.hpp>
#include <memory>
#include <vector>
#include <map>
#include <string>

using namespace sf;
using namespace std;

struct ComboItem {
    string id;              // "CB01"
    wstring name;           // L"CocaCola"
    wstring subtitle;       // L"1x Ly CocaCola tươi 22oz nhiều đá mát lạnh"
    wstring category;       // L"Đồ uống", L"Bắp ngọt", L"Bắp mặn", L"Combo Đơn", L"Combo Nhóm"
    int price = 30000;
    bool isHot = false;
    bool isAvailable = true;
    bool isChecked = false;
};

class ComboPanel {
private:
    Font& font;
    Font boldFont;
    float width;
    float height;
    Vector2f position;

    map<string, Texture> comboCache;
    const Texture* getComboTexture(const string& comboId, const wstring& name);

    vector<ComboItem> allCombos;
    vector<ComboItem> filteredCombos;

    // Filters & Search
    string searchQuery;
    bool searchActive = false;
    int activeFilterIndex = 0; // 0: Tất cả, 1: Nước lẻ, 2: Bắp đơn, 3: Combo 1 Người, 4: Combo Nhóm

    // Table state
    int selectedRow = -1;
    int hoveredRow = -1;
    int scrollOffset = 0;
    const int maxVisibleRows = 6; // Chiều cao mỗi dòng 72px

    // Buttons hover states
    bool btnAddHover = false;
    bool btnEditHover = false;
    bool btnDeleteHover = false;
    bool btnReloadHover = false;
    bool btnExportHover = false;
    bool btnSyncHover = false;

    // Toast notification
    string notificationMessage;
    Clock notificationClock;
    bool notificationVisible = false;

    Texture reloadTex;
    bool hasReloadTex = false;

    // Popup Add / Edit
    bool showPopup = false;
    int editingIndex = -1;
    string popupId;
    wstring popupName;
    wstring popupSub;
    wstring popupCat;
    string popupPrice;
    int popupActiveField = 0;

    void initDefaultCombos();
    void loadCombosFromFile();
    void saveCombosToFile();
    void applyFilter();

    void drawRoundedCard(RenderWindow& win, Vector2f pos, Vector2f sz, float r, Color fill, float outline = 0.f, Color outCol = Color::Transparent);

public:
    ComboPanel(Font& fontRef, float w, float h);

    void setPosition(Vector2f pos);
    void handleEvent(const Event& event, const RenderWindow& window);
    void update(Vector2f mousePos, bool mousePressed);
    void render(RenderWindow& window);
};