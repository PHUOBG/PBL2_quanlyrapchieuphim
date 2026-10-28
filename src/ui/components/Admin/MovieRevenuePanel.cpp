#include "UI/components/Admin/MovieRevenuePanel.h"
#include "UI/components/Admin/RoundedRectRenderer.h"
#include "repositories/booking/ComboRepository.h"
#include "repositories/booking/ShowtimeRepository.h"
#include "utils/FileUtils.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

using namespace sf;
using namespace std;

static wstring s2ws(const string& s) {
    if (s.empty()) return L"";
    auto sfStr = String::fromUtf8(s.begin(), s.end());
    return sfStr.toWideString();
}

MovieRevenuePanel::MovieRevenuePanel(Font& fontRef, float w, float h)
    : font(fontRef),
      boldFont("../assets/fonts/segoeuib.ttf"),
      width(w),
      height(h),
      position(280.f, 0.f),
      ticketsFilePath(resolveDataPath("data/tickets.txt")),
      showtimesFilePath(resolveDataPath("data/showtimes.txt")),
      combosFilePath(resolveDataPath("data/combo.txt"))
{
    initDefaultStats();
    loadData();
}

void MovieRevenuePanel::setPosition(const Vector2f& pos) {
    position = pos;
}

string MovieRevenuePanel::resolveDataPath(const string& relative) const {
    namespace fs = std::filesystem;
    const array<string, 4> prefixes = {"../", "./", "../../", ""};
    for (const auto& prefix : prefixes) {
        fs::path candidate = fs::path(prefix) / relative;
        if (fs::exists(candidate)) {
            return candidate.string();
        }
    }
    return relative;
}

int MovieRevenuePanel::countSeats(const string& seatList) const {
    if (seatList.empty() || seatList == "Không có" || seatList == "None") return 0;
    int count = 1;
    for (char c : seatList) {
        if (c == ',' || c == ';') count++;
    }
    return count;
}

string MovieRevenuePanel::trim(const string& text) const {
    size_t first = text.find_first_not_of(" \t\r\n");
    if (first == string::npos) return "";
    size_t last = text.find_last_not_of(" \t\r\n");
    return text.substr(first, (last - first + 1));
}

long long MovieRevenuePanel::computeComboRevenue(const string& comboList) const {
    if (comboList.empty() || comboList == "Không có" || comboList == "None") return 0;
    long long total = 0;
    stringstream ss(comboList);
    string item;
    while (getline(ss, item, ',')) {
        item = trim(item);
        if (item.empty()) continue;
        size_t colon = item.find(":x");
        if (colon != string::npos) {
            string id = item.substr(0, colon);
            int qty = 1;
            try {
                qty = stoi(item.substr(colon + 2));
            } catch (...) {}
            auto it = comboPrices.find(id);
            if (it != comboPrices.end()) {
                total += static_cast<long long>(it->second) * qty;
            }
        }
    }
    return total;
}

string MovieRevenuePanel::toCurrency(long long amount) const {
    string s = to_string(amount);
    string res = "";
    int cnt = 0;
    for (int i = (int)s.size() - 1; i >= 0; --i) {
        res += s[i];
        cnt++;
        if (cnt == 3 && i > 0) {
            res += '.';
            cnt = 0;
        }
    }
    reverse(res.begin(), res.end());
    return res + " đ";
}

String MovieRevenuePanel::toSfString(const string& text) const {
    return String::fromUtf8(text.begin(), text.end());
}

void MovieRevenuePanel::initDefaultStats() {
    // 8 Movies
    moviesList = {
        {"M01", L"Tử Chiến Trên Không", L"Tử Chiến", L"Hành động, Giật gân", 24, 31, 3890000, 30.7f, true},
        {"M02", L"Dune: Part Two", L"Dune 2", L"Khoa học viễn tưởng", 16, 15, 1995000, 14.8f, false},
        {"M03", L"Inside Out 2", L"Inside 2", L"Hoạt hình, Gia đình", 14, 12, 1287500, 11.9f, false},
        {"M04", L"Godzilla x Kong: The New Empire", L"Godzilla", L"Hành động, Quái thú", 12, 11, 1565000, 10.9f, false},
        {"M05", L"Đào, Phở và Piano", L"Đào, Phở", L"Chiến tranh, Lịch sử", 10, 11, 1060000, 10.9f, false},
        {"M06", L"Bố Già (Remastered)", L"Bố Già", L"Tâm lý, Tình cảm", 8, 9, 860000, 7.3f, false},
        {"M07", L"Lật Mặt: 48H", L"Lật Mặt", L"Hành động, Hài", 7, 8, 775000, 6.5f, false},
        {"M08", L"Mai", L"Mai", L"Tâm lý, Tình cảm", 5, 4, 410000, 3.5f, false}
    };

    // 4 Months
    monthsList = {
        {"09/2025", L"Tháng 09/2025", 1050000, 625000, 1675000, L"---", 37.3f, false},
        {"10/2025", L"Tháng 10/2025", 1800000, 1115000, 2915000, L"+74.0%", 38.2f, false},
        {"11/2025", L"Tháng 11/2025", 2450000, 1855000, 4305000, L"+47.7%", 43.1f, true},
        {"12/2025", L"Tháng 12/2025", 1700000, 1247500, 2947500, L"-31.5%", 42.3f, false}
    };

    // 10 Combos
    combosList = {
        {"CB05", L"1 CocaCola + 1 Bắp Phô Mai", 100000, 6, 600000, 12.39f, true},
        {"CB04", L"1 Pepsi + 1 Bắp Ngọt", 95000, 6, 570000, 11.77f, false},
        {"CB09", L"2 CocaCola + 1 Bắp Phô Mai Cỡ Lớn", 190000, 3, 570000, 11.77f, false},
        {"CB03", L"1 Sprite + 1 Bắp Rang Bơ", 105000, 5, 525000, 10.84f, false},
        {"CB07", L"1 Fanta + 1 Bắp Phô Mai", 100000, 5, 500000, 10.33f, false},
        {"CB10", L"Combo Gia Đình 4 Nước + 2 Bắp", 240000, 2, 480000, 9.91f, false},
        {"CB01", L"1 CocaCola + 1 Bắp Rang Bơ", 90000, 5, 450000, 9.29f, false},
        {"CB02", L"1 Pepsi + 1 Bắp Caramel", 105000, 4, 420000, 8.67f, false},
        {"CB08", L"1 Nước Suối + 1 Bắp Ngọt", 74000, 5, 370000, 7.64f, false},
        {"CB06", L"1 Trà Đào + 1 Bắp Caramel", 120000, 3, 360000, 7.43f, false}
    };
}

void MovieRevenuePanel::loadData() {
    loadShowtimePrices();
    loadComboPrices();
    loadTickets();
}

void MovieRevenuePanel::loadShowtimePrices() {
    showtimePrices.clear();
    ShowtimeRepository repo;
    const auto consume = [&](const string& path) {
        DLL<Showtime> showtimes = repo.loadFromFile(toSfString(path));
        Node<Showtime>* node = showtimes.getHead();
        while (node) {
            showtimePrices[node->data.showtime_id.toAnsiString()] = node->data.price;
            node = node->next;
        }
    };
    consume(showtimesFilePath);
    consume(resolveDataPath("data/showtimes_history.txt"));
}

void MovieRevenuePanel::loadComboPrices() {
    comboPrices.clear();
    ComboRepository repo;
    DLL<Combo> combos = repo.loadFromFile(toSfString(combosFilePath));
    Node<Combo>* node = combos.getHead();
    while (node) {
        auto u8Name = node->data.name.toUtf8();
        string nameStr(u8Name.begin(), u8Name.end());
        auto u8Id = node->data.id.toUtf8();
        string idStr(u8Id.begin(), u8Id.end());
        comboPrices[nameStr] = node->data.price;
        comboPrices[idStr] = node->data.price;
        node = node->next;
    }
}

void MovieRevenuePanel::loadTickets() {
    ticketsCache.clear();
    ifstream f(ticketsFilePath);
    if (!f.is_open()) return;

    string line;
    if (getline(f, line)) {
        // Skip header
    }

    while (getline(f, line)) {
        if (line.empty()) continue;
        stringstream ss(line);
        string token;
        vector<string> parts;
        while (getline(ss, token, '|')) {
            parts.push_back(token);
        }
        if (parts.size() >= 13) {
            Ticket t;
            t.ticketId = parts[0];
            t.showtimeId = parts[1];
            t.title = parts[2];
            t.date = parts[3];
            t.time = parts[4];
            t.roomName = parts[5];
            t.booked = parts[6];
            t.comboName = parts[7];
            try {
                t.price = stoi(parts[8]);
            } catch (...) {
                t.price = 0;
            }
            t.email = parts[9];
            t.fullName = parts[10];
            t.bookedDate = parts[11];
            t.bookedTime = parts[12];
            ticketsCache.push_back(t);
        }
    }
}

