#include "UI/components/Admin/VoucherPanel.h"
#include "UI/components/Admin/UITheme.h"
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

VoucherPanel::VoucherPanel(Font& fontRef, float w, float h)
    : font(fontRef),
      boldFont("../assets/fonts/segoeuib.ttf"),
      width(w), height(h), position(280.f, 0.f)
{
    initDefaultData();
    loadVouchersFromFile();
    applyVoucherFilter();
    applyRecipientFilter();
}

void VoucherPanel::setPosition(Vector2f pos) {
    position = pos;
}

void VoucherPanel::initDefaultData() {
    allVouchers = {
        {"WELCOME", L"Ch\u00e0o m\u1eebng th\u00e0nh vi\u00ean CineCore", 2, 20, 0, 145, L"M\u1edbi", true},
        {"METIZ", L"Li\u00ean k\u1ebft \u0111\u1ed1i t\u00e1c", 1, 30000, 50000, 0, L"", false},
        {"NGUYENVU", L"Kh\u00e1ch h\u00e0ng \u0111\u1eb7c quy\u1ec1n CineCore", 2, 50, 100000, 0, L"VIP", false},
        {"CINEMA2024", L"Chi\u1ebfn d\u1ecbch M\u00f9a h\u00e8 bom t\u1ea5n", 1, 45000, 120000, 520, L"", false},
        {"POPCORNFREE", L"T\u1eb7ng k\u00e8m b\u1eafp ng\u1ecdt khi mua 2 v\u00e9", 1, 35000, 150000, 312, L"", false},
        {"STUDENT50", L"\u01afu \u0111\u00e3i h\u1ecdc sinh sinh vi\u00ean th\u1ee9 4", 2, 30, 50000, 89, L"HSSV", false},
        // Thêm 6 voucher cho trang 2 để đủ 12 chiến dịch
        {"SUMMER2026", L"\u01afu \u0111\u00e3i v\u00e9 xem phim h\u00e8", 2, 15, 0, 95, L"HOT", false},
        {"VIPMEMBER", L"\u0110\u1eb7c quy\u1ec1n h\u1ed9i vi\u00ean v\u00e0ng", 1, 50000, 150000, 60, L"VIP", false},
        {"COMBO10K", L"Gi\u1ea3m tr\u1ef1c ti\u1ebfp combo b\u1eafp n\u01b0\u1edbc", 1, 10000, 60000, 120, L"", false},
        {"FREEDRINK", L"T\u1eb7ng 1 ly CocaCola c\u1ee1 l\u1edbn", 1, 30000, 100000, 78, L"", false},
        {"WEEKEND20", L"Cu\u1ed1i tu\u1ea7n th\u1ea3 ga kh\u00f4ng lo v\u1ec1 gi\u00e1", 2, 20, 120000, 110, L"", false},
        {"BDAYGIFT", L"Qu\u00e0 t\u1eb7ng sinh nh\u1eadt th\u00e0nh vi\u00ean", 2, 100, 0, 42, L"QU\u00c0", false}
    };

    currentRecipients = {
        {"nguyen.van.an@gmail.com", L"Nguy\u1ec5n V\u0103n An", "12/05/2024", 1, true},
        {"lethibinh.cinema@yahoo.com", L"L\u00ea Th\u1ecb B\u00ecnh", "14/05/2024", 1, false},
        {"tranminhcuong99@gmail.com", L"Tr\u1ea7n Minh C\u01b0\u1eddng", "15/05/2024", 2, false},
        {"pham.hoang.dung@outlook.com", L"Ph\u1ea1m Ho\u00e0ng D\u0169ng", "16/05/2024", 1, true},
        {"hoangthuylinh@gmail.com", L"Ho\u00e0ng Th\u00f9y Linh", "17/05/2024", 1, true},
        {"vuminhtuan.dn@gmail.com", L"V\u0169 Minh Tu\u1ea5n", "18/05/2024", 1, false}
    };
}

void VoucherPanel::loadVouchersFromFile() {
    string path = FileUtils::resolveDataPath("data/voucher_defs.txt");
    ifstream f(path);
    if (!f.is_open()) return;

    string line;
    bool isHeader = true;
    while (getline(f, line)) {
        if (line.empty()) continue;
        if (isHeader) { isHeader = false; continue; }
        stringstream ss(line);
        string code, typeStr, valStr, minStr, desc;
        getline(ss, code, '|');
        getline(ss, typeStr, '|');
        getline(ss, valStr, '|');
        getline(ss, minStr, '|');
        getline(ss, desc, '|');

        for (auto& v : allVouchers) {
            if (v.code == code) {
                try {
                    v.type = stoi(typeStr);
                    v.value = stoi(valStr);
                    v.minBill = stoi(minStr);
                    if (!desc.empty()) v.subtitle = s2ws(desc);
                } catch (...) {}
                break;
            }
        }
    }
}

void VoucherPanel::saveVouchersToFile() {
    string path = FileUtils::resolveDataPath("data/voucher_defs.txt");
    ofstream f(path);
    if (!f.is_open()) return;

    f << "code|type|value|min_bill|description\n";
    for (const auto& v : allVouchers) {
        f << v.code << "|" << v.type << "|" << v.value << "|" << v.minBill << "|" << ws2s(v.subtitle) << "\n";
    }
}

void VoucherPanel::applyVoucherFilter() {
    filteredVouchers.clear();
    string q = searchVoucherQuery;
    for (auto& c : q) c = (char)tolower(c);

    for (const auto& v : allVouchers) {
        // Lọc theo loại
        if (voucherFilterType == 1 && v.type != 2) continue; // Giảm %
        if (voucherFilterType == 2 && v.type != 1) continue; // Giảm tiền

        // Tìm kiếm
        if (!q.empty()) {
            string codeLower = v.code;
            string subLower = ws2s(v.subtitle);
            for (auto& c : codeLower) c = (char)tolower(c);
            for (auto& c : subLower) c = (char)tolower(c);
            if (codeLower.find(q) == string::npos && subLower.find(q) == string::npos) {
                continue;
            }
        }

        filteredVouchers.push_back(v);
    }

    totalVoucherPages = max(1, (int)((filteredVouchers.size() + pageSize - 1) / pageSize));
    if (currentVoucherPage > totalVoucherPages) currentVoucherPage = totalVoucherPages;
    if (currentVoucherPage < 1) currentVoucherPage = 1;
}

void VoucherPanel::applyRecipientFilter() {
    filteredRecipients.clear();
    string q = searchRecipientQuery;
    for (auto& c : q) c = (char)tolower(c);

    for (const auto& r : currentRecipients) {
        if (!q.empty()) {
            string emLower = r.email;
            string nameLower = ws2s(r.fullName);
            for (auto& c : emLower) c = (char)tolower(c);
            for (auto& c : nameLower) c = (char)tolower(c);
            if (emLower.find(q) == string::npos && nameLower.find(q) == string::npos) {
                continue;
            }
        }
        filteredRecipients.push_back(r);
    }
}

