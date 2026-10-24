#include "UI/components/Admin/MoviePanel.h"
#include "UI/components/Admin/RoundedRectRenderer.h"
#include "services/ShowtimeCleanupService.h"
#include "utils/FileUtils.h"
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <array>
#include <ctime>
#include <cctype>

namespace {
bool parseDdMmYyyy(const std::string& s, int& d, int& m, int& y) {
    d = m = y = 0;
    std::stringstream ss(s);
    std::string dd, mm, yy;
    if (!std::getline(ss, dd, '/')) return false;
    if (!std::getline(ss, mm, '/')) return false;
    if (!std::getline(ss, yy, '/')) return false;
    try {
        d = std::stoi(dd);
        m = std::stoi(mm);
        y = std::stoi(yy);
    } catch (...) {
        return false;
    }
    return d >= 1 && d <= 31 && m >= 1 && m <= 12 && y >= 1900;
}

std::string formatDdMmYyyy(int d, int m, int y) {
    std::stringstream out;
    out << std::setfill('0') << std::setw(2) << d << "/" << std::setfill('0') << std::setw(2) << m << "/" << y;
    return out.str();
}

std::string addOneMonthDdMmYyyy(const std::string& start) {
    int d, m, y;
    if (!parseDdMmYyyy(start, d, m, y)) return "";
    std::tm t{};
    t.tm_mday = d;
    t.tm_mon = m - 1;
    t.tm_year = y - 1900;
    t.tm_hour = 12;
    std::time_t tt = std::mktime(&t);
    if (tt == (std::time_t)-1) return "";
    // Add 1 month using tm normalization
    t.tm_mon += 1;
    tt = std::mktime(&t);
    if (tt == (std::time_t)-1) return "";
    std::tm* out = std::localtime(&tt);
    return formatDdMmYyyy(out->tm_mday, out->tm_mon + 1, out->tm_year + 1900);
}

// Compare dd/mm/yyyy by converting to yyyymmdd int (simple, sufficient for this UI)
int dateKeyDdMmYyyy(const std::string& s) {
    int d, m, y;
    if (!parseDdMmYyyy(s, d, m, y)) return 0;
    return y * 10000 + m * 100 + d;
}

std::string todayDdMmYyyy() {
    std::time_t now = std::time(nullptr);
    std::tm* t = std::localtime(&now);
    return formatDdMmYyyy(t->tm_mday, t->tm_mon + 1, t->tm_year + 1900);
}

std::string computeStatus(const std::string& start, const std::string& end) {
    const int today = dateKeyDdMmYyyy(todayDdMmYyyy());
    const int s = dateKeyDdMmYyyy(start);
    const int e = dateKeyDdMmYyyy(end);
    if (s == 0 || e == 0) return "Đang chiếu";
    if (today < s) return "Sắp chiếu";
    // end_date is treated as the first day the movie STOPs showing (exclusive bound).
    if (today >= e) return "Ngừng chiếu";
    return "Đang chiếu";
}

bool tryExtractDdMmYyyyFromText(const std::string& text, std::string& outDdMmYyyy) {
    outDdMmYyyy.clear();
    if (text.size() < 10) return false;
    for (size_t i = 0; i + 10 <= text.size(); ++i) {
        const char c0 = text[i];
        if (c0 < '0' || c0 > '9') continue;

        const std::string candidate = text.substr(i, 10);
        int d, m, y;
        if (parseDdMmYyyy(candidate, d, m, y)) {
            outDdMmYyyy = candidate;
            return true;
        }
    }
    return false;
}

bool tryGetScheduledStopDateIfAlreadyDeleted(const std::vector<std::string>& record, std::string& outDdMmYyyy) {
    outDdMmYyyy.clear();
    if (record.size() < 14) return false;
    const std::string& status = record[13];
    if (status.find("Ngừng chiếu vào") == std::string::npos && status.find("ngừng chiếu vào") == std::string::npos) {
        return false;
    }

    if (tryExtractDdMmYyyyFromText(status, outDdMmYyyy)) return true;

    int d, m, y;
    if (parseDdMmYyyy(record[8], d, m, y)) {
        outDdMmYyyy = record[8];
        return true;
    }

    return true;
}

bool parseIsoYyyyMmDd(const std::string& s, int& y, int& m, int& d) {
    y = m = d = 0;
    if (s.size() < 10) return false;
    try {
        y = std::stoi(s.substr(0, 4));
        m = std::stoi(s.substr(5, 2));
        d = std::stoi(s.substr(8, 2));
    } catch (...) {
        return false;
    }
    return y >= 1900 && m >= 1 && m <= 12 && d >= 1 && d <= 31;
}

std::string isoToDdMmYyyy(const std::string& iso) {
    int y, m, d;
    if (!parseIsoYyyyMmDd(iso, y, m, d)) return "";
    return formatDdMmYyyy(d, m, y);
}

std::string todayIsoYyyyMmDd() {
    std::time_t now = std::time(nullptr);
    std::tm* t = std::localtime(&now);
    std::stringstream out;
    out << (t->tm_year + 1900) << "-" << std::setfill('0') << std::setw(2) << (t->tm_mon + 1)
        << "-" << std::setfill('0') << std::setw(2) << t->tm_mday;
    return out.str();
}

std::string ddMmYyyyToIsoYyyyMmDd(const std::string& ddmmyyyy) {
    int d, m, y;
    if (!parseDdMmYyyy(ddmmyyyy, d, m, y)) return "";
    std::stringstream out;
    out << y << "-" << std::setfill('0') << std::setw(2) << m << "-" << std::setfill('0') << std::setw(2) << d;
    return out.str();
}

std::string addDaysIsoYyyyMmDd(const std::string& iso, int days) {
    int y, m, d;
    if (!parseIsoYyyyMmDd(iso, y, m, d)) return "";
    std::tm t{};
    t.tm_year = y - 1900;
    t.tm_mon = m - 1;
    t.tm_mday = d;
    t.tm_hour = 12;
    std::time_t tt = std::mktime(&t);
    if (tt == (std::time_t)-1) return "";
    tt += static_cast<std::time_t>(days) * 24 * 60 * 60;
    std::tm* outTm = std::localtime(&tt);
    if (!outTm) return "";
    std::stringstream out;
    out << (outTm->tm_year + 1900) << "-" << std::setfill('0') << std::setw(2) << (outTm->tm_mon + 1)
        << "-" << std::setfill('0') << std::setw(2) << outTm->tm_mday;
    return out.str();
}

std::string trimCopy(const std::string& input) {
    auto first = std::find_if_not(input.begin(), input.end(), [](unsigned char ch) {
        return std::isspace(ch);
    });
    auto last = std::find_if_not(input.rbegin(), input.rend(), [](unsigned char ch) {
        return std::isspace(ch);
    }).base();
    if (first >= last) return "";
    return std::string(first, last);
}

std::string toUpperAscii(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::toupper(ch));
    });
    return value;
}

bool tryParsePositiveDuration(const std::string& value, int& minutes) {
    minutes = 0;
    if (value.empty()) return false;
    if (!std::all_of(value.begin(), value.end(), [](unsigned char ch) { return std::isdigit(ch); })) return false;
    try {
        minutes = std::stoi(value);
    } catch (...) {
        return false;
    }
    return minutes >= 30 && minutes <= 400;
}