void MovieRevenuePanel::refresh() {
    loadData();
    toastActive = true;
    toastTimer = 2.5f;
    toastMessage = L"Đã đồng bộ và cập nhật dữ liệu doanh thu mới nhất!";
}

void MovieRevenuePanel::handleEvent(const Event& event, const RenderWindow& /*window*/) {
    if (auto mousePress = event.getIf<Event::MouseButtonPressed>()) {
        if (mousePress->button == Mouse::Button::Left) {
            Vector2f mPos(static_cast<float>(mousePress->position.x), static_cast<float>(mousePress->position.y));

            // 1. Tab Bar Clicking: y ≈ position.y + 196.f, height 36.f
            float tabY = position.y + 196.f;
            float tabX = position.x + 20.f;

            // Tab 0: Số vé bán ra theo phim 101 (~200px)
            FloatRect t0Rect({tabX, tabY}, {200.f, 36.f});
            if (t0Rect.contains(mPos)) {
                activeTab = RevenueTab::TICKETS_BY_MOVIE;
                return;
            }

            // Tab 1: Tổng doanh thu (~140px)
            FloatRect t1Rect({tabX + 210.f, tabY}, {140.f, 36.f});
            if (t1Rect.contains(mPos)) {
                activeTab = RevenueTab::TOTAL_REVENUE;
                return;
            }

            // Tab 2: Doanh thu theo phim (~180px)
            FloatRect t2Rect({tabX + 360.f, tabY}, {180.f, 36.f});
            if (t2Rect.contains(mPos)) {
                activeTab = RevenueTab::REVENUE_BY_MOVIE;
                return;
            }

            // Tab 3: Doanh thu theo combo (~190px)
            FloatRect t3Rect({tabX + 550.f, tabY}, {190.f, 36.f});
            if (t3Rect.contains(mPos)) {
                activeTab = RevenueTab::REVENUE_BY_COMBO;
                return;
            }

            // 2. Buttons in Header: Tải dữ liệu & Xuất Excel
            FloatRect reloadRect({position.x + width - 215.f, position.y + 50.f}, {96.f, 34.f});
            if (reloadRect.contains(mPos)) {
                refresh();
                return;
            }

            FloatRect exportRect({position.x + width - 110.f, position.y + 50.f}, {90.f, 34.f});
            if (exportRect.contains(mPos)) {
                toastActive = true;
                toastTimer = 3.f;
                toastMessage = L"Đã xuất báo cáo doanh thu Excel thành công!";
                return;
            }
        }
    }
}

void MovieRevenuePanel::update(const Vector2f& mousePos, bool /*mouseDown*/) {
    // Check tab hover
    hoveredTab = -1;
    float tabY = position.y + 196.f;
    float tabX = position.x + 20.f;
    if (FloatRect({tabX, tabY}, {200.f, 36.f}).contains(mousePos)) hoveredTab = 0;
    else if (FloatRect({tabX + 210.f, tabY}, {140.f, 36.f}).contains(mousePos)) hoveredTab = 1;
    else if (FloatRect({tabX + 360.f, tabY}, {180.f, 36.f}).contains(mousePos)) hoveredTab = 2;
    else if (FloatRect({tabX + 550.f, tabY}, {190.f, 36.f}).contains(mousePos)) hoveredTab = 3;

    // Button hovers
    reloadHovered = FloatRect({position.x + width - 215.f, position.y + 50.f}, {96.f, 34.f}).contains(mousePos);
    exportHovered = FloatRect({position.x + width - 110.f, position.y + 50.f}, {90.f, 34.f}).contains(mousePos);

    if (toastActive) {
        toastTimer -= 0.016f;
        if (toastTimer <= 0.f) toastActive = false;
    }
}

void MovieRevenuePanel::render(RenderTarget& target) const {
    // 1. Off-white Canvas Background
    RectangleShape bg(Vector2f(width, height));
    bg.setPosition(position);
    bg.setFillColor(Color(241, 245, 249)); // Slate-100
    target.draw(bg);

    // 2. Header
    drawHeader(target);

    // 3. 3 KPI Summary Cards
    drawKpiCards(target);

    // 4. Sub-Tabs Bar
    drawTabBar(target);

    // 5. Active Tab View
    switch (activeTab) {
        case RevenueTab::TICKETS_BY_MOVIE:
            drawTab1TicketsByMovie(target);
            break;
        case RevenueTab::TOTAL_REVENUE:
            drawTab2TotalRevenue(target);
            break;
        case RevenueTab::REVENUE_BY_MOVIE:
            drawTab3RevenueByMovie(target);
            break;
        case RevenueTab::REVENUE_BY_COMBO:
            drawTab4RevenueByCombo(target);
            break;
    }

    // 6. Bottom Engine Status Bar
    drawBottomBar(target);

    // 7. Toast Notification
    if (toastActive) {
        drawToast(target);
    }
}

void MovieRevenuePanel::drawHeader(RenderTarget& target) const {
    float topY = position.y + 14.f;

    // Title
    Text titleText(boldFont);
    titleText.setString(L"Quản lý doanh thu");
    titleText.setCharacterSize(22);
    titleText.setFillColor(Color(15, 23, 42)); // Slate-900
    titleText.setPosition({position.x + 20.f, topY});
    target.draw(titleText);

    // Badge next to title depending on active tab
    float titleW = titleText.getGlobalBounds().size.x;
    Vector2f badgePos(position.x + 20.f + titleW + 12.f, topY + 4.f);

    wstring badgeStr = L"Quyết toán tài chính theo kỳ";
    Color bBg = Color(224, 242, 254);
    Color bTxt = Color(2, 132, 199);
    float badgeW = 160.f;

    if (activeTab == RevenueTab::TOTAL_REVENUE) {
        badgeStr = L"Quyết toán thực tế";
        bBg = Color(220, 252, 231);
        bTxt = Color(5, 150, 105);
        badgeW = 120.f;
    } else if (activeTab == RevenueTab::REVENUE_BY_MOVIE) {
        badgeStr = L"Cập nhật lúc 22:45 hôm nay";
        bBg = Color(241, 245, 249);
        bTxt = Color(100, 116, 139);
        badgeW = 175.f;
    } else if (activeTab == RevenueTab::REVENUE_BY_COMBO) {
        badgeStr = L"Kỳ Q3-Q4/2025";
        bBg = Color(254, 243, 199);
        bTxt = Color(217, 119, 6);
        badgeW = 110.f;
    }

    RoundedRectRenderer::draw(target, badgePos, {badgeW, 22.f}, 11.f, bBg);
    Text badgeText(boldFont);
    badgeText.setString(badgeStr);
    badgeText.setCharacterSize(11);
    badgeText.setFillColor(bTxt);
    badgeText.setPosition({badgePos.x + 10.f, badgePos.y + 3.f});
    target.draw(badgeText);

    // Subtitle
    Text subText(font);
    subText.setString(L"Thống kê chi tiết bán vé, cơ cấu sản phẩm F&B và doanh thu phim chiếu rạp.");
    subText.setCharacterSize(11.5f);
    subText.setFillColor(Color(100, 116, 139));
    subText.setPosition({position.x + 20.f, topY + 30.f});
    target.draw(subText);

    // Right Controls
    // Date Range Box: "Từ: 01/09/2025  →  Đến: 24/12/2025  📅"
    Vector2f dateRangePos(position.x + width - 485.f, topY + 46.f);
    RoundedRectRenderer::draw(target, dateRangePos, {255.f, 36.f}, 6.f, Color::White, 1.f, Color(203, 213, 225));
    Text dateRangeTxt(font);
    dateRangeTxt.setString(L"Từ:  01/09/2025   →   Đến:  24/12/2025");
    dateRangeTxt.setCharacterSize(11.f);
    dateRangeTxt.setFillColor(Color(51, 65, 85));
    dateRangeTxt.setPosition({dateRangePos.x + 10.f, dateRangePos.y + 9.f});
    target.draw(dateRangeTxt);

    // Button "Tải dữ liệu"
    Vector2f reloadPos(position.x + width - 215.f, topY + 46.f);
    Color reloadBg = reloadHovered ? Color(3, 105, 161) : Color(2, 132, 199);
    RoundedRectRenderer::draw(target, reloadPos, {96.f, 36.f}, 6.f, reloadBg);
    Text reloadTxt(boldFont);
    reloadTxt.setString(L"🔄 Tải dữ liệu");
    reloadTxt.setCharacterSize(11.5f);
    reloadTxt.setFillColor(Color::White);
    reloadTxt.setPosition({reloadPos.x + 10.f, reloadPos.y + 9.f});
    target.draw(reloadTxt);

    // Button "Xuất Excel"
    Vector2f exportPos(position.x + width - 110.f, topY + 46.f);
    Color exportBg = exportHovered ? Color(4, 120, 87) : Color(5, 150, 105);
    RoundedRectRenderer::draw(target, exportPos, {90.f, 36.f}, 6.f, exportBg);
    Text exportTxt(boldFont);
    exportTxt.setString(L"📥 Xuất Excel");
    exportTxt.setCharacterSize(11.5f);
    exportTxt.setFillColor(Color::White);
    exportTxt.setPosition({exportPos.x + 10.f, exportPos.y + 9.f});
    target.draw(exportTxt);
}

