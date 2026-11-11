#include "UI/components/Admin/TicketPanel.h"
#include "UI/components/Admin/UITheme.h"
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
      repository(FileUtils::resolveDataPath("data/tickets.csv"))
{
    if (reloadTex.loadFromFile("assets/elements/reload.png") ||
        reloadTex.loadFromFile("../assets/elements/reload.png")) {
        reloadTex.setSmooth(true);
        hasReloadTex = true;
    }
    loadTickets();
}

void TicketPanel::setPosition(Vector2f pos) {
    position = pos;
}

void TicketPanel::loadTickets() {
    allTickets.clear();
    ticketIndexMap.clear();
    DLL<Ticket> list = repository.loadAll();
    for (const auto& t : list) {
        allTickets.push_back(t);
    }

    // Hiển thị vé mới nhất lên trên đầu như trong ảnh tham khảo (T0057 -> T0001)
    reverse(allTickets.begin(), allTickets.end());

    // Xây dựng bảng băm HashTable O(1) theo mã vé (ticketId chuẩn hóa chữ thường)
    for (size_t i = 0; i < allTickets.size(); ++i) {
        string normId = UITheme::normalizeViUtf8(allTickets[i].ticketId);
        ticketIndexMap.insert(normId, (int)i);
    }
    currentPage = 1;
    applySearch();
}