bool isAllowedAgeRating(const std::string& rating) {
    static constexpr std::array<const char*, 4> allowed = {"T13", "T16", "T18", "PG"};
    return std::any_of(allowed.begin(), allowed.end(), [&](const char* expect) {
        return rating == expect;
    });
}

bool tryGetLastBookedShowDateIsoForMovie(const std::string& movieId, std::string& outLastIso) {
    outLastIso.clear();
    if (movieId.empty()) return false;

    std::unordered_set<std::string> bookedShowtimeIds;
    {
        std::ifstream tf(FileUtils::resolveDataPath("data/tickets.txt"));
        if (!tf.is_open()) return false;
        std::string line;
        bool first = true;
        while (std::getline(tf, line)) {
            if (first) { first = false; continue; }
            if (line.empty()) continue;
            std::stringstream ss(line);
            std::string ticketId, showtimeId;
            if (!std::getline(ss, ticketId, '|')) continue;
            if (!std::getline(ss, showtimeId, '|')) continue;
            if (!showtimeId.empty()) bookedShowtimeIds.insert(showtimeId);
        }
    }
    if (bookedShowtimeIds.empty()) return false;

    {
        std::ifstream sf(FileUtils::resolveDataPath("data/showtimes.txt"));
        if (!sf.is_open()) return false;
        std::string line;
        bool first = true;
        while (std::getline(sf, line)) {
            if (first) { first = false; continue; }
            if (line.empty()) continue;
            std::stringstream ss(line);
            std::string showtimeId, mid, room, dateIso;
            if (!std::getline(ss, showtimeId, '|')) continue;
            if (!std::getline(ss, mid, '|')) continue;
            if (!std::getline(ss, room, '|')) continue;
            if (!std::getline(ss, dateIso, '|')) continue;
            if (mid != movieId) continue;
            if (bookedShowtimeIds.count(showtimeId) == 0) continue;
            if (dateIso.size() < 10) continue;
            if (outLastIso.empty() || dateIso > outLastIso) outLastIso = dateIso;
        }
    }

    return !outLastIso.empty();
}
}

MoviePanel::MoviePanel(Font& font, float width, float height)
        : font(font), width(width), height(height),
            reloadTexture("../assets/elements/reload.png"),
            reloadSprite(reloadTexture),
            currentPopup(NONE),
            selectedRow(-1),
            scrollOffset(0),
            hoveredRow(-1) {
    
    // Initialize repository
    repository = make_unique<AdminMovieRepository>(FileUtils::resolveDataPath("data/movies.txt"));
    
    // Setup UI
    setupUI();
}

void MoviePanel::setupUI() {
    // Background - make it smaller to not cover buttons
    background.setSize(Vector2f(width, height)); // Leave space for buttons at bottom
    background.setFillColor(bgColor);
    
    // Title (reduced size for compact layout)
    string titleStr = "Quản lý phim";
    titleText = make_unique<Text>(font, String::fromUtf8(titleStr.begin(), titleStr.end()), 26);
    titleText->setFillColor(Color(27, 38, 59)); // #1B263B
    titleText->setStyle(Text::Bold);
    
    // Table header background
    tableHeaderBg.setSize(Vector2f(TABLE_WIDTH, HEADER_HEIGHT));
    tableHeaderBg.setFillColor(headerColor);
    
    // CRUD Buttons (aligned top-left, evenly spaced)
    const float btnW = 150.f;
    const float btnH = 44.f;

    // Add button
    btnAddBg.setSize(Vector2f(btnW, btnH));
    btnAddBg.setFillColor(Color(40, 167, 69));
    btnAddBg.setOutlineThickness(0.f);

    string btnAddStr = "Thêm phim";
    btnAddText = make_unique<Text>(font);
    btnAddText->setString(String::fromUtf8(btnAddStr.begin(), btnAddStr.end()));
    btnAddText->setCharacterSize(16);
    btnAddText->setFillColor(Color::White);

    // Edit button
    btnEditBg.setSize(Vector2f(btnW, btnH));
    btnEditBg.setFillColor(Color(233,164,0)); // #E9A400
    btnEditBg.setOutlineThickness(0.f);

    string btnEditStr = "Sửa thông tin";
    btnEditText = make_unique<Text>(font);
    btnEditText->setString(String::fromUtf8(btnEditStr.begin(), btnEditStr.end()));
    btnEditText->setCharacterSize(16);
    btnEditText->setFillColor(Color::White);

    // Delete button
    btnDeleteBg.setSize(Vector2f(btnW, btnH));
    btnDeleteBg.setFillColor(Color(211,47,47)); // #D32F2F
    btnDeleteBg.setOutlineThickness(0.f);

    string btnDeleteStr = "Xóa phim";
    btnDeleteText = make_unique<Text>(font);
    btnDeleteText->setString(String::fromUtf8(btnDeleteStr.begin(), btnDeleteStr.end()));
    btnDeleteText->setCharacterSize(16);
    btnDeleteText->setFillColor(Color::White);

    reloadButtonBg.setSize(Vector2f(btnH, btnH));
    reloadButtonBg.setFillColor(Color(20,118,172));
    reloadButtonBg.setOutlineThickness(0.f);

    reloadTexture.setSmooth(true);
    reloadSprite.setScale({0.1, 0.1});
    reloadSprite.setColor(Color::White);
    
    // Notification setup
    notificationBg.setSize(Vector2f(400, 60));
    notificationBg.setFillColor(Color(211, 47, 47, 230));
    notificationTextObj = make_unique<Text>(font);
    notificationTextObj->setCharacterSize(18);
    notificationTextObj->setFillColor(Color::White);
}

void MoviePanel::setPosition(Vector2f pos) {
    position = pos;
    background.setPosition(pos);
    
    if (titleText) titleText->setPosition(Vector2f(pos.x + 40, pos.y + 20));
    
    tableHeaderBg.setPosition(Vector2f(pos.x + TABLE_X, pos.y + TABLE_Y));

    // Align CRUD buttons in a single row above the table
    const float buttonRowY = TABLE_Y - 64.f;
    const float spacing = 18.f;
    const float btnHeight = btnAddBg.getSize().y;

    btnAddBg.setPosition(Vector2f(pos.x + TABLE_X, pos.y + buttonRowY));
    btnEditBg.setPosition(Vector2f(btnAddBg.getPosition().x + btnAddBg.getSize().x + spacing, pos.y + buttonRowY));
    btnDeleteBg.setPosition(Vector2f(btnEditBg.getPosition().x + btnEditBg.getSize().x + spacing, pos.y + buttonRowY));
    float reloadX = pos.x + TABLE_X + TABLE_WIDTH - reloadButtonBg.getSize().x;
    float reloadY = pos.y + buttonRowY;

    reloadButtonBg.setPosition({reloadX, reloadY});

    FloatRect spriteBounds = reloadSprite.getGlobalBounds();
    reloadSprite.setPosition({reloadX + (reloadButtonBg.getSize().x - spriteBounds.size.x) / 2.f, reloadY + (btnHeight - spriteBounds.size.y) / 2.f});
}