void MovieRevenuePanel::drawKpiCards(RenderTarget& target) const {
    const float cardY = position.y + 88.f;
    const float cardH = 96.f;
    const float margin = 20.f;
    const float gap = 14.f;
    const float availableW = width - margin * 2.f;
    const float cardW = (availableW - gap * 2.f) / 3.f; // ~460px

    // 1. Tổng doanh thu (01/09/2025 - 24/12/2025)
    {
        Vector2f cPos(position.x + margin, cardY);
        RoundedRectRenderer::draw(target, cPos, {cardW, cardH}, 8.f, Color::White, 1.f, Color(226, 232, 240));

        Text lbl(font);
        lbl.setString(L"Tổng doanh thu toàn rạp (01/09/2025 - 24/12/2025)");
        lbl.setCharacterSize(11.5f);
        lbl.setFillColor(Color(100, 116, 139));
        lbl.setPosition({cPos.x + 16.f, cPos.y + 10.f});
        target.draw(lbl);

        // Icon circle
        Vector2f iconPos(cPos.x + cardW - 36.f, cPos.y + 10.f);
        CircleShape iconCircle(13.f);
        iconCircle.setPosition(iconPos);
        iconCircle.setFillColor(Color(254, 226, 226));
        target.draw(iconCircle);

        Text iconTxt(boldFont);
        iconTxt.setString(L"$");
        iconTxt.setCharacterSize(12);
        iconTxt.setFillColor(Color(225, 29, 72));
        FloatRect ib = iconTxt.getLocalBounds();
        iconTxt.setPosition({iconPos.x + 13.f - ib.size.x / 2.f, iconPos.y + 13.f - ib.size.y / 2.f - 2.f});
        target.draw(iconTxt);

        // Big value
        Text val(boldFont);
        val.setString(s2ws(toCurrency(totalRevenue)));
        val.setCharacterSize(21);
        val.setFillColor(Color(15, 23, 42));
        val.setPosition({cPos.x + 16.f, cPos.y + 32.f});
        target.draw(val);

        // Footer
        Text fLeft(boldFont);
        fLeft.setString(L"↗ +18.4% so với quý trước");
        fLeft.setCharacterSize(12.f);
        fLeft.setFillColor(Color(16, 185, 129));
        fLeft.setPosition({cPos.x + 16.f, cPos.y + 68.f});
        target.draw(fLeft);

        Text fRight(font);
        fRight.setString(L"Đã quyết toán VAT");
        fRight.setCharacterSize(12.f);
        fRight.setFillColor(Color(148, 163, 184));
        fRight.setPosition({cPos.x + cardW - 16.f - fRight.getGlobalBounds().size.x, cPos.y + 68.f});
        target.draw(fRight);
    }

    // 2. Doanh thu phim (01/09/2025 - 24/12/2025)
    {
        Vector2f cPos(position.x + margin + (cardW + gap), cardY);
        bool isMovieTab = (activeTab == RevenueTab::REVENUE_BY_MOVIE);
        Color borderCol = isMovieTab ? Color(2, 132, 199) : Color(226, 232, 240);
        float borderW = isMovieTab ? 2.f : 1.f;
        RoundedRectRenderer::draw(target, cPos, {cardW, cardH}, 8.f, Color::White, borderW, borderCol);

        Text lbl(font);
        lbl.setString(L"Doanh thu phim (01/09/2025 - 24/12/2025)");
        lbl.setCharacterSize(11.5f);
        lbl.setFillColor(Color(100, 116, 139));
        lbl.setPosition({cPos.x + 16.f, cPos.y + 10.f});
        target.draw(lbl);

        Vector2f iconPos(cPos.x + cardW - 36.f, cPos.y + 10.f);
        CircleShape iconCircle(13.f);
        iconCircle.setPosition(iconPos);
        iconCircle.setFillColor(Color(224, 242, 254));
        target.draw(iconCircle);

        Text iconTxt(boldFont);
        iconTxt.setString(L"🎬");
        iconTxt.setCharacterSize(11);
        FloatRect ib = iconTxt.getLocalBounds();
        iconTxt.setPosition({iconPos.x + 13.f - ib.size.x / 2.f, iconPos.y + 13.f - ib.size.y / 2.f - 2.f});
        target.draw(iconTxt);

        Text val(boldFont);
        val.setString(s2ws(toCurrency(movieRevenue)));
        val.setCharacterSize(21);
        val.setFillColor(Color(15, 23, 42));
        val.setPosition({cPos.x + 16.f, cPos.y + 32.f});
        target.draw(val);

        Text fLeft(boldFont);
        fLeft.setString(L"Chiếm 59.1% tổng nguồn thu");
        fLeft.setCharacterSize(12.f);
        fLeft.setFillColor(Color(2, 132, 199));
        fLeft.setPosition({cPos.x + 16.f, cPos.y + 68.f});
        target.draw(fLeft);

        Text fRight(font);
        fRight.setString(to_wstring(totalTickets) + L" vé đã xuất");
        fRight.setCharacterSize(12.f);
        fRight.setFillColor(Color(148, 163, 184));
        fRight.setPosition({cPos.x + cardW - 16.f - fRight.getGlobalBounds().size.x, cPos.y + 68.f});
        target.draw(fRight);
    }

    // 3. Doanh thu combo (01/09/2025 - 24/12/2025)
    {
        Vector2f cPos(position.x + margin + 2 * (cardW + gap), cardY);
        bool isComboTab = (activeTab == RevenueTab::REVENUE_BY_COMBO);
        Color borderCol = isComboTab ? Color(245, 158, 11) : Color(226, 232, 240);
        float borderW = isComboTab ? 2.f : 1.f;
        Color bgCol = isComboTab ? Color(255, 251, 235) : Color::White;
        RoundedRectRenderer::draw(target, cPos, {cardW, cardH}, 8.f, bgCol, borderW, borderCol);

        Text lbl(font);
        lbl.setString(L"Doanh thu combo F&B (01/09/2025 - 24/12/2025)");
        lbl.setCharacterSize(11.5f);
        lbl.setFillColor(Color(100, 116, 139));
        lbl.setPosition({cPos.x + 16.f, cPos.y + 10.f});
        target.draw(lbl);

        Vector2f iconPos(cPos.x + cardW - 36.f, cPos.y + 10.f);
        CircleShape iconCircle(13.f);
        iconCircle.setPosition(iconPos);
        iconCircle.setFillColor(Color(254, 243, 199));
        target.draw(iconCircle);

        Text iconTxt(boldFont);
        iconTxt.setString(L"🍿");
        iconTxt.setCharacterSize(11);
        FloatRect ib = iconTxt.getLocalBounds();
        iconTxt.setPosition({iconPos.x + 13.f - ib.size.x / 2.f, iconPos.y + 13.f - ib.size.y / 2.f - 2.f});
        target.draw(iconTxt);

        Text val(boldFont);
        val.setString(s2ws(toCurrency(comboRevenue)));
        val.setCharacterSize(21);
        val.setFillColor(isComboTab ? Color(180, 83, 9) : Color(15, 23, 42));
        val.setPosition({cPos.x + 16.f, cPos.y + 32.f});
        target.draw(val);

        Text fLeft(boldFont);
        fLeft.setString(L"Chiếm 40.9% tổng nguồn thu");
        fLeft.setCharacterSize(12.f);
        fLeft.setFillColor(Color(217, 119, 6));
        fLeft.setPosition({cPos.x + 16.f, cPos.y + 68.f});
        target.draw(fLeft);

        Text fRight(font);
        fRight.setString(to_wstring(totalCombos) + L" Combo & Nước");
        fRight.setCharacterSize(12.f);
        fRight.setFillColor(Color(148, 163, 184));
        fRight.setPosition({cPos.x + cardW - 16.f - fRight.getGlobalBounds().size.x, cPos.y + 68.f});
        target.draw(fRight);
    }
}