void VoucherPanel::drawRoundedCard(RenderWindow& win, Vector2f pos, Vector2f sz, float r, Color fill, float outline, Color outCol) {
    RoundedRectRenderer::draw(win, pos, sz, r, fill, outline, outCol);
}

void VoucherPanel::handleEvent(const Event& event, const RenderWindow& win) {
    if (const auto* mouseBtn = event.getIf<Event::MouseButtonPressed>()) {
        if (mouseBtn->button == Mouse::Button::Left) {
            Vector2f mousePos = win.mapPixelToCoords(Mouse::getPosition(win));

            // Popup Thêm/Sửa Voucher
            if (showAddPopup) {
                float popW = 500.f;
                float popH = 340.f;
                float popX = position.x + (width - popW) / 2.f;
                float popY = position.y + (height - popH) / 2.f - 20.f;

                FloatRect btnPopSave({popX + 70.f, popY + 270.f}, {160.f, 38.f});
                FloatRect btnPopCancel({popX + 270.f, popY + 270.f}, {160.f, 38.f});

                if (btnPopSave.contains(mousePos)) {
                    if (editingIndex >= 0 && editingIndex < (int)allVouchers.size()) {
                        allVouchers[editingIndex].code = popupCode;
                        allVouchers[editingIndex].subtitle = popupSub;
                        allVouchers[editingIndex].type = popupType;
                        try { allVouchers[editingIndex].value = stoi(popupValue); } catch (...) {}
                        try { allVouchers[editingIndex].minBill = stoi(popupMinBill); } catch (...) {}
                    } else {
                        VoucherItem nv;
                        nv.code = popupCode.empty() ? ("VOUCHER" + to_string(allVouchers.size() + 1)) : popupCode;
                        nv.subtitle = popupSub;
                        nv.type = popupType;
                        try { nv.value = stoi(popupValue); } catch (...) { nv.value = 20; }
                        try { nv.minBill = stoi(popupMinBill); } catch (...) { nv.minBill = 0; }
                        nv.recipientCount = 0;
                        allVouchers.push_back(nv);
                    }
                    saveVouchersToFile();
                    applyVoucherFilter();
                    showAddPopup = false;
                    notificationMessage = "Đã lưu thông tin voucher thành công!";
                    notificationClock.restart();
                    notificationVisible = true;
                    return;
                }

                if (btnPopCancel.contains(mousePos)) {
                    showAddPopup = false;
                    return;
                }
                return;
            }

            // Nút Header: Xuất Excel, Hướng dẫn
            FloatRect btnExp({position.x + width - 200.f, position.y + 76.f}, {90.f, 36.f});
            FloatRect btnGuide({position.x + width - 100.f, position.y + 76.f}, {85.f, 36.f});

            if (btnExp.contains(mousePos)) {
                notificationMessage = "Đã xuất dữ liệu voucher ra file Excel!";
                notificationClock.restart();
                notificationVisible = true;
                return;
            }
            if (btnGuide.contains(mousePos)) {
                notificationMessage = "Đang mở tài liệu hướng dẫn cấu hình voucher!";
                notificationClock.restart();
                notificationVisible = true;
                return;
            }

            // Cột Trái - Hàng nút thao tác: + Thêm mới, Sửa, Xóa, Phát, Reload
            float actY = position.y + 246.f;
            FloatRect btnAdd({position.x + 30.f, actY}, {100.f, 36.f});
            FloatRect btnEdit({position.x + 138.f, actY}, {75.f, 36.f});
            FloatRect btnDel({position.x + 220.f, actY}, {75.f, 36.f});
            FloatRect btnDistribute({position.x + 302.f, actY}, {80.f, 36.f});
            FloatRect btnRel({position.x + 390.f, actY}, {36.f, 36.f});

            if (btnAdd.contains(mousePos)) {
                showAddPopup = true;
                editingIndex = -1;
                popupCode = "NEWVOUCHER";
                popupSub = L"\u01afu \u0111\u00e3i kh\u00e1ch h\u00e0ng th\u00e2n thi\u1ebft";
                popupType = 2;
                popupValue = "20";
                popupMinBill = "50000";
                return;
            }

            if (btnEdit.contains(mousePos)) {
                if (selectedVoucherIndex >= 0 && selectedVoucherIndex < (int)filteredVouchers.size()) {
                    string selCode = filteredVouchers[selectedVoucherIndex].code;
                    for (size_t i = 0; i < allVouchers.size(); ++i) {
                        if (allVouchers[i].code == selCode) {
                            editingIndex = (int)i;
                            popupCode = allVouchers[i].code;
                            popupSub = allVouchers[i].subtitle;
                            popupType = allVouchers[i].type;
                            popupValue = to_string(allVouchers[i].value);
                            popupMinBill = to_string(allVouchers[i].minBill);
                            showAddPopup = true;
                            return;
                        }
                    }
                } else {
                    notificationMessage = "Vui lòng chọn 1 voucher để sửa!";
                    notificationClock.restart();
                    notificationVisible = true;
                }
                return;
            }

            if (btnDel.contains(mousePos)) {
                if (selectedVoucherIndex >= 0 && selectedVoucherIndex < (int)filteredVouchers.size()) {
                    string selCode = filteredVouchers[selectedVoucherIndex].code;
                    for (auto it = allVouchers.begin(); it != allVouchers.end(); ++it) {
                        if (it->code == selCode) {
                            allVouchers.erase(it);
                            break;
                        }
                    }
                    saveVouchersToFile();
                    applyVoucherFilter();
                    notificationMessage = "Đã xóa voucher thành công!";
                    notificationClock.restart();
                    notificationVisible = true;
                }
                return;
            }

            if (btnDistribute.contains(mousePos)) {
                notificationMessage = "Đã phát voucher đến danh sách khách hàng!";
                notificationClock.restart();
                notificationVisible = true;
                return;
            }

            if (btnRel.contains(mousePos)) {
                loadVouchersFromFile();
                applyVoucherFilter();
                notificationMessage = "Đã làm mới dữ liệu!";
                notificationClock.restart();
                notificationVisible = true;
                return;
            }

            // Cột Trái - Bộ lọc Pill: Tất cả, Giảm %, Giảm tiền
            float fltY = position.y + 294.f;
            FloatRect searchVB({position.x + 30.f, fltY}, {260.f, 34.f});
            searchVoucherActive = searchVB.contains(mousePos);

            FloatRect pAll({position.x + 305.f, fltY}, {55.f, 32.f});
            FloatRect pPct({position.x + 368.f, fltY}, {65.f, 32.f});
            FloatRect pVal({position.x + 440.f, fltY}, {70.f, 32.f});

            if (pAll.contains(mousePos)) { voucherFilterType = 0; applyVoucherFilter(); return; }
            if (pPct.contains(mousePos)) { voucherFilterType = 1; applyVoucherFilter(); return; }
            if (pVal.contains(mousePos)) { voucherFilterType = 2; applyVoucherFilter(); return; }

            // Cột Trái - Click chọn dòng trong bảng Voucher
            float tableY = position.y + 340.f;
            float leftW = 630.f;
            float rowH = 68.f;
            int startIdx = (currentVoucherPage - 1) * pageSize;
            int count = min((int)filteredVouchers.size() - startIdx, pageSize);

            for (int r = 0; r < count; ++r) {
                float rY = tableY + 44.f + r * rowH;
                FloatRect rowRect({position.x + 30.f, rY}, {leftW, rowH});

                if (rowRect.contains(mousePos)) {
                    selectedVoucherIndex = startIdx + r;
                    for (size_t i = 0; i < filteredVouchers.size(); ++i) {
                        filteredVouchers[i].isChecked = ((int)i == selectedVoucherIndex);
                    }
                    return;
                }
            }

            // Phân trang Cột Trái
            float pagY = position.y + 780.f;
            FloatRect btnPrev({position.x + leftW - 60.f, pagY}, {26.f, 26.f});
            FloatRect btnP1({position.x + leftW - 30.f, pagY}, {26.f, 26.f});
            FloatRect btnP2({position.x + leftW, pagY}, {26.f, 26.f});
            FloatRect btnNext({position.x + leftW + 30.f, pagY}, {26.f, 26.f});

            if (btnPrev.contains(mousePos) && currentVoucherPage > 1) { currentVoucherPage--; applyVoucherFilter(); return; }
            if (btnNext.contains(mousePos) && currentVoucherPage < totalVoucherPages) { currentVoucherPage++; applyVoucherFilter(); return; }
            if (btnP1.contains(mousePos)) { currentVoucherPage = 1; applyVoucherFilter(); return; }
            if (btnP2.contains(mousePos) && totalVoucherPages >= 2) { currentVoucherPage = 2; applyVoucherFilter(); return; }

            // Cột Phải - Hàng nút: Quay lại, Thêm người, Thu hồi
            float rightX = position.x + 680.f;
            float rHeadY = position.y + 246.f;
            FloatRect btnBack({rightX, rHeadY}, {80.f, 34.f});
            FloatRect btnAddUser({rightX + 90.f, rHeadY}, {110.f, 34.f});
            FloatRect btnRevoke({rightX + 210.f, rHeadY}, {90.f, 34.f});

            if (btnAddUser.contains(mousePos)) {
                RecipientItem nr;
                nr.email = "khachhang" + to_string(currentRecipients.size() + 1) + "@gmail.com";
                nr.fullName = L"Kh\u00e1ch H\u00e0ng M\u1edbi";
                nr.receivedDate = "27/10/2026";
                nr.quantity = 1;
                nr.isUsed = false;
                currentRecipients.push_back(nr);
                applyRecipientFilter();
                notificationMessage = "Đã thêm người nhận voucher!";
                notificationClock.restart();
                notificationVisible = true;
                return;
            }

            if (btnRevoke.contains(mousePos)) {
                if (!currentRecipients.empty()) {
                    currentRecipients.pop_back();
                    applyRecipientFilter();
                    notificationMessage = "Đã thu hồi 1 voucher của khách hàng!";
                    notificationClock.restart();
                    notificationVisible = true;
                }
                return;
            }

            // Search Recipient
            FloatRect searchRB({rightX, position.y + 294.f}, {250.f, 34.f});
            searchRecipientActive = searchRB.contains(mousePos);
        }
    }

    // Gõ phím tìm kiếm
    if (const auto* textEv = event.getIf<Event::TextEntered>()) {
        char32_t c = textEv->unicode;
        if (searchVoucherActive && !showAddPopup) {
            if (c == 8) {
                if (!searchVoucherQuery.empty()) { searchVoucherQuery.pop_back(); applyVoucherFilter(); }
            } else if (c >= 32 && c != 127) {
                if (searchVoucherQuery.size() < 24) { searchVoucherQuery.push_back((char)c); applyVoucherFilter(); }
            }
        } else if (searchRecipientActive && !showAddPopup) {
            if (c == 8) {
                if (!searchRecipientQuery.empty()) { searchRecipientQuery.pop_back(); applyRecipientFilter(); }
            } else if (c >= 32 && c != 127) {
                if (searchRecipientQuery.size() < 24) { searchRecipientQuery.push_back((char)c); applyRecipientFilter(); }
            }
        }
    }
}