void MoviePanel::openAddPopup() {
    currentPopup = ADD;
    
    // Popup overlay (full dark like RegisterScreen)
    popupOverlay.setSize(Vector2f(1728, 972));
    popupOverlay.setPosition(Vector2f(0, 0));
    popupOverlay.setFillColor(Color(0, 0, 0, 170));
    
    // Popup background - WIDER for 2 columns, shorter height
    const float popupW = 940.f;  // Wide enough for 2 columns
    const float popupH = 720.f;  // Taller to avoid overlapping controls
    const float popupX = (1728.f - popupW) / 2.f;
    const float popupY = (972.f - popupH) / 2.f;
    
    popupBackground.setSize(Vector2f(popupW, popupH));
    popupBackground.setPosition(Vector2f(popupX, popupY));
    popupBackground.setFillColor(Color(255, 255, 255, 240)); // Semi-transparent white
    popupBackground.setOutlineThickness(0);
    
    // Popup title (RegisterScreen style - bold, black, 26px)
    string popupTitleStr = "Thêm phim mới";
    popupTitle = make_unique<Text>(font, String::fromUtf8(popupTitleStr.begin(), popupTitleStr.end()), 26);
    popupTitle->setFillColor(Color::Black);
    popupTitle->setPosition(Vector2f(popupX + (popupW - 200) / 2, popupY + 20));
    
    // TWO COLUMNS LAYOUT
    // Left column: 6 fields (0-5), Right column: 5 fields (6-10) + dropdown
    const float leftColX = popupX + 40;
    const float rightColX = popupX + popupW / 2 + 20;
    const float inputW = 420.f;  // Increased from 400 to 420 for better fit
    const float inputH = 40.f;
    const float inputStartY = popupY + 100;
    const float inputSpacing = 76.f;
    
    // Fields definition (12 TextBox fields)
    vector<pair<string, string>> fields = {
        // Left column (0-5)
        {"Tên phim*", "Nhập tên phim"},
        {"Phân loại tuổi*", "T13, T16, T18, PG"},
        {"Quốc gia*", "Việt Nam, USA..."},
        {"Ngôn ngữ*", "Tiếng Việt, English"},
        {"Thể loại*", "Hành động, Tâm lý"},
        {"Thời lượng (phút)*", "120"},
        // Right column (6-11)
        {"Ngày khởi chiếu*", "dd/mm/yyyy"},
        {"Ngày ngừng chiếu", "dd/mm/yyyy (để trống = +1 tháng)"},
        {"Đạo diễn*", "Tên đạo diễn"},
        {"Diễn viên*", "Danh sách diễn viên"},
        {"Tóm tắt*", "Mô tả phim"},
        {"Poster", "../assets/posters/..."}
    };
    
    inputBoxes.clear();
    for (size_t i = 0; i < fields.size(); i++) {
        float x = (i < 6) ? leftColX : rightColX;  // Left or right column
        float y = inputStartY + (i % 6) * inputSpacing;
        
        auto box = make_unique<TextBox>(font, fields[i].first, x, y, inputW, inputH);
        box->setPlaceholder(fields[i].second);
        inputBoxes.push_back(move(box));
    }

    
    // Popup buttons (RegisterScreen style: blue "Lưu", gray "Quay lại")
    const float btnW = 190.f;
    const float btnH = 48.f;
    const float btnY = popupY + popupH - btnH - 32.f;
    const float btnSpacing = 24.f;
    const float btnTotalW = btnW * 2 + btnSpacing;
    const float btnStartX = popupX + (popupW - btnTotalW) / 2.f;
    
    string btnSaveStr = "Lưu";
    string btnCancelStr = "Quay lại";
    btnPopupSave = make_unique<Button>(font, String::fromUtf8(btnSaveStr.begin(), btnSaveStr.end()), btnW, btnH, 18);
    btnPopupSave->setPosition(Vector2f(btnStartX, btnY));
    btnPopupSave->setFillColor(Color(100, 149, 237)); // Blue like RegisterScreen
    btnPopupSave->setTextColor(Color::White);
    
    btnPopupCancel = make_unique<Button>(font, String::fromUtf8(btnCancelStr.begin(), btnCancelStr.end()), btnW, btnH, 18);
    btnPopupCancel->setPosition(Vector2f(btnStartX + btnW + btnSpacing, btnY));
    btnPopupCancel->setFillColor(Color(90, 90, 90)); // Gray like RegisterScreen
    btnPopupCancel->setTextColor(Color::White);
}

void MoviePanel::openEditPopup() {
    if (selectedRow < 0) {
        showNotification("Vui lòng chọn phim cần sửa");
        return;
    }
    
    currentPopup = EDIT;
    
    // Same layout as Add popup (2-column)
    popupOverlay.setSize(Vector2f(1728, 972));
    popupOverlay.setPosition(Vector2f(0, 0));
    popupOverlay.setFillColor(Color(0, 0, 0, 170));
    
    const float popupW = 940.f;
    const float popupH = 720.f;
    const float popupX = (1728.f - popupW) / 2.f;
    const float popupY = (972.f - popupH) / 2.f;
    
    popupBackground.setSize(Vector2f(popupW, popupH));
    popupBackground.setPosition(Vector2f(popupX, popupY));
    popupBackground.setFillColor(Color(255, 255, 255, 240));
    popupBackground.setOutlineThickness(0);
    
    string editTitleStr = "Sửa thông tin phim";
    popupTitle = make_unique<Text>(font, String::fromUtf8(editTitleStr.begin(), editTitleStr.end()), 26);
    popupTitle->setFillColor(Color::Black);
    popupTitle->setPosition(Vector2f(popupX + (popupW - 240) / 2, popupY + 20));
    
    // Get current record data
    vector<string> record = repository->getRecord(selectedRow);
    
    // TWO COLUMNS LAYOUT (same as Add)
    const float leftColX = popupX + 40;
    const float rightColX = popupX + popupW / 2 + 20;
    const float inputW = 420.f;
    const float inputH = 40.f;
    const float inputStartY = popupY + 100;
    const float inputSpacing = 76.f;
    
    vector<pair<string, string>> fields = {
        {"Tên phim*", "Nhập tên phim"},
        {"Phân loại tuổi*", "T13, T16, T18, PG"},
        {"Quốc gia*", "Việt Nam, USA..."},
        {"Ngôn ngữ*", "Tiếng Việt, English"},
        {"Thể loại*", "Hành động, Tâm lý"},
        {"Thời lượng (phút)*", "120"},
        {"Ngày khởi chiếu*", "dd/mm/yyyy"},
        {"Ngày ngừng chiếu", "dd/mm/yyyy"},
        {"Đạo diễn*", "Tên đạo diễn"},
        {"Diễn viên*", "Danh sách diễn viên"},
        {"Tóm tắt*", "Mô tả phim"},
        {"Poster", "../assets/posters/..."}
    };
    
    inputBoxes.clear();
    for (size_t i = 0; i < fields.size(); i++) {
        float x = (i < 6) ? leftColX : rightColX;
        float y = inputStartY + (i % 6) * inputSpacing;
        
        auto box = make_unique<TextBox>(font, fields[i].first, x, y, inputW, inputH);
        box->setPlaceholder(fields[i].second);
        inputBoxes.push_back(move(box));
    }
    
    // Pre-fill values from selected record
    if (record.size() >= 12) {
        inputBoxes[0]->setValue(record[1]);   // title
        inputBoxes[1]->setValue(record[2]);   // age_rating
        inputBoxes[2]->setValue(record[3]);   // country
        inputBoxes[3]->setValue(record[4]);   // language
        inputBoxes[4]->setValue(record[5]);   // genres
        inputBoxes[5]->setValue(record[6]);   // duration_min
        inputBoxes[6]->setValue(record[7]);   // release_date (start_date)
        inputBoxes[7]->setValue(record.size() >= 9 ? record[8] : "");    // end_date
        inputBoxes[8]->setValue(record.size() >= 10 ? record[9] : "");   // director
        inputBoxes[9]->setValue(record.size() >= 11 ? record[10] : "");  // cast
        inputBoxes[10]->setValue(record.size() >= 12 ? record[11] : ""); // synopsis
        inputBoxes[11]->setValue(record.size() >= 13 ? record[12] : ""); // poster_path
    }
    
    // Buttons (same as Add popup)
    const float btnW = 190.f;
    const float btnH = 48.f;
    const float btnY = popupY + popupH - btnH - 32.f;
    const float btnSpacing = 24.f;
    const float btnTotalW = btnW * 2 + btnSpacing;
    const float btnStartX = popupX + (popupW - btnTotalW) / 2.f;
    
    string btnSaveStr = "Lưu";
    string btnCancelStr = "Quay lại";
    btnPopupSave = make_unique<Button>(font, String::fromUtf8(btnSaveStr.begin(), btnSaveStr.end()), btnW, btnH, 18);
    btnPopupSave->setPosition(Vector2f(btnStartX, btnY));
    btnPopupSave->setFillColor(Color(100, 149, 237));
    btnPopupSave->setTextColor(Color::White);
    
    btnPopupCancel = make_unique<Button>(font, String::fromUtf8(btnCancelStr.begin(), btnCancelStr.end()), btnW, btnH, 18);
    btnPopupCancel->setPosition(Vector2f(btnStartX + btnW + btnSpacing, btnY));
    btnPopupCancel->setFillColor(Color(90, 90, 90));
    btnPopupCancel->setTextColor(Color::White);
}