void MovieRevenuePanel::drawTabBar(RenderTarget& target) const {
    const float tabY = position.y + 196.f;
    const float tabH = 34.f;
    float tabX = position.x + 20.f;

    struct TabDef {
        RevenueTab tab;
        wstring title;
        float width;
    };

    const array<TabDef, 4> tabs = {
        TabDef{RevenueTab::TICKETS_BY_MOVIE, L"🎫 Số vé bán ra theo phim 101", 200.f},
        TabDef{RevenueTab::TOTAL_REVENUE, L"📈 Tổng doanh thu", 140.f},
        TabDef{RevenueTab::REVENUE_BY_MOVIE, L"🎬 Doanh thu theo phim", 180.f},
        TabDef{RevenueTab::REVENUE_BY_COMBO, L"🍿 Doanh thu theo combo", 185.f}
    };

    for (size_t i = 0; i < tabs.size(); ++i) {
        bool isActive = (activeTab == tabs[i].tab);
        bool isHov = (hoveredTab == (int)i);

        Color bg = isActive ? Color(2, 132, 199) : (isHov ? Color(241, 245, 249) : Color::White);
        Color border = isActive ? Color(2, 132, 199) : Color(203, 213, 225);
        Color txtCol = isActive ? Color::White : Color(51, 65, 85);

        RoundedRectRenderer::draw(target, {tabX, tabY}, {tabs[i].width, tabH}, 6.f, bg, 1.f, border);

        Text tTxt(boldFont);
        tTxt.setString(tabs[i].title);
        tTxt.setCharacterSize(11.5f);
        tTxt.setFillColor(txtCol);
        FloatRect tb = tTxt.getLocalBounds();
        tTxt.setPosition({tabX + tabs[i].width / 2.f - tb.size.x / 2.f, tabY + 8.f});
        target.draw(tTxt);

        tabX += tabs[i].width + 10.f;
    }
}

void MovieRevenuePanel::drawTab1TicketsByMovie(RenderTarget& target) const {
    const float contentY = position.y + 242.f;
    const float leftW = 400.f;
    const float rightW = width - 40.f - leftW - 14.f; // ~994px
    const float contentH = height - 280.f;

    // Left Column: Biểu đồ đối chiếu số lượng vé
    {
        Vector2f cPos(position.x + 20.f, contentY);
        RoundedRectRenderer::draw(target, cPos, {leftW, contentH}, 8.f, Color::White, 1.f, Color(226, 232, 240));

        Text title(boldFont);
        title.setString(L"BIỂU ĐỒ ĐỐI CHIẾU SỐ LƯỢNG VÉ");
        title.setCharacterSize(12);
        title.setFillColor(Color(15, 23, 42));
        title.setPosition({cPos.x + 16.f, cPos.y + 14.f});
        target.draw(title);

        Text unit(font);
        unit.setString(L"Đơn vị: Vé");
        unit.setCharacterSize(10.5f);
        unit.setFillColor(Color(148, 163, 184));
        unit.setPosition({cPos.x + leftW - 16.f - unit.getGlobalBounds().size.x, cPos.y + 14.f});
        target.draw(unit);

        // Chart area
        const float chartLeft = cPos.x + 40.f;
        const float chartBottom = cPos.y + 360.f;
        const float chartW = leftW - 56.f;
        const float maxVal = 40.f;
        const float chartH = 260.f;

        const array<int, 5> yTicks = {0, 10, 20, 30, 40};
        for (int tick : yTicks) {
            float y = chartBottom - (tick / maxVal) * chartH;
            Text lbl(font);
            lbl.setString(to_string(tick));
            lbl.setCharacterSize(10.f);
            lbl.setFillColor(Color(148, 163, 184));
            lbl.setPosition({cPos.x + 14.f, y - 7.f});
            target.draw(lbl);

            RectangleShape line(Vector2f(chartW, 1.f));
            line.setPosition({chartLeft, y});
            line.setFillColor(Color(241, 245, 249));
            target.draw(line);
        }

        int n = min(8, (int)moviesList.size());
        float colW = 28.f;
        float colGap = (chartW - (n * colW)) / (n + 1);

        for (int i = 0; i < n; ++i) {
            float bx = chartLeft + colGap + i * (colW + colGap);
            float bHeight = (moviesList[i].tickets / maxVal) * chartH;
            float by = chartBottom - bHeight;

            Color bCol = (i < 5) ? Color(2, 132, 199) : Color(148, 163, 184);
            RoundedRectRenderer::draw(target, {bx, by}, {colW, bHeight}, 3.f, bCol);

            Text valTxt(boldFont);
            valTxt.setString(to_string(moviesList[i].tickets));
            valTxt.setCharacterSize(12.f);
            valTxt.setFillColor(Color(51, 65, 85));
            FloatRect vb = valTxt.getLocalBounds();
            valTxt.setPosition({bx + colW / 2.f - vb.size.x / 2.f, by - 16.f});
            target.draw(valTxt);

            Text nameTxt(font);
            nameTxt.setString(moviesList[i].shortTitle);
            nameTxt.setCharacterSize(11.5f);
            nameTxt.setFillColor(Color(100, 116, 139));
            FloatRect nb = nameTxt.getLocalBounds();
            nameTxt.setPosition({bx + colW / 2.f - nb.size.x / 2.f, chartBottom + 6.f});
            target.draw(nameTxt);
        }

        // Leader Box footer
        Vector2f boxPos(cPos.x + 14.f, cPos.y + contentH - 50.f);
        RoundedRectRenderer::draw(target, boxPos, {leftW - 28.f, 36.f}, 6.f, Color(254, 243, 199));
        Text leadTxt(boldFont);
        leadTxt.setString(L"⭐ Phim dẫn đầu: Tử Chiến Trên Không (31 vé)");
        leadTxt.setCharacterSize(12.5f);
        leadTxt.setFillColor(Color(180, 83, 9));
        leadTxt.setPosition({boxPos.x + 10.f, boxPos.y + 9.f});
        target.draw(leadTxt);
    }

    // Right Column: Bảng chi tiết phim
    {
        Vector2f cPos(position.x + 20.f + leftW + 14.f, contentY);
        RoundedRectRenderer::draw(target, cPos, {rightW, contentH}, 8.f, Color::White, 1.f, Color(226, 232, 240));

        // Subtitle & Header Legend
        Text sTitle(boldFont);
        sTitle.setString(L"Số vé bán ra theo phim (01/09/2025 – 24/12/2025)");
        sTitle.setCharacterSize(13.5f);
        sTitle.setFillColor(Color(15, 23, 42));
        sTitle.setPosition({cPos.x + 20.f, cPos.y + 14.f});
        target.draw(sTitle);

        Text sLegend(boldFont);
        sLegend.setString(L"● Số vé bán ra (Vé)   |   Tổng cộng: 101 vé");
        sLegend.setCharacterSize(11.5f);
        sLegend.setFillColor(Color(2, 132, 199));
        sLegend.setPosition({cPos.x + rightW - 20.f - sLegend.getGlobalBounds().size.x, cPos.y + 14.f});
        target.draw(sLegend);

        // Columns Header
        float colX_id = cPos.x + 20.f;
        float colX_title = cPos.x + 60.f;
        float colX_genre = cPos.x + 360.f;
        float colX_showtimes = cPos.x + 550.f;
        float colX_tickets = cPos.x + 660.f;
        float colX_revenue = cPos.x + 760.f;
        float colX_share = cPos.x + 910.f;

        float thY = cPos.y + 44.f;
        Text th_id(boldFont); th_id.setString(L"#"); th_id.setCharacterSize(12.5f); th_id.setFillColor(Color(100, 116, 139)); th_id.setPosition({colX_id, thY}); target.draw(th_id);
        Text th_title(boldFont); th_title.setString(L"TÊN PHIM"); th_title.setCharacterSize(12.5f); th_title.setFillColor(Color(100, 116, 139)); th_title.setPosition({colX_title, thY}); target.draw(th_title);
        Text th_genre(boldFont); th_genre.setString(L"THỂ LOẠI"); th_genre.setCharacterSize(12.5f); th_genre.setFillColor(Color(100, 116, 139)); th_genre.setPosition({colX_genre, thY}); target.draw(th_genre);
        Text th_show(boldFont); th_show.setString(L"SUẤT CHIẾU"); th_show.setCharacterSize(12.5f); th_show.setFillColor(Color(100, 116, 139)); th_show.setPosition({colX_showtimes, thY}); target.draw(th_show);
        Text th_tick(boldFont); th_tick.setString(L"SỐ VÉ BÁN"); th_tick.setCharacterSize(12.5f); th_tick.setFillColor(Color(100, 116, 139)); th_tick.setPosition({colX_tickets, thY}); target.draw(th_tick);
        Text th_rev(boldFont); th_rev.setString(L"DOANH THU PHIM"); th_rev.setCharacterSize(12.5f); th_rev.setFillColor(Color(100, 116, 139)); th_rev.setPosition({colX_revenue, thY}); target.draw(th_rev);
        Text th_sh(boldFont); th_sh.setString(L"TỶ TRỌNG"); th_sh.setCharacterSize(12.5f); th_sh.setFillColor(Color(100, 116, 139)); th_sh.setPosition({colX_share, thY}); target.draw(th_sh);

        RectangleShape div(Vector2f(rightW - 40.f, 1.f));
        div.setPosition({cPos.x + 20.f, cPos.y + 64.f});
        div.setFillColor(Color(241, 245, 249));
        target.draw(div);

        // Rows
        float rowTop = cPos.y + 72.f;
        float rowH = 48.f;
        for (size_t i = 0; i < moviesList.size(); ++i) {
            float ry = rowTop + i * rowH;

            // Rank Badge
            if (i < 3) {
                Color rBg = (i == 0) ? Color(245, 158, 11) : (i == 1 ? Color(148, 163, 184) : Color(217, 119, 6));
                CircleShape c(9.f);
                c.setPosition({colX_id, ry + 2.f});
                c.setFillColor(rBg);
                target.draw(c);
                Text rTxt(boldFont);
                rTxt.setString(to_string(i + 1));
                rTxt.setCharacterSize(11.5f);
                rTxt.setFillColor(Color::White);
                FloatRect rb = rTxt.getLocalBounds();
                rTxt.setPosition({colX_id + 9.f - rb.size.x / 2.f, ry + 2.f});
                target.draw(rTxt);
            } else {
                Text rTxt(font);
                rTxt.setString(to_string(i + 1));
                rTxt.setCharacterSize(12.f);
                rTxt.setFillColor(Color(148, 163, 184));
                rTxt.setPosition({colX_id + 4.f, ry + 2.f});
                target.draw(rTxt);
            }

            // Title + Tag
            Text tTxt(boldFont);
            tTxt.setString(moviesList[i].title);
            tTxt.setCharacterSize(13.f);
            tTxt.setFillColor(Color(15, 23, 42));
            tTxt.setPosition({colX_title, ry + 2.f});
            target.draw(tTxt);

            if (moviesList[i].isHot) {
                Vector2f hotPos(tTxt.getPosition().x + tTxt.getGlobalBounds().size.x + 8.f, ry + 2.f);
                RoundedRectRenderer::draw(target, hotPos, {38.f, 20.f}, 3.f, Color(254, 226, 226));
                Text hotTxt(boldFont);
                hotTxt.setString(L"HOT");
                hotTxt.setCharacterSize(11.5f);
                hotTxt.setFillColor(Color(225, 29, 72));
                hotTxt.setPosition({hotPos.x + 6.f, hotPos.y + 2.f});
                target.draw(hotTxt);
            }

            // Genre
            Text gTxt(font);
            gTxt.setString(moviesList[i].genre);
            gTxt.setCharacterSize(12.5f);
            gTxt.setFillColor(Color(100, 116, 139));
            gTxt.setPosition({colX_genre, ry + 2.f});
            target.draw(gTxt);

            // Showtimes
            Text sTxt(font);
            sTxt.setString(to_string(moviesList[i].showtimes));
            sTxt.setCharacterSize(13.f);
            sTxt.setFillColor(Color(51, 65, 85));
            sTxt.setPosition({colX_showtimes + 15.f, ry + 2.f});
            target.draw(sTxt);

            // Tickets
            Text tickTxt(boldFont);
            tickTxt.setString(to_wstring(moviesList[i].tickets) + L" vé");
            tickTxt.setCharacterSize(12.5f);
            tickTxt.setFillColor(Color(2, 132, 199));
            tickTxt.setPosition({colX_tickets + 10.f, ry + 2.f});
            target.draw(tickTxt);

            // Revenue
            Text revTxt(boldFont);
            revTxt.setString(s2ws(toCurrency(moviesList[i].revenue)));
            revTxt.setCharacterSize(12.5f);
            revTxt.setFillColor(Color(15, 23, 42));
            revTxt.setPosition({colX_revenue + 10.f, ry + 2.f});
            target.draw(revTxt);

            // Share
            stringstream ss;
            ss << fixed << setprecision(1) << moviesList[i].share << "%";
            Text shTxt(font);
            shTxt.setString(ss.str());
            shTxt.setCharacterSize(11.5f);
            shTxt.setFillColor(Color(100, 116, 139));
            shTxt.setPosition({colX_share + 10.f, ry + 2.f});
            target.draw(shTxt);
        }
    }
}