void TicketPanel::applySearch() {
    filteredTickets.clear();

    // Trim khoảng trắng hai đầu của từ khóa tìm kiếm
    wstring q = searchQuery;
    size_t first = q.find_first_not_of(L" \t");
    if (first == wstring::npos) {
        filteredTickets = allTickets;
        totalPages = max(1, (static_cast<int>(filteredTickets.size()) + ticketsPerPage - 1) / ticketsPerPage);
        if (currentPage > totalPages) currentPage = totalPages;
        if (currentPage < 1) currentPage = 1;
        selectedRow = filteredTickets.empty() ? -1 : 0;
        return;
    }
    size_t last = q.find_last_not_of(L" \t");
    q = q.substr(first, last - first + 1);

    // 1. Tra cứu O(1) bằng HashTable nếu người dùng nhập đúng mã vé (ví dụ: T0056, t0056)
    string normQueryUtf8 = UITheme::normalizeViUtf8(UITheme::wstrToUtf8(q));
    int* exactIdx = ticketIndexMap.find(normQueryUtf8);
    if (exactIdx && *exactIdx >= 0 && *exactIdx < (int)allTickets.size()) {
        filteredTickets.push_back(allTickets[*exactIdx]);
    }

    // 2. Kết hợp tìm kiếm chuỗi con không dấu (ID vé, ID suất chiếu, Email, Họ tên, Tên phim, Phòng, Ngày đặt)
    for (const auto& t : allTickets) {
        if (exactIdx && t.ticketId == allTickets[*exactIdx].ticketId) continue;

        bool match = UITheme::matchesViQuery(s2ws(t.ticketId), q) ||
                     UITheme::matchesViQuery(s2ws(t.showtimeId), q) ||
                     UITheme::matchesViQuery(s2ws(t.email), q) ||
                     UITheme::matchesViQuery(s2ws(t.fullName), q) ||
                     UITheme::matchesViQuery(s2ws(t.title), q) ||
                     UITheme::matchesViQuery(s2ws(t.roomName), q) ||
                     UITheme::matchesViQuery(s2ws(t.bookedDate), q);

        if (match) {
            filteredTickets.push_back(t);
        }
    }

    totalPages = max(1, (static_cast<int>(filteredTickets.size()) + ticketsPerPage - 1) / ticketsPerPage);
    if (currentPage > totalPages) currentPage = totalPages;
    if (currentPage < 1) currentPage = 1;
    selectedRow = filteredTickets.empty() ? -1 : 0;
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
        deleteConfirmVisible = false;
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
    // 1. ƯU TIÊN XỬ LÝ MODAL XÁC NHẬN XÓA VÉ
    if (deleteConfirmVisible) {
        float popW = 420.f;
        float popH = 210.f;
        float popX = position.x + (width - popW) / 2.f;
        float popY = position.y + (height - popH) / 2.f - 20.f;

        if (const auto* mouseBtn = event.getIf<Event::MouseButtonPressed>()) {
            if (mouseBtn->button == Mouse::Button::Left) {
                Vector2f mousePos = win.mapPixelToCoords(Mouse::getPosition(win));
                FloatRect btnConfirm({popX + 55.f, popY + 145.f}, {145.f, 38.f});
                FloatRect btnCancel({popX + 220.f, popY + 145.f}, {145.f, 38.f});

                if (btnConfirm.contains(mousePos)) {
                    deleteSelectedTicket();
                    return;
                }
                if (btnCancel.contains(mousePos) || !FloatRect({popX, popY}, {popW, popH}).contains(mousePos)) {
                    deleteConfirmVisible = false;
                    return;
                }
            }
            return;
        }
        if (const auto* keyEv = event.getIf<Event::KeyPressed>()) {
            if (keyEv->code == Keyboard::Key::Escape) { deleteConfirmVisible = false; return; }
            if (keyEv->code == Keyboard::Key::Enter) { deleteSelectedTicket(); return; }
        }
        return;
    }

    // 2. ƯU TIÊN XỬ LÝ MODAL CHI TIẾT VÉ
    if (detailVisible) {
        float popW = 540.f;
        float popH = 470.f;
        float popX = position.x + (width - popW) / 2.f;
        float popY = position.y + (height - popH) / 2.f - 20.f;

        if (const auto* mouseBtn = event.getIf<Event::MouseButtonPressed>()) {
            if (mouseBtn->button == Mouse::Button::Left) {
                Vector2f mousePos = win.mapPixelToCoords(Mouse::getPosition(win));
                FloatRect btnClose({popX + (popW - 140.f) / 2.f, popY + popH - 52.f}, {140.f, 38.f});
                FloatRect btnX({popX + popW - 45.f, popY + 14.f}, {32.f, 32.f});

                if (btnClose.contains(mousePos) || btnX.contains(mousePos) || !FloatRect({popX, popY}, {popW, popH}).contains(mousePos)) {
                    detailVisible = false;
                    return;
                }
            }
            return;
        }
        if (const auto* keyEv = event.getIf<Event::KeyPressed>()) {
            if (keyEv->code == Keyboard::Key::Escape || keyEv->code == Keyboard::Key::Enter) {
                detailVisible = false;
                return;
            }
        }
        return;
    }

    // 3. XỬ LÝ CLICK CHUỘT TRÊN MÀN HÌNH CHÍNH
    if (const auto* mouseBtn = event.getIf<Event::MouseButtonPressed>()) {
        if (mouseBtn->button == Mouse::Button::Left) {
            Vector2f mousePos = win.mapPixelToCoords(Mouse::getPosition(win));

            // Toolbar buttons
            float barY = position.y + 104.f;
            FloatRect btnViewRect({position.x + 30.f, barY}, {135.f, 38.f});
            FloatRect btnDeleteRect({position.x + 175.f, barY}, {95.f, 38.f});
            FloatRect searchRect({position.x + 280.f, barY}, {300.f, 38.f});
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
                if (selectedRow >= 0 && selectedRow < (int)filteredTickets.size()) {
                    deleteConfirmVisible = true;
                } else {
                    notificationMessage = "Vui lòng chọn vé cần xóa!";
                    notificationClock.restart();
                    notificationVisible = true;
                }
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
                // Nếu click vào dấu X xóa nhanh bên phải ô tìm kiếm
                if (!searchQuery.empty() && mousePos.x >= position.x + 550.f) {
                    searchQuery.clear();
                    applySearch();
                }
                searchActive = true;
                return;
            } else {
                searchActive = false;
            }

            // Click vào các dòng của bảng
            float tableY = position.y + 158.f;
            float tableW = width - 60.f;
            float rowH = 42.f;
            int startIdx = (currentPage - 1) * ticketsPerPage;
            int count = max(0, min((int)filteredTickets.size() - startIdx, ticketsPerPage));

            for (int r = 0; r < count; ++r) {
                float rY = tableY + 46.f + r * rowH;
                FloatRect rowRect({position.x + 30.f, rY}, {tableW, rowH});

                if (rowRect.contains(mousePos)) {
                    int clickedIdx = startIdx + r;
                    if (selectedRow == clickedIdx) {
                        // Click lần 2 vào dòng đã chọn sẽ mở luôn chi tiết vé
                        currentDetailTicket = filteredTickets[clickedIdx];
                        detailVisible = true;
                    } else {
                        selectedRow = clickedIdx;
                    }
                    return;
                }
            }

            // Click vào các nút chuyển trang (Pagination Bar)
            float pagY = tableY + 46.f + ticketsPerPage * rowH + 16.f;
            FloatRect pagBarRect({position.x + 30.f, pagY}, {tableW, 36.f});

            if (pagBarRect.contains(mousePos)) {
                float rightEdge = position.x + 30.f + tableW;
                int numPageBtns = min(5, totalPages);
                int startP = max(1, min(currentPage - 2, totalPages - numPageBtns + 1));
                int endP = min(totalPages, startP + numPageBtns - 1);
                float btnPrevW = 64.f;
                float btnNextW = 64.f;
                float btnPageW = 34.f;
                float gap = 6.f;
                float totalBtnsW = btnPrevW + gap + (endP - startP + 1) * (btnPageW + gap) - gap + gap + btnNextW;
                float curBtnX = rightEdge - totalBtnsW;

                // Nút Trước
                FloatRect prevRect({curBtnX, pagY}, {btnPrevW, 34.f});
                if (prevRect.contains(mousePos) && currentPage > 1) {
                    currentPage--;
                    selectedRow = (currentPage - 1) * ticketsPerPage;
                    return;
                }
                curBtnX += btnPrevW + gap;

                // Các nút số trang
                for (int p = startP; p <= endP; ++p) {
                    FloatRect pRect({curBtnX, pagY}, {btnPageW, 34.f});
                    if (pRect.contains(mousePos)) {
                        currentPage = p;
                        selectedRow = (currentPage - 1) * ticketsPerPage;
                        return;
                    }
                    curBtnX += btnPageW + gap;
                }

                // Nút Tiếp
                FloatRect nextRect({curBtnX, pagY}, {btnNextW, 34.f});
                if (nextRect.contains(mousePos) && currentPage < totalPages) {
                    currentPage++;
                    selectedRow = (currentPage - 1) * ticketsPerPage;
                    return;
                }
                return;
            }
        }
    }

    // 4. XỬ LÝ CUỘN CHUỘT ĐỂ CHUYỂN TRANG
    if (const auto* wheel = event.getIf<Event::MouseWheelScrolled>()) {
        if (wheel->delta > 0 && currentPage > 1) {
            currentPage--;
            selectedRow = (currentPage - 1) * ticketsPerPage;
        } else if (wheel->delta < 0 && currentPage < totalPages) {
            currentPage++;
            selectedRow = (currentPage - 1) * ticketsPerPage;
        }
    }

    // 5. XỬ LÝ BÀN PHÍM CHO Ô TÌM KIẾM VÉ (Hỗ trợ đầy đủ Unicode & Tiếng Việt)
    if (const auto* keyEv = event.getIf<Event::KeyPressed>()) {
        if (searchActive) {
            if (keyEv->code == Keyboard::Key::Backspace) {
                if (!searchQuery.empty()) {
                    searchQuery.pop_back();
                    applySearch();
                }
                return;
            }
            if (keyEv->code == Keyboard::Key::Escape) {
                searchQuery.clear();
                searchActive = false;
                applySearch();
                return;
            }
            if (keyEv->code == Keyboard::Key::Enter) {
                if (selectedRow >= 0 && selectedRow < (int)filteredTickets.size()) {
                    currentDetailTicket = filteredTickets[selectedRow];
                    detailVisible = true;
                }
                return;
            }
        }
    }

    if (const auto* textEv = event.getIf<Event::TextEntered>()) {
        if (searchActive) {
            char32_t c = textEv->unicode;
            if (c >= 32 && c != 127) {
                if (searchQuery.size() < 40) {
                    searchQuery += static_cast<wchar_t>(c);
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
        int startIdx = (currentPage - 1) * ticketsPerPage;
        int count = min((int)filteredTickets.size() - startIdx, ticketsPerPage);

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
    // 2. TOP BAR (Standardized Navigation & Profile)
    // ==========================================
    UITheme::drawTopBar(win, position.x + 30.f, position.y, width - 60.f, L"QUẢN LÝ VÉ", font, boldFont);

    // ==========================================
    // 3. PAGE TITLE
    // ==========================================
    Text title(boldFont, L"Qu\u1ea3n l\u00fd v\u00e9", UITheme::PageTitleSize);
    title.setFillColor(UITheme::TextPrimary);
    title.setPosition({position.x + 30.f, position.y + 64.f});
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
    drawRoundedCard(win, {position.x + 280.f, barY}, {300.f, 38.f}, 6.f, Color::White, searchActive ? 1.8f : 1.f, searchActive ? Color(2, 132, 199) : Color(203, 213, 225));

    // Ký hiệu kính lúp tìm kiếm vẽ bằng SFML shapes chuẩn sắc nét
    Vector2f iconPos(position.x + 293.f, barY + 12.f);
    Color icCol = searchActive ? Color(2, 132, 199) : Color(148, 163, 184);
    CircleShape lens(5.f, 16);
    lens.setFillColor(Color::Transparent);
    lens.setOutlineThickness(1.6f);
    lens.setOutlineColor(icCol);
    lens.setPosition(iconPos);
    win.draw(lens);

    RectangleShape stem({5.5f, 1.8f});
    stem.setFillColor(icCol);
    stem.setRotation(degrees(45.f));
    stem.setPosition({iconPos.x + 8.f, iconPos.y + 8.f});
    win.draw(stem);

    wstring sDisplay = searchQuery.empty()
        ? (searchActive ? L"|" : L"T\u00ecm theo ID v\u00e9, Email, Phim...")
        : (searchQuery + (searchActive ? L"|" : L""));
    Text searchTxt(font, sDisplay, 12.5f);
    searchTxt.setFillColor(searchQuery.empty() && !searchActive ? Color(148, 163, 184) : Color(15, 23, 42));
    searchTxt.setPosition({position.x + 314.f, barY + 10.f});
    win.draw(searchTxt);

    if (!searchQuery.empty()) {
        UITheme::drawCloseIcon(win, {position.x + 563.f, barY + 19.f}, 10.f, Color(148, 163, 184), 1.8f);
    }

    // Badge hiển thị tổng số kết quả tìm được
    Text cntBadge(boldFont, L"K\u1ebft qu\u1ea3: " + to_wstring(filteredTickets.size()) + L" v\u00e9", 12);
    cntBadge.setFillColor(Color(100, 116, 139));
    cntBadge.setPosition({position.x + 595.f, barY + 11.f});
    win.draw(cntBadge);

    // Nút Reload góc phải
    float relX = position.x + width - 75.f;
    Color relBg = btnReloadHover ? Color(3, 105, 161) : Color(2, 132, 199);
    drawRoundedCard(win, {relX, barY}, {45.f, 38.f}, 6.f, relBg);
    if (hasReloadTex) {
        Sprite spRel(reloadTex);
        spRel.setScale({20.f / 500.f, 20.f / 500.f});
        spRel.setPosition({relX + 12.f, barY + 9.f});
        win.draw(spRel);
    }

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
    int startIdx = (currentPage - 1) * ticketsPerPage;
    int count = max(0, min((int)filteredTickets.size() - startIdx, ticketsPerPage));

    for (int r = 0; r < count; ++r) {
        int idx = startIdx + r;
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
    // 5b. THANH PHÂN TRANG (PAGINATION BAR)
    // ==========================================
    float pagY = tableY + 46.f + ticketsPerPage * rowH + 16.f;
    int fromIdx = filteredTickets.empty() ? 0 : (startIdx + 1);
    int toIdx = min(static_cast<int>(filteredTickets.size()), startIdx + ticketsPerPage);
    wstring pageInfoStr = filteredTickets.empty()
        ? L"Không có vé nào phù hợp"
        : (L"Hiển thị " + to_wstring(fromIdx) + L" – " + to_wstring(toIdx) + L" trên tổng số " + to_wstring(filteredTickets.size()) + L" vé");
    Text pageInfo(font, pageInfoStr, 12.5f);
    pageInfo.setFillColor(Color(100, 116, 139));
    pageInfo.setPosition({position.x + 30.f, pagY + 7.f});
    win.draw(pageInfo);

    float rightEdge = position.x + 30.f + tableW;
    int numPageBtns = min(5, totalPages);
    int startP = max(1, min(currentPage - 2, totalPages - numPageBtns + 1));
    int endP = min(totalPages, startP + numPageBtns - 1);
    float btnPrevW = 64.f;
    float btnNextW = 64.f;
    float btnPageW = 34.f;
    float gap = 6.f;
    float totalBtnsW = btnPrevW + gap + (endP - startP + 1) * (btnPageW + gap) - gap + gap + btnNextW;
    float curBtnX = rightEdge - totalBtnsW;

    // Nút Trước
    bool canPrev = (currentPage > 1);
    drawRoundedCard(win, {curBtnX, pagY}, {btnPrevW, 34.f}, 6.f, Color::White, 1.2f, Color(226, 232, 240));
    Text prTxt(boldFont, L"Trước", 12.5f);
    prTxt.setFillColor(canPrev ? Color(71, 85, 105) : Color(203, 213, 225));
    FloatRect prb = prTxt.getLocalBounds();
    prTxt.setPosition({curBtnX + (btnPrevW - prb.size.x) / 2.f - prb.position.x, pagY + (34.f - prb.size.y) / 2.f - prb.position.y});
    win.draw(prTxt);
    curBtnX += btnPrevW + gap;

    // Các nút số trang 1, 2, 3, ...
    for (int p = startP; p <= endP; ++p) {
        bool isAct = (p == currentPage);
        Color pBg = isAct ? Color(0, 119, 182) : Color::White;
        Color pBorder = isAct ? Color(0, 119, 182) : Color(226, 232, 240);
        Color pFg = isAct ? Color::White : Color(51, 65, 85);

        drawRoundedCard(win, {curBtnX, pagY}, {btnPageW, 34.f}, 6.f, pBg, 1.2f, pBorder);
        Text pTxt(boldFont, to_wstring(p), 12.5f);
        pTxt.setFillColor(pFg);
        FloatRect pb = pTxt.getLocalBounds();
        pTxt.setPosition({curBtnX + (btnPageW - pb.size.x) / 2.f - pb.position.x, pagY + (34.f - pb.size.y) / 2.f - pb.position.y});
        win.draw(pTxt);
        curBtnX += btnPageW + gap;
    }

    // Nút Tiếp
    bool canNext = (currentPage < totalPages);
    drawRoundedCard(win, {curBtnX, pagY}, {btnNextW, 34.f}, 6.f, Color::White, 1.2f, Color(226, 232, 240));
    Text nxTxt(boldFont, L"Tiếp", 12.5f);
    nxTxt.setFillColor(canNext ? Color(71, 85, 105) : Color(203, 213, 225));
    FloatRect nxb = nxTxt.getLocalBounds();
    nxTxt.setPosition({curBtnX + (btnNextW - nxb.size.x) / 2.f - nxb.position.x, pagY + (34.f - nxb.size.y) / 2.f - nxb.position.y});
    win.draw(nxTxt);

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

        UITheme::drawCloseIcon(win, {popX + popW - 32.f, popY + 30.f}, 13.f, Color(148, 163, 184), 2.f);

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
    // 6B. MODAL POPUP: XÁC NHẬN XÓA VÉ
    // ==========================================
    if (deleteConfirmVisible && selectedRow >= 0 && selectedRow < (int)filteredTickets.size()) {
        RectangleShape overlay({width, height});
        overlay.setPosition(position);
        overlay.setFillColor(Color(15, 23, 42, 150));
        win.draw(overlay);

        float popW = 420.f;
        float popH = 210.f;
        float popX = position.x + (width - popW) / 2.f;
        float popY = position.y + (height - popH) / 2.f - 20.f;

        drawRoundedCard(win, {popX, popY}, {popW, popH}, 12.f, Color::White);

        Text pTitle(boldFont, L"X\u00e1c nh\u1eadn h\u1ee7y / x\u00f3a v\u00e9", 17);
        pTitle.setFillColor(Color(220, 38, 38));
        pTitle.setPosition({popX + 28.f, popY + 22.f});
        win.draw(pTitle);

        wstring tIdW = s2ws(filteredTickets[selectedRow].ticketId);
        wstring msgW = L"B\u1ea1n c\u00f3 ch\u1eafc ch\u1eafn mu\u1ed1n x\u00f3a v\u00e9 [" + tIdW + L"]\nkh\u1ecfi h\u1ec7 th\u1ed1ng? Thao t\u00e1c n\u00e0y kh\u00f4ng th\u1ec3 ho\u00e0n t\u00e1c.";
        Text pMsg(font, msgW, 13);
        pMsg.setFillColor(Color(51, 65, 85));
        pMsg.setPosition({popX + 28.f, popY + 62.f});
        win.draw(pMsg);

        // Nút Xác nhận xóa & Hủy bỏ
        drawRoundedCard(win, {popX + 55.f, popY + 145.f}, {145.f, 38.f}, 6.f, Color(220, 38, 38));
        Text cfmTxt(boldFont, L"X\u00e1c nh\u1eadn x\u00f3a", 13);
        cfmTxt.setFillColor(Color::White);
        FloatRect cfb = cfmTxt.getLocalBounds();
        cfmTxt.setPosition({popX + 55.f + (145.f - cfb.size.x) / 2.f - cfb.position.x, popY + 145.f + (38.f - cfb.size.y) / 2.f - cfb.position.y});
        win.draw(cfmTxt);

        drawRoundedCard(win, {popX + 220.f, popY + 145.f}, {145.f, 38.f}, 6.f, Color(241, 245, 249), 1.f, Color(203, 213, 225));
        Text canTxt(boldFont, L"H\u1ee7y b\u1ecf", 13);
        canTxt.setFillColor(Color(71, 85, 105));
        FloatRect cab = canTxt.getLocalBounds();
        canTxt.setPosition({popX + 220.f + (145.f - cab.size.x) / 2.f - cab.position.x, popY + 145.f + (38.f - cab.size.y) / 2.f - cab.position.y});
        win.draw(canTxt);
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