void MoviePanel::openDeleteConfirm() {
    if (selectedRow < 0) {
        showNotification("Vui lòng chọn phim cần xóa");
        return;
    }
    
    currentPopup = DELETE_CONFIRM;
    
    popupOverlay.setSize(Vector2f(1728, 972));
    popupOverlay.setPosition(Vector2f(0, 0));
    popupOverlay.setFillColor(Color(0, 0, 0, 128));
    
    // Delete confirmation popup (compact: 380x160)
    const float popupW = 380.f;
    const float popupH = 160.f;
    const float popupX = (1728.f - popupW) / 2.f;
    const float popupY = (972.f - popupH) / 2.f;
    
    popupBackground.setSize(Vector2f(popupW, popupH));
    popupBackground.setPosition(Vector2f(popupX, popupY));
    popupBackground.setFillColor(Color::White);
    popupBackground.setOutlineThickness(1);
    popupBackground.setOutlineColor(borderColor);
    
    string deleteTitleStr = "Xác nhận xóa phim này?";
    popupTitle = make_unique<Text>(font, String::fromUtf8(deleteTitleStr.begin(), deleteTitleStr.end()), 16);
    popupTitle->setFillColor(textColor);
    
    // Center title
    FloatRect titleBounds = popupTitle->getLocalBounds();
    popupTitle->setPosition(Vector2f(
        popupX + (popupW - titleBounds.size.x) / 2.f,
        popupY + 30
    ));
    
    // Buttons (centered, compact)
    const float btnW = 100.f;
    const float btnH = 36.f;
    const float btnY = popupY + popupH - btnH - 20;
    const float btnSpacing = 20.f;
    const float btnTotalW = btnW * 2 + btnSpacing;
    const float btnStartX = popupX + (popupW - btnTotalW) / 2.f;
    
    string btnDeleteStr = "Xóa";
    string btnCancelStr3 = "Hủy";
    btnPopupSave = make_unique<Button>(font, String::fromUtf8(btnDeleteStr.begin(), btnDeleteStr.end()), btnW, btnH, 16);
    btnPopupSave->setPosition(Vector2f(btnStartX, btnY));
    btnPopupSave->setFillColor(Color(211, 47, 47)); // Red for delete
    btnPopupSave->setTextColor(Color::White);
    
    btnPopupCancel = make_unique<Button>(font, String::fromUtf8(btnCancelStr3.begin(), btnCancelStr3.end()), btnW, btnH, 16);
    btnPopupCancel->setPosition(Vector2f(btnStartX + btnW + btnSpacing, btnY));
    btnPopupCancel->setFillColor(Color(160, 160, 160)); // Gray for cancel
    btnPopupCancel->setTextColor(Color::White);
}

void MoviePanel::closePopup() {
    currentPopup = NONE;
    inputBoxes.clear();
    btnPopupSave.reset();
    btnPopupCancel.reset();
}

void MoviePanel::handleAdd() {
    if (inputBoxes.getSize() < 12) return;
    
    // Get and trim all field values from TextBoxes
    string title = trimCopy(inputBoxes[0]->getValue());
    string ageRating = trimCopy(inputBoxes[1]->getValue());
    string country = trimCopy(inputBoxes[2]->getValue());
    string language = trimCopy(inputBoxes[3]->getValue());
    string genres = trimCopy(inputBoxes[4]->getValue());
    string duration = trimCopy(inputBoxes[5]->getValue());
    string releaseDate = trimCopy(inputBoxes[6]->getValue());
    string endDateInput = trimCopy(inputBoxes[7]->getValue());
    string director = trimCopy(inputBoxes[8]->getValue());
    string cast = trimCopy(inputBoxes[9]->getValue());
    string synopsis = trimCopy(inputBoxes[10]->getValue());
    string posterPath = trimCopy(inputBoxes[11]->getValue());

    auto requireField = [&](const string& value, const string& label) -> bool {
        if (value.empty()) {
            showNotification("Vui lòng nhập " + label + "!");
            return false;
        }
        return true;
    };

    if (!requireField(title, "tên phim")) return;
    if (!requireField(ageRating, "phân loại tuổi")) return;
    ageRating = toUpperAscii(ageRating);
    if (!isAllowedAgeRating(ageRating)) {
        showNotification("Phân loại tuổi chỉ chấp nhận T13, T16, T18 hoặc PG.");
        return;
    }
    if (!requireField(country, "quốc gia")) return;
    if (!requireField(language, "ngôn ngữ")) return;
    if (!requireField(genres, "thể loại")) return;
    if (!requireField(duration, "thời lượng")) return;

    int durationMinutes = 0;
    if (!tryParsePositiveDuration(duration, durationMinutes)) {
        showNotification("Thời lượng phải nằm trong khoảng 30-400 phút.");
        return;
    }
    duration = std::to_string(durationMinutes);

    if (!requireField(releaseDate, "ngày khởi chiếu")) return;
    if (!requireField(director, "đạo diễn")) return;
    if (!requireField(cast, "diễn viên")) return;
    if (!requireField(synopsis, "tóm tắt")) return;

    const string todayIso = todayIsoYyyyMmDd();
    const string minStartIso = addDaysIsoYyyyMmDd(todayIso, 5);
    const string minStartDd = isoToDdMmYyyy(minStartIso);
    const string releaseIso = ddMmYyyyToIsoYyyyMmDd(releaseDate);
    if (releaseIso.empty()) {
        showNotification("Ngày khởi chiếu phải theo định dạng dd/mm/yyyy.");
        return;
    }
    if (!minStartIso.empty() && releaseIso < minStartIso) {
        showNotification("Ngày khởi chiếu phải từ " + (minStartDd.empty() ? "sau 5 ngày" : minStartDd) + " trở đi.");
        return;
    }

    string endDate = endDateInput.empty() ? addOneMonthDdMmYyyy(releaseDate) : endDateInput;
    endDate = trimCopy(endDate);
    const string endIso = ddMmYyyyToIsoYyyyMmDd(endDate);
    if (endDate.empty() || endIso.empty()) {
        showNotification("Ngày ngừng chiếu phải theo định dạng dd/mm/yyyy.");
        return;
    }
    if (endIso <= releaseIso) {
        showNotification("Ngày ngừng chiếu phải sau ngày khởi chiếu ít nhất 1 ngày.");
        return;
    }

    string status = computeStatus(releaseDate, endDate);
    if (posterPath.empty()) {
        posterPath = "../assets/posters/default.png";
    }

    // Create full record with all fields
    vector<string> record = {
        "",             // 0. ID (auto-generated)
        title,           // 1. title
        ageRating,       // 2. age_rating
        country,         // 3. country
        language,        // 4. language
        genres,          // 5. genres
        duration,        // 6. duration_min
        releaseDate,     // 7. release_date (start_date)
        endDate,         // 8. end_date
        director,        // 9. director
        cast,            // 10. cast
        synopsis,        // 11. synopsis
        posterPath,      // 12. poster_path
        status           // 13. status
    };
    
    repository->addRecord(record);
    repository->saveToFile();
    repository->loadFromFile();

    // Refresh showtimes immediately so Admin can see the schedule (today..today+5).
    ShowtimeCleanupService::maintainShowtimes(FileUtils::resolveDataPath("data/showtimes.txt"), 6);
    
    closePopup();
    showNotification("Đã thêm phim mới thành công!");
}