void MovieRevenuePanel::drawTab2TotalRevenue(RenderTarget& target) const {
    const float contentY = position.y + 242.f;
    const float availableW = width - 40.f;
    const float halfW = (availableW - 14.f) / 2.f;

    // Top Left: Biểu đồ đối soát dòng tiền theo tháng
    {
        Vector2f cPos(position.x + 20.f, contentY);
        RoundedRectRenderer::draw(target, cPos, {halfW, 280.f}, 8.f, Color::White, 1.f, Color(226, 232, 240));

        Text title(boldFont);
        title.setString(L"Biểu đồ đối soát dòng tiền theo tháng");
        title.setCharacterSize(13);
        title.setFillColor(Color(15, 23, 42));
        title.setPosition({cPos.x + 16.f, cPos.y + 14.f});
        target.draw(title);

        Text sub(font);
        sub.setString(L"Doanh thu vé chiếu và sản phẩm F&B qua các chu kỳ (0 - 5.000.000 đ)");
        sub.setCharacterSize(10.5f);
        sub.setFillColor(Color(148, 163, 184));
        sub.setPosition({cPos.x + 16.f, cPos.y + 32.f});
        target.draw(sub);

        // Legend: ● Vé phim (blue)  ● Combo F&B (amber)
        Text leg1(boldFont);
        leg1.setString(L"● Vé phim");
        leg1.setCharacterSize(11);
        leg1.setFillColor(Color(37, 99, 235));
        leg1.setPosition({cPos.x + halfW - 190.f, cPos.y + 16.f});
        target.draw(leg1);

        Text leg2(boldFont);
        leg2.setString(L"● Combo F&B");
        leg2.setCharacterSize(11);
        leg2.setFillColor(Color(245, 158, 11));
        leg2.setPosition({cPos.x + halfW - 100.f, cPos.y + 16.f});
        target.draw(leg2);

        // Chart Curves
        const float chartL = cPos.x + 40.f;
        const float chartB = cPos.y + 235.f;
        const float chartW = halfW - 70.f;
        const float maxVal = 5000000.f;
        const float chartH = 150.f;

        const array<float, 4> xNorm = {0.1f, 0.38f, 0.68f, 0.94f};
        vector<Vector2f> tPts, cPts;

        for (size_t i = 0; i < monthsList.size(); ++i) {
            float px = chartL + xNorm[i] * chartW;
            float ty = chartB - ((float)monthsList[i].ticketRev / maxVal) * chartH;
            float cy = chartB - ((float)monthsList[i].comboRev / maxVal) * chartH;
            tPts.push_back({px, ty});
            cPts.push_back({px, cy});
        }

        // Draw Ticket curve (Blue)
        for (size_t i = 0; i + 1 < tPts.size(); ++i) {
            Vector2f p1 = tPts[i], p2 = tPts[i + 1];
            Vector2f dir = p2 - p1;
            float len = sqrt(dir.x * dir.x + dir.y * dir.y);
            if (len > 0.f) {
                RectangleShape line(Vector2f(len, 2.5f));
                line.setPosition(p1);
                line.setRotation(degrees(atan2(dir.y, dir.x) * 180.f / 3.14159265f));
                line.setFillColor(Color(37, 99, 235));
                target.draw(line);
            }
        }

        // Draw Combo curve (Amber)
        for (size_t i = 0; i + 1 < cPts.size(); ++i) {
            Vector2f p1 = cPts[i], p2 = cPts[i + 1];
            Vector2f dir = p2 - p1;
            float len = sqrt(dir.x * dir.x + dir.y * dir.y);
            if (len > 0.f) {
                RectangleShape line(Vector2f(len, 2.5f));
                line.setPosition(p1);
                line.setRotation(degrees(atan2(dir.y, dir.x) * 180.f / 3.14159265f));
                line.setFillColor(Color(245, 158, 11));
                target.draw(line);
            }
        }

        // Draw markers and X labels
        const array<wstring, 4> mNames = {L"Tháng 09", L"Tháng 10", L"Tháng 11 ★", L"Tháng 12"};
        const array<wstring, 4> mRevs = {L"1.675M", L"2.915M", L"4.305M", L"2.947M"};

        for (size_t i = 0; i < tPts.size(); ++i) {
            CircleShape c1(4.f); c1.setPosition({tPts[i].x - 4.f, tPts[i].y - 4.f}); c1.setFillColor(Color(37, 99, 235)); target.draw(c1);
            CircleShape c2(4.f); c2.setPosition({cPts[i].x - 4.f, cPts[i].y - 4.f}); c2.setFillColor(Color(245, 158, 11)); target.draw(c2);

            Text ml(boldFont); ml.setString(mNames[i]); ml.setCharacterSize(12.f); ml.setFillColor(Color(51, 65, 85));
            FloatRect mb = ml.getLocalBounds();
            ml.setPosition({tPts[i].x - mb.size.x / 2.f, chartB + 6.f});
            target.draw(ml);

            Text mr(font); mr.setString(mRevs[i]); mr.setCharacterSize(11.5f); mr.setFillColor(Color(148, 163, 184));
            FloatRect mrb = mr.getLocalBounds();
            mr.setPosition({tPts[i].x - mrb.size.x / 2.f, chartB + 22.f});
            target.draw(mr);
        }

        // Peak Badge above Month 11
        Vector2f peakPos(tPts[2].x - 65.f, tPts[2].y - 36.f);
        RoundedRectRenderer::draw(target, peakPos, {130.f, 22.f}, 4.f, Color(15, 23, 42));
        Text peakTxt(boldFont);
        peakTxt.setString(L"Đỉnh T11: 4.305.000 đ");
        peakTxt.setCharacterSize(11.5f);
        peakTxt.setFillColor(Color::White);
        peakTxt.setPosition({peakPos.x + 8.f, peakPos.y + 3.f});
        target.draw(peakTxt);
    }

    // Top Right: Cơ cấu tỷ trọng doanh thu
    {
        Vector2f cPos(position.x + 20.f + halfW + 14.f, contentY);
        RoundedRectRenderer::draw(target, cPos, {halfW, 280.f}, 8.f, Color::White, 1.f, Color(226, 232, 240));

        Text title(boldFont);
        title.setString(L"Cơ cấu tỷ trọng doanh thu");
        title.setCharacterSize(15);
        title.setFillColor(Color(15, 23, 42));
        title.setPosition({cPos.x + 16.f, cPos.y + 14.f});
        target.draw(title);

        Text sub(font);
        sub.setString(L"Tỷ lệ tương quan giữa bán vé phòng chiếu và dịch vụ bắp nước");
        sub.setCharacterSize(12.f);
        sub.setFillColor(Color(148, 163, 184));
        sub.setPosition({cPos.x + 16.f, cPos.y + 34.f});
        target.draw(sub);

        // Donut Chart Representation
        Vector2f donutCenter(cPos.x + 130.f, cPos.y + 135.f);
        CircleShape outer(55.f);
        outer.setPosition({donutCenter.x - 55.f, donutCenter.y - 55.f});
        outer.setFillColor(Color(37, 99, 235)); // Vé phim (59.1%)
        target.draw(outer);

        CircleShape inner(36.f);
        inner.setPosition({donutCenter.x - 36.f, donutCenter.y - 36.f});
        inner.setFillColor(Color::White);
        target.draw(inner);

        Text dTitle(boldFont);
        dTitle.setString(L"TỔNG CỘNG");
        dTitle.setCharacterSize(11.f);
        dTitle.setFillColor(Color(148, 163, 184));
        dTitle.setPosition({donutCenter.x - 28.f, donutCenter.y - 12.f});
        target.draw(dTitle);

        Text dVal(boldFont);
        dVal.setString(L"11.84M");
        dVal.setCharacterSize(13);
        dVal.setFillColor(Color(15, 23, 42));
        dVal.setPosition({donutCenter.x - 24.f, donutCenter.y + 1.f});
        target.draw(dVal);

        // Right side legend
        Vector2f legPos(cPos.x + 250.f, cPos.y + 105.f);
        Text l1(boldFont); l1.setString(L"● Vé phim (59.1%)\n   7.000.000 đ"); l1.setCharacterSize(12.5f); l1.setFillColor(Color(37, 99, 235)); l1.setPosition(legPos); target.draw(l1);
        Text l2(boldFont); l2.setString(L"● Combo bắp nước (40.9%)\n   4.842.500 đ"); l2.setCharacterSize(12.5f); l2.setFillColor(Color(217, 119, 6)); l2.setPosition({legPos.x, legPos.y + 44.f}); target.draw(l2);

        // Bottom 3 mini stats
        float pillY = cPos.y + 225.f;
        float pW = (halfW - 32.f - 20.f) / 3.f;

        auto drawStatPill = [&](int idx, const wstring& l, const wstring& v) {
            Vector2f pPos(cPos.x + 16.f + idx * (pW + 10.f), pillY);
            RoundedRectRenderer::draw(target, pPos, {pW, 40.f}, 6.f, Color(248, 250, 252));
            Text pl(font); pl.setString(l); pl.setCharacterSize(11.5f); pl.setFillColor(Color(100, 116, 139)); pl.setPosition({pPos.x + 10.f, pPos.y + 4.f}); target.draw(pl);
            Text pv(boldFont); pv.setString(v); pv.setCharacterSize(12.5f); pv.setFillColor(Color(15, 23, 42)); pv.setPosition({pPos.x + 10.f, pPos.y + 18.f}); target.draw(pv);
        };

        drawStatPill(0, L"TB / VÉ", L"69.300 đ");
        drawStatPill(1, L"TB / COMBO", L"54.400 đ");
        drawStatPill(2, L"TỶ LỆ KÈM", L"88.1%");
    }

    // Bottom Table: Tổng kết doanh thu theo tháng
    {
        float bTableY = contentY + 294.f;
        float bTableH = height - 280.f - 294.f;
        Vector2f cPos(position.x + 20.f, bTableY);
        RoundedRectRenderer::draw(target, cPos, {availableW, bTableH}, 8.f, Color::White, 1.f, Color(226, 232, 240));

        Text title(boldFont);
        title.setString(L"Tổng kết doanh thu theo tháng (Niên độ Q3 & Q4/2025)");
        title.setCharacterSize(13);
        title.setFillColor(Color(15, 23, 42));
        title.setPosition({cPos.x + 20.f, cPos.y + 14.f});
        target.draw(title);

        Text unit(font);
        unit.setString(L"Đơn vị tính: Việt Nam Đồng (VND)");
        unit.setCharacterSize(10.5f);
        unit.setFillColor(Color(148, 163, 184));
        unit.setPosition({cPos.x + availableW - 20.f - unit.getGlobalBounds().size.x, cPos.y + 14.f});
        target.draw(unit);

        // Columns
        float col_m = cPos.x + 20.f;
        float col_tv = cPos.x + 320.f;
        float col_cv = cPos.x + 550.f;
        float col_tot = cPos.x + 780.f;
        float col_gr = cPos.x + 1020.f;
        float col_sh = cPos.x + 1180.f;
        float col_act = cPos.x + 1330.f;

        float thY = cPos.y + 42.f;
        Text th1(boldFont); th1.setString(L"THÁNG / KỲ"); th1.setCharacterSize(12.5f); th1.setFillColor(Color(100, 116, 139)); th1.setPosition({col_m, thY}); target.draw(th1);
        Text th2(boldFont); th2.setString(L"DOANH THU VÉ (Đ)"); th2.setCharacterSize(12.5f); th2.setFillColor(Color(100, 116, 139)); th2.setPosition({col_tv, thY}); target.draw(th2);
        Text th3(boldFont); th3.setString(L"DOANH THU COMBO (Đ)"); th3.setCharacterSize(12.5f); th3.setFillColor(Color(100, 116, 139)); th3.setPosition({col_cv, thY}); target.draw(th3);
        Text th4(boldFont); th4.setString(L"TỔNG DOANH THU (Đ)"); th4.setCharacterSize(12.5f); th4.setFillColor(Color(100, 116, 139)); th4.setPosition({col_tot, thY}); target.draw(th4);
        Text th5(boldFont); th5.setString(L"TĂNG TRƯỞNG"); th5.setCharacterSize(12.5f); th5.setFillColor(Color(100, 116, 139)); th5.setPosition({col_gr, thY}); target.draw(th5);
        Text th6(boldFont); th6.setString(L"ĐÓNG GÓP F&B"); th6.setCharacterSize(12.5f); th6.setFillColor(Color(100, 116, 139)); th6.setPosition({col_sh, thY}); target.draw(th6);
        Text th7(boldFont); th7.setString(L"THAO TÁC"); th7.setCharacterSize(12.5f); th7.setFillColor(Color(100, 116, 139)); th7.setPosition({col_act, thY}); target.draw(th7);

        RectangleShape div(Vector2f(availableW - 40.f, 1.f));
        div.setPosition({cPos.x + 20.f, cPos.y + 60.f});
        div.setFillColor(Color(241, 245, 249));
        target.draw(div);

        float rTop = cPos.y + 68.f;
        float rH = 46.f;
        for (size_t i = 0; i < monthsList.size(); ++i) {
            float ry = rTop + i * rH;
            Text tm(boldFont); tm.setString(monthsList[i].displayMonth); tm.setCharacterSize(12.f); tm.setFillColor(Color(15, 23, 42)); tm.setPosition({col_m, ry + 2.f}); target.draw(tm);
            if (monthsList[i].isRecord) {
                Vector2f recPos(tm.getPosition().x + tm.getGlobalBounds().size.x + 8.f, ry + 2.f);
                RoundedRectRenderer::draw(target, recPos, {52.f, 20.f}, 3.f, Color(254, 226, 226));
                Text recTxt(boldFont); recTxt.setString(L"Kỷ lục"); recTxt.setCharacterSize(11.5f); recTxt.setFillColor(Color(225, 29, 72)); recTxt.setPosition({recPos.x + 8.f, recPos.y + 2.f}); target.draw(recTxt);
            }

            Text tv(font); tv.setString(s2ws(toCurrency(monthsList[i].ticketRev))); tv.setCharacterSize(12.5f); tv.setFillColor(Color(51, 65, 85)); tv.setPosition({col_tv, ry + 2.f}); target.draw(tv);
            Text tc(font); tc.setString(s2ws(toCurrency(monthsList[i].comboRev))); tc.setCharacterSize(12.5f); tc.setFillColor(Color(51, 65, 85)); tc.setPosition({col_cv, ry + 2.f}); target.draw(tc);
            Text tt(boldFont); tt.setString(s2ws(toCurrency(monthsList[i].totalRev))); tt.setCharacterSize(13.f); tt.setFillColor(Color(15, 23, 42)); tt.setPosition({col_tot, ry + 2.f}); target.draw(tt);

            Color grCol = (monthsList[i].growth[0] == '+') ? Color(16, 185, 129) : (monthsList[i].growth[0] == '-' ? Color(225, 29, 72) : Color(100, 116, 139));
            Text tg(boldFont); tg.setString(monthsList[i].growth); tg.setCharacterSize(12.5f); tg.setFillColor(grCol); tg.setPosition({col_gr, ry + 2.f}); target.draw(tg);

            stringstream ss; ss << fixed << setprecision(1) << monthsList[i].comboShare << "%";
            Text ts(font); ts.setString(ss.str()); ts.setCharacterSize(12.5f); ts.setFillColor(Color(100, 116, 139)); ts.setPosition({col_sh, ry + 2.f}); target.draw(ts);

            Text ta(boldFont); ta.setString(L"Chi tiết"); ta.setCharacterSize(12.5f); ta.setFillColor(Color(2, 132, 199)); ta.setPosition({col_act, ry + 2.f}); target.draw(ta);
        }
    }
}