void VoucherPanel::update(Vector2f mousePos, bool) {
    hoveredVoucherRow = -1;
    hoveredRecipientRow = -1;

    if (!showAddPopup) {
        float tableY = position.y + 340.f;
        float leftW = 630.f;
        float rowH = 68.f;
        int startIdx = (currentVoucherPage - 1) * pageSize;
        int count = min((int)filteredVouchers.size() - startIdx, pageSize);

        for (int r = 0; r < count; ++r) {
            float rY = tableY + 44.f + r * rowH;
            if (FloatRect({position.x + 30.f, rY}, {leftW, rowH}).contains(mousePos)) {
                hoveredVoucherRow = r;
                break;
            }
        }

        // Cột phải hover
        float rightX = position.x + 680.f;
        float rightW = width - 710.f;
        float rTableY = position.y + 340.f;
        float rRowH = 60.f;
        for (size_t r = 0; r < filteredRecipients.size(); ++r) {
            float rY = rTableY + 44.f + r * rRowH;
            if (FloatRect({rightX, rY}, {rightW, rRowH}).contains(mousePos)) {
                hoveredRecipientRow = (int)r;
                break;
            }
        }
    }
}

void VoucherPanel::render(RenderWindow& win) {
    // 1. Nền canvas (#F4F6F9)
    RectangleShape bg({width, height});
    bg.setPosition(position);
    bg.setFillColor(Color(244, 246, 249));
    win.draw(bg);

    // ==========================================
    // 2. TOP BAR (Standardized Navigation & Profile)
    // ==========================================
    UITheme::drawTopBar(win, position.x + 30.f, position.y, width - 60.f, L"QUẢN LÝ VOUCHER", font, boldFont);

    // ==========================================
    // 3. PAGE HEADER
    // ==========================================
    Text title(boldFont, L"Qu\u1ea3n l\u00fd Voucher", UITheme::PageTitleSize);
    title.setFillColor(UITheme::TextPrimary);
    title.setPosition({position.x + 30.f, position.y + 68.f});
    win.draw(title);

    FloatRect tb = title.getGlobalBounds();
    drawRoundedCard(win, {tb.position.x + tb.size.x + 16.f, position.y + 70.f}, {190.f, 26.f}, 13.f, Color(224, 242, 254));
    Text bBadge(boldFont, to_wstring(allVouchers.size()) + L" Ch\u01b0\u01a1ng tr\u00ecnh ho\u1ea1t \u0111\u1ed9ng", 12);
    bBadge.setFillColor(Color(2, 132, 199));
    bBadge.setPosition({tb.position.x + tb.size.x + 24.f, position.y + 75.f});
    win.draw(bBadge);

    Text subTitle(font, L"C\u1ea5u h\u00ecnh \u0111i\u1ec1u ki\u1ec7n \u01b0u \u0111\u00e3i, danh s\u00e1ch ph\u00e1t h\u00e0nh v\u00e0 ki\u1ec3m so\u00e1t ph\u00e2n b\u1ed5 ng\u01b0\u1eddi nh\u1eadn tr\u00ean to\u00e0n chu\u1ed7i r\u1ea1p CineCore.", 13);
    subTitle.setFillColor(Color(100, 116, 139));
    subTitle.setPosition({position.x + 30.f, position.y + 104.f});
    win.draw(subTitle);

    // Nút Header: Xuất Excel & Hướng dẫn
    drawRoundedCard(win, {position.x + width - 215.f, position.y + 74.f}, {100.f, 36.f}, 6.f, Color::White, 1.f, Color(203, 213, 225));
    Text expTxt(boldFont, L"\U0001F4C4 Xu\u1ea5t Excel", 12.5f); expTxt.setFillColor(Color(22, 101, 52)); expTxt.setPosition({position.x + width - 205.f, position.y + 84.f}); win.draw(expTxt);

    drawRoundedCard(win, {position.x + width - 105.f, position.y + 74.f}, {90.f, 36.f}, 6.f, Color::White, 1.f, Color(186, 230, 253));
    Text guiTxt(boldFont, L"\U0001F6C8 H\u01b0\u1edbng d\u1eabn", 12.5f); guiTxt.setFillColor(Color(2, 132, 199)); guiTxt.setPosition({position.x + width - 97.f, position.y + 84.f}); win.draw(guiTxt);

    // ==========================================
    // 4. 4 THẺ THỐNG KÊ KPI (STAT CARDS GRID)
    // ==========================================
    float statY = position.y + 144.f;
    float cardW = (width - 60.f - 45.f) / 4.f;
    float cardH = 82.f;

    // Card 1: Tổng voucher khả dụng
    float c1X = position.x + 30.f;
    drawRoundedCard(win, {c1X, statY}, {cardW, cardH}, 10.f, Color::White, 1.f, Color(226, 232, 240));
    Text sc1T(boldFont, L"T\u1ed5ng voucher kh\u1ea3 d\u1ee5ng", 13); sc1T.setFillColor(Color(100, 116, 139)); sc1T.setPosition({c1X + 16.f, statY + 12.f}); win.draw(sc1T);
    CircleShape ic1(12.f); ic1.setFillColor(Color(224, 242, 254)); ic1.setPosition({c1X + cardW - 36.f, statY + 10.f}); win.draw(ic1);
    Text sc1V(boldFont, to_wstring(allVouchers.size()), 22); sc1V.setFillColor(Color(15, 23, 42)); sc1V.setPosition({c1X + 16.f, statY + 34.f}); win.draw(sc1V);
    FloatRect v1b = sc1V.getGlobalBounds();
    Text sc1U(font, L"chi\u1ebfn d\u1ecbch", 12.5f); sc1U.setFillColor(Color(100, 116, 139)); sc1U.setPosition({v1b.position.x + v1b.size.x + 8.f, statY + 41.f}); win.draw(sc1U);

    // Card 2: Đã quy đổi / Sử dụng
    float c2X = c1X + cardW + 15.f;
    drawRoundedCard(win, {c2X, statY}, {cardW, cardH}, 10.f, Color::White, 1.f, Color(226, 232, 240));
    Text sc2T(boldFont, L"\u0110\u00e3 quy \u0111\u1ed5i / S\u1eed d\u1ee5ng", 13); sc2T.setFillColor(Color(100, 116, 139)); sc2T.setPosition({c2X + 16.f, statY + 12.f}); win.draw(sc2T);
    CircleShape ic2(12.f); ic2.setFillColor(Color(220, 252, 231)); ic2.setPosition({c2X + cardW - 36.f, statY + 10.f}); win.draw(ic2);
    Text sc2V(boldFont, L"1,420", 22); sc2V.setFillColor(Color(15, 23, 42)); sc2V.setPosition({c2X + 16.f, statY + 34.f}); win.draw(sc2V);
    FloatRect v2b = sc2V.getGlobalBounds();
    Text sc2P(boldFont, L"(68%)", 12.5f); sc2P.setFillColor(Color(5, 150, 105)); sc2P.setPosition({v2b.position.x + v2b.size.x + 8.f, statY + 41.f}); win.draw(sc2P);

    // Card 3: Tổng ngân sách chiết khấu
    float c3X = c2X + cardW + 15.f;
    drawRoundedCard(win, {c3X, statY}, {cardW, cardH}, 10.f, Color::White, 1.f, Color(226, 232, 240));
    Text sc3T(boldFont, L"T\u1ed5ng ng\u00e2n s\u00e1ch chi\u1ebft kh\u1ea5u", 13); sc3T.setFillColor(Color(100, 116, 139)); sc3T.setPosition({c3X + 16.f, statY + 12.f}); win.draw(sc3T);
    CircleShape ic3(12.f); ic3.setFillColor(Color(254, 243, 199)); ic3.setPosition({c3X + cardW - 36.f, statY + 10.f}); win.draw(ic3);
    Text sc3V(boldFont, L"42.500.000 \u0111", 20); sc3V.setFillColor(Color(15, 23, 42)); sc3V.setPosition({c3X + 16.f, statY + 36.f}); win.draw(sc3V);

    // Card 4: Đang chọn khảo sát
    float c4X = c3X + cardW + 15.f;
    drawRoundedCard(win, {c4X, statY}, {cardW, cardH}, 10.f, Color::White, 1.f, Color(226, 232, 240));
    Text sc4T(font, L"\u0110ang ch\u1ecdn kh\u1ea3o s\u00e1t", 13); sc4T.setFillColor(Color(100, 116, 139)); sc4T.setPosition({c4X + 16.f, statY + 12.f}); win.draw(sc4T);
    CircleShape ic4(12.f); ic4.setFillColor(Color(224, 242, 254)); ic4.setPosition({c4X + cardW - 36.f, statY + 10.f}); win.draw(ic4);
    string selCode = (selectedVoucherIndex >= 0 && selectedVoucherIndex < (int)filteredVouchers.size()) ? filteredVouchers[selectedVoucherIndex].code : "WELCOME";
    Text sc4V(boldFont, s2ws(selCode), 20); sc4V.setFillColor(Color(2, 132, 199)); sc4V.setPosition({c4X + 16.f, statY + 36.f}); win.draw(sc4V);

    // ==========================================
    // 5. CỘT TRÁI: DANH SÁCH VOUCHER (MASTER VIEW)
    // ==========================================
    float leftW = 630.f;
    Text lHead(boldFont, L"\u2022 Danh s\u00e1ch Voucher", 16);
    lHead.setFillColor(Color(15, 23, 42));
    lHead.setPosition({position.x + 30.f, position.y + 242.f});
    win.draw(lHead);

    FloatRect lhb = lHead.getGlobalBounds();
    Text lCount(font, L"(12 b\u1ea3n ghi)", 13);
    lCount.setFillColor(Color(100, 116, 139));
    lCount.setPosition({lhb.position.x + lhb.size.x + 10.f, position.y + 244.f});
    win.draw(lCount);

    // Hàng nút thao tác Cột Trái
    float actY = position.y + 268.f;
    drawRoundedCard(win, {position.x + 30.f, actY}, {105.f, 34.f}, 6.f, Color(5, 150, 105));
    Text addTxt(boldFont, L"+ Th\u00eam m\u1edbi", 13); addTxt.setFillColor(Color::White); addTxt.setPosition({position.x + 42.f, actY + 8.f}); win.draw(addTxt);

    drawRoundedCard(win, {position.x + 142.f, actY}, {75.f, 34.f}, 6.f, Color::White, 1.f, Color(217, 119, 6));
    Text edTxt(boldFont, L"\u270e  S\u1eeda", 13); edTxt.setFillColor(Color(217, 119, 6)); edTxt.setPosition({position.x + 152.f, actY + 8.f}); win.draw(edTxt);

    drawRoundedCard(win, {position.x + 224.f, actY}, {75.f, 34.f}, 6.f, Color::White, 1.f, Color(220, 38, 38));
    Text dlTxt(boldFont, L"\u2296  X\u00f3a", 13); dlTxt.setFillColor(Color(220, 38, 38)); dlTxt.setPosition({position.x + 236.f, actY + 8.f}); win.draw(dlTxt);

    drawRoundedCard(win, {position.x + 306.f, actY}, {80.f, 34.f}, 6.f, Color(238, 242, 255), 1.f, Color(199, 210, 254));
    Text dstTxt(boldFont, L"\U0001F680 Ph\u00e1t", 13); dstTxt.setFillColor(Color(67, 56, 202)); dstTxt.setPosition({position.x + 316.f, actY + 8.f}); win.draw(dstTxt);

    drawRoundedCard(win, {position.x + 394.f, actY}, {34.f, 34.f}, 6.f, Color::White, 1.f, Color(203, 213, 225));
    Text relTxt(boldFont, L"\u21bb", 16); relTxt.setFillColor(Color(71, 85, 105)); relTxt.setPosition({position.x + 404.f, actY + 7.f}); win.draw(relTxt);

    // Hàng tìm kiếm & Bộ lọc Pill
    float fltY = position.y + 312.f;
    drawRoundedCard(win, {position.x + 30.f, fltY}, {260.f, 34.f}, 6.f, Color::White, searchVoucherActive ? 1.5f : 1.f, searchVoucherActive ? Color(2, 132, 199) : Color(203, 213, 225));
    wstring sVPrompt = searchVoucherQuery.empty() ? L"T\u00ecm theo m\u00e3 voucher ho\u1eb7c t\u00ean chi\u1ebfn d\u1ecbch..." : s2ws(searchVoucherQuery) + (searchVoucherActive ? L"|" : L"");
    Text sVTxt(font, sVPrompt, 12); sVTxt.setFillColor(searchVoucherQuery.empty() ? Color(148, 163, 184) : Color(15, 23, 42)); sVTxt.setPosition({position.x + 40.f, fltY + 8.f}); win.draw(sVTxt);

    vector<pair<wstring, float>> vPills = {{L"T\u1ea5t c\u1ea3", 65.f}, {L"Gi\u1ea3m %", 75.f}, {L"Gi\u1ea3m ti\u1ec1n", 85.f}};
    float vPX = position.x + 305.f;
    for (size_t i = 0; i < vPills.size(); ++i) {
        bool isAct = ((int)i == voucherFilterType);
        drawRoundedCard(win, {vPX, fltY}, {vPills[i].second, 34.f}, 17.f, isAct ? Color(2, 132, 199) : Color::White, isAct ? 0.f : 1.f, Color(226, 232, 240));
        Text vpTxt(boldFont, vPills[i].first, 12.5f);
        vpTxt.setFillColor(isAct ? Color::White : Color(71, 85, 105));
        FloatRect vptb = vpTxt.getLocalBounds();
        vpTxt.setPosition({vPX + (vPills[i].second - vptb.size.x) / 2.f - vptb.position.x, fltY + (34.f - vptb.size.y) / 2.f - vptb.position.y});
        win.draw(vpTxt);
        vPX += vPills[i].second + 8.f;
    }

    // Bảng Voucher
    float tableY = position.y + 356.f;
    float rowH = 68.f;
    drawRoundedCard(win, {position.x + 30.f, tableY}, {leftW, 40.f}, 4.f, Color(0, 119, 182));

    Text vh1(boldFont, L"M\u00c3 VOUCHER", 12.5f); vh1.setFillColor(Color::White); vh1.setPosition({position.x + 75.f, tableY + 11.f}); win.draw(vh1);
    Text vh2(boldFont, L"LO\u1ea0I", 12.5f); vh2.setFillColor(Color::White); vh2.setPosition({position.x + 225.f, tableY + 11.f}); win.draw(vh2);
    Text vh3(boldFont, L"GI\u00c1 TR\u1eca", 12.5f); vh3.setFillColor(Color::White); vh3.setPosition({position.x + 305.f, tableY + 11.f}); win.draw(vh3);
    Text vh4(boldFont, L"\u0110\u01a0N T\u1ed0I THI\u1ec2U", 12.5f); vh4.setFillColor(Color::White); vh4.setPosition({position.x + 380.f, tableY + 11.f}); win.draw(vh4);
    Text vh5(boldFont, L"NG\u01af\u1edcI NH\u1eacN", 12.5f); vh5.setFillColor(Color::White); vh5.setPosition({position.x + 480.f, tableY + 11.f}); win.draw(vh5);
    Text vh6(boldFont, L"THAO T\u00c1C", 12.5f); vh6.setFillColor(Color::White); vh6.setPosition({position.x + 565.f, tableY + 11.f}); win.draw(vh6);

    int startIdx = (currentVoucherPage - 1) * pageSize;
    int count = min((int)filteredVouchers.size() - startIdx, pageSize);

    for (int r = 0; r < count; ++r) {
        int idx = startIdx + r;
        const auto& v = filteredVouchers[idx];
        float rY = tableY + 40.f + r * rowH;

        bool isSel = (selectedVoucherIndex == idx);
        bool isHov = (hoveredVoucherRow == r);

        Color rowBg = isSel ? Color(224, 242, 254) : (isHov ? Color(248, 250, 252) : Color::White);
        drawRoundedCard(win, {position.x + 30.f, rY}, {leftW, rowH}, 0.f, rowBg, 0.5f, Color(226, 232, 240));

        // Checkbox
        drawRoundedCard(win, {position.x + 40.f, rY + 26.f}, {16.f, 16.f}, 3.f, isSel ? Color(2, 132, 199) : Color::White, 1.f, Color(203, 213, 225));
        if (isSel) {
            Text ck(boldFont, L"\u2714", 11); ck.setFillColor(Color::White); ck.setPosition({position.x + 42.f, rY + 26.f}); win.draw(ck);
        }

        // Mã Voucher & Tag & Subtitle
        Text codeTxt(boldFont, s2ws(v.code), 13.5f); codeTxt.setFillColor(Color(15, 23, 42)); codeTxt.setPosition({position.x + 75.f, rY + 14.f}); win.draw(codeTxt);

        if (!v.tag.empty()) {
            FloatRect cb = codeTxt.getGlobalBounds();
            Color tagBg = Color(220, 252, 231);
            Color tagFg = Color(22, 101, 52);
            if (v.tag == L"VIP") { tagBg = Color(254, 243, 199); tagFg = Color(180, 83, 9); }
            else if (v.tag == L"HSSV") { tagBg = Color(224, 242, 254); tagFg = Color(2, 132, 199); }
            drawRoundedCard(win, {cb.position.x + cb.size.x + 6.f, rY + 14.f}, {38.f, 20.f}, 4.f, tagBg);
            Text tgTxt(boldFont, v.tag, 11.5f); tgTxt.setFillColor(tagFg); tgTxt.setPosition({cb.position.x + cb.size.x + 10.f, rY + 16.f}); win.draw(tgTxt);
        }

        Text subTxt(font, v.subtitle, 12); subTxt.setFillColor(Color(100, 116, 139)); subTxt.setPosition({position.x + 75.f, rY + 38.f}); win.draw(subTxt);

        // Loại Voucher (Badge)
        wstring typeLabel = (v.type == 2) ? L"Lo\u1ea1i 2 (%)\n" : L"Lo\u1ea1i 1 (Ti\u1ec1n)";
        if (v.type == 2) typeLabel = L"Lo\u1ea1i 2\n[%]";
        Color typeBg = (v.type == 2) ? Color(253, 242, 248) : Color(238, 242, 255);
        Color typeFg = (v.type == 2) ? Color(219, 39, 119) : Color(79, 70, 229);
        drawRoundedCard(win, {position.x + 220.f, rY + 16.f}, {62.f, 34.f}, 6.f, typeBg);
        Text tpTxt(boldFont, typeLabel, 11.5f); tpTxt.setFillColor(typeFg); tpTxt.setPosition({position.x + 225.f, rY + 17.f}); win.draw(tpTxt);

        // Giá trị
        wstring valStr = (v.type == 2) ? (to_wstring(v.value) + L"%") : s2ws(formatPrice(v.value));
        Text valTxt(boldFont, valStr, 13.5f); valTxt.setFillColor(Color(5, 150, 105)); valTxt.setPosition({position.x + 305.f, rY + 22.f}); win.draw(valTxt);

        // Đơn tối thiểu
        Text minTxt(font, s2ws(formatPrice(v.minBill)), 13); minTxt.setFillColor(Color(71, 85, 105)); minTxt.setPosition({position.x + 380.f, rY + 22.f}); win.draw(minTxt);

        // Người nhận
        drawRoundedCard(win, {position.x + 480.f, rY + 20.f}, {50.f, 24.f}, 12.f, Color(241, 245, 249));
        Text rcTxt(boldFont, to_wstring(v.recipientCount), 12); rcTxt.setFillColor(Color(2, 132, 199)); rcTxt.setPosition({position.x + 495.f, rY + 23.f}); win.draw(rcTxt);

        // Thao tác
        Text icGo(boldFont, L"\u2192", 14); icGo.setFillColor(Color(148, 163, 184)); icGo.setPosition({position.x + 565.f, rY + 22.f}); win.draw(icGo);
        Text icEd(boldFont, L"\u270e", 13); icEd.setFillColor(Color(148, 163, 184)); icEd.setPosition({position.x + 590.f, rY + 22.f}); win.draw(icEd);
    }

    // Phân trang Cột Trái
    float pagY = position.y + 780.f;
    Text lPag(font, L"Hi\u1ec3n th\u1ecb 1 \u2013 6 tr\u00ean 12 voucher", 12.5f);
    lPag.setFillColor(Color(100, 116, 139));
    lPag.setPosition({position.x + 30.f, pagY + 4.f});
    win.draw(lPag);

    drawRoundedCard(win, {position.x + leftW - 60.f, pagY}, {28.f, 28.f}, 4.f, Color::White, 1.f, Color(226, 232, 240));
    Text pgPrev(boldFont, L"<", 12); pgPrev.setFillColor(Color(100, 116, 139)); pgPrev.setPosition({position.x + leftW - 52.f, pagY + 4.f}); win.draw(pgPrev);

    bool p1Act = (currentVoucherPage == 1);
    drawRoundedCard(win, {position.x + leftW - 30.f, pagY}, {28.f, 28.f}, 4.f, p1Act ? Color(2, 132, 199) : Color::White, 1.f, Color(226, 232, 240));
    Text pg1(boldFont, L"1", 12); pg1.setFillColor(p1Act ? Color::White : Color(71, 85, 105)); pg1.setPosition({position.x + leftW - 22.f, pagY + 4.f}); win.draw(pg1);

    bool p2Act = (currentVoucherPage == 2);
    drawRoundedCard(win, {position.x + leftW, pagY}, {28.f, 28.f}, 4.f, p2Act ? Color(2, 132, 199) : Color::White, 1.f, Color(226, 232, 240));
    Text pg2(boldFont, L"2", 12); pg2.setFillColor(p2Act ? Color::White : Color(71, 85, 105)); pg2.setPosition({position.x + leftW + 8.f, pagY + 4.f}); win.draw(pg2);

    drawRoundedCard(win, {position.x + leftW + 30.f, pagY}, {28.f, 28.f}, 4.f, Color::White, 1.f, Color(226, 232, 240));
    Text pgNext(boldFont, L">", 12); pgNext.setFillColor(Color(100, 116, 139)); pgNext.setPosition({position.x + leftW + 38.f, pagY + 4.f}); win.draw(pgNext);

    // ==========================================
    // 6. CỘT PHẢI: CHI TIẾT NGƯỜI NHẬN (DETAIL VIEW)
    // ==========================================
    float rightX = position.x + 680.f;
    float rightW = width - 710.f;

    drawRoundedCard(win, {rightX, position.y + 236.f}, {rightW, 580.f}, 12.f, Color::White, 1.f, Color(226, 232, 240));

    // Header Cột Phải
    Text rHead(boldFont, L"Chi ti\u1ebft ng\u01b0\u1eddi nh\u1eadn", 16);
    rHead.setFillColor(Color(15, 23, 42));
    rHead.setPosition({rightX + 16.f, position.y + 248.f});
    win.draw(rHead);

    FloatRect rhb = rHead.getGlobalBounds();
    drawRoundedCard(win, {rhb.position.x + rhb.size.x + 10.f, position.y + 248.f}, {85.f, 24.f}, 4.f, Color(224, 242, 254));
    Text vcBadge(boldFont, s2ws(selCode), 12); vcBadge.setFillColor(Color(2, 132, 199)); vcBadge.setPosition({rhb.position.x + rhb.size.x + 16.f, position.y + 252.f}); win.draw(vcBadge);

    Text rSub(font, L"Gi\u1ea3m 20% \u0111\u01a1n h\u00e0ng \u2022 \u0110\u01a1n t\u1ed1i thi\u1ec3u: 0 \u0111", 12.5f);
    rSub.setFillColor(Color(100, 116, 139));
    rSub.setPosition({rightX + 16.f, position.y + 276.f});
    win.draw(rSub);

    // Hàng nút Cột Phải: Quay lại, + Thêm người, Thu hồi
    float rActY = position.y + 300.f;
    drawRoundedCard(win, {rightX + 16.f, rActY}, {85.f, 32.f}, 6.f, Color::White, 1.f, Color(203, 213, 225));
    Text bBack(boldFont, L"\u2190 Quay l\u1ea1i", 12.5f); bBack.setFillColor(Color(71, 85, 105)); bBack.setPosition({rightX + 22.f, rActY + 8.f}); win.draw(bBack);

    drawRoundedCard(win, {rightX + 108.f, rActY}, {110.f, 32.f}, 6.f, Color(5, 150, 105));
    Text bAddU(boldFont, L"+ Th\u00eam ng\u01b0\u1eddi", 12.5f); bAddU.setFillColor(Color::White); bAddU.setPosition({rightX + 118.f, rActY + 8.f}); win.draw(bAddU);

    drawRoundedCard(win, {rightX + 225.f, rActY}, {80.f, 32.f}, 6.f, Color(254, 242, 242), 1.f, Color(254, 202, 202));
    Text bRev(boldFont, L"\u2296 Thu h\u1ed3i", 12.5f); bRev.setFillColor(Color(220, 38, 38)); bRev.setPosition({rightX + 235.f, rActY + 8.f}); win.draw(bRev);

    // Tìm kiếm người nhận
    float rFltY = position.y + 342.f;
    drawRoundedCard(win, {rightX + 16.f, rFltY}, {rightW - 130.f, 32.f}, 6.f, Color::White, searchRecipientActive ? 1.5f : 1.f, searchRecipientActive ? Color(2, 132, 199) : Color(203, 213, 225));
    wstring sRPrompt = searchRecipientQuery.empty() ? L"T\u00ecm theo Email ho\u1eb7c T\u00ean kh\u00e1ch h\u00e0ng..." : s2ws(searchRecipientQuery) + (searchRecipientActive ? L"|" : L"");
    Text sRTxt(font, sRPrompt, 12); sRTxt.setFillColor(searchRecipientQuery.empty() ? Color(148, 163, 184) : Color(15, 23, 42)); sRTxt.setPosition({rightX + 24.f, rFltY + 8.f}); win.draw(sRTxt);

    drawRoundedCard(win, {rightX + rightW - 105.f, rFltY}, {90.f, 32.f}, 6.f, Color(241, 245, 249));
    Text uTotTxt(boldFont, L"145 t\u00e0i kho\u1ea3n", 12); uTotTxt.setFillColor(Color(5, 150, 105)); uTotTxt.setPosition({rightX + rightW - 100.f, rFltY + 8.f}); win.draw(uTotTxt);

    // Header Bảng Người Nhận
    float rTblY = position.y + 384.f;
    drawRoundedCard(win, {rightX + 16.f, rTblY}, {rightW - 32.f, 34.f}, 4.f, Color(248, 250, 252));
    Text rh1(boldFont, L"EMAIL KH\u00c1CH H\u00c0NG", 12); rh1.setFillColor(Color(100, 116, 139)); rh1.setPosition({rightX + 24.f, rTblY + 9.f}); win.draw(rh1);
    Text rh2(boldFont, L"H\u1ecc V\u00c0 T\u00caN", 12); rh2.setFillColor(Color(100, 116, 139)); rh2.setPosition({rightX + 220.f, rTblY + 9.f}); win.draw(rh2);
    Text rh3(boldFont, L"SL", 12); rh3.setFillColor(Color(100, 116, 139)); rh3.setPosition({rightX + 310.f, rTblY + 9.f}); win.draw(rh3);
    Text rh4(boldFont, L"TR\u1ea0NG TH\u00c1I", 12); rh4.setFillColor(Color(100, 116, 139)); rh4.setPosition({rightX + 355.f, rTblY + 9.f}); win.draw(rh4);

    // Dòng dữ liệu Người Nhận
    float rRowH = 58.f;
    for (size_t r = 0; r < filteredRecipients.size(); ++r) {
        float curY = rTblY + 36.f + r * rRowH;
        const auto& itm = filteredRecipients[r];

        // Đường kẻ ngăn cách
        RectangleShape sep({rightW - 32.f, 1.f});
        sep.setFillColor(Color(241, 245, 249));
        sep.setPosition({rightX + 16.f, curY + rRowH - 1.f});
        win.draw(sep);

        // Email & ngày nhận
        Text emTxt(boldFont, s2ws(itm.email), 13); emTxt.setFillColor(Color(15, 23, 42)); emTxt.setPosition({rightX + 24.f, curY + 10.f}); win.draw(emTxt);
        Text recDt(font, L"\u0110\u00e3 nh\u1eadn: " + s2ws(itm.receivedDate), 12.f); recDt.setFillColor(Color(148, 163, 184)); recDt.setPosition({rightX + 24.f, curY + 30.f}); win.draw(recDt);

        // Họ tên
        Text nmTxt(font, itm.fullName, 13); nmTxt.setFillColor(Color(51, 65, 85)); nmTxt.setPosition({rightX + 220.f, curY + 18.f}); win.draw(nmTxt);

        // Số lượng
        Text qTxt(boldFont, to_wstring(itm.quantity), 13); qTxt.setFillColor(Color(15, 23, 42)); qTxt.setPosition({rightX + 315.f, curY + 18.f}); win.draw(qTxt);

        // Trạng thái: Đã dùng (Xanh lá) hoặc Chưa dùng (Xanh dương)
        Color bStBg = itm.isUsed ? Color(220, 252, 231) : Color(224, 242, 254);
        Color bStFg = itm.isUsed ? Color(22, 101, 52) : Color(2, 132, 199);
        wstring stLabel = itm.isUsed ? L"\u2022 \u0110\u00e3 d\u00f9ng" : L"\u2022 Ch\u01b0a d\u00f9ng";
        drawRoundedCard(win, {rightX + 345.f, curY + 14.f}, {88.f, 26.f}, 13.f, bStBg);
        Text stTxt(boldFont, stLabel, 12.f); stTxt.setFillColor(bStFg); stTxt.setPosition({rightX + 353.f, curY + 19.f}); win.draw(stTxt);
    }

    // Footer Cột Phải
    float rFootY = position.y + 776.f;
    Text rUsage(font, L"\u0110\u00e3 s\u1eed d\u1ee5ng: 89/145 (61.4%)", 12.5f);
    rUsage.setFillColor(Color(100, 116, 139));
    rUsage.setPosition({rightX + 16.f, rFootY});
    win.draw(rUsage);

    Text rAllLnk(boldFont, L"Xem to\u00e0n b\u1ed9 145 kh\u00e1ch \u2192", 12.5f);
    rAllLnk.setFillColor(Color(2, 132, 199));
    rAllLnk.setPosition({rightX + rightW - 170.f, rFootY});
    win.draw(rAllLnk);

    // ==========================================
    // 7. BOTTOM STATUS BAR
    // ==========================================
    float botY = height - 26.f;
    RectangleShape botBg({width, 26.f});
    botBg.setPosition({position.x, botY});
    botBg.setFillColor(Color(11, 20, 32));
    win.draw(botBg);

    Text dbStat(font, L"\u2261  CineCore Host DB: Connected (0.12ms)  |  B\u1ed9 nh\u1edb Cache: 98.4% kh\u1ea3 d\u1ee5ng", 12.f);
    dbStat.setFillColor(Color(148, 163, 184));
    dbStat.setPosition({position.x + 20.f, botY + 5.f});
    win.draw(dbStat);

    Text sysStat(font, L"UTF-8  \u2022  PBL2 DUYTAN CINEMA ENGINE", 12.f);
    sysStat.setFillColor(Color(148, 163, 184));
    sysStat.setPosition({position.x + width - 260.f, botY + 5.f});
    win.draw(sysStat);

    // ==========================================
    // 8. POPUP THÊM / SỬA VOUCHER
    // ==========================================
    if (showAddPopup) {
        RectangleShape overlay({width, height});
        overlay.setPosition(position);
        overlay.setFillColor(Color(15, 23, 42, 160));
        win.draw(overlay);

        float popW = 500.f;
        float popH = 340.f;
        float popX = position.x + (width - popW) / 2.f;
        float popY = position.y + (height - popH) / 2.f - 20.f;

        drawRoundedCard(win, {popX, popY}, {popW, popH}, 12.f, Color::White);

        Text pTitle(boldFont, (editingIndex >= 0) ? L"S\u1eeda th\u00f4ng tin voucher" : L"Th\u00eam voucher m\u1edbi", 18);
        pTitle.setFillColor(Color(15, 23, 42));
        pTitle.setPosition({popX + 30.f, popY + 22.f});
        win.draw(pTitle);

        Text l1(boldFont, L"M\u00e3 voucher:", 13); l1.setFillColor(Color(71, 85, 105)); l1.setPosition({popX + 30.f, popY + 65.f}); win.draw(l1);
        drawRoundedCard(win, {popX + 30.f, popY + 85.f}, {210.f, 36.f}, 6.f, Color(248, 250, 252), 1.f, Color(203, 213, 225));
        Text v1(font, s2ws(popupCode), 13); v1.setFillColor(Color(15, 23, 42)); v1.setPosition({popX + 40.f, popY + 93.f}); win.draw(v1);

        Text l2(boldFont, L"Lo\u1ea1i voucher (1: Ti\u1ec1n, 2: %):", 13); l2.setFillColor(Color(71, 85, 105)); l2.setPosition({popX + 260.f, popY + 65.f}); win.draw(l2);
        drawRoundedCard(win, {popX + 260.f, popY + 85.f}, {210.f, 36.f}, 6.f, Color(248, 250, 252), 1.f, Color(203, 213, 225));
        Text v2(font, to_wstring(popupType) + (popupType == 2 ? L" (Gi\u1ea3m %)" : L" (Gi\u1ea3m ti\u1ec1n \u0111)"), 13); v2.setFillColor(Color(15, 23, 42)); v2.setPosition({popX + 270.f, popY + 93.f}); win.draw(v2);

        Text l3(boldFont, L"Chi ti\u1ebft / M\u00f4 t\u1ea3:", 13); l3.setFillColor(Color(71, 85, 105)); l3.setPosition({popX + 30.f, popY + 130.f}); win.draw(l3);
        drawRoundedCard(win, {popX + 30.f, popY + 150.f}, {popW - 60.f, 36.f}, 6.f, Color(248, 250, 252), 1.f, Color(203, 213, 225));
        Text v3(font, popupSub, 13); v3.setFillColor(Color(15, 23, 42)); v3.setPosition({popX + 40.f, popY + 158.f}); win.draw(v3);

        Text l4(boldFont, L"Gi\u00e1 tr\u1ecb \u01b0u \u0111\u00e3i (% ho\u1eb7c \u0111):", 13); l4.setFillColor(Color(71, 85, 105)); l4.setPosition({popX + 30.f, popY + 195.f}); win.draw(l4);
        drawRoundedCard(win, {popX + 30.f, popY + 215.f}, {210.f, 36.f}, 6.f, Color(248, 250, 252), 1.f, Color(203, 213, 225));
        Text v4(font, s2ws(popupValue), 13); v4.setFillColor(Color(15, 23, 42)); v4.setPosition({popX + 40.f, popY + 223.f}); win.draw(v4);

        Text l5(boldFont, L"\u0110\u01a1n t\u1ed1i thi\u1ec3u (\u0111):", 13); l5.setFillColor(Color(71, 85, 105)); l5.setPosition({popX + 260.f, popY + 195.f}); win.draw(l5);
        drawRoundedCard(win, {popX + 260.f, popY + 215.f}, {210.f, 36.f}, 6.f, Color(248, 250, 252), 1.f, Color(203, 213, 225));
        Text v5(font, s2ws(popupMinBill), 13); v5.setFillColor(Color(15, 23, 42)); v5.setPosition({popX + 270.f, popY + 223.f}); win.draw(v5);

        // Buttons
        drawRoundedCard(win, {popX + 70.f, popY + 270.f}, {160.f, 38.f}, 6.f, Color(5, 150, 105));
        Text sTxt(boldFont, L"L\u01b0u th\u00f4ng tin", 13); sTxt.setFillColor(Color::White); sTxt.setPosition({popX + 105.f, popY + 279.f}); win.draw(sTxt);

        drawRoundedCard(win, {popX + 270.f, popY + 270.f}, {160.f, 38.f}, 6.f, Color(241, 245, 249), 1.f, Color(203, 213, 225));
        Text cTxt(boldFont, L"H\u1ee7y b\u1ecf", 13); cTxt.setFillColor(Color(71, 85, 105)); cTxt.setPosition({popX + 325.f, popY + 279.f}); win.draw(cTxt);
    }

    // ==========================================
    // 9. TOAST NOTIFICATION
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
            Text notText(boldFont, s2ws(notificationMessage), 13);
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