void MoviePanel::handleEdit() {
    if (selectedRow < 0 || inputBoxes.getSize() < 12) return;
    
    // Get all 11 field values from TextBoxes
    string title = inputBoxes[0]->getValue();
    string ageRating = inputBoxes[1]->getValue();
    string country = inputBoxes[2]->getValue();
    string language = inputBoxes[3]->getValue();
    string genres = inputBoxes[4]->getValue();
    string duration = inputBoxes[5]->getValue();
    string releaseDate = inputBoxes[6]->getValue();
    string endDateInput = inputBoxes[7]->getValue();
    string director = inputBoxes[8]->getValue();
    string cast = inputBoxes[9]->getValue();
    string synopsis = inputBoxes[10]->getValue();
    string posterPath = inputBoxes[11]->getValue();
    string endDate = endDateInput.empty() ? addOneMonthDdMmYyyy(releaseDate) : endDateInput;
    string status = computeStatus(releaseDate, endDate);
    
    // Get existing record
    vector<string> record = repository->getRecord(selectedRow);
    if (record.empty()) return;

    if (record.size() < 14) record.resize(14);

    const bool releaseDateChanged = releaseDate != record[7];
    const bool endDateChanged = endDate != record[8];

    // For consistency with scheduling rules: only re-validate release_date if Admin actually changed it.
    if (releaseDateChanged) {
        const string todayIso = todayIsoYyyyMmDd();
        const string minStartIso = addDaysIsoYyyyMmDd(todayIso, 5);
        const string minStartDd = isoToDdMmYyyy(minStartIso);
        const string releaseIso = ddMmYyyyToIsoYyyyMmDd(releaseDate);
        if (releaseIso.empty() || minStartIso.empty() || releaseIso < minStartIso) {
            showNotification("Ngày khởi chiếu phải từ " + (minStartDd.empty() ? "sau 5 ngày" : minStartDd) + " trở đi.");
            return;
        }
    }
    
    // Constraint: end_date is the STOP date (exclusive). Booking shows 5 days: today..today+4.
    // Only evaluate when Admin changes the stop date.
    if (endDateChanged) {
        const string todayIso = todayIsoYyyyMmDd();
        const string minStopIso = addDaysIsoYyyyMmDd(todayIso, 5);
        const string newEndIso = ddMmYyyyToIsoYyyyMmDd(endDate);
        if (!todayIso.empty() && !minStopIso.empty() && !newEndIso.empty()) {
            if (newEndIso < minStopIso) {
                const string minStopDd = isoToDdMmYyyy(minStopIso);
                showNotification("Chỉ được ngừng chiếu từ " + (minStopDd.empty() ? "ngày sau 5 ngày" : minStopDd) + " trở đi.");
                return;
            }
        }
    }

    // Also ensure we do not stop on/before a day that already has BOOKED showtimes.
    if (endDateChanged) {
        const string movieId = record[0];
        const string newEndIso = ddMmYyyyToIsoYyyyMmDd(endDate);
        string lastBookedIso;
        if (!movieId.empty() && !newEndIso.empty() && tryGetLastBookedShowDateIsoForMovie(movieId, lastBookedIso)) {
            if (!lastBookedIso.empty() && lastBookedIso >= newEndIso) {
                const string minDd = isoToDdMmYyyy(addDaysIsoYyyyMmDd(lastBookedIso, 1));
                showNotification("Không thể ngừng chiếu vì đã có vé. Chỉ được đặt từ " + (minDd.empty() ? "ngày sau" : minDd) + " trở đi.");
                return;
            }
        }
    }

    // Update all fields (keep ID at index 0)
    record[1] = title;
    record[2] = ageRating;
    record[3] = country;
    record[4] = language;
    record[5] = genres;
    record[6] = duration;
    record[7] = releaseDate;
    record[8] = endDate;
    record[9] = director;
    record[10] = cast;
    record[11] = synopsis;
    record[12] = posterPath;
    record[13] = status;
    
    repository->updateRecord(selectedRow, record);
    repository->saveToFile();
    repository->loadFromFile();

    // Refresh showtimes immediately so Admin can verify day+5 (6th day) correctness.
    ShowtimeCleanupService::maintainShowtimes(FileUtils::resolveDataPath("data/showtimes.txt"), 6);
    
    closePopup();
    showNotification("Đã cập nhật thành công!");
    selectedRow = -1;
}