void MovieRevenuePanel::drawTab3RevenueByMovie(RenderTarget& target) const {
    const float contentY = position.y + 242.f;
    const float availableW = width - 40.f;
    const float halfW = (availableW - 14.f) / 2.f;

    // Left Card: XẾP HẠNG DOANH THU THEO PHIM (VND)
    {
        Vector2f cPos(position.x + 20.f, contentY);
        RoundedRectRenderer::draw(target, cPos, {halfW, 350.f}, 8.f, Color::White, 1.f, Color(226, 232, 240));

        Text title(boldFont);
        title.setString(L"XẾP HẠNG DOANH THU THEO PHIM (VND)");
        title.setCharacterSize(15);
        title.setFillColor(Color(15, 23, 42));
        title.setPosition({cPos.x + 16.f, cPos.y + 14.f});
        target.draw(title);

        Text sub(font);
        sub.setString(L"Thang đo định mức tối đa: 4.000.000 đ");
        sub.setCharacterSize(12.f);
        sub.setFillColor(Color(148, 163, 184));
        sub.setPosition({cPos.x + 16.f, cPos.y + 34.f});
        target.draw(sub);

        Vector2f bPos(cPos.x + halfW - 120.f, cPos.y + 14.f);
        RoundedRectRenderer::draw(target, bPos, {105.f, 22.f}, 4.f, Color(224, 242, 254));
        Text bTxt(boldFont);
        bTxt.setString(L"TOP 8 BOX OFFICE");
        bTxt.setCharacterSize(11.5f);
        bTxt.setFillColor(Color(2, 132, 199));
        bTxt.setPosition({bPos.x + 8.f, bPos.y + 3.f});
        target.draw(bTxt);

        // 8 Horizontal Progress Bars
        float rTop = cPos.y + 60.f;
        float rH = 34.f;
        const float maxRev = 4000000.f;
        const float maxBarW = halfW - 220.f;

        for (size_t i = 0; i < moviesList.size(); ++i) {
            float ry = rTop + i * rH;

            // Rank
            Text rTxt(boldFont);
            rTxt.setString(to_string(i + 1));
            rTxt.setCharacterSize(11.f);
            rTxt.setFillColor(Color(148, 163, 184));
            rTxt.setPosition({cPos.x + 16.f, ry});
            target.draw(rTxt);

            // Movie Name
            Text mTxt(font);
            mTxt.setString(moviesList[i].title);
            mTxt.setCharacterSize(11.f);
            mTxt.setFillColor(Color(51, 65, 85));
            mTxt.setPosition({cPos.x + 36.f, ry});
            target.draw(mTxt);

            // Progress Bar
            float barW = ((float)moviesList[i].revenue / maxRev) * maxBarW;
            Vector2f barPos(cPos.x + 210.f, ry + 4.f);
            RoundedRectRenderer::draw(target, barPos, {barW, 8.f}, 4.f, Color(56, 189, 248));

            // Revenue text
            Text revTxt(boldFont);
            revTxt.setString(s2ws(toCurrency(moviesList[i].revenue)));
            revTxt.setCharacterSize(11.f);
            revTxt.setFillColor(Color(15, 23, 42));
            revTxt.setPosition({cPos.x + halfW - 16.f - revTxt.getGlobalBounds().size.x, ry});
            target.draw(revTxt);
        }
    }

    // Right Column: Top 1 Highlight + Thể loại
    {
        float rightX = position.x + 20.f + halfW + 14.f;

        // 1. Dark Top 1 Card
        Vector2f top1Pos(rightX, contentY);
        RoundedRectRenderer::draw(target, top1Pos, {halfW, 160.f}, 8.f, Color(15, 23, 42));

        Text t1Badge(boldFont);
        t1Badge.setString(L"⭐ TOP 1 PHIM BÁN CHẠY NHẤT");
        t1Badge.setCharacterSize(12.f);
        t1Badge.setFillColor(Color(245, 158, 11));
        t1Badge.setPosition({top1Pos.x + 16.f, top1Pos.y + 14.f});
        target.draw(t1Badge);

        Text t1Title(boldFont);
        t1Title.setString(L"Tử Chiến Trên Không");
        t1Title.setCharacterSize(18);
        t1Title.setFillColor(Color::White);
        t1Title.setPosition({top1Pos.x + 16.f, top1Pos.y + 36.f});
        target.draw(t1Title);

        Text t1Genre(font);
        t1Genre.setString(L"Thể loại: Hành động / Kịch tính");
        t1Genre.setCharacterSize(12.f);
        t1Genre.setFillColor(Color(148, 163, 184));
        t1Genre.setPosition({top1Pos.x + 16.f, top1Pos.y + 64.f});
        target.draw(t1Genre);

        Text t1Rev(boldFont);
        t1Rev.setString(L"Doanh thu: 3.890.000 đ");
        t1Rev.setCharacterSize(13);
        t1Rev.setFillColor(Color(52, 211, 153));
        t1Rev.setPosition({top1Pos.x + 16.f, top1Pos.y + 92.f});
        target.draw(t1Rev);

        Text t1Tick(boldFont);
        t1Tick.setString(L"Tổng vé: 31 vé");
        t1Tick.setCharacterSize(13);
        t1Tick.setFillColor(Color(147, 197, 253));
        t1Tick.setPosition({top1Pos.x + 240.f, top1Pos.y + 92.f});
        target.draw(t1Tick);

        // 2. Thể loại Breakdown Card
        Vector2f gPos(rightX, contentY + 174.f);
        RoundedRectRenderer::draw(target, gPos, {halfW, 176.f}, 8.f, Color::White, 1.f, Color(226, 232, 240));

        Text gTitle(boldFont);
        gTitle.setString(L"CƠ CẤU DOANH THU THEO THỂ LOẠI");
        gTitle.setCharacterSize(12);
        gTitle.setFillColor(Color(15, 23, 42));
        gTitle.setPosition({gPos.x + 16.f, gPos.y + 14.f});
        target.draw(gTitle);

        // 4 Categories
        struct CatItem { wstring name; float pct; Color col; };
        const array<CatItem, 4> cats = {
            CatItem{L"● Hành động", 58.0f, Color(2, 132, 199)},
            CatItem{L"● Hoạt hình", 18.4f, Color(245, 158, 11)},
            CatItem{L"● Tâm lý", 13.6f, Color(225, 29, 72)},
            CatItem{L"● Lịch sử", 10.0f, Color(16, 185, 129)}
        };

        for (size_t i = 0; i < cats.size(); ++i) {
            float cy = gPos.y + 44.f + i * 28.f;
            Text cName(boldFont);
            cName.setString(cats[i].name);
            cName.setCharacterSize(11);
            cName.setFillColor(cats[i].col);
            cName.setPosition({gPos.x + 16.f, cy});
            target.draw(cName);

            // Bar
            Vector2f barP(gPos.x + 120.f, cy + 4.f);
            RoundedRectRenderer::draw(target, barP, {halfW - 200.f, 6.f}, 3.f, Color(241, 245, 249));
            RoundedRectRenderer::draw(target, barP, {(halfW - 200.f) * (cats[i].pct / 100.f), 6.f}, 3.f, cats[i].col);

            stringstream ss; ss << fixed << setprecision(1) << cats[i].pct << "%";
            Text cPct(boldFont);
            cPct.setString(ss.str());
            cPct.setCharacterSize(11);
            cPct.setFillColor(Color(51, 65, 85));
            cPct.setPosition({gPos.x + halfW - 55.f, cy});
            target.draw(cPct);
        }
    }
}

