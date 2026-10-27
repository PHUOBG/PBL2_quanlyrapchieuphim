#include "UI/components/Admin/TicketPanel.h"
#include "UI/components/Admin/RoundedRectRenderer.h"
#include "utils/FileUtils.h"
#include <algorithm>
#include <cwctype>
#include <iomanip>
#include <sstream>

static wstring s2ws(const string& s) {
    if (s.empty()) return L"";
    auto sfStr = String::fromUtf8(s.begin(), s.end());
    return sfStr.toWideString();
}

static string formatPrice(int price) {
    string s = to_string(price);
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

TicketPanel::TicketPanel(Font& fontRef, float w, float h)
    : font(fontRef),
      boldFont("../assets/fonts/segoeuib.ttf"),
      width(w), height(h), position(280.f, 0.f),
      repository(FileUtils::resolveDataPath("data/tickets.txt"))
{
    loadTickets();
}

void TicketPanel::setPosition(Vector2f pos) {
    position = pos;
}

void TicketPanel::loadTickets() {
    allTickets.clear();
    DLL<Ticket> list = repository.loadAll();
    for (const auto& t : list) {
        allTickets.push_back(t);
    }

    // Hiển thị vé mới nhất lên trên đầu như trong ảnh tham khảo (T0057 -> T0001)
    reverse(allTickets.begin(), allTickets.end());
    applySearch();
}

void TicketPanel::applySearch() {
    filteredTickets.clear();
    string q = searchQuery;
    for (auto& c : q) c = (char)tolower(c);

    for (const auto& t : allTickets) {
        if (q.empty()) {
            filteredTickets.push_back(t);
            continue;
        }
        string idLower = t.ticketId;
        string emailLower = t.email;
        string stLower = t.showtimeId;
        for (auto& c : idLower) c = (char)tolower(c);
        for (auto& c : emailLower) c = (char)tolower(c);
        for (auto& c : stLower) c = (char)tolower(c);

        if (idLower.find(q) != string::npos || emailLower.find(q) != string::npos || stLower.find(q) != string::npos) {
            filteredTickets.push_back(t);
        }
    }

    selectedRow = -1;
    scrollOffset = 0;
}

void TicketPanel::deleteSelectedTicket() {
    if (selectedRow < 0 || selectedRow >= (int)filteredTickets.size()) {
        notificationMessage = "Vui lòng chọn vé cần xóa!";
        notificationClock.restart();
        notificationVisible = true;
        return;
    }

    string tId = filteredTickets[selectedRow].ticketId;
    if (repository.deleteTicket(tId)) {
        notificationMessage = "Đã xóa vé " + tId + " thành công!";
        notificationClock.restart();
        notificationVisible = true;
        loadTickets();
    }
}

void TicketPanel::drawRoundedCard(RenderWindow& win, Vector2f pos, Vector2f sz, float r, Color fill, float outline, Color outCol) {
    RoundedRectRenderer::draw(win, pos, sz, r, fill, outline, outCol);
}

void TicketPanel::handleEvent(const Event& event, const RenderWindow& win) {
    if (const auto* mouseBtn = event.getIf<Event::MouseButtonPressed>()) {
        if (mouseBtn->button == Mouse::Button::Left) {
            Vector2f mousePos = win.mapPixelToCoords(Mouse::getPosition(win));

            // Nếu đang mở Modal Popup
            if (detailVisible) {
                float popW = 540.f;
                float popH = 460.f;
                float popX = position.x + (width - popW) / 2.f;
                float popY = position.y + (height - popH) / 2.f - 20.f;

                FloatRect btnClose({popX + (popW - 140.f) / 2.f, popY + popH - 52.f}, {140.f, 38.f});
                FloatRect btnX({popX + popW - 42.f, popY + 16.f}, {28.f, 28.f});

                if (btnClose.contains(mousePos) || btnX.contains(mousePos)) {
                    detailVisible = false;
                    return;
                }
                return;
            }

            // Toolbar buttons
            float barY = position.y + 104.f;
            FloatRect btnViewRect({position.x + 30.f, barY}, {135.f, 38.f});
            FloatRect btnDeleteRect({position.x + 175.f, barY}, {95.f, 38.f});
            FloatRect searchRect({position.x + 280.f, barY}, {260.f, 38.f});
            FloatRect btnReloadRect({position.x + width - 75.f, barY}, {45.f, 38.f});

            if (btnViewRect.contains(mousePos)) {
                if (selectedRow >= 0 && selectedRow < (int)filteredTickets.size()) {
                    currentDetailTicket = filteredTickets[selectedRow];
                    detailVisible = true;
                } else {
                    notificationMessage = "Vui lòng click chọn 1 vé trong bảng!";
                    notificationClock.restart();
                    notificationVisible = true;
                }
                return;
            }

            if (btnDeleteRect.contains(mousePos)) {
                deleteSelectedTicket();
                return;
            }

            if (btnReloadRect.contains(mousePos)) {
                searchQuery.clear();
                loadTickets();
                notificationMessage = "Đã làm mới danh sách vé!";
                notificationClock.restart();
                notificationVisible = true;
                return;
            }

            if (searchRect.contains(mousePos)) {
                searchActive = true;
            } else {
                searchActive = false;
            }

            // Click vào các dòng của bảng
            float tableY = position.y + 158.f;
            float rowH = 42.f;
            int count = min((int)filteredTickets.size() - scrollOffset, maxVisibleRows);

            for (int r = 0; r < count; ++r) {
                float rY = tableY + 46.f + r * rowH;
                FloatRect rowRect({position.x + 30.f, rY}, {width - 60.f, rowH});

                if (rowRect.contains(mousePos)) {
                    selectedRow = scrollOffset + r;
                    return;
                }
            }
        }
    }

    // Xử lý cuộn chuột
    if (const auto* wheel = event.getIf<Event::MouseWheelScrolled>()) {
        if (!detailVisible) {
            if (wheel->delta > 0) {
                if (scrollOffset > 0) scrollOffset--;
            } else if (wheel->delta < 0) {
                if (scrollOffset + maxVisibleRows < (int)filteredTickets.size()) scrollOffset++;
            }
        }
    }

    // Nhập tìm kiếm
    if (const auto* textEv = event.getIf<Event::TextEntered>()) {
        if (searchActive && !detailVisible) {
            char32_t c = textEv->unicode;
            if (c == 8) { // Backspace
                if (!searchQuery.empty()) {
                    searchQuery.pop_back();
                    applySearch();
                }
            } else if (c >= 32 && c != 127) {
                if (searchQuery.size() < 30) {
                    searchQuery.push_back((char)c);
                    applySearch();
                }
            }
        }
    }
}

void TicketPanel::update(Vector2f mousePos, bool) {
    hoveredRow = -1;
    float barY = position.y + 104.f;
    btnViewHover = FloatRect({position.x + 30.f, barY}, {135.f, 38.f}).contains(mousePos);
    btnDeleteHover = FloatRect({position.x + 175.f, barY}, {95.f, 38.f}).contains(mousePos);
    btnReloadHover = FloatRect({position.x + width - 75.f, barY}, {45.f, 38.f}).contains(mousePos);

    if (!detailVisible) {
        float tableY = position.y + 158.f;
        float rowH = 42.f;
        int count = min((int)filteredTickets.size() - scrollOffset, maxVisibleRows);

        for (int r = 0; r < count; ++r) {
            float rY = tableY + 46.f + r * rowH;
            if (FloatRect({position.x + 30.f, rY}, {width - 60.f, rowH}).contains(mousePos)) {
                hoveredRow = r;
                break;
            }
        }
    }
}

void TicketPanel::render(RenderWindow& win) {
    // 1. Nền canvas (#F4F6F9)
    RectangleShape bg({width, height});
    bg.setPosition(position);
    bg.setFillColor(Color(244, 246, 249));
    win.draw(bg);

    // ==========================================
    // 2. TOP BAR
    // ==========================================
    Text bc1(boldFont, L"C\u1ed4NG QU\u1ea2N TR\u1eca N\u1ed8I B\u1ed8", 12);
    bc1.setFillColor(Color(2, 132, 199));
    bc1.setPosition({position.x + 30.f, position.y + 20.f});
    win.draw(bc1);

    Text slash1(boldFont, L" /  H\u1ec6 TH\u1ed0NG  /  ", 12);
    slash1.setFillColor(Color(148, 163, 184));
    slash1.setPosition({position.x + 190.f, position.y + 20.f});
    win.draw(slash1);

    Text bc2(boldFont, L"QU\u1ea2N L\u00dd V\u00c9", 12);
    bc2.setFillColor(Color(2, 132, 199));
    bc2.setPosition({position.x + 305.f, position.y + 20.f});
    win.draw(bc2);

    float rOffset = position.x + width - 30.f;
    drawRoundedCard(win, {rOffset - 390.f, position.y + 14.f}, {185.f, 28.f}, 14.f, Color(241, 245, 249), 1.f, Color(226, 232, 240));
    CircleShape dotGrn(3.5f); dotGrn.setFillColor(Color(34, 197, 94)); dotGrn.setPosition({rOffset - 378.f, position.y + 24.f}); win.draw(dotGrn);
    Text sqlText(boldFont, L"MS-SQL R\u1ea0P \u0110\u00c0 N\u1eb5NG : ONLINE", 10); sqlText.setFillColor(Color(71, 85, 105)); sqlText.setPosition({rOffset - 366.f, position.y + 21.f}); win.draw(sqlText);

    CircleShape bellCircle(15.f); bellCircle.setFillColor(Color(241, 245, 249)); bellCircle.setPosition({rOffset - 190.f, position.y + 13.f}); win.draw(bellCircle);
    CircleShape bellRedDot(3.5f); bellRedDot.setFillColor(Color(239, 68, 68)); bellRedDot.setPosition({rOffset - 173.f, position.y + 14.f}); win.draw(bellRedDot);

    CircleShape avatarBg(16.f); avatarBg.setFillColor(Color(2, 132, 199)); avatarBg.setPosition({rOffset - 145.f, position.y + 12.f}); win.draw(avatarBg);
    Text avText(boldFont, L"AD", 12); avText.setFillColor(Color::White); avText.setPosition({rOffset - 138.f, position.y + 20.f}); win.draw(avText);

    Text uName(boldFont, L"Admin Danang01", 12); uName.setFillColor(Color(15, 23, 42)); uName.setPosition({rOffset - 105.f, position.y + 12.f}); win.draw(uName);
    Text uSub(font, L"DANANG-CN01", 11); uSub.setFillColor(Color(100, 116, 139)); uSub.setPosition({rOffset - 105.f, position.y + 28.f}); win.draw(uSub);

    // ==========================================
    // 3. PAGE TITLE
    // ==========================================
    Text title(boldFont, L"Qu\u1ea3n l\u00fd v\u00e9", 26);
    title.setFillColor(Color(15, 23, 42));
    title.setPosition({position.x + 30.f, position.y + 60.f});
    win.draw(title);

    // ==========================================
    // 4. TOOLBAR
    // ==========================================
    float barY = position.y + 104.f;

    // Nút 1: Xem thông tin (Màu xanh dương)
    Color viewBg = btnViewHover ? Color(3, 105, 161) : Color(2, 132, 199);
    drawRoundedCard(win, {position.x + 30.f, barY}, {135.f, 38.f}, 6.f, viewBg);
    Text viewTxt(boldFont, L"Xem th\u00f4ng tin", 13);
    viewTxt.setFillColor(Color::White);
    FloatRect vtb = viewTxt.getLocalBounds();
    viewTxt.setPosition({position.x + 30.f + (135.f - vtb.size.x) / 2.f - vtb.position.x, barY + (38.f - vtb.size.y) / 2.f - vtb.position.y});
    win.draw(viewTxt);

    // Nút 2: Xóa vé (Màu đỏ)
    Color delBg = btnDeleteHover ? Color(185, 28, 28) : Color(220, 38, 38);
    drawRoundedCard(win, {position.x + 175.f, barY}, {95.f, 38.f}, 6.f, delBg);
    Text delTxt(boldFont, L"X\u00f3a v\u00e9", 13);
    delTxt.setFillColor(Color::White);
    FloatRect dtb = delTxt.getLocalBounds();
    delTxt.setPosition({position.x + 175.f + (95.f - dtb.size.x) / 2.f - dtb.position.x, barY + (38.f - dtb.size.y) / 2.f - dtb.position.y});
    win.draw(delTxt);

    // Ô Input Tìm kiếm: Tìm theo ID vé, Email...
    drawRoundedCard(win, {position.x + 280.f, barY}, {260.f, 38.f}, 6.f, Color::White, searchActive ? 1.5f : 1.f, searchActive ? Color(2, 132, 199) : Color(203, 213, 225));
    Text searchIcon(boldFont, L"\U0001F50D", 12);
    searchIcon.setFillColor(Color(148, 163, 184));
    searchIcon.setPosition({position.x + 292.f, barY + 10.f});
    win.draw(searchIcon);

    wstring sDisplay = searchQuery.empty() ? L"T\u00ecm theo ID v\u00e9, Email..." : s2ws(searchQuery) + (searchActive ? L"|" : L"");
    Text searchTxt(font, sDisplay, 12);
    searchTxt.setFillColor(searchQuery.empty() ? Color(148, 163, 184) : Color(15, 23, 42));
    searchTxt.setPosition({position.x + 316.f, barY + 10.f});
    win.draw(searchTxt);

    // Nút Reload góc phải
    Color relBg = btnReloadHover ? Color(3, 105, 161) : Color(2, 132, 199);
    drawRoundedCard(win, {position.x + width - 75.f, barY}, {45.f, 38.f}, 6.f, relBg);
    Text relIcon(boldFont, L"\u21bb", 20);
    relIcon.setFillColor(Color::White);
    FloatRect rtb = relIcon.getLocalBounds();
    relIcon.setPosition({position.x + width - 75.f + (45.f - rtb.size.x) / 2.f - rtb.position.x, barY + (38.f - rtb.size.y) / 2.f - rtb.position.y});
    win.draw(relIcon);

    // ==========================================
    // 5. BẢNG DỮ LIỆU VÉ (DATA TABLE)
    // ==========================================
    float tableY = position.y + 158.f;
    float tableW = width - 60.f;
    float rowH = 42.f;

    // Header bảng (#0077B6 / #0284C7)
    drawRoundedCard(win, {position.x + 30.f, tableY}, {tableW, 46.f}, 4.f, Color(0, 119, 182));

    Text h1(boldFont, L"ID v\u00e9", 13); h1.setFillColor(Color::White); h1.setPosition({position.x + 60.f, tableY + 14.f}); win.draw(h1);
    Text h2(boldFont, L"ID su\u1ea5t chi\u1ebfu", 13); h2.setFillColor(Color::White); h2.setPosition({position.x + 190.f, tableY + 14.f}); win.draw(h2);
    Text h3(boldFont, L"Email \u0111\u1eb7t v\u00e9", 13); h3.setFillColor(Color::White); h3.setPosition({position.x + 520.f, tableY + 14.f}); win.draw(h3);
    Text h4(boldFont, L"Ng\u00e0y \u0111\u1eb7t v\u00e9", 13); h4.setFillColor(Color::White); h4.setPosition({position.x + 850.f, tableY + 14.f}); win.draw(h4);
    Text h5(boldFont, L"Gi\u1edd \u0111\u1eb7t v\u00e9", 13); h5.setFillColor(Color::White); h5.setPosition({position.x + 1040.f, tableY + 14.f}); win.draw(h5);

    // Các dòng dữ liệu
    int count = min((int)filteredTickets.size() - scrollOffset, maxVisibleRows);

    for (int r = 0; r < count; ++r) {
        int idx = scrollOffset + r;
        const auto& t = filteredTickets[idx];
        float rY = tableY + 46.f + r * rowH;

        bool isSel = (selectedRow == idx);
        bool isHov = (hoveredRow == r);

        Color rowBg = isSel ? Color(224, 242, 254) : (isHov ? Color(248, 250, 252) : Color::White);
        drawRoundedCard(win, {position.x + 30.f, rY}, {tableW, rowH}, 0.f, rowBg, 0.5f, Color(226, 232, 240));

        // ID vé
        Text tId(boldFont, s2ws(t.ticketId), 13);
        tId.setFillColor(isSel ? Color(2, 132, 199) : Color(71, 85, 105));
        tId.setPosition({position.x + 60.f, rY + 12.f});
        win.draw(tId);

        // ID suất chiếu
        Text tSt(font, s2ws(t.showtimeId), 13);
        tSt.setFillColor(Color(51, 65, 85));
        tSt.setPosition({position.x + 190.f, rY + 12.f});
        win.draw(tSt);

        // Email đặt vé
        Text tEm(font, s2ws(t.email), 13);
        tEm.setFillColor(Color(51, 65, 85));
        tEm.setPosition({position.x + 520.f, rY + 12.f});
        win.draw(tEm);

        // Ngày đặt vé
        Text tDt(font, s2ws(t.bookedDate.empty() ? t.date : t.bookedDate), 13);
        tDt.setFillColor(Color(51, 65, 85));
        tDt.setPosition({position.x + 850.f, rY + 12.f});
        win.draw(tDt);

        // Giờ đặt vé
        Text tTm(font, s2ws(t.bookedTime.empty() ? t.time : t.bookedTime), 13);
        tTm.setFillColor(Color(51, 65, 85));
        tTm.setPosition({position.x + 1040.f, rY + 12.f});
        win.draw(tTm);
    }

    // ==========================================
    // 6. MODAL POPUP: CHI TIẾT VÉ
    // ==========================================
    if (detailVisible) {
        RectangleShape overlay({width, height});
        overlay.setPosition(position);
        overlay.setFillColor(Color(15, 23, 42, 160));
        win.draw(overlay);

        float popW = 540.f;
        float popH = 470.f;
        float popX = position.x + (width - popW) / 2.f;
        float popY = position.y + (height - popH) / 2.f - 20.f;

        drawRoundedCard(win, {popX, popY}, {popW, popH}, 12.f, Color::White);

        // Header Popup
        Text popTitle(boldFont, L"TH\u00d4NG TIN CHI TI\u1ebeT V\u00c9", 18);
        popTitle.setFillColor(Color(15, 23, 42));
        popTitle.setPosition({popX + 30.f, popY + 22.f});
        win.draw(popTitle);

        Text closeBtn(boldFont, L"\u2715", 16);
        closeBtn.setFillColor(Color(148, 163, 184));
        closeBtn.setPosition({popX + popW - 38.f, popY + 20.f});
        win.draw(closeBtn);

        // Đường kẻ ngang mỏng
        RectangleShape sep({popW - 60.f, 1.f});
        sep.setFillColor(Color(226, 232, 240));
        sep.setPosition({popX + 30.f, popY + 58.f});
        win.draw(sep);

        // Các trường chi tiết
        vector<pair<wstring, wstring>> fields = {
            {L"M\u00e3 v\u00e9:", s2ws(currentDetailTicket.ticketId)},
            {L"M\u00e3 su\u1ea5t chi\u1ebfu:", s2ws(currentDetailTicket.showtimeId)},
            {L"T\u00ean phim:", s2ws(currentDetailTicket.title)},
            {L"Ph\u00f2ng chi\u1ebfu:", s2ws(currentDetailTicket.roomName)},
            {L"Gi\u1edd chi\u1ebfu:", s2ws(currentDetailTicket.date + " " + currentDetailTicket.time)},
            {L"Gh\u1ebf \u0111\u00e3 \u0111\u1eb7t:", s2ws(currentDetailTicket.booked)},
            {L"Combo b\u1eafp n\u01b0\u1edbc:", s2ws(currentDetailTicket.comboName.empty() ? "Kh\u00f4ng c\u00f3" : currentDetailTicket.comboName)},
            {L"Kh\u00e1ch h\u00e0ng:", s2ws(currentDetailTicket.fullName + " (" + currentDetailTicket.email + ")")},
            {L"Th\u1eddi gian \u0111\u1eb7t:", s2ws(currentDetailTicket.bookedDate + " " + currentDetailTicket.bookedTime)},
            {L"T\u1ed5ng ti\u1ec1n:", s2ws(formatPrice(currentDetailTicket.price))}
        };

        float curY = popY + 70.f;
        for (const auto& item : fields) {
            Text lbl(boldFont, item.first, 12);
            lbl.setFillColor(Color(100, 116, 139));
            lbl.setPosition({popX + 30.f, curY});
            win.draw(lbl);

            Text val(boldFont, item.second, 12);
            val.setFillColor(item.first == L"T\u1ed5ng ti\u1ec1n:" ? Color(2, 132, 199) : Color(15, 23, 42));
            val.setPosition({popX + 175.f, curY});
            win.draw(val);

            curY += 32.f;
        }

        // Nút Đóng
        drawRoundedCard(win, {popX + (popW - 140.f) / 2.f, popY + popH - 52.f}, {140.f, 36.f}, 6.f, Color(2, 132, 199));
        Text btnClTxt(boldFont, L"\u0110\u00f3ng", 13);
        btnClTxt.setFillColor(Color::White);
        FloatRect bctb = btnClTxt.getLocalBounds();
        btnClTxt.setPosition({popX + (popW - 140.f) / 2.f + (140.f - bctb.size.x) / 2.f - bctb.position.x, popY + popH - 52.f + (36.f - bctb.size.y) / 2.f - bctb.position.y});
        win.draw(btnClTxt);
    }

    // ==========================================
    // 7. TOAST NOTIFICATION
    // ==========================================
    if (notificationVisible) {
        if (notificationClock.getElapsedTime().asSeconds() > 3.0f) {
            notificationVisible = false;
        } else {
            float notW = 340.f;
            float notH = 42.f;
            float notX = position.x + (width - notW) / 2.f;
            float notY = position.y + height - 60.f;
            drawRoundedCard(win, {notX, notY}, {notW, notH}, 8.f, Color(15, 23, 42, 240), 1.f, Color(2, 132, 199));
            Text notText(boldFont, s2ws(notificationMessage), 12);
            notText.setFillColor(Color::White);
            FloatRect ntb = notText.getLocalBounds();
            notText.setPosition({
                notX + (notW - ntb.size.x) / 2.f - ntb.position.x,
                notY + (notH - ntb.size.y) / 2.f - ntb.position.y
            });
            win.draw(notText);
        }
    }
}