void MoviePanel::handleDelete() {
    if (selectedRow < 0) return;

    vector<string> record = repository->getRecord(selectedRow);
    if (record.empty()) return;
    if (record.size() < 14) record.resize(14);

    {
        string scheduledStopDdMmYyyy;
        if (tryGetScheduledStopDateIfAlreadyDeleted(record, scheduledStopDdMmYyyy)) {
            closePopup();
            if (!scheduledStopDdMmYyyy.empty()) {
                showNotification("Phim sẽ được xóa vào ngày " + scheduledStopDdMmYyyy);
            } else {
                showNotification("Phim sẽ được xóa vào ngày đã đặt trước đó");
            }
            return;
        }
    }

    const string movieId = record[0];
    const string todayIso = todayIsoYyyyMmDd();

    // Do not remove the record immediately.
    // Instead, mark it as stopped so Admin can distinguish movies that were "deleted".
    // - If it still has future showtimes, stop at the last scheduled show date.
    // - Otherwise stop at today.
    // end_date is stop date (exclusive). Choose the earliest valid stop date:
    // - At least today+5 (Booking window is 5 days)
    // - After the last BOOKED showtime day (so we never stop on a day that still has booked showtimes)
    string stopIso = addDaysIsoYyyyMmDd(todayIso, 5);

    string lastBookedIso;
    if (tryGetLastBookedShowDateIsoForMovie(movieId, lastBookedIso)) {
        string afterLastBooked = addDaysIsoYyyyMmDd(lastBookedIso, 1);
        if (!afterLastBooked.empty() && afterLastBooked > stopIso) stopIso = afterLastBooked;
    }

    string stopDdMmYyyy = isoToDdMmYyyy(stopIso);
    if (stopDdMmYyyy.empty()) stopDdMmYyyy = addDaysIsoYyyyMmDd(todayIso, 5).empty() ? todayDdMmYyyy() : isoToDdMmYyyy(addDaysIsoYyyyMmDd(todayIso, 5));

    record[8] = stopDdMmYyyy;
    record[13] = "Ngừng chiếu vào " + stopDdMmYyyy;

    repository->updateRecord(selectedRow, record);
    repository->saveToFile();
    repository->loadFromFile();

    // Refresh showtimes immediately (6 days) so day+5 (e.g. 28) is regenerated for remaining movies.
    ShowtimeCleanupService::maintainShowtimes(FileUtils::resolveDataPath("data/showtimes.txt"), 6);

    closePopup();
    showNotification("Đã ngừng chiếu vào " + stopDdMmYyyy);
    selectedRow = -1;
}

void MoviePanel::handleReload() {
    repository->loadFromFile();
    selectedRow = -1;
    showNotification("Đã tải lại dữ liệu");
}

void MoviePanel::showNotification(const string& message) {
    notificationText = message;
    notificationVisible = !message.empty();
    if (notificationVisible) {
        notificationClock.restart();
    }
}

void MoviePanel::showSelectionWarning(const string& message) {
    selectionWarningVisible = true;
    selectionWarningMessage = message;
    selectionWarningOverlay.setSize(Vector2f(1728.f, 972.f));
    selectionWarningOverlay.setPosition(Vector2f(0.f, 0.f));
    selectionWarningOverlay.setFillColor(Color(0, 0, 0, 150));

    selectionWarningBg.setSize(Vector2f(420.f, 200.f));
    selectionWarningBg.setFillColor(Color::White);
    selectionWarningBg.setOutlineThickness(0.f);
    selectionWarningBg.setPosition(Vector2f((1728.f - 420.f) / 2.f, (972.f - 200.f) / 2.f));

    selectionWarningText = make_unique<Text>(font, String::fromUtf8(message.begin(), message.end()), 20);
    selectionWarningText->setFillColor(Color(27, 38, 59));
    selectionWarningText->setStyle(Text::Bold);
    FloatRect textBounds = selectionWarningText->getLocalBounds();
    selectionWarningText->setPosition(Vector2f(
        selectionWarningBg.getPosition().x + (selectionWarningBg.getSize().x - textBounds.size.x) / 2.f - textBounds.position.x,
        selectionWarningBg.getPosition().y + 40.f
    ));

    selectionWarningButton.setSize(Vector2f(160.f, 46.f));
    selectionWarningButton.setFillColor(Color(20, 118, 172));
    selectionWarningButton.setPosition(Vector2f(
        selectionWarningBg.getPosition().x + (selectionWarningBg.getSize().x - selectionWarningButton.getSize().x) / 2.f,
        selectionWarningBg.getPosition().y + selectionWarningBg.getSize().y - 70.f
    ));

    selectionWarningButtonText = make_unique<Text>(font, L"Đã hiểu", 18);
    selectionWarningButtonText->setFillColor(Color::White);
    FloatRect btnBounds = selectionWarningButtonText->getLocalBounds();
    selectionWarningButtonText->setPosition(Vector2f(
        selectionWarningButton.getPosition().x + (selectionWarningButton.getSize().x - btnBounds.size.x) / 2.f - btnBounds.position.x,
        selectionWarningButton.getPosition().y + (selectionWarningButton.getSize().y - btnBounds.size.y) / 2.f - btnBounds.position.y
    ));
    selectionWarningButtonHover = false;
}

void MoviePanel::handleSelectionWarningEvent(const Event& event, const RenderWindow& window) {
    if (!selectionWarningVisible) return;
    if (const auto* mouseEvent = event.getIf<Event::MouseButtonPressed>()) {
        if (mouseEvent->button == Mouse::Button::Left) {
            Vector2f mousePos(mouseEvent->position.x, mouseEvent->position.y);
            if (selectionWarningButton.getGlobalBounds().contains(mousePos)) {
                selectionWarningVisible = false;
            }
        }
    }
    (void)window;
}

void MoviePanel::updateSelectionWarning(Vector2f mousePos, bool /*mousePressed*/) {
    if (!selectionWarningVisible) return;
    selectionWarningButtonHover = selectionWarningButton.getGlobalBounds().contains(mousePos);
    selectionWarningButton.setFillColor(selectionWarningButtonHover ? Color(17, 98, 144) : Color(20, 118, 172));
}

void MoviePanel::renderSelectionWarning(RenderWindow& window) {
    if (!selectionWarningVisible) return;
    window.draw(selectionWarningOverlay);
    window.draw(selectionWarningBg);
    if (selectionWarningText) {
        window.draw(*selectionWarningText);
    }
    window.draw(selectionWarningButton);
    if (selectionWarningButtonText) {
        window.draw(*selectionWarningButtonText);
    }
}

void MoviePanel::handleEvent(const Event& event, const RenderWindow& window) {
    if (selectionWarningVisible) {
        handleSelectionWarningEvent(event, window);
        return;
    }
    // Change cursor on hover over CRUD buttons
    if (const auto* moveEvent = event.getIf<Event::MouseMoved>()) {
        // We intentionally do not change OS cursor (compatibility across platforms/SFML versions).
        // Hover visual is handled in update() by brightening the button colors.
        (void)moveEvent;
    }
    // Handle popup events
    if (currentPopup != NONE && currentPopup != DELETE_CONFIRM) {
        if (const auto* keyEvent = event.getIf<Event::KeyPressed>()) {
            if (keyEvent->code == Keyboard::Key::Tab) {
                int direction = keyEvent->shift ? -1 : 1;
                int focusedIndex = -1;
                for (int i = 0; i < inputBoxes.getSize(); ++i) {
                    if (inputBoxes[i] && inputBoxes[i]->getFocus()) {
                        focusedIndex = i;
                        break;
                    }
                }
                if (focusedIndex < 0) {
                    for (int i = 0; i < inputBoxes.getSize(); ++i) {
                        if (inputBoxes[i]) {
                            inputBoxes[i]->setFocus(i == 0);
                        }
                    }
                } else {
                    int nextIndex = focusedIndex;
                    int total = inputBoxes.getSize();
                    for (int attempt = 0; attempt < total; ++attempt) {
                        nextIndex = (nextIndex + direction + total) % total;
                        if (inputBoxes[nextIndex]) break;
                    }
                    if (inputBoxes[focusedIndex]) inputBoxes[focusedIndex]->setFocus(false);
                    if (inputBoxes[nextIndex]) inputBoxes[nextIndex]->setFocus(true);
                }
                return;
            }
        }

        // Handle input boxes
        for (int i = 0; i < inputBoxes.getSize(); ++i) {
            if (inputBoxes[i]) {
                inputBoxes[i]->handleEvent(event);
            }
        }

    }
    
    // Handle mouse clicks
    if (const auto* mouseEvent = event.getIf<Event::MouseButtonPressed>()) {
        if (mouseEvent->button == Mouse::Button::Left) {
            Vector2f mousePos(mouseEvent->position.x, mouseEvent->position.y);
            
            // Check if click is on table row
            if (currentPopup == NONE) {
                float startY = position.y + TABLE_Y + HEADER_HEIGHT;
                int rowCount = repository->getRecordCount();
                
                for (int i = 0; i < rowCount; i++) {
                    float rowY = startY + i * ROW_HEIGHT;
                    FloatRect rowBounds({position.x + TABLE_X, rowY}, {TABLE_WIDTH, ROW_HEIGHT});
                    
                    if (rowBounds.contains(mousePos)) {
                        selectedRow = i;
                        break;
                    }
                }
            }
        }
    }
}

