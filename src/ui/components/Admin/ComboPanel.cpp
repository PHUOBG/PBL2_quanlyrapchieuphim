#include "UI/components/Admin/ComboPanel.h"
#include "UI/components/Admin/RoundedRectRenderer.h"
#include "utils/FileUtils.h"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iomanip>

static wstring s2ws(const string& s) {
    if (s.empty()) return L"";
    auto sfStr = String::fromUtf8(s.begin(), s.end());
    return sfStr.toWideString();
}

static string ws2s(const wstring& ws) {
    if (ws.empty()) return "";
    String sfStr(ws);
    auto u8 = sfStr.toUtf8();
    return string(u8.begin(), u8.end());
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

ComboPanel::ComboPanel(Font& fontRef, float w, float h)
    : font(fontRef),
      boldFont("../assets/fonts/segoeuib.ttf"),
      width(w), height(h), position(280.f, 0.f)
{
    initDefaultCombos();
    loadCombosFromFile();
    applyFilter();
}

void ComboPanel::setPosition(Vector2f pos) {
    position = pos;
}

void ComboPanel::initDefaultCombos() {
    allCombos = {
        {"CB01", L"CocaCola", L"1x Ly CocaCola t\u01b0\u01a1i 22oz nhi\u1ec1u \u0111\u00e1 m\u00e1t l\u1ea1nh", L"\u0110\u1ed3 u\u1ed1ng", 30000, false, true, false},
        {"CB02", L"B\u1eafp Caramel", L"1x B\u1eafp rang b\u01a1 \u00e1o Caramel th\u01a1m ng\u1ecdt gi\u00f2n r\u1ee5m (H\u1ed9p l\u1edbn 64oz)", L"B\u1eafp ng\u1ecdt", 70000, true, true, false},
        {"CB03", L"B\u1eafp Ph\u00f4 Mai", L"1x B\u1eafp rang l\u1eafc b\u1ed9t ph\u00f4 mai Cheddar th\u01a1m b\u00e9o \u0111\u1eadm \u0111\u00e0 (64oz)", L"B\u1eafp m\u1eb7n", 75000, false, true, false},
        {"CB04", L"1 CocaCola + 1 B\u1eafp Caramel", L"G\u00f3i Solo ng\u1ecdt: 1 Ly Coca 22oz + 1 H\u1ed9p B\u1eafp Caramel (Ti\u1ebft ki\u1ec7m 5k)", L"Combo \u0110\u01a1n", 95000, false, true, false},
        {"CB05", L"1 CocaCola + 1 B\u1eafp Ph\u00f4 Mai", L"G\u00f3i Solo m\u1eb7n: 1 Ly Coca 22oz + 1 H\u1ed9p B\u1eafp Ph\u00f4 Mai (Ti\u1ebft ki\u1ec7m 5k)", L"Combo \u0110\u01a1n", 100000, false, true, false},
        {"CB06", L"2 CocaCola + 1 B\u1eafp Caramel", L"G\u00f3i C\u1eb7p \u0110\u00f4i ng\u1ecdt: 2 Ly Coca 22oz + 1 H\u1ed9p B\u1eafp Caramel l\u1edbn", L"Combo Nh\u00f3m", 120000, false, true, false},
        {"CB07", L"2 CocaCola + 1 B\u1eafp Ph\u00f4 Mai", L"G\u00f3i C\u1eb7p \u0110\u00f4i m\u1eb7n: 2 Ly Coca 22oz + 1 H\u1ed9p B\u1eafp Ph\u00f4 Mai l\u1edbn", L"Combo Nh\u00f3m", 125000, false, true, false},
        {"CB08", L"2 CocaCola + 2 B\u1eafp Caramel", L"G\u00f3i Party ng\u1ecdt: 2 Ly Coca 22oz + 2 H\u1ed9p B\u1eafp Caramel l\u1edbn", L"Combo Nh\u00f3m", 185000, false, true, false},
        {"CB09", L"2 CocaCola + 2 B\u1eafp Ph\u00f4 Mai", L"G\u00f3i Party m\u1eb7n: 2 Ly Coca 22oz + 2 H\u1ed9p B\u1eafp Ph\u00f4 Mai l\u1edbn", L"Combo Nh\u00f3m", 190000, false, true, false},
        {"CB10", L"4 CocaCola + 2 B\u1eafp Ph\u00f4 Mai", L"G\u00f3i Gia \u0110\u00ecnh: 4 Ly Coca 22oz + 2 H\u1ed9p B\u1eafp Ph\u00f4 Mai l\u1edbn", L"Combo Nh\u00f3m", 240000, false, true, false}
    };
}

void ComboPanel::loadCombosFromFile() {
    string path = FileUtils::resolveDataPath("data/combo.txt");
    ifstream f(path);
    if (!f.is_open()) return;

    string line;
    bool isHeader = true;
    while (getline(f, line)) {
        if (line.empty()) continue;
        if (isHeader) { isHeader = false; continue; }
        stringstream ss(line);
        string id, name, pr;
        getline(ss, id, '|');
        getline(ss, name, '|');
        getline(ss, pr, '|');

        for (auto& item : allCombos) {
            if (item.id == id) {
                item.name = s2ws(name);
                try { item.price = stoi(pr); } catch (...) {}
                break;
            }
        }
    }
}

void ComboPanel::saveCombosToFile() {
    string path = FileUtils::resolveDataPath("data/combo.txt");
    ofstream f(path);
    if (!f.is_open()) return;

    f << "combo_id|combo_name|price\n";
    for (const auto& item : allCombos) {
        f << item.id << "|" << ws2s(item.name) << "|" << item.price << "\n";
    }
}

void ComboPanel::applyFilter() {
    filteredCombos.clear();
    string q = searchQuery;
    for (auto& c : q) c = (char)tolower(c);

    for (const auto& itm : allCombos) {
        // Lọc theo tabs phân loại
        if (activeFilterIndex == 1 && itm.category != L"\u0110\u1ed3 u\u1ed1ng") continue;
        if (activeFilterIndex == 2 && (itm.category != L"B\u1eafp ng\u1ecdt" && itm.category != L"B\u1eafp m\u1eb7n")) continue;
        if (activeFilterIndex == 3 && itm.category != L"Combo \u0110\u01a1n") continue;
        if (activeFilterIndex == 4 && itm.category != L"Combo Nh\u00f3m") continue;

        // Lọc theo search box
        if (!q.empty()) {
            string idLower = itm.id;
            string nameLower = ws2s(itm.name);
            for (auto& c : idLower) c = (char)tolower(c);
            for (auto& c : nameLower) c = (char)tolower(c);
            if (idLower.find(q) == string::npos && nameLower.find(q) == string::npos) {
                continue;
            }
        }

        filteredCombos.push_back(itm);
    }

    selectedRow = -1;
    scrollOffset = 0;
}

void ComboPanel::drawRoundedCard(RenderWindow& win, Vector2f pos, Vector2f sz, float r, Color fill, float outline, Color outCol) {
    RoundedRectRenderer::draw(win, pos, sz, r, fill, outline, outCol);
}

void ComboPanel::handleEvent(const Event& event, const RenderWindow& win) {
    if (const auto* mouseBtn = event.getIf<Event::MouseButtonPressed>()) {
        if (mouseBtn->button == Mouse::Button::Left) {
            Vector2f mousePos = win.mapPixelToCoords(Mouse::getPosition(win));

            // Popup save/cancel
            if (showPopup) {
                float popW = 500.f;
                float popH = 340.f;
                float popX = position.x + (width - popW) / 2.f;
                float popY = position.y + (height - popH) / 2.f - 20.f;

                FloatRect btnPopSave({popX + 70.f, popY + 270.f}, {160.f, 38.f});
                FloatRect btnPopCancel({popX + 270.f, popY + 270.f}, {160.f, 38.f});

                if (btnPopSave.contains(mousePos)) {
                    if (editingIndex >= 0 && editingIndex < (int)allCombos.size()) {
                        allCombos[editingIndex].name = popupName;
                        allCombos[editingIndex].subtitle = popupSub;
                        try { allCombos[editingIndex].price = stoi(popupPrice); } catch (...) {}
                    } else {
                        ComboItem ni;
                        ni.id = popupId.empty() ? ("CB" + to_string(allCombos.size() + 1)) : popupId;
                        ni.name = popupName;
                        ni.subtitle = popupSub;
                        ni.category = popupCat.empty() ? L"Combo \u0110\u01a1n" : popupCat;
                        try { ni.price = stoi(popupPrice); } catch (...) { ni.price = 50000; }
                        allCombos.push_back(ni);
                    }
                    saveCombosToFile();
                    applyFilter();
                    showPopup = false;
                    notificationMessage = "Đã lưu thông tin combo thành công!";
                    notificationClock.restart();
                    notificationVisible = true;
                    return;
                }

                if (btnPopCancel.contains(mousePos)) {
                    showPopup = false;
                    return;
                }
                return;
            }

            // Nút Header: Reload, Xuất Excel
            FloatRect btnRel({position.x + width - 170.f, position.y + 76.f}, {40.f, 38.f});
            FloatRect btnExp({position.x + width - 120.f, position.y + 76.f}, {90.f, 38.f});

            if (btnRel.contains(mousePos)) {
                loadCombosFromFile();
                applyFilter();
                notificationMessage = "Đã làm mới dữ liệu combo bắp nước!";
                notificationClock.restart();
                notificationVisible = true;
                return;
            }

            if (btnExp.contains(mousePos)) {
                notificationMessage = "Đã xuất bảng giá F&B ra file Excel thành công!";
                notificationClock.restart();
                notificationVisible = true;
                return;
            }

            // Toolbar buttons: Thêm combo, Sửa combo, Xóa combo, Đồng bộ POS
            float tbY = position.y + 246.f;
            FloatRect btnAdd({position.x + 30.f, tbY}, {130.f, 38.f});
            FloatRect btnEdit({position.x + 170.f, tbY}, {120.f, 38.f});
            FloatRect btnDel({position.x + 300.f, tbY}, {120.f, 38.f});
            FloatRect btnSync({position.x + width - 170.f, tbY}, {140.f, 38.f});

            if (btnAdd.contains(mousePos)) {
                showPopup = true;
                editingIndex = -1;
                popupId = "CB" + to_string(allCombos.size() + 1);
                popupName = L"Combo M\u1edbi";
                popupSub = L"Quy chu\u1ea9n chi ti\u1ebft combo...";
                popupCat = L"Combo \u0110\u01a1n";
                popupPrice = "85000";
                return;
            }

            if (btnEdit.contains(mousePos)) {
                if (selectedRow >= 0 && selectedRow < (int)filteredCombos.size()) {
                    string selId = filteredCombos[selectedRow].id;
                    for (size_t i = 0; i < allCombos.size(); ++i) {
                        if (allCombos[i].id == selId) {
                            editingIndex = (int)i;
                            popupId = allCombos[i].id;
                            popupName = allCombos[i].name;
                            popupSub = allCombos[i].subtitle;
                            popupCat = allCombos[i].category;
                            popupPrice = to_string(allCombos[i].price);
                            showPopup = true;
                            return;
                        }
                    }
                } else {
                    notificationMessage = "Vui lòng chọn 1 combo để sửa!";
                    notificationClock.restart();
                    notificationVisible = true;
                }
                return;
            }

            if (btnDel.contains(mousePos)) {
                if (selectedRow >= 0 && selectedRow < (int)filteredCombos.size()) {
                    string selId = filteredCombos[selectedRow].id;
                    for (auto it = allCombos.begin(); it != allCombos.end(); ++it) {
                        if (it->id == selId) {
                            allCombos.erase(it);
                            break;
                        }
                    }
                    saveCombosToFile();
                    applyFilter();
                    notificationMessage = "Đã xóa combo thành công!";
                    notificationClock.restart();
                    notificationVisible = true;
                } else {
                    notificationMessage = "Vui lòng chọn combo cần xóa!";
                    notificationClock.restart();
                    notificationVisible = true;
                }
                return;
            }

            if (btnSync.contains(mousePos)) {
                notificationMessage = "Đã đồng bộ thực đơn xuống hệ thống máy POS quầy!";
                notificationClock.restart();
                notificationVisible = true;
                return;
            }

            // Search box click
            float fltY = position.y + 296.f;
            FloatRect searchBox({position.x + 30.f, fltY}, {300.f, 36.f});
            searchActive = searchBox.contains(mousePos);

            // Filter pills: Tất cả (10), Nước lẻ, Bắp đơn, Combo 1 Người, Combo Nhóm
            vector<pair<float, float>> pillRects = {
                {400.f, 80.f},   // Tất cả
                {490.f, 75.f},   // Nước lẻ
                {575.f, 75.f},   // Bắp đơn
                {660.f, 110.f},  // Combo 1 Người
                {780.f, 150.f}   // Combo Nhóm / Cặp đôi
            };

            for (size_t i = 0; i < pillRects.size(); ++i) {
                FloatRect pB({position.x + pillRects[i].first, fltY}, {pillRects[i].second, 36.f});
                if (pB.contains(mousePos)) {
                    activeFilterIndex = (int)i;
                    applyFilter();
                    return;
                }
            }

            // Table Row Click & Actions
            float tableY = position.y + 350.f;
            float rowH = 72.f;
            int count = min((int)filteredCombos.size() - scrollOffset, maxVisibleRows);

            for (int r = 0; r < count; ++r) {
                float rY = tableY + 44.f + r * rowH;
                FloatRect rowRect({position.x + 30.f, rY}, {width - 60.f, rowH});

                if (rowRect.contains(mousePos)) {
                    selectedRow = scrollOffset + r;

                    // Click checkbox
                    FloatRect cbRect({position.x + 40.f, rY + 26.f}, {20.f, 20.f});
                    if (cbRect.contains(mousePos)) {
                        filteredCombos[selectedRow].isChecked = !filteredCombos[selectedRow].isChecked;
                        return;
                    }

                    // Click action edit
                    FloatRect btnRowEdit({position.x + width - 110.f, rY + 24.f}, {24.f, 24.f});
                    if (btnRowEdit.contains(mousePos)) {
                        string selId = filteredCombos[selectedRow].id;
                        for (size_t i = 0; i < allCombos.size(); ++i) {
                            if (allCombos[i].id == selId) {
                                editingIndex = (int)i;
                                popupId = allCombos[i].id;
                                popupName = allCombos[i].name;
                                popupSub = allCombos[i].subtitle;
                                popupCat = allCombos[i].category;
                                popupPrice = to_string(allCombos[i].price);
                                showPopup = true;
                                return;
                            }
                        }
                        return;
                    }

                    // Click action delete
                    FloatRect btnRowDel({position.x + width - 75.f, rY + 24.f}, {24.f, 24.f});
                    if (btnRowDel.contains(mousePos)) {
                        string selId = filteredCombos[selectedRow].id;
                        for (auto it = allCombos.begin(); it != allCombos.end(); ++it) {
                            if (it->id == selId) {
                                allCombos.erase(it);
                                break;
                            }
                        }
                        saveCombosToFile();
                        applyFilter();
                        notificationMessage = "Đã xóa combo thành công!";
                        notificationClock.restart();
                        notificationVisible = true;
                        return;
                    }
                    return;
                }
            }
        }
    }

    // Scroll
    if (const auto* wheel = event.getIf<Event::MouseWheelScrolled>()) {
        if (!showPopup) {
            if (wheel->delta > 0) {
                if (scrollOffset > 0) scrollOffset--;
            } else if (wheel->delta < 0) {
                if (scrollOffset + maxVisibleRows < (int)filteredCombos.size()) scrollOffset++;
            }
        }
    }

    // Search typing
    if (const auto* textEv = event.getIf<Event::TextEntered>()) {
        if (searchActive && !showPopup) {
            char32_t c = textEv->unicode;
            if (c == 8) {
                if (!searchQuery.empty()) {
                    searchQuery.pop_back();
                    applyFilter();
                }
            } else if (c >= 32 && c != 127) {
                if (searchQuery.size() < 30) {
                    searchQuery.push_back((char)c);
                    applyFilter();
                }
            }
        }
    }
}

void ComboPanel::update(Vector2f mousePos, bool) {
    hoveredRow = -1;
    if (!showPopup) {
        float tableY = position.y + 350.f;
        float rowH = 72.f;
        int count = min((int)filteredCombos.size() - scrollOffset, maxVisibleRows);

        for (int r = 0; r < count; ++r) {
            float rY = tableY + 44.f + r * rowH;
            if (FloatRect({position.x + 30.f, rY}, {width - 60.f, rowH}).contains(mousePos)) {
                hoveredRow = r;
                break;
            }
        }
    }
}

void ComboPanel::render(RenderWindow& win) {
    // 1. Nền Canvas
    RectangleShape bg({width, height});
    bg.setPosition(position);
    bg.setFillColor(Color(244, 246, 249));
    win.draw(bg);

    // ==========================================
    // 2. TOP BAR
    // ==========================================
    Text bc1(boldFont, L"\u2263  PBL2 - H\u1ec6 TH\u1ed0NG QU\u1ea2N L\u00dd R\u1ea0P CHI\u1ebeU PHIM & TH\u1ed0NG K\u00ca DOANH THU", 12);
    bc1.setFillColor(Color(2, 132, 199));
    bc1.setPosition({position.x + 30.f, position.y + 20.f});
    win.draw(bc1);

    Text slash1(boldFont, L"  /  Khu v\u1ef1c F&B Cinema", 12);
    slash1.setFillColor(Color(100, 116, 139));
    slash1.setPosition({position.x + 520.f, position.y + 20.f});
    win.draw(slash1);

    float rOffset = position.x + width - 30.f;
    CircleShape bellCircle(15.f); bellCircle.setFillColor(Color(241, 245, 249)); bellCircle.setPosition({rOffset - 270.f, position.y + 13.f}); win.draw(bellCircle);
    CircleShape bellRedDot(3.5f); bellRedDot.setFillColor(Color(239, 68, 68)); bellRedDot.setPosition({rOffset - 253.f, position.y + 14.f}); win.draw(bellRedDot);

    Text locTxt(font, L"\u0110\u00e0 N\u1eb5ng, Vi\u1ec7t Nam", 11); locTxt.setFillColor(Color(51, 65, 85)); locTxt.setPosition({rOffset - 230.f, position.y + 12.f}); win.draw(locTxt);
    Text shiftTxt(font, L"Ca tr\u1ef1c: Chi\u1ec1u - T\u1ed1i", 10); shiftTxt.setFillColor(Color(100, 116, 139)); shiftTxt.setPosition({rOffset - 230.f, position.y + 28.f}); win.draw(shiftTxt);

    CircleShape avatarBg(16.f); avatarBg.setFillColor(Color(2, 132, 199)); avatarBg.setPosition({rOffset - 90.f, position.y + 12.f}); win.draw(avatarBg);
    Text avText(boldFont, L"AD", 12); avText.setFillColor(Color::White); avText.setPosition({rOffset - 83.f, position.y + 20.f}); win.draw(avText);

    Text uName(boldFont, L"Admin CineXine", 12); uName.setFillColor(Color(15, 23, 42)); uName.setPosition({rOffset - 50.f, position.y + 12.f}); win.draw(uName);
    Text uRole(font, L"To\u00e0n quy\u1ec1n v\u1eadn h\u00e0nh", 10); uRole.setFillColor(Color(100, 116, 139)); uRole.setPosition({rOffset - 50.f, position.y + 28.f}); win.draw(uRole);

    // ==========================================
    // 3. PAGE HEADER
    // ==========================================
    Text title(boldFont, L"Qu\u1ea3n l\u00fd combo b\u1eafp n\u01b0\u1edbc", 24);
    title.setFillColor(Color(15, 23, 42));
    title.setPosition({position.x + 30.f, position.y + 68.f});
    win.draw(title);

    FloatRect tb = title.getGlobalBounds();
    drawRoundedCard(win, {tb.position.x + tb.size.x + 16.f, position.y + 74.f}, {150.f, 24.f}, 12.f, Color(220, 252, 231));
    Text bBadge(boldFont, to_wstring(allCombos.size()) + L" Combo \u0110ang B\u00e1n", 11);
    bBadge.setFillColor(Color(22, 101, 52));
    bBadge.setPosition({tb.position.x + tb.size.x + 24.f, position.y + 78.f});
    win.draw(bBadge);

    Text subTitle(font, L"C\u1eadp nh\u1eadt b\u1ea3ng gi\u00e1, c\u00f4ng th\u1ee9c m\u00f3n b\u1eafp rang, n\u01b0\u1edbc ng\u1ecdt v\u00e0 c\u00e1c g\u00f3i \u01b0u \u0111\u00e3i t\u1ea1i qu\u1ea7y b\u1eafp n\u01b0\u1edbc.", 13);
    subTitle.setFillColor(Color(100, 116, 139));
    subTitle.setPosition({position.x + 30.f, position.y + 104.f});
    win.draw(subTitle);

    // Nút Reload và Xuất Excel góc phải
    drawRoundedCard(win, {position.x + width - 170.f, position.y + 76.f}, {40.f, 38.f}, 6.f, Color::White, 1.f, Color(203, 213, 225));
    Text relIcon(boldFont, L"\u21bb", 18); relIcon.setFillColor(Color(71, 85, 105)); relIcon.setPosition({position.x + width - 158.f, position.y + 83.f}); win.draw(relIcon);

    drawRoundedCard(win, {position.x + width - 120.f, position.y + 76.f}, {90.f, 38.f}, 6.f, Color::White, 1.f, Color(203, 213, 225));
    Text expIcon(boldFont, L"\u229e", 13); expIcon.setFillColor(Color(22, 101, 52)); expIcon.setPosition({position.x + width - 110.f, position.y + 87.f}); win.draw(expIcon);
    Text expTxt(boldFont, L"Xu\u1ea5t Excel", 12); expTxt.setFillColor(Color(15, 23, 42)); expTxt.setPosition({position.x + width - 92.f, position.y + 87.f}); win.draw(expTxt);

    // ==========================================
    // 4. 4 THẺ THỐNG KÊ KPI (STAT CARDS GRID)
    // ==========================================
    float statY = position.y + 144.f;
    float cardW = (width - 60.f - 45.f) / 4.f;
    float cardH = 88.f;

    // Card 1: Tổng danh mục
    float c1X = position.x + 30.f;
    drawRoundedCard(win, {c1X, statY}, {cardW, cardH}, 10.f, Color::White, 1.f, Color(226, 232, 240));
    Text sc1T(boldFont, L"T\u1ed4NG DANH M\u1ee4C", 11); sc1T.setFillColor(Color(100, 116, 139)); sc1T.setPosition({c1X + 16.f, statY + 12.f}); win.draw(sc1T);
    CircleShape ic1(12.f); ic1.setFillColor(Color(224, 242, 254)); ic1.setPosition({c1X + cardW - 36.f, statY + 10.f}); win.draw(ic1);
    Text sc1V(boldFont, to_wstring(allCombos.size()), 24); sc1V.setFillColor(Color(15, 23, 42)); sc1V.setPosition({c1X + 16.f, statY + 28.f}); win.draw(sc1V);
    FloatRect v1b = sc1V.getGlobalBounds();
    drawRoundedCard(win, {v1b.position.x + v1b.size.x + 10.f, statY + 34.f}, {85.f, 20.f}, 10.f, Color(220, 252, 231));
    Text sc1B(boldFont, L"100% Kh\u1ea3 d\u1ee5ng", 10); sc1B.setFillColor(Color(22, 101, 52)); sc1B.setPosition({v1b.position.x + v1b.size.x + 16.f, statY + 37.f}); win.draw(sc1B);
    Text sc1S(font, L"G\u1ed3m n\u01b0\u1edbc ng\u1ecdt, b\u1eafp v\u1ecb, combo \u0111\u00f4i", 10); sc1S.setFillColor(Color(148, 163, 184)); sc1S.setPosition({c1X + 16.f, statY + 62.f}); win.draw(sc1S);

    // Card 2: Doanh thu F&B hôm nay
    float c2X = c1X + cardW + 15.f;
    drawRoundedCard(win, {c2X, statY}, {cardW, cardH}, 10.f, Color::White, 1.f, Color(226, 232, 240));
    Text sc2T(boldFont, L"DOANH THU F&B H\u00d4M NAY", 11); sc2T.setFillColor(Color(100, 116, 139)); sc2T.setPosition({c2X + 16.f, statY + 12.f}); win.draw(sc2T);
    CircleShape ic2(12.f); ic2.setFillColor(Color(220, 252, 231)); ic2.setPosition({c2X + cardW - 36.f, statY + 10.f}); win.draw(ic2);
    Text sc2V(boldFont, L"14.850.000 \u0111", 18); sc2V.setFillColor(Color(15, 23, 42)); sc2V.setPosition({c2X + 16.f, statY + 32.f}); win.draw(sc2V);
    FloatRect v2b = sc2V.getGlobalBounds();
    Text sc2Inc(boldFont, L"+18.4%", 10); sc2Inc.setFillColor(Color(16, 185, 129)); sc2Inc.setPosition({v2b.position.x + v2b.size.x + 8.f, statY + 36.f}); win.draw(sc2Inc);
    Text sc2S(font, L"Chi\u1ebfm 36% t\u1ed5ng doanh thu v\u00e9", 10); sc2S.setFillColor(Color(148, 163, 184)); sc2S.setPosition({c2X + 16.f, statY + 62.f}); win.draw(sc2S);

    // Card 3: Best Seller hôm nay
    float c3X = c2X + cardW + 15.f;
    drawRoundedCard(win, {c3X, statY}, {cardW, cardH}, 10.f, Color::White, 1.f, Color(226, 232, 240));
    Text sc3T(boldFont, L"BEST SELLER H\u00d4M NAY", 11); sc3T.setFillColor(Color(100, 116, 139)); sc3T.setPosition({c3X + 16.f, statY + 12.f}); win.draw(sc3T);
    CircleShape ic3(12.f); ic3.setFillColor(Color(254, 243, 199)); ic3.setPosition({c3X + cardW - 36.f, statY + 10.f}); win.draw(ic3);
    Text sc3V(boldFont, L"CB02: B\u1eafp Caramel", 16); sc3V.setFillColor(Color(15, 23, 42)); sc3V.setPosition({c3X + 16.f, statY + 34.f}); win.draw(sc3V);
    Text sc3S(boldFont, L"184 ph\u1ea7n \u0111\u00e3 b\u00e1n ra", 10); sc3S.setFillColor(Color(217, 119, 6)); sc3S.setPosition({c3X + 16.f, statY + 62.f}); win.draw(sc3S);

    // Card 4: Giá trị trung bình F&B
    float c4X = c3X + cardW + 15.f;
    drawRoundedCard(win, {c4X, statY}, {cardW, cardH}, 10.f, Color::White, 1.f, Color(226, 232, 240));
    Text sc4T(boldFont, L"GI\u00c1 TR\u1eca TRUNG B\u00ccNH F&B", 11); sc4T.setFillColor(Color(100, 116, 139)); sc4T.setPosition({c4X + 16.f, statY + 12.f}); win.draw(sc4T);
    CircleShape ic4(12.f); ic4.setFillColor(Color(243, 232, 255)); ic4.setPosition({c4X + cardW - 36.f, statY + 10.f}); win.draw(ic4);
    Text sc4V(boldFont, L"115.000 \u0111", 18); sc4V.setFillColor(Color(15, 23, 42)); sc4V.setPosition({c4X + 16.f, statY + 32.f}); win.draw(sc4V);
    FloatRect v4b = sc4V.getGlobalBounds();
    Text sc4Tx(font, L"/ giao d\u1ecbch", 10); sc4Tx.setFillColor(Color(100, 116, 139)); sc4Tx.setPosition({v4b.position.x + v4b.size.x + 4.f, statY + 36.f}); win.draw(sc4Tx);
    Text sc4S(font, L"Th\u01b0\u1eddng mua k\u00e8m v\u00e9 \u0111\u00f4i", 10); sc4S.setFillColor(Color(147, 51, 234)); sc4S.setPosition({c4X + 16.f, statY + 62.f}); win.draw(sc4S);

    // ==========================================
    // 5. TOOLBAR & FILTER PILLS
    // ==========================================
    float tbY = position.y + 246.f;

    // Nút 1: + Thêm combo
    drawRoundedCard(win, {position.x + 30.f, tbY}, {130.f, 38.f}, 6.f, Color(5, 150, 105));
    Text addTxt(boldFont, L"+ Th\u00eam combo", 13); addTxt.setFillColor(Color::White); addTxt.setPosition({position.x + 42.f, tbY + 9.f}); win.draw(addTxt);

    // Nút 2: Sửa combo
    drawRoundedCard(win, {position.x + 170.f, tbY}, {120.f, 38.f}, 6.f, Color(217, 119, 6));
    Text editTxt(boldFont, L"\u270e  S\u1eeda combo", 13); editTxt.setFillColor(Color::White); editTxt.setPosition({position.x + 182.f, tbY + 9.f}); win.draw(editTxt);

    // Nút 3: Xóa combo
    drawRoundedCard(win, {position.x + 300.f, tbY}, {120.f, 38.f}, 6.f, Color(220, 38, 38));
    Text delTxt(boldFont, L"\u2296  X\u00f3a combo", 13); delTxt.setFillColor(Color::White); delTxt.setPosition({position.x + 312.f, tbY + 9.f}); win.draw(delTxt);

    // Nút 4: Đồng bộ máy POS
    drawRoundedCard(win, {position.x + width - 170.f, tbY}, {140.f, 38.f}, 6.f, Color(238, 242, 255), 1.f, Color(199, 210, 254));
    Text syncTxt(boldFont, L"\u2601  \u0110\u1ed3ng b\u1ed9 m\u00e1y POS", 12); syncTxt.setFillColor(Color(67, 56, 202)); syncTxt.setPosition({position.x + width - 162.f, tbY + 10.f}); win.draw(syncTxt);

    // Dòng 2: Ô tìm kiếm & Bộ lọc
    float fltY = position.y + 296.f;
    drawRoundedCard(win, {position.x + 30.f, fltY}, {300.f, 36.f}, 6.f, Color::White, searchActive ? 1.5f : 1.f, searchActive ? Color(2, 132, 199) : Color(203, 213, 225));
    Text sIcon(boldFont, L"\U0001F50D", 12); sIcon.setFillColor(Color(148, 163, 184)); sIcon.setPosition({position.x + 40.f, fltY + 9.f}); win.draw(sIcon);

    wstring sPrompt = searchQuery.empty() ? L"T\u00ecm theo m\u00e3 combo (CB01...) ho\u1eb7c t\u00ean s\u1ea3n ph\u1ea9m..." : s2ws(searchQuery) + (searchActive ? L"|" : L"");
    Text sTxt(font, sPrompt, 11); sTxt.setFillColor(searchQuery.empty() ? Color(148, 163, 184) : Color(15, 23, 42)); sTxt.setPosition({position.x + 64.f, fltY + 9.f}); win.draw(sTxt);

    Text locLbl(boldFont, L"L\u1eccC:", 11); locLbl.setFillColor(Color(100, 116, 139)); locLbl.setPosition({position.x + 350.f, fltY + 9.f}); win.draw(locLbl);

    // Các Pill filter
    vector<pair<wstring, float>> pills = {
        {L"T\u1ea5t c\u1ea3 (10)", 80.f},
        {L"N\u01b0\u1edbc l\u1ebb", 75.f},
        {L"B\u1eafp \u0111\u01a1n", 75.f},
        {L"Combo 1 Ng\u01b0\u1eddi", 110.f},
        {L"Combo Nh\u00f3m / C\u1eb7p \u0111\u00f4i", 150.f}
    };
    float pX = position.x + 390.f;
    for (size_t i = 0; i < pills.size(); ++i) {
        bool isAct = ((int)i == activeFilterIndex);
        drawRoundedCard(win, {pX, fltY}, {pills[i].second, 34.f}, 17.f, isAct ? Color(2, 132, 199) : Color::White, isAct ? 0.f : 1.f, Color(226, 232, 240));
        Text pTxt(boldFont, pills[i].first, 11);
        pTxt.setFillColor(isAct ? Color::White : Color(71, 85, 105));
        FloatRect ptb = pTxt.getLocalBounds();
        pTxt.setPosition({pX + (pills[i].second - ptb.size.x) / 2.f - ptb.position.x, fltY + (34.f - ptb.size.y) / 2.f - ptb.position.y});
        win.draw(pTxt);
        pX += pills[i].second + 10.f;
    }

    // ==========================================
    // 6. BẢNG DỮ LIỆU COMBO (DATA TABLE)
    // ==========================================
    float tableY = position.y + 346.f;
    float tableW = width - 60.f;
    float rowH = 72.f;

    // Header bảng (#0077B6)
    drawRoundedCard(win, {position.x + 30.f, tableY}, {tableW, 44.f}, 6.f, Color(0, 119, 182));

    drawRoundedCard(win, {position.x + 44.f, tableY + 14.f}, {16.f, 16.f}, 3.f, Color(255, 255, 255, 60));
    Text h1(boldFont, L"M\u00c3 COMBO", 11); h1.setFillColor(Color::White); h1.setPosition({position.x + 85.f, tableY + 14.f}); win.draw(h1);
    Text h2(boldFont, L"T\u00caN COMBO & QUY CHU\u1ea8N CHI TI\u1ebeT", 11); h2.setFillColor(Color::White); h2.setPosition({position.x + 230.f, tableY + 14.f}); win.draw(h2);
    Text h3(boldFont, L"PH\u00c2N LO\u1ea0I", 11); h3.setFillColor(Color::White); h3.setPosition({position.x + 550.f, tableY + 14.f}); win.draw(h3);
    Text h4(boldFont, L"GI\u00c1 B\u00c1N NI\u00caM Y\u1ebeT", 11); h4.setFillColor(Color::White); h4.setPosition({position.x + 720.f, tableY + 14.f}); win.draw(h4);
    Text h5(boldFont, L"TR\u1ea0NG TH\u00c1I", 11); h5.setFillColor(Color::White); h5.setPosition({position.x + 880.f, tableY + 14.f}); win.draw(h5);
    Text h6(boldFont, L"THAO T\u00c1C", 11); h6.setFillColor(Color::White); h6.setPosition({position.x + width - 110.f, tableY + 14.f}); win.draw(h6);

    // Dòng dữ liệu
    int count = min((int)filteredCombos.size() - scrollOffset, maxVisibleRows);

    for (int r = 0; r < count; ++r) {
        int idx = scrollOffset + r;
        const auto& itm = filteredCombos[idx];
        float rY = tableY + 44.f + r * rowH;

        bool isSel = (selectedRow == idx);
        bool isHov = (hoveredRow == r);

        Color rowBg = isSel ? Color(224, 242, 254) : (isHov ? Color(248, 250, 252) : Color::White);
        drawRoundedCard(win, {position.x + 30.f, rY}, {tableW, rowH}, 0.f, rowBg, 0.5f, Color(226, 232, 240));

        // 1. Checkbox
        drawRoundedCard(win, {position.x + 44.f, rY + 28.f}, {16.f, 16.f}, 3.f, itm.isChecked ? Color(2, 132, 199) : Color::White, 1.f, Color(203, 213, 225));
        if (itm.isChecked) {
            Text ckTxt(boldFont, L"\u2714", 10); ckTxt.setFillColor(Color::White); ckTxt.setPosition({position.x + 46.f, rY + 28.f}); win.draw(ckTxt);
        }

        // 2. Mã combo (Badge viền mỏng)
        drawRoundedCard(win, {position.x + 80.f, rY + 24.f}, {55.f, 24.f}, 4.f, Color(248, 250, 252), 1.f, Color(226, 232, 240));
        Text idTxt(boldFont, s2ws(itm.id), 11); idTxt.setFillColor(Color(51, 65, 85)); idTxt.setPosition({position.x + 92.f, rY + 28.f}); win.draw(idTxt);

        // 3. Tên combo & Quy chuẩn chi tiết
        // Icon hình tượng (Cốc nước / Hộp bắp)
        drawRoundedCard(win, {position.x + 180.f, rY + 16.f}, {40.f, 40.f}, 8.f, itm.category == L"\u0110\u1ed3 u\u1ed1ng" ? Color(254, 226, 226) : Color(254, 243, 199));
        Text icDish(boldFont, itm.category == L"\u0110\u1ed3 u\u1ed1ng" ? L"\U0001F964" : L"\U0001F37F", 14);
        icDish.setFillColor(Color(180, 83, 9));
        icDish.setPosition({position.x + 192.f, rY + 26.f});
        win.draw(icDish);

        Text nameTxt(boldFont, itm.name, 13);
        nameTxt.setFillColor(Color(15, 23, 42));
        nameTxt.setPosition({position.x + 230.f, rY + 16.f});
        win.draw(nameTxt);

        // Badge HOT nếu có
        if (itm.isHot) {
            FloatRect nb = nameTxt.getGlobalBounds();
            drawRoundedCard(win, {nb.position.x + nb.size.x + 8.f, rY + 16.f}, {34.f, 18.f}, 4.f, Color(245, 158, 11));
            Text hotTxt(boldFont, L"HOT", 9); hotTxt.setFillColor(Color::White); hotTxt.setPosition({nb.position.x + nb.size.x + 12.f, rY + 18.f}); win.draw(hotTxt);
        }

        Text subTxt(font, itm.subtitle, 11);
        subTxt.setFillColor(Color(100, 116, 139));
        subTxt.setPosition({position.x + 230.f, rY + 38.f});
        win.draw(subTxt);

        // 4. Phân loại (Badge pill)
        Color catBg = Color(241, 245, 249);
        Color catFg = Color(71, 85, 105);
        if (itm.category == L"B\u1eafp ng\u1ecdt") { catBg = Color(254, 243, 199); catFg = Color(180, 83, 9); }
        else if (itm.category == L"B\u1eafp m\u1eb7n") { catBg = Color(254, 215, 170); catFg = Color(194, 65, 12); }
        else if (itm.category == L"Combo \u0110\u01a1n") { catBg = Color(224, 242, 254); catFg = Color(2, 132, 199); }
        else if (itm.category == L"Combo Nh\u00f3m") { catBg = Color(243, 232, 255); catFg = Color(126, 34, 206); }

        drawRoundedCard(win, {position.x + 550.f, rY + 24.f}, {80.f, 24.f}, 12.f, catBg);
        Text catTxt(boldFont, itm.category, 10); catTxt.setFillColor(catFg);
        FloatRect ctb = catTxt.getLocalBounds();
        catTxt.setPosition({position.x + 550.f + (80.f - ctb.size.x) / 2.f - ctb.position.x, rY + 24.f + (24.f - ctb.size.y) / 2.f - ctb.position.y});
        win.draw(catTxt);

        // 5. Giá bán niêm yết
        Text prTxt(boldFont, s2ws(formatPrice(itm.price)), 14);
        prTxt.setFillColor(Color(15, 23, 42));
        prTxt.setPosition({position.x + 720.f, rY + 26.f});
        win.draw(prTxt);

        // 6. Trạng thái: ● Đang bán
        drawRoundedCard(win, {position.x + 880.f, rY + 24.f}, {85.f, 24.f}, 12.f, Color(220, 252, 231));
        CircleShape dotG(3.f); dotG.setFillColor(Color(22, 101, 52)); dotG.setPosition({position.x + 888.f, rY + 33.f}); win.draw(dotG);
        Text stTxt(boldFont, L"\u0110ang b\u00e1n", 10); stTxt.setFillColor(Color(22, 101, 52)); stTxt.setPosition({position.x + 898.f, rY + 29.f}); win.draw(stTxt);

        // 7. Thao tác: Bút chì & Thùng rác
        float actX = position.x + width - 110.f;
        Text icEd(boldFont, L"\u270e", 15); icEd.setFillColor(Color(148, 163, 184)); icEd.setPosition({actX, rY + 26.f}); win.draw(icEd);
        Text icDl(boldFont, L"\u2296", 15); icDl.setFillColor(Color(148, 163, 184)); icDl.setPosition({actX + 35.f, rY + 26.f}); win.draw(icDl);
    }

    // ==========================================
    // 7. MODAL POPUP THÊM / SỬA COMBO
    // ==========================================
    if (showPopup) {
        RectangleShape overlay({width, height});
        overlay.setPosition(position);
        overlay.setFillColor(Color(15, 23, 42, 160));
        win.draw(overlay);

        float popW = 500.f;
        float popH = 340.f;
        float popX = position.x + (width - popW) / 2.f;
        float popY = position.y + (height - popH) / 2.f - 20.f;

        drawRoundedCard(win, {popX, popY}, {popW, popH}, 12.f, Color::White);

        Text pTitle(boldFont, (editingIndex >= 0) ? L"S\u1eeda th\u00f4ng tin combo" : L"Th\u00eam combo m\u1edbi", 18);
        pTitle.setFillColor(Color(15, 23, 42));
        pTitle.setPosition({popX + 30.f, popY + 22.f});
        win.draw(pTitle);

        Text l1(boldFont, L"T\u00ean combo:", 12); l1.setFillColor(Color(71, 85, 105)); l1.setPosition({popX + 30.f, popY + 65.f}); win.draw(l1);
        drawRoundedCard(win, {popX + 30.f, popY + 85.f}, {popW - 60.f, 36.f}, 6.f, Color(248, 250, 252), 1.f, Color(203, 213, 225));
        Text v1(font, popupName, 12); v1.setFillColor(Color(15, 23, 42)); v1.setPosition({popX + 40.f, popY + 94.f}); win.draw(v1);

        Text l2(boldFont, L"Quy chu\u1ea9n chi ti\u1ebft:", 12); l2.setFillColor(Color(71, 85, 105)); l2.setPosition({popX + 30.f, popY + 130.f}); win.draw(l2);
        drawRoundedCard(win, {popX + 30.f, popY + 150.f}, {popW - 60.f, 36.f}, 6.f, Color(248, 250, 252), 1.f, Color(203, 213, 225));
        Text v2(font, popupSub, 12); v2.setFillColor(Color(15, 23, 42)); v2.setPosition({popX + 40.f, popY + 159.f}); win.draw(v2);

        Text l3(boldFont, L"Gi\u00e1 ni\u00eam y\u1ebft (\u0111):", 12); l3.setFillColor(Color(71, 85, 105)); l3.setPosition({popX + 30.f, popY + 195.f}); win.draw(l3);
        drawRoundedCard(win, {popX + 30.f, popY + 215.f}, {popW - 60.f, 36.f}, 6.f, Color(248, 250, 252), 1.f, Color(203, 213, 225));
        Text v3(font, s2ws(popupPrice), 12); v3.setFillColor(Color(15, 23, 42)); v3.setPosition({popX + 40.f, popY + 224.f}); win.draw(v3);

        // Nút Lưu và Hủy
        drawRoundedCard(win, {popX + 70.f, popY + 270.f}, {160.f, 38.f}, 6.f, Color(5, 150, 105));
        Text sTxt(boldFont, L"L\u01b0u th\u00f4ng tin", 13); sTxt.setFillColor(Color::White); sTxt.setPosition({popX + 105.f, popY + 279.f}); win.draw(sTxt);

        drawRoundedCard(win, {popX + 270.f, popY + 270.f}, {160.f, 38.f}, 6.f, Color(241, 245, 249), 1.f, Color(203, 213, 225));
        Text cTxt(boldFont, L"H\u1ee7y b\u1ecf", 13); cTxt.setFillColor(Color(71, 85, 105)); cTxt.setPosition({popX + 325.f, popY + 279.f}); win.draw(cTxt);
    }

    // ==========================================
    // 8. TOAST NOTIFICATION
    // ==========================================
    if (notificationVisible) {
        if (notificationClock.getElapsedTime().asSeconds() > 3.0f) {
            notificationVisible = false;
        } else {
            float notW = 380.f;
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