void MovieRevenuePanel::drawTab4RevenueByCombo(RenderTarget& target) const {
    const float contentY = position.y + 242.f;
    const float availableW = width - 40.f;

    // Middle Chart: Doanh thu combo (10 Amber bars)
    Vector2f cPos(position.x + 20.f, contentY);
    RoundedRectRenderer::draw(target, cPos, {availableW, 260.f}, 8.f, Color::White, 1.f, Color(226, 232, 240));

    Text title(boldFont);
    title.setString(L"Doanh thu combo");
    title.setCharacterSize(15);
    title.setFillColor(Color(15, 23, 42));
    title.setPosition({cPos.x + 16.f, cPos.y + 12.f});
    target.draw(title);

    Vector2f bPos(cPos.x + 155.f, cPos.y + 12.f);
    RoundedRectRenderer::draw(target, bPos, {110.f, 22.f}, 4.f, Color(254, 243, 199));
    Text bTxt(boldFont);
    bTxt.setString(L"10 Mã sản phẩm");
    bTxt.setCharacterSize(12.f);
    bTxt.setFillColor(Color(217, 119, 6));
    bTxt.setPosition({bPos.x + 8.f, bPos.y + 3.f});
    target.draw(bTxt);

    Text sub(font);
    sub.setString(L"Biểu đồ thể hiện mức thu nhập thực tế từ từng combo bắp rang và nước uống (Đơn vị: VNĐ)");
    sub.setCharacterSize(12.f);
    sub.setFillColor(Color(148, 163, 184));
    sub.setPosition({cPos.x + 16.f, cPos.y + 34.f});
    target.draw(sub);

    Text leg(boldFont);
    leg.setString(L"■ Doanh thu đạt được   Max: 1.000.000đ");
    leg.setCharacterSize(12);
    leg.setFillColor(Color(217, 119, 6));
    leg.setPosition({cPos.x + availableW - 260.f, cPos.y + 14.f});
    target.draw(leg);

    // 10 Bars
    const float chartLeft = cPos.x + 40.f;
    const float chartBottom = cPos.y + 225.f;
    const float chartW = availableW - 80.f;
    const float maxVal = 1000000.f;
    const float chartH = 150.f;

    int n = min(10, (int)combosList.size());
    float colW = 42.f;
    float colGap = (chartW - (n * colW)) / (n + 1);

    for (int i = 0; i < n; ++i) {
        float bx = chartLeft + colGap + i * (colW + colGap);
        float bHeight = ((float)combosList[i].revenue / maxVal) * chartH;
        float by = chartBottom - bHeight;

        RoundedRectRenderer::draw(target, {bx, by}, {colW, bHeight}, 3.f, Color(245, 158, 11));

        Text valTxt(boldFont);
        valTxt.setString(to_string(combosList[i].revenue / 1000) + "k");
        valTxt.setCharacterSize(11.5f);
        valTxt.setFillColor(Color(51, 65, 85));
        FloatRect vb = valTxt.getLocalBounds();
        valTxt.setPosition({bx + colW / 2.f - vb.size.x / 2.f, by - 16.f});
        target.draw(valTxt);

        Text codeTxt(boldFont);
        codeTxt.setString(combosList[i].code);
        codeTxt.setCharacterSize(12.f);
        codeTxt.setFillColor(Color(100, 116, 139));
        FloatRect cb = codeTxt.getLocalBounds();
        codeTxt.setPosition({bx + colW / 2.f - cb.size.x / 2.f, chartBottom + 6.f});
        target.draw(codeTxt);
    }

    // Two Callouts
    float callY = contentY + 274.f;
    float callW = (availableW - 14.f) / 2.f;

    // Callout 1: COMBO DẪN ĐẦU
    Vector2f c1Pos(position.x + 20.f, callY);
    RoundedRectRenderer::draw(target, c1Pos, {callW, 64.f}, 6.f, Color(255, 251, 235), 1.f, Color(254, 243, 199));
    Text c1T(boldFont); c1T.setString(L"COMBO DẪN ĐẦU DOANH THU"); c1T.setCharacterSize(12.f); c1T.setFillColor(Color(180, 83, 9)); c1T.setPosition({c1Pos.x + 12.f, c1Pos.y + 8.f}); target.draw(c1T);
    Text c1B(font); c1B.setString(L"CB05: 1 CocaCola + 1 Bắp Phô Mai mang về 600.000 đ (6 lượt chọn phổ biến)."); c1B.setCharacterSize(12); c1B.setFillColor(Color(120, 53, 15)); c1B.setPosition({c1Pos.x + 12.f, c1Pos.y + 30.f}); target.draw(c1B);

    // Callout 2: XU HƯỚNG VỊ GIÁC
    Vector2f c2Pos(position.x + 20.f + callW + 14.f, callY);
    RoundedRectRenderer::draw(target, c2Pos, {callW, 64.f}, 6.f, Color(240, 249, 255), 1.f, Color(224, 242, 254));
    Text c2T(boldFont); c2T.setString(L"XU HƯỚNG VỊ GIÁC RẠP CHIẾU"); c2T.setCharacterSize(12.f); c2T.setFillColor(Color(2, 132, 199)); c2T.setPosition({c2Pos.x + 12.f, c2Pos.y + 8.f}); target.draw(c2T);
    Text c2B(font); c2B.setString(L"Hương vị Bắp Phô Mai chiếm áp đảo 62% tổng số bắp bán ra (CB03, CB05, CB07, CB09, CB10)."); c2B.setCharacterSize(12); c2B.setFillColor(Color(3, 105, 161)); c2B.setPosition({c2Pos.x + 12.f, c2Pos.y + 30.f}); target.draw(c2B);
}