void MoviePanel::update(Vector2f mousePos, bool mousePressed) {
    if (selectionWarningVisible) {
        updateSelectionWarning(mousePos, mousePressed);
        return;
    }
    // Update hover state for rows
    if (currentPopup == NONE) {
        float startY = position.y + TABLE_Y + HEADER_HEIGHT;
        int rowCount = repository->getRecordCount();
        hoveredRow = -1;
        
        for (int i = 0; i < rowCount; i++) {
            float rowY = startY + i * ROW_HEIGHT;
            FloatRect rowBounds({position.x + TABLE_X, rowY}, {TABLE_WIDTH, ROW_HEIGHT});
            
            if (rowBounds.contains(mousePos)) {
                hoveredRow = i;
                break;
            }
        }
    }
    
    // Update buttons
    if (currentPopup == NONE) {
        // Handle custom add/edit/delete/reload buttons hover and clicks
        FloatRect addBounds = btnAddBg.getGlobalBounds();
        FloatRect editBounds = btnEditBg.getGlobalBounds();
        FloatRect delBounds = btnDeleteBg.getGlobalBounds();
        FloatRect reloadBounds = reloadButtonBg.getGlobalBounds();

        btnAddHover = addBounds.contains(mousePos);
        btnEditHover = editBounds.contains(mousePos);
        btnDeleteHover = delBounds.contains(mousePos);
        btnReloadHover = reloadBounds.contains(mousePos);

        // Hover color tweak (10% brighter)
        auto brighten = [](const Color& c, float pct) {
            auto clamp = [](int v){ return (v<0?0:(v>255?255:v)); };
            int r = clamp((int)(c.r + (255 - c.r) * pct));
            int g = clamp((int)(c.g + (255 - c.g) * pct));
            int b = clamp((int)(c.b + (255 - c.b) * pct));
            return Color(r,g,b);
        };

        // Base colors
        Color addBase(40, 167, 69);
        Color editBase(233, 164, 0);
        Color delBase(211, 47, 47);
        Color reloadBase(20, 118, 172);

        // Apply hover/press visuals
        if (btnAddHover) btnAddBg.setFillColor(brighten(addBase, 0.10f)); else btnAddBg.setFillColor(addBase);
        if (btnEditHover) btnEditBg.setFillColor(brighten(editBase, 0.10f)); else btnEditBg.setFillColor(editBase);
        if (btnDeleteHover) btnDeleteBg.setFillColor(brighten(delBase, 0.10f)); else btnDeleteBg.setFillColor(delBase);
        reloadButtonBg.setFillColor(btnReloadHover ? brighten(reloadBase, 0.10f) : reloadBase);
        reloadSprite.setColor(btnReloadHover ? Color(180, 220, 255) : Color::White);

        if (mousePressed) {
            if (btnAddHover) {
                btnAddPressed = true;
            } else if (btnEditHover) {
                btnEditPressed = true;
            } else if (btnDeleteHover) {
                btnDeletePressed = true;
            } else if (btnReloadHover) {
                btnReloadPressed = true;
            }
        } else {
            // on mouse release trigger action if it was pressed
            if (btnAddPressed && btnAddHover) openAddPopup();
            if (btnEditPressed && btnEditHover) openEditPopup();
            if (btnDeletePressed && btnDeleteHover) openDeleteConfirm();
            if (btnReloadPressed && btnReloadHover) handleReload();
            btnAddPressed = btnEditPressed = btnDeletePressed = btnReloadPressed = false;
        }
    }
    
    // Update popup
    if (currentPopup != NONE) {
        // Update input boxes
        for (int i = 0; i < inputBoxes.getSize(); ++i) {
            if (inputBoxes[i]) {
                inputBoxes[i]->update(mousePos, mousePressed);
            }
        }

        
        // Update popup buttons
        if (btnPopupSave && btnPopupCancel) {
            Color btnSave(20, 118, 172);
            Color btnSaveHover(30, 138, 192);
            Color btnCancel(211, 47, 47);
            Color btnCancelHover(231, 67, 67);
            
            if (currentPopup == DELETE_CONFIRM) {
                btnCancel = Color(160, 160, 160);
                btnCancelHover = Color(180, 180, 180);
            }
            
            btnPopupSave->update(mousePos, mousePressed, btnSave, btnSaveHover);
            btnPopupCancel->update(mousePos, mousePressed, btnCancel, btnCancelHover);
            
            if (mousePressed) {
                if (btnPopupSave->isClicked(mousePos, true)) {
                    if (currentPopup == ADD) {
                        handleAdd();
                    } else if (currentPopup == EDIT) {
                        handleEdit();
                    } else if (currentPopup == DELETE_CONFIRM) {
                        handleDelete();
                    }
                } else if (btnPopupCancel->isClicked(mousePos, true)) {
                    closePopup();
                }
            }
        }
    }
}

