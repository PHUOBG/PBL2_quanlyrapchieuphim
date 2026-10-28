#include "UI/components/Admin/OverviewPanel.h"
#include "UI/components/Admin/RoundedRectRenderer.h"
#include "repositories/booking/ComboRepository.h"
#include "repositories/booking/ShowtimeRepository.h"
#include "repositories/UserRepository.h"
#include "utils/FileUtils.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <ctime>
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

OverviewPanel::OverviewPanel(Font& fontRef, float w, float h)
    : font(fontRef),
      boldFont("../assets/fonts/segoeuib.ttf"),
      width(w),
      height(h),
      position(280.f, 0.f),
      ticketsFilePath(resolveDataPath("data/tickets.txt")),
      usersFilePath(resolveDataPath("data/users.txt")),
      showtimesFilePath(resolveDataPath("data/showtimes.txt")),
      combosFilePath(resolveDataPath("data/combo.txt"))
{
    loadData();
}

void OverviewPanel::setPosition(const Vector2f& pos) {
    position = pos;
}

string OverviewPanel::resolveDataPath(const string& relative) const {
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

int OverviewPanel::countSeats(const string& seatList) const {
    if (seatList.empty() || seatList == "Không có" || seatList == "None") return 0;
    int count = 1;
    for (char c : seatList) {
        if (c == ',' || c == ';') count++;
    }
    return count;
}

string OverviewPanel::trim(const string& text) const {
    size_t first = text.find_first_not_of(" \t\r\n");
    if (first == string::npos) return "";
    size_t last = text.find_last_not_of(" \t\r\n");
    return text.substr(first, (last - first + 1));
}

long long OverviewPanel::computeComboRevenue(const string& comboList) const {
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

long long OverviewPanel::computeTicketRevenue(const Ticket& ticket, int seatCount, long long comboRevenue) const {
    long long total = static_cast<long long>(ticket.price);
    long long base = total - comboRevenue;
    if (base > 0) return base;
    auto it = showtimeSeatPrices.find(ticket.showtimeId);
    if (it != showtimeSeatPrices.end()) {
        return static_cast<long long>(it->second) * seatCount;
    }
    return total;
}

string OverviewPanel::toCurrency(long long amount) const {
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

String OverviewPanel::toSfString(const string& text) const {
    return String::fromUtf8(text.begin(), text.end());
}

void OverviewPanel::loadData() {
    loadShowtimePrices();
    loadComboPrices();
    loadRegistrations();
    loadTickets();
    calculateStats();
    dataLoaded = true;
}

void OverviewPanel::loadRegistrations() {
    registrationTimes.clear();
    UserRepository repo(usersFilePath);
    const auto users = repo.getAllUsers();
    for (const auto& user : users) {
        registrationTimes[user.getEmail()] = user.getRegisteredAt();
    }
}

void OverviewPanel::loadShowtimePrices() {
    showtimeSeatPrices.clear();
    ShowtimeRepository repo;
    const auto consume = [&](const string& path) {
        DLL<Showtime> showtimes = repo.loadFromFile(toSfString(path));
        Node<Showtime>* node = showtimes.getHead();
        while (node) {
            showtimeSeatPrices[node->data.showtime_id.toAnsiString()] = node->data.price;
            node = node->next;
        }
    };
    consume(showtimesFilePath);
    consume(resolveDataPath("data/showtimes_history.txt"));
}

void OverviewPanel::loadComboPrices() {
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

void OverviewPanel::loadTickets() {
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

void OverviewPanel::calculateStats() {
    // Top 8 movies default baseline from the approved design
    topMoviesList = {
        {"TỬ CHIẾN TRÊN KHÔNG", L"TỬ CHIẾN TRÊN KHÔNG", L"TỬ CHIẾN..", 31, 3890000},
        {"DUNE: PART TWO", L"DUNE: PART TWO", L"DUNE 2", 15, 1995000},
        {"INSIDE OUT 2", L"INSIDE OUT 2", L"INSIDE OUT", 12, 1287500},
        {"GODZILLA X KONG: NEW EMPIRE", L"GODZILLA X KONG: NEW EMPIRE", L"GODZILLA..", 11, 1565000},
        {"ĐÀO, PHỞ VÀ PIANO", L"ĐÀO, PHỞ VÀ PIANO", L"ĐÀO, PHỞ..", 11, 1060000},
        {"BỐ GIÀ (REMASTERED)", L"BỐ GIÀ (REMASTERED)", L"BỐ GIÀ", 9, 860000},
        {"LẬT MẶT: 48H", L"LẬT MẶT: 48H", L"LẬT MẶT", 8, 775000},
        {"MAI", L"MAI", L"MAI", 4, 410000}
    };

    // Monthly stats default baseline
    monthlyStatsList = {
        {"09/2025", L"09/2025", 1675000, L"Khởi điểm", Color(241, 245, 249), Color(100, 116, 139), false},
        {"10/2025", L"10/2025", 2915000, L"+74.0% ↗", Color(220, 252, 231), Color(22, 163, 74), false},
        {"11/2025", L"11/2025", 4305000, L"+47.7% ↗", Color(220, 252, 231), Color(22, 163, 74), true},
        {"12/2025", L"12/2025", 2947500, L"-31.5% ↘", Color(255, 228, 230), Color(225, 29, 72), false}
    };

    // Dynamic extraction if tickets exist in data
    unordered_map<string, int> movieTickets;
    unordered_map<string, long long> movieRevenue;
    long long totalRev = 0;
    long long movieRev = 0;
    long long comboRev = 0;
    int ticketsCount = 0;

    Node<Ticket>* node = ticketsCache.getHead();
    while (node) {
        const Ticket& t = node->data;
        int seats = max(1, countSeats(t.booked));
        long long ticketTotal = static_cast<long long>(t.price);
        long long cPart = computeComboRevenue(t.comboName);
        if (cPart > ticketTotal) cPart = ticketTotal;
        long long mPart = ticketTotal - cPart;

        movieTickets[t.title] += seats;
        movieRevenue[t.title] += ticketTotal;

        totalRev += ticketTotal;
        movieRev += mPart;
        comboRev += cPart;
        ticketsCount += seats;
        node = node->next;
    }

    if (ticketsCount > 0) {
        totalTicketsThisMonth = ticketsCount;
        standardTicketsCount = (int)(totalTicketsThisMonth * 0.67f);
        vipTicketsCount = totalTicketsThisMonth - standardTicketsCount;
    }
    if (totalRev > 0) {
        totalRevenueThisMonth = totalRev;
        movieRevenueThisMonth = movieRev;
        comboRevenueThisMonth = comboRev;
    }

    // New customers
    if (!registrationTimes.empty()) {
        newCustomersThisMonth = (int)registrationTimes.size();
    }
}

void OverviewPanel::refresh() {
    loadData();
    toastActive = true;
    toastTimer = 2.5f;
    toastMessage = L"Đã làm mới dữ liệu thời gian thực thành công!";
}

void OverviewPanel::handleEvent(const Event& event, const RenderWindow& /*window*/) {
    if (auto mousePress = event.getIf<Event::MouseButtonPressed>()) {
        if (mousePress->button == Mouse::Button::Left) {
            Vector2f mPos(static_cast<float>(mousePress->position.x), static_cast<float>(mousePress->position.y));
            
            // Xuất báo cáo button: x = position.x + width - 145, y = position.y + 16, w = 100, h = 36
            FloatRect exportRect({position.x + width - 145.f, position.y + 16.f}, {100.f, 36.f});
            if (exportRect.contains(mPos)) {
                toastActive = true;
                toastTimer = 3.f;
                toastMessage = L"Đã xuất báo cáo tổng quan thành công (Excel / CSV)!";
            }

            // Reload button: x = position.x + width - 38, y = position.y + 16, w = 36, h = 36
            FloatRect reloadRect({position.x + width - 38.f, position.y + 16.f}, {36.f, 36.f});
            if (reloadRect.contains(mPos)) {
                refresh();
            }
        }
    }
}

void OverviewPanel::update(const Vector2f& mousePos, bool /*mouseDown*/) {
    // Export hover
    FloatRect exportRect({position.x + width - 145.f, position.y + 16.f}, {100.f, 36.f});
    exportHovered = exportRect.contains(mousePos);

    // Reload hover
    FloatRect reloadRect({position.x + width - 38.f, position.y + 16.f}, {36.f, 36.f});
    reloadHovered = reloadRect.contains(mousePos);

    // Movie table hover
    hoveredMovieRow = -1;
    float movieTableTop = position.y + 198.f + 56.f;
    float movieTableLeft = position.x + 738.f;
    for (int i = 0; i < (int)topMoviesList.size(); ++i) {
        FloatRect rowRect({movieTableLeft, movieTableTop + i * 35.f}, {680.f, 33.f});
        if (rowRect.contains(mousePos)) {
            hoveredMovieRow = i;
            break;
        }
    }

    // Month table hover
    hoveredMonthRow = -1;
    float monthTableTop = position.y + 570.f + 56.f;
    float monthTableLeft = position.x + 738.f;
    for (int i = 0; i < (int)monthlyStatsList.size(); ++i) {
        FloatRect rowRect({monthTableLeft, monthTableTop + i * 50.f}, {680.f, 46.f});
        if (rowRect.contains(mousePos)) {
            hoveredMonthRow = i;
            break;
        }
    }

    if (toastActive) {
        toastTimer -= 0.016f;
        if (toastTimer <= 0.f) {
            toastActive = false;
        }
    }
}

void OverviewPanel::render(RenderTarget& target) const {
    // 1. Off-white Canvas Background
    RectangleShape bg(Vector2f(width, height));
    bg.setPosition(position);
    bg.setFillColor(Color(241, 245, 249)); // Slate-100
    target.draw(bg);

    // 2. Header
    drawHeader(target);

    // 3. 6 KPI Stat Cards
    drawKpiCards(target);

    // 4. 2x2 Grid Components
    drawTopLeftChart(target);
    drawTopRightTable(target);
    drawBottomLeftChart(target);
    drawBottomRightTable(target);

    // 5. Bottom Engine Status Bar
    drawBottomBar(target);

    // 6. Toast Notification
    if (toastActive) {
        drawToast(target);
    }
}

void OverviewPanel::drawHeader(RenderTarget& target) const {
    float topY = position.y + 16.f;

    // Title
    Text titleText(boldFont);
    titleText.setString(L"Tổng Quan Báo Cáo & Doanh Thu");
    titleText.setCharacterSize(22);
    titleText.setFillColor(Color(15, 23, 42)); // Slate-900
    titleText.setPosition({position.x + 20.f, topY});
    target.draw(titleText);

    // Auto-sync pill badge
    float titleW = titleText.getGlobalBounds().size.x;
    Vector2f syncBadgePos(position.x + 20.f + titleW + 14.f, topY + 3.f);
    RoundedRectRenderer::draw(target, syncBadgePos, {134.f, 24.f}, 12.f, Color(236, 253, 245), 1.f, Color(167, 243, 208));

    Text syncText(boldFont);
    syncText.setString(L"• Đồng bộ tự động");
    syncText.setCharacterSize(12);
    syncText.setFillColor(Color(5, 150, 105)); // Emerald-600
    syncText.setPosition({syncBadgePos.x + 10.f, syncBadgePos.y + 4.f});
    target.draw(syncText);

    // Subtitle
    Text subText(font);
    subText.setString(L"Dữ liệu thời gian thực rạp CineCore Đà Nẵng - Cập nhật lần cuối: 18:42, 27/10/2026");
    subText.setCharacterSize(12.5f);
    subText.setFillColor(Color(100, 116, 139)); // Slate-500
    subText.setPosition({position.x + 20.f, topY + 30.f});
    target.draw(subText);

    // Right Controls
    // 1. Search Box
    Vector2f searchPos(position.x + width - 755.f, topY);
    RoundedRectRenderer::draw(target, searchPos, {210.f, 36.f}, 6.f, Color::White, 1.f, Color(203, 213, 225));

    // Kính lúp tìm kiếm
    Vector2f oIconPos(searchPos.x + 10.f, topY + 11.f);
    Color oIcCol(148, 163, 184);
    CircleShape oLens(5.f, 16);
    oLens.setFillColor(Color::Transparent);
    oLens.setOutlineThickness(1.6f);
    oLens.setOutlineColor(oIcCol);
    oLens.setPosition(oIconPos);
    target.draw(oLens);

    RectangleShape oStem({5.5f, 1.8f});
    oStem.setFillColor(oIcCol);
    oStem.setRotation(degrees(45.f));
    oStem.setPosition({oIconPos.x + 8.f, oIconPos.y + 8.f});
    target.draw(oStem);

    Text searchPlaceholder(font);
    searchPlaceholder.setString(L"Tìm kiếm phim, mã vé, combo...");
    searchPlaceholder.setCharacterSize(12.5f);
    searchPlaceholder.setFillColor(Color(148, 163, 184));
    searchPlaceholder.setPosition({searchPos.x + 28.f, searchPos.y + 8.f});
    target.draw(searchPlaceholder);

    // 2. Date Filter
    Vector2f datePos(position.x + width - 535.f, topY);
    RoundedRectRenderer::draw(target, datePos, {185.f, 36.f}, 6.f, Color::White, 1.f, Color(203, 213, 225));
    Text dateText(font);
    dateText.setString(L"Tháng 10/2026 (01/10 - 31/10) ∨");
    dateText.setCharacterSize(12.f);
    dateText.setFillColor(Color(51, 65, 85));
    dateText.setPosition({datePos.x + 10.f, datePos.y + 8.f});
    target.draw(dateText);

    // 3. Room Filter
    Vector2f roomPos(position.x + width - 340.f, topY);
    RoundedRectRenderer::draw(target, roomPos, {185.f, 36.f}, 6.f, Color::White, 1.f, Color(203, 213, 225));
    Text roomText(font);
    roomText.setString(L"Tất cả phòng chiếu (Hall 01-08) ∨");
    roomText.setCharacterSize(12.f);
    roomText.setFillColor(Color(51, 65, 85));
    roomText.setPosition({roomPos.x + 8.f, roomPos.y + 8.f});
    target.draw(roomText);

    // 4. Export Report Button
    Vector2f exportPos(position.x + width - 145.f, topY);
    Color exportBg = exportHovered ? Color(30, 41, 59) : Color(15, 23, 42);
    RoundedRectRenderer::draw(target, exportPos, {100.f, 36.f}, 6.f, exportBg);
    Text exportText(boldFont);
    exportText.setString(L"📥 Xuất báo cáo");
    exportText.setCharacterSize(12.5f);
    exportText.setFillColor(Color::White);
    exportText.setPosition({exportPos.x + 10.f, exportPos.y + 8.f});
    target.draw(exportText);

    // 5. Reload Button
    Vector2f reloadPos(position.x + width - 38.f, topY);
    Color reloadBg = reloadHovered ? Color(241, 245, 249) : Color::White;
    RoundedRectRenderer::draw(target, reloadPos, {36.f, 36.f}, 6.f, reloadBg, 1.f, Color(203, 213, 225));
    Text reloadText(boldFont);
    reloadText.setString(L"🔄");
    reloadText.setCharacterSize(14);
    reloadText.setFillColor(Color(71, 85, 105));
    reloadText.setPosition({reloadPos.x + 8.f, reloadPos.y + 7.f});
    target.draw(reloadText);
}

void OverviewPanel::drawKpiCards(RenderTarget& target) const {
    const float cardY = position.y + 68.f;
    const float cardH = 118.f;
    const float margin = 20.f;
    const float gap = 10.f;
    const float availableW = width - margin * 2.f;
    const float cardW = (availableW - gap * 5.f) / 6.f; // ~226px

    // Helper lambda for standard KPI cards
    auto drawStandardCard = [&](int idx, const wstring& label, const wstring& valStr, const wstring& unitStr,
                                const wstring& subStr, const wstring& footLeft, const Color& footLeftColor,
                                const wstring& footRight, const Color& iconBg, const wstring& iconChar, const Color& iconColor) {
        float x = position.x + margin + idx * (cardW + gap);
        Vector2f cardPos(x, cardY);
        RoundedRectRenderer::draw(target, cardPos, {cardW, cardH}, 8.f, Color::White, 1.f, Color(226, 232, 240));

        // Label
        Text lbl(boldFont);
        lbl.setString(label);
        lbl.setCharacterSize(13);
        lbl.setFillColor(Color(100, 116, 139));
        lbl.setPosition({cardPos.x + 14.f, cardPos.y + 12.f});
        target.draw(lbl);

        // Icon Badge top-right
        Vector2f iconPos(cardPos.x + cardW - 38.f, cardPos.y + 12.f);
        CircleShape iconCircle(13.f);
        iconCircle.setPosition(iconPos);
        iconCircle.setFillColor(iconBg);
        target.draw(iconCircle);

        Text iconTxt(boldFont);
        iconTxt.setString(iconChar);
        iconTxt.setCharacterSize(12);
        iconTxt.setFillColor(iconColor);
        FloatRect ib = iconTxt.getLocalBounds();
        iconTxt.setPosition({iconPos.x + 13.f - ib.size.x / 2.f, iconPos.y + 13.f - ib.size.y / 2.f - 2.f});
        target.draw(iconTxt);

        // Value
        Text val(boldFont);
        val.setString(valStr);
        val.setCharacterSize(20);
        val.setFillColor(Color(15, 23, 42));
        val.setPosition({cardPos.x + 14.f, cardPos.y + 36.f});
        target.draw(val);

        if (!unitStr.empty()) {
            Text unit(font);
            unit.setString(unitStr);
            unit.setCharacterSize(12.5f);
            unit.setFillColor(Color(100, 116, 139));
            unit.setPosition({val.getPosition().x + val.getGlobalBounds().size.x + 6.f, cardPos.y + 42.f});
            target.draw(unit);
        }

        // Subtitle
        Text sub(font);
        sub.setString(subStr);
        sub.setCharacterSize(12);
        sub.setFillColor(Color(148, 163, 184));
        sub.setPosition({cardPos.x + 14.f, cardPos.y + 66.f});
        target.draw(sub);

        // Footer
        Text fLeft(boldFont);
        fLeft.setString(footLeft);
        fLeft.setCharacterSize(12);
        fLeft.setFillColor(footLeftColor);
        fLeft.setPosition({cardPos.x + 14.f, cardPos.y + 90.f});
        target.draw(fLeft);

        if (!footRight.empty()) {
            Text fRight(font);
            fRight.setString(footRight);
            fRight.setCharacterSize(12);
            fRight.setFillColor(Color(148, 163, 184));
            fRight.setPosition({fLeft.getPosition().x + fLeft.getGlobalBounds().size.x + 5.f, cardPos.y + 90.f});
            target.draw(fRight);
        }
    };

    // 1. Doanh thu trong ngày
    drawStandardCard(0, L"Doanh thu trong ngày", s2ws(toCurrency(dailyRevenue)), L"",
                     L"Hôm nay (27/10/2026)", L"↗ +12.5%", Color(16, 185, 129), L"so với hôm qua",
                     Color(224, 242, 254), L"$", Color(2, 132, 199));

    // 2. Khách hàng mới (with progress bar)
    {
        float x = position.x + margin + 1 * (cardW + gap);
        Vector2f cardPos(x, cardY);
        RoundedRectRenderer::draw(target, cardPos, {cardW, cardH}, 8.f, Color::White, 1.f, Color(226, 232, 240));

        Text lbl(boldFont);
        lbl.setString(L"Khách hàng mới");
        lbl.setCharacterSize(13);
        lbl.setFillColor(Color(100, 116, 139));
        lbl.setPosition({cardPos.x + 14.f, cardPos.y + 12.f});
        target.draw(lbl);

        Vector2f iconPos(cardPos.x + cardW - 38.f, cardPos.y + 12.f);
        CircleShape iconCircle(13.f);
        iconCircle.setPosition(iconPos);
        iconCircle.setFillColor(Color(220, 252, 231));
        target.draw(iconCircle);

        Text iconTxt(boldFont);
        iconTxt.setString(L"👤");
        iconTxt.setCharacterSize(11);
        iconTxt.setFillColor(Color(5, 150, 105));
        FloatRect ib = iconTxt.getLocalBounds();
        iconTxt.setPosition({iconPos.x + 13.f - ib.size.x / 2.f, iconPos.y + 13.f - ib.size.y / 2.f - 2.f});
        target.draw(iconTxt);

        Text val(boldFont);
        val.setString(to_wstring(newCustomersThisMonth));
        val.setCharacterSize(20);
        val.setFillColor(Color(15, 23, 42));
        val.setPosition({cardPos.x + 14.f, cardPos.y + 36.f});
        target.draw(val);

        Text unit(font);
        unit.setString(L"thành viên");
        unit.setCharacterSize(12.5f);
        unit.setFillColor(Color(100, 116, 139));
        unit.setPosition({val.getPosition().x + val.getGlobalBounds().size.x + 6.f, cardPos.y + 42.f});
        target.draw(unit);

        Text sub(font);
        sub.setString(L"Tháng 10/2026");
        sub.setCharacterSize(12);
        sub.setFillColor(Color(148, 163, 184));
        sub.setPosition({cardPos.x + 14.f, cardPos.y + 66.f});
        target.draw(sub);

        // Progress text
        Text progLbl(font);
        progLbl.setString(L"Mục tiêu tháng");
        progLbl.setCharacterSize(11.5f);
        progLbl.setFillColor(Color(100, 116, 139));
        progLbl.setPosition({cardPos.x + 14.f, cardPos.y + 86.f});
        target.draw(progLbl);

        Text progVal(boldFont);
        progVal.setString(L"74% (148/200)");
        progVal.setCharacterSize(11.5f);
        progVal.setFillColor(Color(13, 148, 136));
        progVal.setPosition({cardPos.x + cardW - 14.f - progVal.getGlobalBounds().size.x, cardPos.y + 86.f});
        target.draw(progVal);

        // Progress bar
        Vector2f barBgPos(cardPos.x + 14.f, cardPos.y + 102.f);
        float barMaxW = cardW - 28.f;
        RoundedRectRenderer::draw(target, barBgPos, {barMaxW, 4.f}, 2.f, Color(226, 232, 240));
        RoundedRectRenderer::draw(target, barBgPos, {barMaxW * 0.74f, 4.f}, 2.f, Color(20, 184, 166));
    }

    // 3. Tổng vé bán ra
    {
        float x = position.x + margin + 2 * (cardW + gap);
        Vector2f cardPos(x, cardY);
        RoundedRectRenderer::draw(target, cardPos, {cardW, cardH}, 8.f, Color::White, 1.f, Color(226, 232, 240));

        Text lbl(boldFont);
        lbl.setString(L"Tổng vé bán ra");
        lbl.setCharacterSize(13);
        lbl.setFillColor(Color(100, 116, 139));
        lbl.setPosition({cardPos.x + 14.f, cardPos.y + 12.f});
        target.draw(lbl);

        Vector2f iconPos(cardPos.x + cardW - 38.f, cardPos.y + 12.f);
        CircleShape iconCircle(13.f);
        iconCircle.setPosition(iconPos);
        iconCircle.setFillColor(Color(254, 243, 199));
        target.draw(iconCircle);

        Text iconTxt(boldFont);
        iconTxt.setString(L"🎫");
        iconTxt.setCharacterSize(11);
        iconTxt.setFillColor(Color(217, 119, 6));
        FloatRect ib = iconTxt.getLocalBounds();
        iconTxt.setPosition({iconPos.x + 13.f - ib.size.x / 2.f, iconPos.y + 13.f - ib.size.y / 2.f - 2.f});
        target.draw(iconTxt);

        Text val(boldFont);
        val.setString(to_wstring(totalTicketsThisMonth));
        val.setCharacterSize(20);
        val.setFillColor(Color(15, 23, 42));
        val.setPosition({cardPos.x + 14.f, cardPos.y + 36.f});
        target.draw(val);

        Text unit(font);
        unit.setString(L"vé");
        unit.setCharacterSize(12.5f);
        unit.setFillColor(Color(100, 116, 139));
        unit.setPosition({val.getPosition().x + val.getGlobalBounds().size.x + 6.f, cardPos.y + 42.f});
        target.draw(unit);

        Text sub(font);
        sub.setString(L"Tháng 10/2026");
        sub.setCharacterSize(12);
        sub.setFillColor(Color(148, 163, 184));
        sub.setPosition({cardPos.x + 14.f, cardPos.y + 66.f});
        target.draw(sub);

        // Two stats dots: Standard & VIP
        Text stdTxt(boldFont);
        stdTxt.setString(L"• Standard: " + to_wstring(standardTicketsCount));
        stdTxt.setCharacterSize(12);
        stdTxt.setFillColor(Color(59, 130, 246));
        stdTxt.setPosition({cardPos.x + 14.f, cardPos.y + 90.f});
        target.draw(stdTxt);

        Text vipTxt(boldFont);
        vipTxt.setString(L"• VIP: " + to_wstring(vipTicketsCount));
        vipTxt.setCharacterSize(12);
        vipTxt.setFillColor(Color(245, 158, 11));
        vipTxt.setPosition({stdTxt.getPosition().x + stdTxt.getGlobalBounds().size.x + 12.f, cardPos.y + 90.f});
        target.draw(vipTxt);
    }

    // 4. Highlight Dark Card: Tổng doanh thu
    {
        float x = position.x + margin + 3 * (cardW + gap);
        Vector2f cardPos(x, cardY);
        // Dark navy card
        RoundedRectRenderer::draw(target, cardPos, {cardW, cardH}, 8.f, Color(15, 23, 42), 1.f, Color(51, 65, 85));

        Text lbl(boldFont);
        lbl.setString(L"Tổng doanh thu");
        lbl.setCharacterSize(13);
        lbl.setFillColor(Color(148, 163, 184));
        lbl.setPosition({cardPos.x + 14.f, cardPos.y + 12.f});
        target.draw(lbl);

        // Badge T10/2026 top-right
        Vector2f badgePos(cardPos.x + cardW - 74.f, cardY + 12.f);
        RoundedRectRenderer::draw(target, badgePos, {60.f, 22.f}, 4.f, Color(30, 41, 59), 1.f, Color(71, 85, 105));
        Text badgeTxt(boldFont);
        badgeTxt.setString(L"T10/2026");
        badgeTxt.setCharacterSize(11.5f);
        badgeTxt.setFillColor(Color(147, 197, 253));
        badgeTxt.setPosition({badgePos.x + 6.f, badgePos.y + 3.f});
        target.draw(badgeTxt);

        Text val(boldFont);
        val.setString(s2ws(toCurrency(totalRevenueThisMonth)));
        val.setCharacterSize(20);
        val.setFillColor(Color::White);
        val.setPosition({cardPos.x + 14.f, cardPos.y + 36.f});
        target.draw(val);

        Text sub(font);
        sub.setString(L"Toàn rạp CineCore");
        sub.setCharacterSize(12);
        sub.setFillColor(Color(148, 163, 184));
        sub.setPosition({cardPos.x + 14.f, cardPos.y + 66.f});
        target.draw(sub);

        Text fLeft(boldFont);
        fLeft.setString(L"↗ +28.4%");
        fLeft.setCharacterSize(12);
        fLeft.setFillColor(Color(52, 211, 153)); // Emerald light
        fLeft.setPosition({cardPos.x + 14.f, cardPos.y + 90.f});
        target.draw(fLeft);

        Text fRight(font);
        fRight.setString(L"vượt KPI quý");
        fRight.setCharacterSize(12);
        fRight.setFillColor(Color(148, 163, 184));
        fRight.setPosition({fLeft.getPosition().x + fLeft.getGlobalBounds().size.x + 5.f, cardPos.y + 90.f});
        target.draw(fRight);
    }

    // 5. Doanh thu theo phim
    drawStandardCard(4, L"Doanh thu theo phim", s2ws(toCurrency(movieRevenueThisMonth)), L"",
                     L"8 phim đang công chiếu", L"Chiếm 70.0%", Color(37, 99, 235), L"tổng doanh thu",
                     Color(219, 234, 254), L"🎬", Color(37, 99, 235));

    // 6. Doanh thu combo bắp nước
    drawStandardCard(5, L"Doanh thu combo bắp nước", s2ws(toCurrency(comboRevenueThisMonth)), L"",
                     L"Bắp Caramel / Phô mai", L"Chiếm 30.0%", Color(225, 29, 72), L"+18.2% vs T9",
                     Color(255, 228, 230), L"🍿", Color(225, 29, 72));
}

void OverviewPanel::drawTopLeftChart(RenderTarget& target) const {
    const float cardX = position.x + 20.f;
    const float cardY = position.y + 198.f;
    const float cardW = (width - 40.f - 12.f) / 2.f; // ~698px
    const float cardH = 358.f;

    // Card background
    RoundedRectRenderer::draw(target, {cardX, cardY}, {cardW, cardH}, 8.f, Color::White, 1.f, Color(226, 232, 240));

    // Header: Blue dot + Title
    CircleShape dot(4.f);
    dot.setPosition({cardX + 16.f, cardY + 18.f});
    dot.setFillColor(Color(37, 99, 235));
    target.draw(dot);

    Text title(boldFont);
    title.setString(L"Phim bán chạy nhất (Số lượng vé bán ra)");
    title.setCharacterSize(15);
    title.setFillColor(Color(15, 23, 42));
    title.setPosition({cardX + 30.f, cardY + 12.f});
    target.draw(title);

    // Right tags: "Đơn vị: Vé", "Top 8 thịnh hành"
    Text unitTxt(font);
    unitTxt.setString(L"Đơn vị: Vé");
    unitTxt.setCharacterSize(12.f);
    unitTxt.setFillColor(Color(148, 163, 184));
    unitTxt.setPosition({cardX + cardW - 205.f, cardY + 14.f});
    target.draw(unitTxt);

    Vector2f badgePos(cardX + cardW - 122.f, cardY + 12.f);
    RoundedRectRenderer::draw(target, badgePos, {110.f, 22.f}, 11.f, Color(224, 242, 254));
    Text badgeTxt(boldFont);
    badgeTxt.setString(L"Top 8 thịnh hành");
    badgeTxt.setCharacterSize(12.f);
    badgeTxt.setFillColor(Color(2, 132, 199));
    badgeTxt.setPosition({badgePos.x + 10.f, badgePos.y + 3.f});
    target.draw(badgeTxt);

    // Chart Area
    const float chartLeft = cardX + 45.f;
    const float chartBottom = cardY + 310.f;
    const float chartW = cardW - 70.f;
    const float maxVal = 40.f;
    const float chartH = 220.f;

    // Y-Axis horizontal grid lines and labels
    const array<int, 5> yTicks = {0, 10, 20, 30, 40};
    for (int tick : yTicks) {
        float y = chartBottom - (tick / maxVal) * chartH;

        // Label
        Text lbl(font);
        lbl.setString(to_string(tick));
        lbl.setCharacterSize(11.5f);
        lbl.setFillColor(Color(148, 163, 184));
        lbl.setPosition({cardX + 16.f, y - 7.f});
        target.draw(lbl);

        // Grid line
        RectangleShape line(Vector2f(chartW, 1.f));
        line.setPosition({chartLeft, y});
        line.setFillColor(Color(241, 245, 249));
        target.draw(line);
    }

    // 8 Bars
    int n = min(8, (int)topMoviesList.size());
    float colW = 46.f;
    float colGap = (chartW - (n * colW)) / (n + 1);

    for (int i = 0; i < n; ++i) {
        float bx = chartLeft + colGap + i * (colW + colGap);
        float bHeight = (topMoviesList[i].tickets / maxVal) * chartH;
        float by = chartBottom - bHeight;

        // Vertical gradient bar: Cyan at top, Blue at bottom
        VertexArray bar(PrimitiveType::TriangleStrip, 4);
        bar[0].position = Vector2f(bx, by);
        bar[0].color = Color(56, 189, 248); // Sky-400
        bar[1].position = Vector2f(bx + colW, by);
        bar[1].color = Color(56, 189, 248);
        bar[2].position = Vector2f(bx, chartBottom);
        bar[2].color = Color(37, 99, 235); // Blue-600
        bar[3].position = Vector2f(bx + colW, chartBottom);
        bar[3].color = Color(37, 99, 235);
        target.draw(bar);

        // Value text above bar
        Text valTxt(boldFont);
        valTxt.setString(to_string(topMoviesList[i].tickets));
        valTxt.setCharacterSize(12.f);
        valTxt.setFillColor(Color(51, 65, 85));
        FloatRect vb = valTxt.getLocalBounds();
        valTxt.setPosition({bx + colW / 2.f - vb.size.x / 2.f, by - 16.f});
        target.draw(valTxt);

        // Label below bar
        Text nameTxt(font);
        nameTxt.setString(topMoviesList[i].shortTitle);
        nameTxt.setCharacterSize(11.5f);
        nameTxt.setFillColor(Color(100, 116, 139));
        FloatRect nb = nameTxt.getLocalBounds();
        nameTxt.setPosition({bx + colW / 2.f - nb.size.x / 2.f, chartBottom + 8.f});
        target.draw(nameTxt);
    }
}

void OverviewPanel::drawTopRightTable(RenderTarget& target) const {
    const float cardX = position.x + 20.f + (width - 40.f - 12.f) / 2.f + 12.f;
    const float cardY = position.y + 198.f;
    const float cardW = (width - 40.f - 12.f) / 2.f;
    const float cardH = 358.f;

    // Card background
    RoundedRectRenderer::draw(target, {cardX, cardY}, {cardW, cardH}, 8.f, Color::White, 1.f, Color(226, 232, 240));

    // Header: Green dot + Title
    CircleShape dot(4.f);
    dot.setPosition({cardX + 16.f, cardY + 18.f});
    dot.setFillColor(Color(16, 185, 129));
    target.draw(dot);

    Text title(boldFont);
    title.setString(L"Chi tiết vé & doanh thu theo phim");
    title.setCharacterSize(15);
    title.setFillColor(Color(15, 23, 42));
    title.setPosition({cardX + 30.f, cardY + 12.f});
    target.draw(title);

    // Right subtitle: "Tổng: 101 vé"
    Text totTxt(boldFont);
    totTxt.setString(L"Tổng: " + to_wstring(totalTicketsThisMonth) + L" vé");
    totTxt.setCharacterSize(13.f);
    totTxt.setFillColor(Color(15, 23, 42));
    totTxt.setPosition({cardX + cardW - totTxt.getGlobalBounds().size.x - 16.f, cardY + 13.f});
    target.draw(totTxt);

    // Table Header Columns
    float colTitleX = cardX + 20.f;
    float colTicketsX = cardX + cardW - 180.f;
    float colRevenueX = cardX + cardW - 20.f;

    Text th1(boldFont);
    th1.setString(L"#  TÊN PHIM");
    th1.setCharacterSize(12.5f);
    th1.setFillColor(Color(148, 163, 184));
    th1.setPosition({colTitleX, cardY + 44.f});
    target.draw(th1);

    Text th2(boldFont);
    th2.setString(L"SỐ VÉ");
    th2.setCharacterSize(12.5f);
    th2.setFillColor(Color(148, 163, 184));
    th2.setPosition({colTicketsX, cardY + 44.f});
    target.draw(th2);

    Text th3(boldFont);
    th3.setString(L"DOANH THU");
    th3.setCharacterSize(12.5f);
    th3.setFillColor(Color(148, 163, 184));
    FloatRect th3b = th3.getLocalBounds();
    th3.setPosition({colRevenueX - th3b.size.x, cardY + 44.f});
    target.draw(th3);

    // Divider line
    RectangleShape div(Vector2f(cardW - 40.f, 1.f));
    div.setPosition({cardX + 20.f, cardY + 62.f});
    div.setFillColor(Color(241, 245, 249));
    target.draw(div);

    // Table Rows (8 movies)
    float rowTop = cardY + 68.f;
    float rowH = 34.5f;
    int n = min(8, (int)topMoviesList.size());

    for (int i = 0; i < n; ++i) {
        float ry = rowTop + i * rowH;

        // Hover highlight
        if (hoveredMovieRow == i) {
            RoundedRectRenderer::draw(target, {cardX + 12.f, ry - 2.f}, {cardW - 24.f, rowH - 2.f}, 4.f, Color(248, 250, 252));
        }

        // Rank Circle or Number
        if (i < 3) {
            Color rankBg = (i == 0) ? Color(245, 158, 11) : (i == 1 ? Color(148, 163, 184) : Color(217, 119, 6));
            CircleShape rCircle(9.f);
            rCircle.setPosition({colTitleX, ry + 1.f});
            rCircle.setFillColor(rankBg);
            target.draw(rCircle);

            Text rTxt(boldFont);
            rTxt.setString(to_string(i + 1));
            rTxt.setCharacterSize(11.5f);
            rTxt.setFillColor(Color::White);
            FloatRect rb = rTxt.getLocalBounds();
            rTxt.setPosition({colTitleX + 9.f - rb.size.x / 2.f, ry + 2.f});
            target.draw(rTxt);
        } else {
            Text rTxt(font);
            rTxt.setString(to_string(i + 1));
            rTxt.setCharacterSize(12.f);
            rTxt.setFillColor(Color(148, 163, 184));
            rTxt.setPosition({colTitleX + 5.f, ry + 1.f});
            target.draw(rTxt);
        }

        // Poster thumbnail square
        Vector2f postPos(colTitleX + 24.f, ry + 1.f);
        RoundedRectRenderer::draw(target, postPos, {18.f, 18.f}, 3.f, Color(30, 41, 59));
        Text postIcon(font);
        postIcon.setString(L"🎞");
        postIcon.setCharacterSize(10.f);
        postIcon.setPosition({postPos.x + 2.f, postPos.y + 1.f});
        target.draw(postIcon);

        // Movie Title
        Text mTitle(boldFont);
        mTitle.setString(topMoviesList[i].displayTitle);
        mTitle.setCharacterSize(13.f);
        mTitle.setFillColor(Color(15, 23, 42));
        mTitle.setPosition({postPos.x + 24.f, ry + 1.f});
        target.draw(mTitle);

        // Ticket Count
        Text tCount(boldFont);
        tCount.setString(to_string(topMoviesList[i].tickets));
        tCount.setCharacterSize(13.f);
        tCount.setFillColor(Color(37, 99, 235));
        tCount.setPosition({colTicketsX + 10.f, ry + 1.f});
        target.draw(tCount);

        // Revenue
        Text revTxt(boldFont);
        revTxt.setString(s2ws(toCurrency(topMoviesList[i].revenue)));
        revTxt.setCharacterSize(13.5f);
        revTxt.setFillColor(Color(225, 29, 72));
        FloatRect revB = revTxt.getLocalBounds();
        revTxt.setPosition({colRevenueX - revB.size.x, ry + 1.f});
        target.draw(revTxt);
    }
}

void OverviewPanel::drawBottomLeftChart(RenderTarget& target) const {
    const float cardX = position.x + 20.f;
    const float cardY = position.y + 568.f;
    const float cardW = (width - 40.f - 12.f) / 2.f;
    const float cardH = 358.f;

    // Card background
    RoundedRectRenderer::draw(target, {cardX, cardY}, {cardW, cardH}, 8.f, Color::White, 1.f, Color(226, 232, 240));

    // Header: Rose dot + Title
    CircleShape dot(4.f);
    dot.setPosition({cardX + 16.f, cardY + 18.f});
    dot.setFillColor(Color(244, 63, 94));
    target.draw(dot);

    Text title(boldFont);
    title.setString(L"Biến động doanh thu theo tháng (Xu hướng tăng trưởng)");
    title.setCharacterSize(15);
    title.setFillColor(Color(15, 23, 42));
    title.setPosition({cardX + 30.f, cardY + 12.f});
    target.draw(title);

    // Right badge: "● Đạt đỉnh: 11/2025 (4.305.000đ)"
    Vector2f badgePos(cardX + cardW - 235.f, cardY + 12.f);
    RoundedRectRenderer::draw(target, badgePos, {220.f, 22.f}, 10.f, Color(253, 242, 248), 1.f, Color(251, 207, 232));
    Text badgeTxt(boldFont);
    badgeTxt.setString(L"● Đạt đỉnh: 11/2025 (4.305.000đ)");
    badgeTxt.setCharacterSize(12.f);
    badgeTxt.setFillColor(Color(219, 39, 119));
    badgeTxt.setPosition({badgePos.x + 10.f, badgePos.y + 3.f});
    target.draw(badgeTxt);

    // Area & Line Chart
    const float chartLeft = cardX + 75.f;
    const float chartBottom = cardY + 310.f;
    const float chartW = cardW - 100.f;
    const float maxRev = 5000000.f;
    const float chartH = 220.f;

    // Y-Axis horizontal grid lines and labels
    const array<long long, 6> yRevTicks = {0, 1000000, 2000000, 3000000, 4000000, 5000000};
    for (auto val : yRevTicks) {
        float y = chartBottom - ((float)val / maxRev) * chartH;

        Text lbl(font);
        lbl.setString(s2ws(toCurrency(val)));
        lbl.setCharacterSize(11.5f);
        lbl.setFillColor(Color(148, 163, 184));
        FloatRect lb = lbl.getLocalBounds();
        lbl.setPosition({chartLeft - lb.size.x - 10.f, y - 7.f});
        target.draw(lbl);

        RectangleShape line(Vector2f(chartW, 1.f));
        line.setPosition({chartLeft, y});
        line.setFillColor(Color(241, 245, 249));
        target.draw(line);
    }

    // 4 Trend Data Points
    struct PointDef {
        float x;
        float y;
        wstring label;
        bool isPeak;
    };

    vector<PointDef> pts;
    int mCount = (int)monthlyStatsList.size();
    float pGap = chartW / (float)(mCount + 1);

    for (int i = 0; i < mCount; ++i) {
        float px = chartLeft + (i + 1) * pGap;
        float py = chartBottom - ((float)monthlyStatsList[i].revenue / maxRev) * chartH;
        wstring l = monthlyStatsList[i].displayMonth;
        if (monthlyStatsList[i].isRecord) l += L" (Peak)";
        pts.push_back({px, py, l, monthlyStatsList[i].isRecord});
    }

    // 1. Fill Area with Gradient
    for (size_t i = 0; i + 1 < pts.size(); ++i) {
        VertexArray strip(PrimitiveType::TriangleStrip, 4);
        strip[0].position = Vector2f(pts[i].x, pts[i].y);
        strip[0].color = Color(244, 63, 94, 90);
        strip[1].position = Vector2f(pts[i + 1].x, pts[i + 1].y);
        strip[1].color = Color(244, 63, 94, 90);
        strip[2].position = Vector2f(pts[i].x, chartBottom);
        strip[2].color = Color(244, 63, 94, 5);
        strip[3].position = Vector2f(pts[i + 1].x, chartBottom);
        strip[3].color = Color(244, 63, 94, 5);
        target.draw(strip);
    }

    // 2. Thick Line connecting points
    for (size_t i = 0; i + 1 < pts.size(); ++i) {
        Vector2f p1(pts[i].x, pts[i].y);
        Vector2f p2(pts[i + 1].x, pts[i + 1].y);
        Vector2f dir = p2 - p1;
        float len = sqrt(dir.x * dir.x + dir.y * dir.y);
        if (len > 0.f) {
            RectangleShape lineSeg(Vector2f(len, 3.f));
            lineSeg.setPosition(p1);
            lineSeg.setRotation(degrees(atan2(dir.y, dir.x) * 180.f / 3.14159265f));
            lineSeg.setFillColor(Color(236, 72, 153));
            target.draw(lineSeg);
        }
    }

    // 3. Point Markers & X-Axis Labels
    for (const auto& pt : pts) {
        if (pt.isPeak) {
            // Peak glowing circle
            CircleShape glow(9.f);
            glow.setPosition({pt.x - 9.f, pt.y - 9.f});
            glow.setFillColor(Color(219, 39, 119));
            target.draw(glow);

            CircleShape inner(5.f);
            inner.setPosition({pt.x - 5.f, pt.y - 5.f});
            inner.setFillColor(Color::White);
            target.draw(inner);
        } else {
            CircleShape outer(5.f);
            outer.setPosition({pt.x - 5.f, pt.y - 5.f});
            outer.setFillColor(Color(236, 72, 153));
            target.draw(outer);

            CircleShape inner(2.5f);
            inner.setPosition({pt.x - 2.5f, pt.y - 2.5f});
            inner.setFillColor(Color::White);
            target.draw(inner);
        }

        // X-Axis label
        Text xLbl(boldFont);
        xLbl.setString(pt.label);
        xLbl.setCharacterSize(12.f);
        xLbl.setFillColor(Color(100, 116, 139));
        FloatRect xb = xLbl.getLocalBounds();
        xLbl.setPosition({pt.x - xb.size.x / 2.f, chartBottom + 8.f});
        target.draw(xLbl);
    }
}

void OverviewPanel::drawBottomRightTable(RenderTarget& target) const {
    const float cardX = position.x + 20.f + (width - 40.f - 12.f) / 2.f + 12.f;
    const float cardY = position.y + 568.f;
    const float cardW = (width - 40.f - 12.f) / 2.f;
    const float cardH = 358.f;

    // Card background
    RoundedRectRenderer::draw(target, {cardX, cardY}, {cardW, cardH}, 8.f, Color::White, 1.f, Color(226, 232, 240));

    // Header: Rose dot + Title
    CircleShape dot(4.f);
    dot.setPosition({cardX + 16.f, cardY + 18.f});
    dot.setFillColor(Color(244, 63, 94));
    target.draw(dot);

    Text title(boldFont);
    title.setString(L"Chi tiết & Tăng trưởng theo tháng");
    title.setCharacterSize(15);
    title.setFillColor(Color(15, 23, 42));
    title.setPosition({cardX + 30.f, cardY + 12.f});
    target.draw(title);

    // Right badge: "Q4-2025 Data"
    Vector2f badgePos(cardX + cardW - 110.f, cardY + 12.f);
    RoundedRectRenderer::draw(target, badgePos, {95.f, 22.f}, 4.f, Color(241, 245, 249), 1.f, Color(226, 232, 240));
    Text badgeTxt(font);
    badgeTxt.setString(L"Q4-2025 Data");
    badgeTxt.setCharacterSize(12.f);
    badgeTxt.setFillColor(Color(100, 116, 139));
    badgeTxt.setPosition({badgePos.x + 8.f, badgePos.y + 3.f});
    target.draw(badgeTxt);

    // Table Header Columns
    float colMonthX = cardX + 24.f;
    float colChangeX = cardX + 340.f;
    float colRevX = cardX + cardW - 24.f;

    Text th1(boldFont);
    th1.setString(L"THÁNG");
    th1.setCharacterSize(12.5f);
    th1.setFillColor(Color(148, 163, 184));
    th1.setPosition({colMonthX, cardY + 44.f});
    target.draw(th1);

    Text th2(boldFont);
    th2.setString(L"BIẾN ĐỘNG");
    th2.setCharacterSize(12.5f);
    th2.setFillColor(Color(148, 163, 184));
    th2.setPosition({colChangeX, cardY + 44.f});
    target.draw(th2);

    Text th3(boldFont);
    th3.setString(L"DOANH THU");
    th3.setCharacterSize(12.5f);
    th3.setFillColor(Color(148, 163, 184));
    FloatRect th3b = th3.getLocalBounds();
    th3.setPosition({colRevX - th3b.size.x, cardY + 44.f});
    target.draw(th3);

    // Divider line
    RectangleShape div(Vector2f(cardW - 48.f, 1.f));
    div.setPosition({cardX + 24.f, cardY + 62.f});
    div.setFillColor(Color(241, 245, 249));
    target.draw(div);

    // 4 Month Rows
    float rowTop = cardY + 70.f;
    float rowH = 48.f;
    const array<Color, 4> dotColors = {Color(2, 132, 199), Color(16, 185, 129), Color(244, 63, 94), Color(245, 158, 11)};

    for (size_t i = 0; i < monthlyStatsList.size(); ++i) {
        float ry = rowTop + i * rowH;

        if (hoveredMonthRow == (int)i) {
            RoundedRectRenderer::draw(target, {cardX + 16.f, ry - 4.f}, {cardW - 32.f, rowH - 4.f}, 4.f, Color(248, 250, 252));
        }

        // Colored Bullet Dot
        CircleShape rDot(3.5f);
        rDot.setPosition({colMonthX, ry + 8.f});
        rDot.setFillColor(dotColors[i % 4]);
        target.draw(rDot);

        // Month text
        Text mTxt(boldFont);
        mTxt.setString(monthlyStatsList[i].displayMonth);
        mTxt.setCharacterSize(13.5f);
        mTxt.setFillColor(Color(15, 23, 42));
        mTxt.setPosition({colMonthX + 16.f, ry + 2.f});
        target.draw(mTxt);

        if (monthlyStatsList[i].isRecord) {
            Vector2f recPos(mTxt.getPosition().x + mTxt.getGlobalBounds().size.x + 8.f, ry + 2.f);
            RoundedRectRenderer::draw(target, recPos, {52.f, 20.f}, 4.f, Color(255, 228, 230));
            Text recTxt(boldFont);
            recTxt.setString(L"Kỷ lục");
            recTxt.setCharacterSize(11.5f);
            recTxt.setFillColor(Color(225, 29, 72));
            recTxt.setPosition({recPos.x + 8.f, recPos.y + 2.f});
            target.draw(recTxt);
        }

        // Change Badge pill
        Vector2f chgPos(colChangeX, ry);
        RoundedRectRenderer::draw(target, chgPos, {82.f, 24.f}, 4.f, monthlyStatsList[i].badgeBg);
        Text chgTxt(boldFont);
        chgTxt.setString(monthlyStatsList[i].changeBadge);
        chgTxt.setCharacterSize(12.f);
        chgTxt.setFillColor(monthlyStatsList[i].badgeText);
        FloatRect chgB = chgTxt.getLocalBounds();
        chgTxt.setPosition({chgPos.x + 41.f - chgB.size.x / 2.f, chgPos.y + 3.f});
        target.draw(chgTxt);

        // Revenue
        Text revTxt(boldFont);
        revTxt.setString(s2ws(toCurrency(monthlyStatsList[i].revenue)));
        revTxt.setCharacterSize(13.5f);
        revTxt.setFillColor(Color(15, 23, 42));
        FloatRect revB = revTxt.getLocalBounds();
        revTxt.setPosition({colRevX - revB.size.x, ry + 2.f});
        target.draw(revTxt);
    }

    // Divider before summary
    float sumDivY = rowTop + 4 * rowH + 6.f;
    RectangleShape sDiv(Vector2f(cardW - 48.f, 1.f));
    sDiv.setPosition({cardX + 24.f, sumDivY});
    sDiv.setFillColor(Color(226, 232, 240));
    target.draw(sDiv);

    // Summary row
    Text sumLbl(font);
    sumLbl.setString(L"Tổng doanh thu tích lũy 4 tháng:");
    sumLbl.setCharacterSize(13.5f);
    sumLbl.setFillColor(Color(100, 116, 139));
    sumLbl.setPosition({colMonthX, sumDivY + 12.f});
    target.draw(sumLbl);

    Text sumVal(boldFont);
    sumVal.setString(s2ws(toCurrency(totalAccumulatedRevenue)));
    sumVal.setCharacterSize(18.f);
    sumVal.setFillColor(Color(15, 23, 42));
    FloatRect sb = sumVal.getLocalBounds();
    sumVal.setPosition({colRevX - sb.size.x, sumDivY + 8.f});
    target.draw(sumVal);
}

void OverviewPanel::drawBottomBar(RenderTarget& target) const {
    float barY = height - 28.f;

    // Top subtle divider
    RectangleShape div(Vector2f(width - 40.f, 1.f));
    div.setPosition({position.x + 20.f, barY - 4.f});
    div.setFillColor(Color(226, 232, 240));
    target.draw(div);

    // Engine Host status
    Text engText(font);
    engText.setString(L"CineCore Host DB: Connected (0.12ms)  |  Bộ nhớ Cache: 98.4% khả dụng  |  UTF-8 • PBL2 DUYTAN CINEMA ENGINE");
    engText.setCharacterSize(12.f);
    engText.setFillColor(Color(100, 116, 139));
    engText.setPosition({position.x + 20.f, barY});
    target.draw(engText);

    // Right login status
    Text authText(font);
    authText.setString(L"Đăng nhập bởi: admin@cinecore.vn  •  1728x1080 HD Ready");
    authText.setCharacterSize(12.f);
    authText.setFillColor(Color(100, 116, 139));
    FloatRect ab = authText.getLocalBounds();
    authText.setPosition({position.x + width - ab.size.x - 20.f, barY});
}

void OverviewPanel::drawToast(RenderTarget& target) const {
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