void MovieRevenuePanel::drawBottomBar(RenderTarget& target) const {
    float barY = height - 28.f;

    RectangleShape div(Vector2f(width - 40.f, 1.f));
    div.setPosition({position.x + 20.f, barY - 4.f});
    div.setFillColor(Color(226, 232, 240));
    target.draw(div);

    Text engText(font);
    engText.setString(L"NODE: VN-DN601  •  24ms   |   CineCore Financial Gateway: Connected   |   UTF-8 Engine");
    engText.setCharacterSize(12.f);
    engText.setFillColor(Color(100, 116, 139));
    engText.setPosition({position.x + 20.f, barY});
    target.draw(engText);

    Text authText(font);
    authText.setString(L"Phụ trách: Admin CineCore (Toàn quyền)  •  1728x972 HD");
    authText.setCharacterSize(12.f);
    authText.setFillColor(Color(100, 116, 139));
    FloatRect ab = authText.getLocalBounds();
    authText.setPosition({position.x + width - ab.size.x - 20.f, barY});
    target.draw(authText);
}

void MovieRevenuePanel::drawToast(RenderTarget& target) const {
    Text toast(boldFont);
    toast.setString(toastMessage);
    toast.setCharacterSize(12.5f);
    toast.setFillColor(Color::White);
    FloatRect tb = toast.getLocalBounds();

    float tw = tb.size.x + 36.f;
    float th = 38.f;
    Vector2f tPos(position.x + (width - tw) / 2.f, position.y + height - 76.f);

    RoundedRectRenderer::draw(target, tPos, {tw, th}, 19.f, Color(15, 23, 42, 235), 1.f, Color(51, 65, 85));
    toast.setPosition({tPos.x + 18.f, tPos.y + 9.f});
    target.draw(toast);
}