void MoviePanel::renderTable(RenderWindow& window) {
    window.draw(tableHeaderBg);

    struct ColumnSpec {
        string label;
        float ratio; // ratio of TABLE_WIDTH
    };

    // Make status column wider to display e.g. "Ngừng chiếu vào ngày DD/MM/YYYY".
    static const array<ColumnSpec, 5> columns = {{{"ID Phim", 0.10f},
        {"Tên phim", 0.32f}, {"Thời lượng", 0.14f}, {"Ngày chiếu", 0.16f}, {"Trạng thái", 0.28f}}};

    vector<float> columnWidths;
    columnWidths.reserve(columns.size());
    for (const auto& col : columns) {
        columnWidths.push_back(TABLE_WIDTH * col.ratio);
    }

    vector<float> columnLefts(columns.size(), position.x + TABLE_X);
    for (size_t i = 1; i < columns.size(); ++i) {
        columnLefts[i] = columnLefts[i - 1] + columnWidths[i - 1];
    }

    float headerTop = position.y + TABLE_Y;
    float tableLeft = position.x + TABLE_X;
    auto allData = repository->getAllData();
    float totalHeight = HEADER_HEIGHT + static_cast<float>(allData.size()) * ROW_HEIGHT;

    // Draw vertical separators (including left and right borders)
    RectangleShape separator(Vector2f(1.f, totalHeight));
    separator.setFillColor(borderColor);
    separator.setPosition(Vector2f(tableLeft, headerTop));
    window.draw(separator);
    for (size_t i = 0; i < columns.size(); ++i) {
        float x = columnLefts[i] + columnWidths[i];
        separator.setPosition(Vector2f(x, headerTop));
        window.draw(separator);
    }

    // Header text centered
    for (size_t i = 0; i < columns.size(); ++i) {
        Text headerText(font, String::fromUtf8(columns[i].label.begin(), columns[i].label.end()), 18);
        headerText.setFillColor(Color::White);
        headerText.setStyle(Text::Bold);
        FloatRect bounds = headerText.getLocalBounds();
        float colLeft = columnLefts[i];
        float colWidth = columnWidths[i];
        headerText.setPosition(Vector2f(
            colLeft + (colWidth - bounds.size.x) / 2.f - bounds.position.x,
            headerTop + (HEADER_HEIGHT - bounds.size.y) / 2.f - bounds.position.y
        ));
        window.draw(headerText);
    }

    // Draw rows
    float startY = headerTop + HEADER_HEIGHT;
    for (size_t i = 0; i < allData.size(); i++) {
        float rowY = startY + static_cast<float>(i) * ROW_HEIGHT;

        RectangleShape rowBg(Vector2f(TABLE_WIDTH, ROW_HEIGHT));
        rowBg.setPosition(Vector2f(tableLeft, rowY));

        if (static_cast<int>(i) == selectedRow) {
            rowBg.setFillColor(selectedColor);
        } else if (static_cast<int>(i) == hoveredRow) {
            rowBg.setFillColor(hoverColor);
        } else {
            rowBg.setFillColor(rowColor);
        }
        window.draw(rowBg);

        RectangleShape rowBorder(Vector2f(TABLE_WIDTH, 1.f));
        rowBorder.setPosition(Vector2f(tableLeft, rowY + ROW_HEIGHT));
        rowBorder.setFillColor(borderColor);
        window.draw(rowBorder);

        const auto& row = allData[i];
        if (row.size() < 12) continue;

        string status = (row.size() >= 14) ? row[13] : "";
        if (status.empty()) status = "Đang chiếu";

        array<string, 5> cellValues = {
            row[0],
            row[1],
            row[6] + " phút",
            row[7],
            status
        };

        for (size_t col = 0; col < columns.size(); ++col) {
            Text cellText(font, String::fromUtf8(cellValues[col].begin(), cellValues[col].end()), 15);
            cellText.setFillColor(textColor);
            FloatRect bounds = cellText.getLocalBounds();
            cellText.setPosition(Vector2f(
                columnLefts[col] + 12.f - bounds.position.x,
                rowY + (ROW_HEIGHT - bounds.size.y) / 2.f - bounds.position.y
            ));
            window.draw(cellText);
        }
    }
}

void MoviePanel::renderPopup(RenderWindow& window) {
    if (currentPopup == NONE) return;
    
    // Draw overlay
    window.draw(popupOverlay);
    
    // Draw popup background
    window.draw(popupBackground);
    
    // Draw title
    if (popupTitle) {
        window.draw(*popupTitle);
    }
    
    // Draw input boxes
    for (int i = 0; i < inputBoxes.getSize(); ++i) {
        if (inputBoxes[i]) {
            inputBoxes[i]->render(window);
        }
    }

    
    // Draw buttons
    if (btnPopupSave && btnPopupCancel) {
        btnPopupSave->draw(window);
        btnPopupCancel->draw(window);
    }
}

void MoviePanel::renderNotification(RenderWindow& window) {
    if (!notificationVisible) return;
    if (notificationClock.getElapsedTime().asSeconds() > 3.0f) {
        notificationVisible = false;
        return;
    }

    notificationBg.setPosition(Vector2f(position.x + width - notificationBg.getSize().x - 30.f, position.y + 20.f));
    window.draw(notificationBg);
    
    notificationTextObj->setString(String::fromUtf8(notificationText.begin(), notificationText.end()));
    FloatRect textBounds = notificationTextObj->getLocalBounds();
    notificationTextObj->setPosition(Vector2f(
        notificationBg.getPosition().x + (notificationBg.getSize().x - textBounds.size.x) / 2.f - textBounds.position.x,
        notificationBg.getPosition().y + (notificationBg.getSize().y - textBounds.size.y) / 2.f - textBounds.position.y
    ));
    window.draw(*notificationTextObj);
}

void MoviePanel::render(RenderWindow& window) {
    // Draw background
    window.draw(background);
    
    // Draw title
    if (titleText) {
        window.draw(*titleText);
    }
    
    // Draw table
    renderTable(window);
    
    // Draw CRUD buttons (custom with rounded corners 6px)
    // Add button
    RoundedRectRenderer::draw(window, btnAddBg.getPosition(), btnAddBg.getSize(), 6.f, btnAddBg.getFillColor());
    if (btnAddText) {
        FloatRect tb = btnAddText->getLocalBounds();
        Vector2f pos = btnAddBg.getPosition();
        btnAddText->setPosition(Vector2f(
            pos.x + (btnAddBg.getSize().x - tb.size.x) / 2.f,
            pos.y + (btnAddBg.getSize().y - tb.size.y) / 2.f
        ));
        window.draw(*btnAddText);
    }

    // Edit button
    RoundedRectRenderer::draw(window, btnEditBg.getPosition(), btnEditBg.getSize(), 6.f, btnEditBg.getFillColor());
    if (btnEditText) {
        FloatRect tb = btnEditText->getLocalBounds();
        Vector2f pos = btnEditBg.getPosition();
        btnEditText->setPosition(Vector2f(
            pos.x + (btnEditBg.getSize().x - tb.size.x) / 2.f,
            pos.y + (btnEditBg.getSize().y - tb.size.y) / 2.f
        ));
        window.draw(*btnEditText);
    }

    // Delete button
    RoundedRectRenderer::draw(window, btnDeleteBg.getPosition(), btnDeleteBg.getSize(), 6.f, btnDeleteBg.getFillColor());
    if (btnDeleteText) {
        FloatRect tb = btnDeleteText->getLocalBounds();
        Vector2f pos = btnDeleteBg.getPosition();
        btnDeleteText->setPosition(Vector2f(
            pos.x + (btnDeleteBg.getSize().x - tb.size.x) / 2.f,
            pos.y + (btnDeleteBg.getSize().y - tb.size.y) / 2.f
        ));
        window.draw(*btnDeleteText);
    }

    // Reload button (icon centered inside rounded background)
    RoundedRectRenderer::draw(window, reloadButtonBg.getPosition(), reloadButtonBg.getSize(), 6.f, reloadButtonBg.getFillColor());
        FloatRect localBounds = reloadSprite.getLocalBounds();
        Vector2f spriteScale = reloadSprite.getScale();
        float scaledW = localBounds.size.x * spriteScale.x;
        float scaledH = localBounds.size.y * spriteScale.y;
        reloadSprite.setPosition({
            reloadButtonBg.getPosition().x + (reloadButtonBg.getSize().x - scaledW) / 2.f,
            reloadButtonBg.getPosition().y + (reloadButtonBg.getSize().y - scaledH) / 2.f
        });
        window.draw(reloadSprite);
    
    // Draw popup if active
    renderPopup(window);
    
    // Draw notification
    renderNotification(window);

    renderSelectionWarning(window);
}
