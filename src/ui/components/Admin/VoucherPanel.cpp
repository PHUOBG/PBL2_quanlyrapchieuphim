#include "UI/components/Admin/VoucherPanel.h"
#include "UI/components/Admin/FontManager.h"
#include "UI/components/Admin/UITheme.h"
#include "UI/components/Admin/RoundedRectRenderer.h"
#include "utils/FileUtils.h"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iomanip>

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#endif

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
    return res + " Đ";
}

VoucherPanel::VoucherPanel(Font& fontRef, float w, float h)
    : font(fontRef),
      boldFont(FontManager::instance().bold()),
      width(w), height(h), position(280.f, 0.f)
{
    initDefaultData();
    loadVouchersFromFile();
    applyVoucherFilter();
    applyRecipientFilter();
}

void VoucherPanel::setPosition(Vector2f pos) {
    position = pos;
    width = 1728.f - pos.x;
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

void VoucherPanel::openExportedFile(const string& path) const {
#ifdef _WIN32
    ShellExecuteA(NULL, "open", path.c_str(), NULL, NULL, SW_SHOWNORMAL);
#else
    system(("start \"\" \"" + path + "\"").c_str());
#endif
}

bool VoucherPanel::exportToExcel() {
    string outPath = FileUtils::resolveDataPath("data/voucher_report.csv");
    ofstream out(outPath, ios::binary);
    if (!out.is_open()) {
        outPath = "voucher_report.csv";
        out.open(outPath, ios::binary);
    }
    if (!out.is_open()) return false;

    // UTF-8 BOM so Excel opens with proper Vietnamese characters
    const unsigned char bom[] = {0xEF, 0xBB, 0xBF};
    out.write(reinterpret_cast<const char*>(bom), sizeof(bom));

    out << "BÁO CÁO CHI TIẾT HỆ THỐNG VOUCHER & KHÁCH HÀNG NHẬN\n";
    out << "Tổng số chiến dịch voucher: " << allVouchers.size() << "\n\n";

    out << "MÃ VOUCHER,TÊN CHIẾN DỊCH,LOẠI VOUCHER,GIÁ TRỊ GIẢM,ĐƠN TỐI THIỂU (VNĐ),SỐ NGƯỜI NHẬN,TRẠNG THÁI\n";
    for (const auto& v : allVouchers) {
        string typeStr = (v.type == 2) ? "Giảm theo %" : "Giảm tiền trực tiếp";
        string valStr = (v.type == 2) ? (to_string(v.value) + "%") : (formatPrice(v.value) + " đ");
        out << "\"" << v.code << "\","
            << "\"" << ws2s(v.subtitle) << "\","
            << "\"" << typeStr << "\","
            << "\"" << valStr << "\","
            << v.minBill << ","
            << v.recipientCount << ","
            << "\"Đang phát hành\"\n";
    }

    out << "\n\n";
    string curCode = (selectedVoucherIndex >= 0 && selectedVoucherIndex < (int)filteredVouchers.size()) ? filteredVouchers[selectedVoucherIndex].code : "WELCOME";
    out << "DANH SÁCH CHI TIẾT KHÁCH HÀNG NHẬN VOUCHER (" << curCode << ")\n";
    out << "STT,EMAIL KHÁCH HÀNG,HỌ VÀ TÊN,SỐ LƯỢNG,NGÀY CẤP,TRẠNG THÁI\n";

    int stt = 1;
    for (const auto& r : currentRecipients) {
        out << stt++ << ","
            << "\"" << r.email << "\","
            << "\"" << ws2s(r.fullName) << "\","
            << r.quantity << ","
            << "\"" << r.receivedDate << "\","
            << "\"" << (r.isUsed ? "Đã dùng" : "Chưa dùng") << "\"\n";
    }

    out.close();
    openExportedFile(outPath);
    return true;
}

int VoucherPanel::getVoucherPageSize() const {
    float mainTopY = position.y + 210.f;
    float tableY = mainTopY + 120.f;
    float hdrH = 42.f;
    float rowH = 68.f;
    float contentBottomY = position.y + height - 40.f;
    float avail = contentBottomY - (tableY + hdrH + 44.f);
    return max(5, (int)std::floor(avail / rowH));
}

int VoucherPanel::getRecipientPageSize() const {
    float mainTopY = position.y + 210.f;
    float contentBottomY = position.y + height - 40.f;
    float rTblY = mainTopY + 156.f;
    float rHdrH = 38.f;
    float rRowH = 60.f;
    float rFootY = contentBottomY - 38.f;
    float avail = (rFootY - 8.f) - (rTblY + rHdrH);
    return max(5, (int)std::floor(avail / rRowH));
}

static wstring ellipsizeText(const Font& f, const wstring& str, unsigned int size, float maxW) {
    if (maxW <= 12.f || str.empty()) return str;
    Text probe(f, str, size);
    if (probe.getLocalBounds().size.x <= maxW) return str;
    wstring out = str;
    while (!out.empty()) {
        out.pop_back();
        probe.setString(out + L"...");
        if (probe.getLocalBounds().size.x <= maxW) return out + L"...";
    }
    return L"...";
}

void VoucherPanel::applyVoucherFilter() {
    filteredVouchers.clear();

    for (const auto& v : allVouchers) {
        // Lọc theo loại
        if (voucherFilterType == 1 && v.type != 2) continue; // Giảm %
        if (voucherFilterType == 2 && v.type != 1) continue; // Giảm tiền

        // Tìm kiếm hỗ trợ tiếng Việt có dấu & không dấu
        if (!searchVoucherQuery.empty()) {
            bool match = UITheme::matchesViQuery(s2ws(v.code), searchVoucherQuery) ||
                         UITheme::matchesViQuery(v.subtitle, searchVoucherQuery) ||
                         UITheme::matchesViQuery(v.tag, searchVoucherQuery);
            if (!match) continue;
        }

        filteredVouchers.push_back(v);
    }

    pageSize = getVoucherPageSize();
    totalVoucherPages = max(1, (int)((filteredVouchers.size() + pageSize - 1) / pageSize));
    if (currentVoucherPage > totalVoucherPages) currentVoucherPage = totalVoucherPages;
    if (currentVoucherPage < 1) currentVoucherPage = 1;
    if (selectedVoucherIndex >= (int)filteredVouchers.size()) {
        selectedVoucherIndex = filteredVouchers.empty() ? -1 : 0;
    }
}

void VoucherPanel::applyRecipientFilter() {
    filteredRecipients.clear();

    for (const auto& r : currentRecipients) {
        if (!searchRecipientQuery.empty()) {
            bool match = UITheme::matchesViQuery(s2ws(r.email), searchRecipientQuery) ||
                         UITheme::matchesViQuery(r.fullName, searchRecipientQuery) ||
                         UITheme::matchesViQuery(s2ws(r.receivedDate), searchRecipientQuery);
            if (!match) continue;
        }
        filteredRecipients.push_back(r);
    }

    recipientPageSize = getRecipientPageSize();
    totalRecipientPages = max(1, (int)((filteredRecipients.size() + recipientPageSize - 1) / recipientPageSize));
    if (currentRecipientPage > totalRecipientPages) currentRecipientPage = totalRecipientPages;
    if (currentRecipientPage < 1) currentRecipientPage = 1;
}

void VoucherPanel::drawRoundedCard(RenderWindow& win, Vector2f pos, Vector2f sz, float r, Color fill, float outline, Color outCol) {
    RoundedRectRenderer::draw(win, pos, sz, r, fill, outline, outCol);
}

namespace {
struct VoucherLayout {
    float contentX, contentW, gap;
    float leftW, rightX, rightW;
    float mainTopY, contentBottomY;

    // Left panel
    float actY, fltY;
    float searchVW;
    float pillX, pillW[3];
    float tableX, tableY, tblRight, hdrH, rowH;
    float div1, div2, div3, div4, div5;
    float pagY;

    // Right panel
    float rightCardH;
    float rActY, rFltY;
    float rCountW, rSearchW;
    float rTblX, rTblW, rTblY, rHdrH, rRowH, rTblRight;
    float rDiv1, rDiv2, rDiv3;
    float rFootY, rPagRight;
};

VoucherLayout computeVoucherLayout(Vector2f pos, float width, float height) {
    VoucherLayout L{};
    L.contentX = pos.x + 30.f;
    L.contentW = max(900.f, width - 60.f);
    L.gap = 18.f;
    L.leftW = std::floor((L.contentW - L.gap) * 0.52f);
    L.rightX = L.contentX + L.leftW + L.gap;
    L.rightW = L.contentW - L.leftW - L.gap;
    L.mainTopY = pos.y + 210.f;
    L.contentBottomY = pos.y + height - 40.f;

    // Left controls
    L.actY = L.mainTopY + 32.f;
    L.fltY = L.mainTopY + 76.f;
    L.pillW[0] = 68.f;
    L.pillW[1] = 76.f;
    L.pillW[2] = 88.f;
    float pillsTotalW = L.pillW[0] + L.pillW[1] + L.pillW[2] + 16.f;
    L.searchVW = max(160.f, L.leftW - pillsTotalW - 12.f);
    L.pillX = L.contentX + L.searchVW + 12.f;

    // Left table
    L.tableX = L.contentX;
    L.tableY = L.mainTopY + 120.f;
    L.tblRight = L.tableX + L.leftW;
    L.hdrH = 42.f;
    L.rowH = 68.f;

    float c1 = std::floor(L.leftW * 0.36f);
    float c2 = std::floor(L.leftW * 0.11f);
    float c3 = std::floor(L.leftW * 0.14f);
    float c4 = std::floor(L.leftW * 0.15f);
    float c5 = std::floor(L.leftW * 0.12f);
    L.div1 = L.tableX + c1;
    L.div2 = L.div1 + c2;
    L.div3 = L.div2 + c3;
    L.div4 = L.div3 + c4;
    L.div5 = L.div4 + c5;

    L.pagY = L.contentBottomY - 32.f;

    // Right panel
    L.rightCardH = L.contentBottomY - L.mainTopY;
    L.rActY = L.mainTopY + 68.f;
    L.rFltY = L.mainTopY + 112.f;
    L.rCountW = 114.f;
    L.rSearchW = max(140.f, L.rightW - 32.f - L.rCountW - 10.f);

    L.rTblX = L.rightX + 16.f;
    L.rTblW = L.rightW - 32.f;
    L.rTblY = L.mainTopY + 156.f;
    L.rHdrH = 38.f;
    L.rRowH = 60.f;
    L.rTblRight = L.rTblX + L.rTblW;

    L.rDiv1 = L.rTblX + std::floor(L.rTblW * 0.43f);
    L.rDiv2 = L.rDiv1 + std::floor(L.rTblW * 0.27f);
    L.rDiv3 = L.rDiv2 + std::floor(L.rTblW * 0.10f);

    L.rFootY = L.contentBottomY - 38.f;
    L.rPagRight = L.rightX + L.rightW - 16.f;
    return L;
}
}

void VoucherPanel::handleEvent(const Event& event, const RenderWindow& win) {
    // 1. ƯU TIÊN XỬ LÝ POPUP THÊM / SỬA VOUCHER (MODAL BLOCKING)
    if (showAddPopup) {
        float popW = 500.f;
        float popH = 340.f;
        float popX = position.x + (width - popW) / 2.f;
        float popY = position.y + (height - popH) / 2.f - 20.f;

        auto commitSaveVoucher = [&]() {
            if (popupCode.empty()) popupCode = "VOUCHER" + to_string(allVouchers.size() + 1);
            if (popupSub.empty()) popupSub = L"\u01afu \u0111\u00e3i kh\u00e1ch h\u00e0ng CineCore";
            int parsedVal = (popupType == 2) ? 20 : 30000;
            int parsedMin = 0;
            try { if (!popupValue.empty()) parsedVal = stoi(popupValue); } catch (...) {}
            try { if (!popupMinBill.empty()) parsedMin = stoi(popupMinBill); } catch (...) {}

            if (editingIndex >= 0 && editingIndex < (int)allVouchers.size()) {
                allVouchers[editingIndex].code = popupCode;
                allVouchers[editingIndex].subtitle = popupSub;
                allVouchers[editingIndex].type = popupType;
                allVouchers[editingIndex].value = parsedVal;
                allVouchers[editingIndex].minBill = parsedMin;
            } else {
                VoucherItem nv;
                nv.code = popupCode;
                nv.subtitle = popupSub;
                nv.type = popupType;
                nv.value = parsedVal;
                nv.minBill = parsedMin;
                nv.recipientCount = 0;
                nv.tag = L"M\u1edbi";
                allVouchers.insert(allVouchers.begin(), nv);
                selectedVoucherIndex = 0;
                currentVoucherPage = 1;
            }
            saveVouchersToFile();
            applyVoucherFilter();
            showAddPopup = false;
            notificationMessage = "Đã lưu thông tin voucher thành công!";
            notificationClock.restart();
            notificationVisible = true;
        };

        if (const auto* mouseBtn = event.getIf<Event::MouseButtonPressed>()) {
            if (mouseBtn->button == Mouse::Button::Left) {
                Vector2f mousePos = win.mapPixelToCoords(Mouse::getPosition(win));

                FloatRect boxCode({popX + 30.f, popY + 85.f}, {210.f, 36.f});
                FloatRect boxType({popX + 260.f, popY + 85.f}, {210.f, 36.f});
                FloatRect boxSub({popX + 30.f, popY + 150.f}, {popW - 60.f, 36.f});
                FloatRect boxVal({popX + 30.f, popY + 215.f}, {210.f, 36.f});
                FloatRect boxMin({popX + 260.f, popY + 215.f}, {210.f, 36.f});
                FloatRect btnPopSave({popX + 70.f, popY + 270.f}, {160.f, 38.f});
                FloatRect btnPopCancel({popX + 270.f, popY + 270.f}, {160.f, 38.f});

                if (boxCode.contains(mousePos)) {
                    popupActiveField = 0;
                    if (editingIndex < 0 && popupCode == "NEWVOUCHER") popupCode.clear();
                    return;
                }
                if (boxType.contains(mousePos)) {
                    popupActiveField = 1;
                    popupType = (popupType == 2) ? 1 : 2;
                    return;
                }
                if (boxSub.contains(mousePos)) {
                    popupActiveField = 2;
                    return;
                }
                if (boxVal.contains(mousePos)) {
                    popupActiveField = 3;
                    return;
                }
                if (boxMin.contains(mousePos)) {
                    popupActiveField = 4;
                    return;
                }
                if (btnPopSave.contains(mousePos)) {
                    commitSaveVoucher();
                    return;
                }
                if (btnPopCancel.contains(mousePos) || !FloatRect({popX, popY}, {popW, popH}).contains(mousePos)) {
                    showAddPopup = false;
                    return;
                }
            }
            return;
        }

        if (const auto* keyEv = event.getIf<Event::KeyPressed>()) {
            if (keyEv->code == Keyboard::Key::Escape) { showAddPopup = false; return; }
            if (keyEv->code == Keyboard::Key::Tab) { popupActiveField = (popupActiveField + 1) % 5; return; }
            if (keyEv->code == Keyboard::Key::Enter) { commitSaveVoucher(); return; }
            if (keyEv->code == Keyboard::Key::Backspace) {
                if (popupActiveField == 0 && !popupCode.empty()) popupCode.pop_back();
                else if (popupActiveField == 2 && !popupSub.empty()) popupSub.pop_back();
                else if (popupActiveField == 3 && !popupValue.empty()) popupValue.pop_back();
                else if (popupActiveField == 4 && !popupMinBill.empty()) popupMinBill.pop_back();
                return;
            }
        }

        if (const auto* txtEv = event.getIf<Event::TextEntered>()) {
            char32_t u = txtEv->unicode;
            if (u >= 32 && u != 127) {
                if (popupActiveField == 0 && popupCode.size() < 18) {
                    char ch = static_cast<char>(toupper((int)u));
                    if ((ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || ch == '_' || ch == '-')
                        popupCode += ch;
                } else if (popupActiveField == 1) {
                    if (u == '1') popupType = 1;
                    else if (u == '2') popupType = 2;
                } else if (popupActiveField == 2 && popupSub.size() < 55) {
                    popupSub += static_cast<wchar_t>(u);
                } else if (popupActiveField == 3 && u >= '0' && u <= '9' && popupValue.size() < 8) {
                    popupValue += static_cast<char>(u);
                } else if (popupActiveField == 4 && u >= '0' && u <= '9' && popupMinBill.size() < 8) {
                    popupMinBill += static_cast<char>(u);
                }
            }
            return;
        }
        return;
    }

    // 1b. ƯU TIÊN XỬ LÝ POPUP THÊM NGƯỜI NHẬN (MODAL BLOCKING)
    if (showAddRecipientPopup) {
        float popW = 480.f;
        float popH = 340.f;
        float popX = position.x + (width - popW) / 2.f;
        float popY = position.y + (height - popH) / 2.f - 20.f;

        auto commitAddRecipient = [&]() {
            if (popupRecipientEmail.empty()) {
                notificationMessage = "Vui lòng nhập email khách hàng!";
                notificationClock.restart();
                notificationVisible = true;
                return;
            }
            RecipientItem nr;
            nr.email = popupRecipientEmail;
            nr.fullName = popupRecipientName.empty() ? L"Khách Hàng Mới" : popupRecipientName;
            nr.receivedDate = "10/11/2026";
            int q = 1;
            try {
                if (!popupRecipientQty.empty()) q = max(1, stoi(popupRecipientQty));
            } catch (...) { q = 1; }
            nr.quantity = q;
            nr.isUsed = false;

            currentRecipients.push_back(nr);
            applyRecipientFilter();
            currentRecipientPage = totalRecipientPages;

            if (selectedVoucherIndex >= 0 && selectedVoucherIndex < (int)filteredVouchers.size()) {
                string selCode = filteredVouchers[selectedVoucherIndex].code;
                for (auto& v : allVouchers) {
                    if (v.code == selCode) {
                        v.recipientCount = (int)currentRecipients.size();
                        break;
                    }
                }
                for (auto& v : filteredVouchers) {
                    if (v.code == selCode) {
                        v.recipientCount = (int)currentRecipients.size();
                        break;
                    }
                }
            }

            showAddRecipientPopup = false;
            notificationMessage = "Đã thêm người nhận voucher thành công!";
            notificationClock.restart();
            notificationVisible = true;
        };

        if (const auto* mouseBtn = event.getIf<Event::MouseButtonPressed>()) {
            if (mouseBtn->button == Mouse::Button::Left) {
                Vector2f mousePos = win.mapPixelToCoords(Mouse::getPosition(win));

                FloatRect boxEmail({popX + 30.f, popY + 85.f}, {popW - 60.f, 36.f});
                FloatRect boxName({popX + 30.f, popY + 150.f}, {popW - 60.f, 36.f});
                FloatRect boxQty({popX + 30.f, popY + 215.f}, {200.f, 36.f});
                FloatRect btnPopAdd({popX + 50.f, popY + 272.f}, {170.f, 38.f});
                FloatRect btnPopCancel({popX + 260.f, popY + 272.f}, {170.f, 38.f});

                if (boxEmail.contains(mousePos)) {
                    recipientPopupActiveField = 0;
                    return;
                }
                if (boxName.contains(mousePos)) {
                    recipientPopupActiveField = 1;
                    return;
                }
                if (boxQty.contains(mousePos)) {
                    recipientPopupActiveField = 2;
                    return;
                }
                if (btnPopAdd.contains(mousePos)) {
                    commitAddRecipient();
                    return;
                }
                if (btnPopCancel.contains(mousePos) || !FloatRect({popX, popY}, {popW, popH}).contains(mousePos)) {
                    showAddRecipientPopup = false;
                    return;
                }
            }
            return;
        }

        if (const auto* keyEv = event.getIf<Event::KeyPressed>()) {
            if (keyEv->code == Keyboard::Key::Escape) { showAddRecipientPopup = false; return; }
            if (keyEv->code == Keyboard::Key::Tab) { recipientPopupActiveField = (recipientPopupActiveField + 1) % 3; return; }
            if (keyEv->code == Keyboard::Key::Enter) { commitAddRecipient(); return; }
            if (keyEv->code == Keyboard::Key::Backspace) {
                if (recipientPopupActiveField == 0 && !popupRecipientEmail.empty()) popupRecipientEmail.pop_back();
                else if (recipientPopupActiveField == 1 && !popupRecipientName.empty()) popupRecipientName.pop_back();
                else if (recipientPopupActiveField == 2 && !popupRecipientQty.empty()) popupRecipientQty.pop_back();
                return;
            }
        }

        if (const auto* txtEv = event.getIf<Event::TextEntered>()) {
            char32_t u = txtEv->unicode;
            if (u >= 32 && u != 127) {
                if (recipientPopupActiveField == 0 && popupRecipientEmail.size() < 45) {
                    if ((u >= 'a' && u <= 'z') || (u >= 'A' && u <= 'Z') || (u >= '0' && u <= '9') || u == '@' || u == '.' || u == '_' || u == '-')
                        popupRecipientEmail += static_cast<char>(u);
                } else if (recipientPopupActiveField == 1 && popupRecipientName.size() < 40) {
                    popupRecipientName += static_cast<wchar_t>(u);
                } else if (recipientPopupActiveField == 2 && u >= '0' && u <= '9' && popupRecipientQty.size() < 4) {
                    popupRecipientQty += static_cast<char>(u);
                }
            }
            return;
        }
        return;
    }

    // 1c. ƯU TIÊN XỬ LÝ POPUP XÁC NHẬN THU HỒI VOUCHER (MODAL BLOCKING)
    if (showRevokeConfirmPopup) {
        float popW = 460.f;
        float popH = 250.f;
        float popX = position.x + (width - popW) / 2.f;
        float popY = position.y + (height - popH) / 2.f - 20.f;

        auto commitRevoke = [&]() {
            if (selectedRecipientIndex >= 0 && selectedRecipientIndex < (int)filteredRecipients.size()) {
                string targetEmail = filteredRecipients[selectedRecipientIndex].email;
                for (auto it = currentRecipients.begin(); it != currentRecipients.end(); ++it) {
                    if (it->email == targetEmail) {
                        currentRecipients.erase(it);
                        break;
                    }
                }
                selectedRecipientIndex = -1;
                applyRecipientFilter();
                if (selectedVoucherIndex >= 0 && selectedVoucherIndex < (int)filteredVouchers.size()) {
                    string selCode = filteredVouchers[selectedVoucherIndex].code;
                    for (auto& v : allVouchers) {
                        if (v.code == selCode) { v.recipientCount = (int)currentRecipients.size(); break; }
                    }
                    for (auto& v : filteredVouchers) {
                        if (v.code == selCode) { v.recipientCount = (int)currentRecipients.size(); break; }
                    }
                }
                notificationMessage = "Đã thu hồi voucher của " + targetEmail + "!";
                notificationClock.restart();
                notificationVisible = true;
            }
            showRevokeConfirmPopup = false;
        };

        if (const auto* mouseBtn = event.getIf<Event::MouseButtonPressed>()) {
            if (mouseBtn->button == Mouse::Button::Left) {
                Vector2f mousePos = win.mapPixelToCoords(Mouse::getPosition(win));
                FloatRect btnConfirm({popX + 40.f, popY + 180.f}, {170.f, 38.f});
                FloatRect btnCancel({popX + 240.f, popY + 180.f}, {170.f, 38.f});

                if (btnConfirm.contains(mousePos)) {
                    commitRevoke();
                    return;
                }
                if (btnCancel.contains(mousePos) || !FloatRect({popX, popY}, {popW, popH}).contains(mousePos)) {
                    showRevokeConfirmPopup = false;
                    return;
                }
            }
            return;
        }

        if (const auto* keyEv = event.getIf<Event::KeyPressed>()) {
            if (keyEv->code == Keyboard::Key::Escape) { showRevokeConfirmPopup = false; return; }
            if (keyEv->code == Keyboard::Key::Enter) { commitRevoke(); return; }
        }
        return;
    }

    if (const auto* mouseBtn = event.getIf<Event::MouseButtonPressed>()) {
        if (mouseBtn->button == Mouse::Button::Left) {
            Vector2f mousePos = win.mapPixelToCoords(Mouse::getPosition(win));

            // Nút Header: Xuất Excel, Hướng dẫn
            FloatRect btnExp({position.x + width - 248.f, position.y + 40.f}, {100.f, 36.f});
            FloatRect btnGuide({position.x + width - 138.f, position.y + 40.f}, {90.f, 36.f});

            if (btnExp.contains(mousePos)) {
                if (exportToExcel()) {
                    notificationMessage = "Đã xuất báo cáo: data/voucher_report.csv (Mở tự động bằng Excel)!";
                } else {
                    notificationMessage = "Không thể xuất file báo cáo voucher!";
                }
                notificationClock.restart();
                notificationVisible = true;
                return;
            }
            if (btnGuide.contains(mousePos)) {
                notificationMessage = "Hướng dẫn: 1. Thêm/Sửa voucher bên trái • 2. Chọn voucher để quản lý người nhận bên phải!";
                notificationClock.restart();
                notificationVisible = true;
                return;
            }

            VoucherLayout L = computeVoucherLayout(position, width, height);
            pageSize = getVoucherPageSize();
            recipientPageSize = getRecipientPageSize();

            // Cột Trái - Hàng nút thao tác: + Thêm mới, Sửa, Xóa, Phát hành, Reload
            float actY = L.actY;
            FloatRect btnAdd({L.contentX, actY}, {108.f, 34.f});
            FloatRect btnEdit({L.contentX + 116.f, actY}, {74.f, 34.f});
            FloatRect btnDel({L.contentX + 198.f, actY}, {74.f, 34.f});
            FloatRect btnDistribute({L.contentX + 280.f, actY}, {98.f, 34.f});
            FloatRect btnRel({L.contentX + 386.f, actY}, {34.f, 34.f});

            if (btnAdd.contains(mousePos)) {
                showAddPopup = true;
                editingIndex = -1;
                popupActiveField = 0;
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
                            popupActiveField = 0;
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
                searchVoucherQuery.clear();
                voucherFilterType = 0;
                loadVouchersFromFile();
                applyVoucherFilter();
                notificationMessage = "Đã làm mới dữ liệu!";
                notificationClock.restart();
                notificationVisible = true;
                return;
            }

            // Cột Trái - Ô tìm kiếm & Bộ lọc Pill
            FloatRect searchVB({L.contentX, L.fltY}, {L.searchVW, 34.f});
            searchVoucherActive = searchVB.contains(mousePos);

            FloatRect pAll({L.pillX, L.fltY}, {L.pillW[0], 34.f});
            FloatRect pPct({L.pillX + L.pillW[0] + 8.f, L.fltY}, {L.pillW[1], 34.f});
            FloatRect pVal({L.pillX + L.pillW[0] + L.pillW[1] + 16.f, L.fltY}, {L.pillW[2], 34.f});

            if (pAll.contains(mousePos)) { voucherFilterType = 0; currentVoucherPage = 1; applyVoucherFilter(); return; }
            if (pPct.contains(mousePos)) { voucherFilterType = 1; currentVoucherPage = 1; applyVoucherFilter(); return; }
            if (pVal.contains(mousePos)) { voucherFilterType = 2; currentVoucherPage = 1; applyVoucherFilter(); return; }

            // Cột Trái - Click chọn dòng hoặc nút Sửa trong bảng Voucher
            int startIdx = (currentVoucherPage - 1) * pageSize;
            int count = max(0, min((int)filteredVouchers.size() - startIdx, pageSize));
            float editBtnW = min(56.f, (L.tblRight - L.div5) - 10.f);
            float editBtnX = L.div5 + ((L.tblRight - L.div5) - editBtnW) / 2.f;

            for (int r = 0; r < count; ++r) {
                float rY = L.tableY + L.hdrH + r * L.rowH;
                FloatRect rowRect({L.tableX, rY}, {L.leftW, L.rowH});
                FloatRect rowEditBtn({editBtnX, rY + (L.rowH - 28.f) / 2.f}, {editBtnW, 28.f});

                if (rowEditBtn.contains(mousePos)) {
                    selectedVoucherIndex = startIdx + r;
                    for (size_t i = 0; i < filteredVouchers.size(); ++i) {
                        filteredVouchers[i].isChecked = ((int)i == selectedVoucherIndex);
                    }
                    string selCode = filteredVouchers[selectedVoucherIndex].code;
                    for (size_t i = 0; i < allVouchers.size(); ++i) {
                        if (allVouchers[i].code == selCode) {
                            editingIndex = (int)i;
                            popupActiveField = 0;
                            popupCode = allVouchers[i].code;
                            popupSub = allVouchers[i].subtitle;
                            popupType = allVouchers[i].type;
                            popupValue = to_string(allVouchers[i].value);
                            popupMinBill = to_string(allVouchers[i].minBill);
                            showAddPopup = true;
                            return;
                        }
                    }
                    return;
                }

                if (rowRect.contains(mousePos)) {
                    selectedVoucherIndex = startIdx + r;
                    for (size_t i = 0; i < filteredVouchers.size(); ++i) {
                        filteredVouchers[i].isChecked = ((int)i == selectedVoucherIndex);
                    }
                    currentRecipientPage = 1;
                    searchRecipientQuery.clear();
                    searchRecipientActive = false;
                    applyRecipientFilter();
                    return;
                }
            }

            // Phân trang Cột Trái
            float pagY = L.pagY;
            float tableRight = L.tblRight;
            FloatRect btnPrev({tableRight - 124.f, pagY}, {28.f, 28.f});
            FloatRect btnP1({tableRight - 92.f, pagY}, {28.f, 28.f});
            FloatRect btnP2({tableRight - 60.f, pagY}, {28.f, 28.f});
            FloatRect btnNext({tableRight - 28.f, pagY}, {28.f, 28.f});

            if (btnPrev.contains(mousePos) && currentVoucherPage > 1) { currentVoucherPage--; applyVoucherFilter(); return; }
            if (btnNext.contains(mousePos) && currentVoucherPage < totalVoucherPages) { currentVoucherPage++; applyVoucherFilter(); return; }
            if (btnP1.contains(mousePos)) { currentVoucherPage = 1; applyVoucherFilter(); return; }
            if (btnP2.contains(mousePos) && totalVoucherPages >= 2) { currentVoucherPage = 2; applyVoucherFilter(); return; }

            // Cột Phải - Hàng nút: Quay lại, Thêm người, Thu hồi
            float rightX = L.rightX;
            float rActY = L.rActY;
            FloatRect btnBack({rightX + 16.f, rActY}, {94.f, 34.f});
            FloatRect btnAddUser({rightX + 120.f, rActY}, {116.f, 34.f});
            FloatRect btnRevoke({rightX + 246.f, rActY}, {88.f, 34.f});

            if (btnBack.contains(mousePos)) {
                currentRecipientPage = 1;
                searchRecipientQuery.clear();
                searchRecipientActive = false;
                selectedVoucherIndex = 0;
                for (size_t i = 0; i < filteredVouchers.size(); ++i) {
                    filteredVouchers[i].isChecked = (i == 0);
                }
                applyRecipientFilter();
                notificationMessage = "Đã quay lại danh sách tổng quan (Trang 1)!";
                notificationClock.restart();
                notificationVisible = true;
                return;
            }

            if (btnAddUser.contains(mousePos)) {
                showAddRecipientPopup = true;
                popupRecipientEmail.clear();
                popupRecipientName.clear();
                popupRecipientQty = "1";
                recipientPopupActiveField = 0;
                return;
            }

            if (btnRevoke.contains(mousePos)) {
                if (selectedRecipientIndex < 0 || selectedRecipientIndex >= (int)filteredRecipients.size()) {
                    notificationMessage = "Vui lòng click chọn email khách hàng cần thu hồi voucher!";
                    notificationClock.restart();
                    notificationVisible = true;
                    return;
                }
                showRevokeConfirmPopup = true;
                return;
            }

            // Search Recipient
            FloatRect searchRB({rightX + 16.f, L.rFltY}, {L.rSearchW, 34.f});
            searchRecipientActive = searchRB.contains(mousePos);

            // Click chọn dòng người nhận trong bảng Cột Phải
            int rStartIdx = (currentRecipientPage - 1) * recipientPageSize;
            int rCount = max(0, min((int)filteredRecipients.size() - rStartIdx, recipientPageSize));
            for (int r = 0; r < rCount; ++r) {
                int actualIdx = rStartIdx + r;
                float curY = L.rTblY + L.rHdrH + r * L.rRowH;
                FloatRect rowRect({L.rTblX, curY}, {L.rTblW, L.rRowH});
                if (rowRect.contains(mousePos)) {
                    if (selectedRecipientIndex == actualIdx) {
                        selectedRecipientIndex = -1;
                    } else {
                        selectedRecipientIndex = actualIdx;
                    }
                    return;
                }
            }

            // Phân trang Cột Phải (Người nhận)
            float rFootY = L.rFootY;
            float rPagRight = L.rPagRight;
            if (totalRecipientPages > 1) {
                FloatRect rBtnPrev({rPagRight - 88.f, rFootY}, {28.f, 28.f});
                FloatRect rBtnNext({rPagRight - 28.f, rFootY}, {28.f, 28.f});
                if (rBtnPrev.contains(mousePos) && currentRecipientPage > 1) {
                    currentRecipientPage--;
                    selectedRecipientIndex = -1;
                    return;
                }
                if (rBtnNext.contains(mousePos) && currentRecipientPage < totalRecipientPages) {
                    currentRecipientPage++;
                    selectedRecipientIndex = -1;
                    return;
                }
            }
        }
    }

    // Gõ phím tìm kiếm (hỗ trợ đầy đủ tiếng Việt Unicode)
    if (const auto* keyEv = event.getIf<Event::KeyPressed>()) {
        if (searchVoucherActive) {
            if (keyEv->code == Keyboard::Key::Backspace && !searchVoucherQuery.empty()) {
                searchVoucherQuery.pop_back();
                currentVoucherPage = 1;
                applyVoucherFilter();
                return;
            }
            if (keyEv->code == Keyboard::Key::Escape) {
                searchVoucherQuery.clear();
                searchVoucherActive = false;
                applyVoucherFilter();
                return;
            }
        } else if (searchRecipientActive) {
            if (keyEv->code == Keyboard::Key::Backspace && !searchRecipientQuery.empty()) {
                searchRecipientQuery.pop_back();
                applyRecipientFilter();
                return;
            }
            if (keyEv->code == Keyboard::Key::Escape) {
                searchRecipientQuery.clear();
                searchRecipientActive = false;
                applyRecipientFilter();
                return;
            }
        }
    }

    if (const auto* textEv = event.getIf<Event::TextEntered>()) {
        char32_t c = textEv->unicode;
        if (c >= 32 && c != 127) {
            if (searchVoucherActive && searchVoucherQuery.size() < 30) {
                searchVoucherQuery += static_cast<wchar_t>(c);
                currentVoucherPage = 1;
                applyVoucherFilter();
            } else if (searchRecipientActive && searchRecipientQuery.size() < 30) {
                searchRecipientQuery += static_cast<wchar_t>(c);
                applyRecipientFilter();
            }
        }
    }
}

void VoucherPanel::update(Vector2f mousePos, bool) {
    hoveredVoucherRow = -1;
    hoveredEditBtnRow = -1;
    hoveredRecipientRow = -1;
    hoveredBackBtn = false;
    hoveredAddUserBtn = false;
    hoveredRevokeBtn = false;

    if (!showAddPopup && !showAddRecipientPopup && !showRevokeConfirmPopup) {
        VoucherLayout L = computeVoucherLayout(position, width, height);
        pageSize = getVoucherPageSize();
        recipientPageSize = getRecipientPageSize();

        int startIdx = (currentVoucherPage - 1) * pageSize;
        int count = min((int)filteredVouchers.size() - startIdx, pageSize);
        float editBtnW = min(56.f, (L.tblRight - L.div5) - 10.f);
        float editBtnX = L.div5 + ((L.tblRight - L.div5) - editBtnW) / 2.f;

        for (int r = 0; r < count; ++r) {
            float rY = L.tableY + L.hdrH + r * L.rowH;
            FloatRect rowEditBtn({editBtnX, rY + (L.rowH - 28.f) / 2.f}, {editBtnW, 28.f});
            if (rowEditBtn.contains(mousePos)) {
                hoveredEditBtnRow = r;
            }
            if (FloatRect({L.tableX, rY}, {L.leftW, L.rowH}).contains(mousePos)) {
                hoveredVoucherRow = r;
            }
        }

        // Cột phải hover
        FloatRect btnBack({L.rightX + 16.f, L.rActY}, {94.f, 34.f});
        FloatRect btnAddUser({L.rightX + 120.f, L.rActY}, {116.f, 34.f});
        FloatRect btnRevoke({L.rightX + 246.f, L.rActY}, {88.f, 34.f});
        hoveredBackBtn = btnBack.contains(mousePos);
        hoveredAddUserBtn = btnAddUser.contains(mousePos);
        hoveredRevokeBtn = btnRevoke.contains(mousePos);

        int rStartIdx = (currentRecipientPage - 1) * recipientPageSize;
        int rCount = max(0, min((int)filteredRecipients.size() - rStartIdx, recipientPageSize));
        for (int r = 0; r < rCount; ++r) {
            float curY = L.rTblY + L.rHdrH + r * L.rRowH;
            if (FloatRect({L.rTblX, curY}, {L.rTblW, L.rRowH}).contains(mousePos)) {
                hoveredRecipientRow = rStartIdx + r;
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
    // ==========================================
    // 3. PAGE HEADER CARD (Unified RoomPanel style)
    // ==========================================
    UITheme::drawSectionHeaderCard(win, position.x + 30.f, position.y + 16.f, width - 60.f,
                                   L"Qu\u1ea3n l\u00fd Voucher",
                                   to_wstring(allVouchers.size()) + L" Ch\u01b0\u01a1ng tr\u00ecnh ho\u1ea1t \u0111\u1ed9ng",
                                   Color(224, 242, 254), Color(8, 125, 181),
                                   L"C\u1ea5u h\u00ecnh \u0111i\u1ec1u ki\u1ec7n \u01b0u \u0111\u00e3i, danh s\u00e1ch ph\u00e1t h\u00e0nh v\u00e0 ki\u1ec3m so\u00e1t ph\u00e2n b\u1ed5 ng\u01b0\u1eddi nh\u1eadn.",
                                   boldFont, font, false);

    // Nút Header: Xuất Excel & Hướng dẫn
    drawRoundedCard(win, {position.x + width - 248.f, position.y + 40.f}, {100.f, 36.f}, 8.f, Color::White, 1.f, Color(203, 213, 225));
    Text expTxt(boldFont, L"Xu\u1ea5t Excel", 14); expTxt.setFillColor(Color(22, 101, 52));
    FloatRect eb = expTxt.getLocalBounds();
    expTxt.setPosition({position.x + width - 248.f + (100.f - eb.size.x) / 2.f - eb.position.x, position.y + 40.f + (36.f - eb.size.y) / 2.f - eb.position.y});
    win.draw(expTxt);

    drawRoundedCard(win, {position.x + width - 138.f, position.y + 40.f}, {90.f, 36.f}, 8.f, Color::White, 1.f, Color(186, 230, 253));
    Text guiTxt(boldFont, L"H\u01b0\u1edbng d\u1eabn", 14); guiTxt.setFillColor(Color(8, 125, 181));
    FloatRect gb = guiTxt.getLocalBounds();
    guiTxt.setPosition({position.x + width - 138.f + (90.f - gb.size.x) / 2.f - gb.position.x, position.y + 40.f + (36.f - gb.size.y) / 2.f - gb.position.y});
    win.draw(guiTxt);

    // ==========================================
    // 4. 4 THẺ THỐNG KÊ KPI (STAT CARDS GRID)
    // ==========================================
    float statY = position.y + 114.f;
    float cardW = (width - 60.f - 45.f) / 4.f;
    float cardH = 82.f;

    // Card 1: Tổng voucher khả dụng
    float c1X = position.x + 30.f;
    drawRoundedCard(win, {c1X, statY}, {cardW, cardH}, 10.f, Color::White, 1.f, Color(226, 232, 240));
    Text sc1T(boldFont, L"T\u1ed5ng voucher kh\u1ea3 d\u1ee5ng", 14); sc1T.setFillColor(Color(100, 116, 139)); sc1T.setPosition({c1X + 16.f, statY + 12.f}); win.draw(sc1T);
    Text sc1V(boldFont, to_wstring(allVouchers.size()), 24); sc1V.setFillColor(Color(23, 32, 51)); sc1V.setPosition({c1X + 16.f, statY + 34.f}); win.draw(sc1V);
    FloatRect v1b = sc1V.getGlobalBounds();
    Text sc1U(font, L"chi\u1ebfn d\u1ecbch", 14); sc1U.setFillColor(Color(100, 116, 139)); sc1U.setPosition({v1b.position.x + v1b.size.x + 8.f, statY + 41.f}); win.draw(sc1U);

    // Card 2: Đã quy đổi / Sử dụng
    float c2X = c1X + cardW + 15.f;
    drawRoundedCard(win, {c2X, statY}, {cardW, cardH}, 10.f, Color::White, 1.f, Color(226, 232, 240));
    Text sc2T(boldFont, L"\u0110\u00e3 quy \u0111\u1ed5i / S\u1eed d\u1ee5ng", 14); sc2T.setFillColor(Color(100, 116, 139)); sc2T.setPosition({c2X + 16.f, statY + 12.f}); win.draw(sc2T);
    Text sc2V(boldFont, L"1,420", 24); sc2V.setFillColor(Color(23, 32, 51)); sc2V.setPosition({c2X + 16.f, statY + 34.f}); win.draw(sc2V);
    FloatRect v2b = sc2V.getGlobalBounds();
    Text sc2P(boldFont, L"(68%)", 14); sc2P.setFillColor(Color(21, 128, 61)); sc2P.setPosition({v2b.position.x + v2b.size.x + 8.f, statY + 41.f}); win.draw(sc2P);

    // Card 3: Tổng ngân sách chiết khấu
    float c3X = c2X + cardW + 15.f;
    drawRoundedCard(win, {c3X, statY}, {cardW, cardH}, 10.f, Color::White, 1.f, Color(226, 232, 240));
    Text sc3T(boldFont, L"T\u1ed5ng ng\u00e2n s\u00e1ch chi\u1ebft kh\u1ea5u", 14); sc3T.setFillColor(Color(100, 116, 139)); sc3T.setPosition({c3X + 16.f, statY + 12.f}); win.draw(sc3T);
    Text sc3V(boldFont, L"42.500.000 \u0110", 24); sc3V.setFillColor(Color(23, 32, 51)); sc3V.setPosition({c3X + 16.f, statY + 36.f}); win.draw(sc3V);

    // Card 4: Đang chọn khảo sát
    float c4X = c3X + cardW + 15.f;
    drawRoundedCard(win, {c4X, statY}, {cardW, cardH}, 10.f, Color::White, 1.f, Color(226, 232, 240));
    Text sc4T(font, L"\u0110ang ch\u1ecdn kh\u1ea3o s\u00e1t", 14); sc4T.setFillColor(Color(100, 116, 139)); sc4T.setPosition({c4X + 16.f, statY + 12.f}); win.draw(sc4T);
    string selCode = (selectedVoucherIndex >= 0 && selectedVoucherIndex < (int)filteredVouchers.size()) ? filteredVouchers[selectedVoucherIndex].code : "WELCOME";
    Text sc4V(boldFont, s2ws(selCode), 24); sc4V.setFillColor(Color(8, 125, 181)); sc4V.setPosition({c4X + 16.f, statY + 36.f}); win.draw(sc4V);

    // ==========================================
    // 5. CỘT TRÁI: DANH SÁCH VOUCHER (MASTER VIEW)
    // ==========================================
    VoucherLayout L = computeVoucherLayout(position, width, height);
    pageSize = getVoucherPageSize();
    recipientPageSize = getRecipientPageSize();
    totalVoucherPages = max(1, (int)((filteredVouchers.size() + pageSize - 1) / pageSize));
    if (currentVoucherPage > totalVoucherPages) currentVoucherPage = totalVoucherPages;
    totalRecipientPages = max(1, (int)((filteredRecipients.size() + recipientPageSize - 1) / recipientPageSize));
    if (currentRecipientPage > totalRecipientPages) currentRecipientPage = totalRecipientPages;

    float leftW = L.leftW;
    Text lHead(boldFont, L"\u2022 Danh s\u00e1ch Voucher", 16);
    lHead.setFillColor(Color(23, 32, 51));
    lHead.setPosition({L.contentX, std::round(L.mainTopY + 2.f)});
    win.draw(lHead);

    FloatRect lhb = lHead.getGlobalBounds();
    Text lCount(font, L"(" + to_wstring(filteredVouchers.size()) + L" b\u1ea3n ghi)", 14);
    lCount.setFillColor(Color(100, 116, 139));
    lCount.setPosition({std::round(lhb.position.x + lhb.size.x + 10.f), std::round(L.mainTopY + 4.f)});
    win.draw(lCount);

    // Hàng nút thao tác Cột Trái
    float actY = L.actY;
    auto drawLeftActionBtn = [&](float bx, float bw, const wstring& lbl, Color bgCol, Color fgCol, float outW = 0.f, Color outCol = Color::Transparent) {
        drawRoundedCard(win, {bx, actY}, {bw, 34.f}, 6.f, bgCol, outW, outCol);
        Text t(boldFont, lbl, 14);
        t.setFillColor(fgCol);
        FloatRect tb = t.getLocalBounds();
        t.setPosition({std::round(bx + (bw - tb.size.x) / 2.f - tb.position.x), std::round(actY + (34.f - tb.size.y) / 2.f - tb.position.y)});
        win.draw(t);
    };
    drawLeftActionBtn(L.contentX, 108.f, L"+ Th\u00eam m\u1edbi", Color(21, 128, 61), Color::White);
    drawLeftActionBtn(L.contentX + 116.f, 74.f, L"S\u1eeda", Color::White, Color(217, 119, 6), 1.f, Color(217, 119, 6));
    drawLeftActionBtn(L.contentX + 198.f, 74.f, L"X\u00f3a", Color::White, Color(220, 38, 38), 1.f, Color(220, 38, 38));
    drawLeftActionBtn(L.contentX + 280.f, 98.f, L"Ph\u00e1t h\u00e0nh", Color(238, 242, 255), Color(67, 56, 202), 1.f, Color(199, 210, 254));

    drawRoundedCard(win, {L.contentX + 386.f, actY}, {34.f, 34.f}, 6.f, Color::White, 1.f, Color(203, 213, 225));
    UITheme::drawReloadIcon(win, {L.contentX + 403.f, actY + 17.f}, 16.f, Color(71, 85, 105));

    // Hàng tìm kiếm & Bộ lọc Pill
    float fltY = L.fltY;
    drawRoundedCard(win, {L.contentX, fltY}, {L.searchVW, 34.f}, 6.f, Color::White, searchVoucherActive ? 1.8f : 1.f, searchVoucherActive ? Color(8, 125, 181) : Color(203, 213, 225));
    wstring sVPrompt = searchVoucherQuery.empty()
        ? (searchVoucherActive ? L"|" : L"T\u00ecm theo m\u00e3 voucher ho\u1eb7c chi\u1ebfn d\u1ecbch...")
        : (searchVoucherQuery + (searchVoucherActive ? L"|" : L""));
    sVPrompt = ellipsizeText(font, sVPrompt, 14, L.searchVW - 20.f);
    Text sVTxt(font, sVPrompt, 14);
    sVTxt.setFillColor(searchVoucherQuery.empty() && !searchVoucherActive ? Color(148, 163, 184) : Color(23, 32, 51));
    sVTxt.setPosition({L.contentX + 10.f, std::round(fltY + 8.f)});
    win.draw(sVTxt);

    vector<pair<wstring, float>> vPills = {{L"T\u1ea5t c\u1ea3", L.pillW[0]}, {L"Gi\u1ea3m %", L.pillW[1]}, {L"Gi\u1ea3m ti\u1ec1n", L.pillW[2]}};
    float vPX = L.pillX;
    for (size_t i = 0; i < vPills.size(); ++i) {
        bool isAct = ((int)i == voucherFilterType);
        drawRoundedCard(win, {vPX, fltY}, {vPills[i].second, 34.f}, 17.f, isAct ? Color(8, 125, 181) : Color::White, isAct ? 0.f : 1.f, Color(226, 232, 240));
        Text vpTxt(boldFont, vPills[i].first, 14);
        vpTxt.setFillColor(isAct ? Color::White : Color(71, 85, 105));
        FloatRect vptb = vpTxt.getLocalBounds();
        vpTxt.setPosition({std::round(vPX + (vPills[i].second - vptb.size.x) / 2.f - vptb.position.x), std::round(fltY + (34.f - vptb.size.y) / 2.f - vptb.position.y)});
        win.draw(vpTxt);
        vPX += vPills[i].second + 8.f;
    }

    // Bảng Voucher
    float tableX = L.tableX;
    float tableY = L.tableY;
    float rowH = L.rowH;
    float tblRight = L.tblRight;

    const float div1 = L.div1;
    const float div2 = L.div2;
    const float div3 = L.div3;
    const float div4 = L.div4;
    const float div5 = L.div5;

    auto drawCenteredHdr = [&](const wstring& str, float xStart, float xEnd) {
        Text h(boldFont, str, 14);
        h.setFillColor(Color::White);
        FloatRect hb = h.getLocalBounds();
        h.setPosition({std::round(xStart + (xEnd - xStart - hb.size.x) / 2.f - hb.position.x), std::round(tableY + (L.hdrH - hb.size.y) / 2.f - hb.position.y)});
        win.draw(h);
    };

    drawRoundedCard(win, {tableX, tableY}, {leftW, L.hdrH}, 6.f, Color(8, 125, 181));

    for (float dx : {div1, div2, div3, div4, div5}) {
        RectangleShape hDiv(Vector2f(1.f, L.hdrH));
        hDiv.setPosition({dx, tableY});
        hDiv.setFillColor(Color(255, 255, 255, 45));
        win.draw(hDiv);
    }

    Text vh1(boldFont, L"M\u00c3 VOUCHER", 14);
    vh1.setFillColor(Color::White);
    FloatRect vh1b = vh1.getLocalBounds();
    vh1.setPosition({tableX + 42.f, std::round(tableY + (L.hdrH - vh1b.size.y) / 2.f - vh1b.position.y)});
    win.draw(vh1);
    drawCenteredHdr(L"LO\u1ea0I", div1, div2);
    drawCenteredHdr(L"GI\u00c1 TR\u1eca", div2, div3);
    drawCenteredHdr(L"T\u1ed0I THI\u1ec2U", div3, div4);
    drawCenteredHdr(L"NG\u01af\u1edcI NH\u1eacN", div4, div5);
    drawCenteredHdr(L"THAO T\u00c1C", div5, tblRight);

    int startIdx = (currentVoucherPage - 1) * pageSize;
    int count = min((int)filteredVouchers.size() - startIdx, pageSize);

    for (int r = 0; r < count; ++r) {
        int idx = startIdx + r;
        const auto& v = filteredVouchers[idx];
        float rY = tableY + L.hdrH + r * rowH;

        bool isSel = (selectedVoucherIndex == idx);
        bool isHov = (hoveredVoucherRow == r);

        Color rowBg = isSel ? Color(224, 242, 254) : (isHov ? Color(248, 250, 252) : Color::White);
        drawRoundedCard(win, {tableX, rY}, {leftW, rowH}, 0.f, rowBg, 0.5f, Color(226, 232, 240));

        for (float dx : {div1, div2, div3, div4, div5}) {
            RectangleShape rDiv(Vector2f(1.f, rowH));
            rDiv.setPosition({dx, rY});
            rDiv.setFillColor(Color(241, 245, 249));
            win.draw(rDiv);
        }

        // Checkbox với dấu check vector (không phụ thuộc font)
        float cbX = tableX + 14.f;
        float cbY = rY + (rowH - 16.f) / 2.f;
        drawRoundedCard(win, {cbX, cbY}, {16.f, 16.f}, 3.f, isSel ? Color(8, 125, 181) : Color::White, 1.f, isSel ? Color(8, 125, 181) : Color(203, 213, 225));
        if (isSel) {
            RectangleShape c1({4.f, 1.8f});
            c1.setFillColor(Color::White);
            c1.setRotation(degrees(45.f));
            c1.setPosition({cbX + 4.f, cbY + 7.5f});
            win.draw(c1);

            RectangleShape c2({8.f, 1.8f});
            c2.setFillColor(Color::White);
            c2.setRotation(degrees(-55.f));
            c2.setPosition({cbX + 6.8f, cbY + 10.3f});
            win.draw(c2);
        }

        // Mã Voucher & Tag & Subtitle
        float codeX = tableX + 42.f;
        float maxCodeColW = div1 - codeX - 12.f;
        Text codeTxt(boldFont, s2ws(v.code), 15);
        codeTxt.setFillColor(Color(23, 32, 51));
        codeTxt.setPosition({codeX, std::round(rY + 12.f)});
        win.draw(codeTxt);

        if (!v.tag.empty()) {
            FloatRect cb = codeTxt.getGlobalBounds();
            Text tgTxt(boldFont, v.tag, 13);
            FloatRect tgb = tgTxt.getLocalBounds();
            float tagW = tgb.size.x + 14.f;
            float tagX = cb.position.x + cb.size.x + 8.f;
            if (tagX + tagW <= div1 - 8.f) {
                Color tagBg = Color(220, 252, 231);
                Color tagFg = Color(22, 101, 52);
                if (v.tag == L"VIP") { tagBg = Color(254, 243, 199); tagFg = Color(180, 83, 9); }
                else if (v.tag == L"HSSV") { tagBg = Color(224, 242, 254); tagFg = Color(8, 125, 181); }
                drawRoundedCard(win, {tagX, rY + 12.f}, {tagW, 20.f}, 4.f, tagBg);
                tgTxt.setFillColor(tagFg);
                tgTxt.setPosition({std::round(tagX + (tagW - tgb.size.x) / 2.f - tgb.position.x), std::round(rY + 12.f + (20.f - tgb.size.y) / 2.f - tgb.position.y)});
                win.draw(tgTxt);
            }
        }

        // Subtitle cắt gọn gàng trong [codeX, div1 - 12]
        wstring sub = ellipsizeText(font, v.subtitle, 14, maxCodeColW);
        Text subTxt(font, sub, 14);
        subTxt.setFillColor(Color(100, 116, 139));
        subTxt.setPosition({codeX, std::round(rY + 38.f)});
        win.draw(subTxt);

        // Loại Voucher (Pill gọn: % hoặc Tiền)
        wstring typeLabel = (v.type == 2) ? L"%" : L"Ti\u1ec1n";
        Color typeBg = (v.type == 2) ? Color(253, 242, 248) : Color(238, 242, 255);
        Color typeFg = (v.type == 2) ? Color(219, 39, 119) : Color(79, 70, 229);
        float badgeW = min(46.f, (div2 - div1) - 10.f);
        float badgeH = 24.f;
        float badgeX = div1 + ((div2 - div1) - badgeW) / 2.f;
        float badgeY = rY + (rowH - badgeH) / 2.f;
        drawRoundedCard(win, {badgeX, badgeY}, {badgeW, badgeH}, 12.f, typeBg);
        Text tpTxt(boldFont, typeLabel, 13);
        tpTxt.setFillColor(typeFg);
        FloatRect tpb = tpTxt.getLocalBounds();
        tpTxt.setPosition({std::round(badgeX + (badgeW - tpb.size.x) / 2.f - tpb.position.x), std::round(badgeY + (badgeH - tpb.size.y) / 2.f - tpb.position.y)});
        win.draw(tpTxt);

        // Giá trị (căn giữa)
        wstring valStr = (v.type == 2) ? (to_wstring(v.value) + L"%") : s2ws(formatPrice(v.value));
        Text valTxt(boldFont, valStr, 14); valTxt.setFillColor(Color(21, 128, 61));
        FloatRect vtb = valTxt.getLocalBounds();
        valTxt.setPosition({std::round(div2 + ((div3 - div2) - vtb.size.x) / 2.f - vtb.position.x), std::round(rY + (rowH - vtb.size.y) / 2.f - vtb.position.y)});
        win.draw(valTxt);

        // Đơn tối thiểu (căn giữa)
        Text minTxt(font, s2ws(formatPrice(v.minBill)), 14); minTxt.setFillColor(Color(71, 85, 105));
        FloatRect mtb = minTxt.getLocalBounds();
        minTxt.setPosition({std::round(div3 + ((div4 - div3) - mtb.size.x) / 2.f - mtb.position.x), std::round(rY + (rowH - mtb.size.y) / 2.f - mtb.position.y)});
        win.draw(minTxt);

        // Người nhận (Badge số lượng căn giữa)
        float rcPillW = min(44.f, (div5 - div4) - 10.f);
        float rcPillH = 24.f;
        float rcPillX = div4 + ((div5 - div4) - rcPillW) / 2.f;
        float rcPillY = rY + (rowH - rcPillH) / 2.f;
        Color rcBg = (v.recipientCount > 0) ? Color(224, 242, 254) : Color(241, 245, 249);
        Color rcFg = (v.recipientCount > 0) ? Color(8, 125, 181) : Color(148, 163, 184);
        drawRoundedCard(win, {rcPillX, rcPillY}, {rcPillW, rcPillH}, 12.f, rcBg);
        Text rcTxt(boldFont, to_wstring(v.recipientCount), 14); rcTxt.setFillColor(rcFg);
        FloatRect rcb = rcTxt.getLocalBounds();
        rcTxt.setPosition({std::round(rcPillX + (rcPillW - rcb.size.x) / 2.f - rcb.position.x), std::round(rcPillY + (rcPillH - rcb.size.y) / 2.f - rcb.position.y)});
        win.draw(rcTxt);

        // Thao tác - Nút Sửa căn giữa trong [div5, tblRight]
        float editBtnW = min(56.f, (tblRight - div5) - 10.f);
        float editBtnH = 28.f;
        float editBtnX = div5 + ((tblRight - div5) - editBtnW) / 2.f;
        float editBtnY = rY + (rowH - editBtnH) / 2.f;
        bool isEditHov = (hoveredEditBtnRow == r);
        drawRoundedCard(win, {editBtnX, editBtnY}, {editBtnW, editBtnH}, 5.f, isEditHov ? Color(185, 28, 28) : Color(220, 38, 38));
        Text icEd(boldFont, L"S\u1eeda", 13);
        icEd.setFillColor(Color::White);
        FloatRect edb = icEd.getLocalBounds();
        icEd.setPosition({std::round(editBtnX + (editBtnW - edb.size.x) / 2.f - edb.position.x), std::round(editBtnY + (editBtnH - edb.size.y) / 2.f - edb.position.y)});
        win.draw(icEd);
    }

    // Phân trang Cột Trái
    float pagY = L.pagY;
    float tableRight = L.tblRight;

    int fromIdx = filteredVouchers.empty() ? 0 : (startIdx + 1);
    int toIdx = min(startIdx + count, (int)filteredVouchers.size());
    wstring pagStr = filteredVouchers.empty() 
        ? L"Kh\u00f4ng c\u00f3 voucher n\u00e0o"
        : (L"Hi\u1ec3n th\u1ecb " + to_wstring(fromIdx) + L" \u2013 " + to_wstring(toIdx) + L" tr\u00ean " + to_wstring(filteredVouchers.size()) + L" voucher");
    Text lPag(font, pagStr, 14);
    lPag.setFillColor(Color(100, 116, 139));
    lPag.setPosition({position.x + 30.f, pagY + 4.f});
    win.draw(lPag);

    // Nút Trước (<)
    bool canPrev = (currentVoucherPage > 1);
    drawRoundedCard(win, {tableRight - 124.f, pagY}, {28.f, 28.f}, 4.f, Color::White, 1.f, Color(226, 232, 240));
    Text pgPrev(boldFont, L"<", 14);
    pgPrev.setFillColor(canPrev ? Color(100, 116, 139) : Color(203, 213, 225));
    FloatRect prb = pgPrev.getLocalBounds();
    pgPrev.setPosition({tableRight - 124.f + (28.f - prb.size.x) / 2.f - prb.position.x, pagY + (28.f - prb.size.y) / 2.f - prb.position.y});
    win.draw(pgPrev);

    // Nút Trang 1
    bool p1Act = (currentVoucherPage == 1);
    drawRoundedCard(win, {tableRight - 92.f, pagY}, {28.f, 28.f}, 4.f, p1Act ? Color(8, 125, 181) : Color::White, 1.f, p1Act ? Color(8, 125, 181) : Color(226, 232, 240));
    Text pg1(boldFont, L"1", 14);
    pg1.setFillColor(p1Act ? Color::White : Color(71, 85, 105));
    FloatRect p1b = pg1.getLocalBounds();
    pg1.setPosition({tableRight - 92.f + (28.f - p1b.size.x) / 2.f - p1b.position.x, pagY + (28.f - p1b.size.y) / 2.f - p1b.position.y});
    win.draw(pg1);

    // Nút Trang 2
    bool p2Act = (currentVoucherPage == 2);
    drawRoundedCard(win, {tableRight - 60.f, pagY}, {28.f, 28.f}, 4.f, p2Act ? Color(8, 125, 181) : Color::White, 1.f, p2Act ? Color(8, 125, 181) : Color(226, 232, 240));
    Text pg2(boldFont, L"2", 14);
    pg2.setFillColor(p2Act ? Color::White : Color(71, 85, 105));
    FloatRect p2b = pg2.getLocalBounds();
    pg2.setPosition({tableRight - 60.f + (28.f - p2b.size.x) / 2.f - p2b.position.x, pagY + (28.f - p2b.size.y) / 2.f - p2b.position.y});
    win.draw(pg2);

    // Nút Sau (>)
    bool canNext = (currentVoucherPage < totalVoucherPages);
    drawRoundedCard(win, {tableRight - 28.f, pagY}, {28.f, 28.f}, 4.f, Color::White, 1.f, Color(226, 232, 240));
    Text pgNext(boldFont, L">", 14);
    pgNext.setFillColor(canNext ? Color(100, 116, 139) : Color(203, 213, 225));
    FloatRect nxb = pgNext.getLocalBounds();
    pgNext.setPosition({tableRight - 28.f + (28.f - nxb.size.x) / 2.f - nxb.position.x, pagY + (28.f - nxb.size.y) / 2.f - nxb.position.y});
    win.draw(pgNext);

    // ==========================================
    // 6. CỘT PHẢI: CHI TIẾT NGƯỜI NHẬN (DETAIL VIEW)
    // ==========================================
    float rightX = L.rightX;
    float rightW = L.rightW;

    drawRoundedCard(win, {rightX, L.mainTopY}, {rightW, L.rightCardH}, 12.f, Color::White, 1.f, Color(226, 232, 240));

    // Header Cột Phải
    Text rHead(boldFont, L"Chi ti\u1ebft ng\u01b0\u1eddi nh\u1eadn", 16);
    rHead.setFillColor(Color(23, 32, 51));
    rHead.setPosition({rightX + 16.f, std::round(L.mainTopY + 14.f)});
    win.draw(rHead);

    FloatRect rhb = rHead.getGlobalBounds();
    Text vcBadge(boldFont, s2ws(selCode), 14);
    FloatRect vcb = vcBadge.getLocalBounds();
    float badgeBoxW = max(72.f, vcb.size.x + 20.f);
    float badgeBoxX = rhb.position.x + rhb.size.x + 12.f;
    if (badgeBoxX + badgeBoxW > rightX + rightW - 16.f) {
        badgeBoxW = max(48.f, (rightX + rightW - 16.f) - badgeBoxX);
    }
    drawRoundedCard(win, {badgeBoxX, L.mainTopY + 13.f}, {badgeBoxW, 24.f}, 6.f, Color(224, 242, 254));
    vcBadge.setFillColor(Color(8, 125, 181));
    vcBadge.setPosition({std::round(badgeBoxX + (badgeBoxW - vcb.size.x) / 2.f - vcb.position.x), std::round(L.mainTopY + 13.f + (24.f - vcb.size.y) / 2.f - vcb.position.y)});
    win.draw(vcBadge);

    wstring rSubStr = L"";
    if (selectedVoucherIndex >= 0 && selectedVoucherIndex < (int)filteredVouchers.size()) {
        const auto& curV = filteredVouchers[selectedVoucherIndex];
        if (curV.type == 2) {
            rSubStr = L"Gi\u1ea3m " + to_wstring(curV.value) + L"% \u0111\u01a1n h\u00e0ng";
        } else {
            rSubStr = L"Gi\u1ea3m " + s2ws(formatPrice(curV.value)) + L" \u0111\u01a1n h\u00e0ng";
        }
        rSubStr += L" \u2022 \u0110\u01a1n t\u1ed1i thi\u1ec3u: " + s2ws(formatPrice(curV.minBill));
    } else {
        rSubStr = L"C\u1ea5u h\u00ecnh ng\u01b0\u1eddi nh\u1eadn voucher";
    }
    rSubStr = ellipsizeText(font, rSubStr, 14, rightW - 32.f);
    Text rSub(font, rSubStr, 14);
    rSub.setFillColor(Color(100, 116, 139));
    rSub.setPosition({rightX + 16.f, std::round(L.mainTopY + 42.f)});
    win.draw(rSub);

    // Hàng nút Cột Phải: Quay lại, + Thêm người, Thu hồi
    float rActY = L.rActY;
    auto drawRightActionBtn = [&](float bx, float bw, const wstring& lbl, Color bgCol, Color fgCol, float outW = 0.f, Color outCol = Color::Transparent) {
        drawRoundedCard(win, {bx, rActY}, {bw, 34.f}, 6.f, bgCol, outW, outCol);
        Text t(boldFont, lbl, 14);
        t.setFillColor(fgCol);
        FloatRect tb = t.getLocalBounds();
        t.setPosition({std::round(bx + (bw - tb.size.x) / 2.f - tb.position.x), std::round(rActY + (34.f - tb.size.y) / 2.f - tb.position.y)});
        win.draw(t);
    };

    drawRightActionBtn(rightX + 16.f, 94.f, L"\u2190 Quay l\u1ea1i",
                       hoveredBackBtn ? Color(241, 245, 249) : Color::White,
                       hoveredBackBtn ? Color(8, 125, 181) : Color(71, 85, 105),
                       1.f, hoveredBackBtn ? Color(8, 125, 181) : Color(203, 213, 225));

    drawRightActionBtn(rightX + 120.f, 116.f, L"+ Th\u00eam ng\u01b0\u1eddi",
                       hoveredAddUserBtn ? Color(4, 120, 87) : Color(21, 128, 61),
                       Color::White);

    bool hasSel = (selectedRecipientIndex >= 0);
    Color revBg = hasSel
        ? (hoveredRevokeBtn ? Color(185, 28, 28) : Color(220, 38, 38))
        : (hoveredRevokeBtn ? Color(254, 226, 226) : Color(254, 242, 242));
    Color revFg = hasSel ? Color::White : Color(220, 38, 38);
    float revOut = hasSel ? 0.f : 1.f;
    Color revBorder = hasSel ? Color::Transparent : Color(254, 202, 202);
    drawRightActionBtn(rightX + 246.f, 88.f, L"Thu h\u1ed3i", revBg, revFg, revOut, revBorder);

    // Tìm kiếm người nhận
    float rFltY = L.rFltY;
    drawRoundedCard(win, {rightX + 16.f, rFltY}, {L.rSearchW, 34.f}, 6.f, Color::White, searchRecipientActive ? 1.8f : 1.f, searchRecipientActive ? Color(8, 125, 181) : Color(203, 213, 225));
    wstring sRPrompt = searchRecipientQuery.empty()
        ? (searchRecipientActive ? L"|" : L"T\u00ecm theo Email ho\u1eb7c T\u00ean kh\u00e1ch h\u00e0ng...")
        : (searchRecipientQuery + (searchRecipientActive ? L"|" : L""));
    sRPrompt = ellipsizeText(font, sRPrompt, 14, L.rSearchW - 18.f);
    Text sRTxt(font, sRPrompt, 14);
    sRTxt.setFillColor(searchRecipientQuery.empty() && !searchRecipientActive ? Color(148, 163, 184) : Color(23, 32, 51));
    sRTxt.setPosition({rightX + 26.f, std::round(rFltY + 8.f)});
    win.draw(sRTxt);

    float rCountX = rightX + rightW - 16.f - L.rCountW;
    drawRoundedCard(win, {rCountX, rFltY}, {L.rCountW, 34.f}, 6.f, Color(241, 245, 249));
    wstring totAccStr = to_wstring(filteredRecipients.size()) + L" t\u00e0i kho\u1ea3n";
    Text uTotTxt(boldFont, totAccStr, 14); uTotTxt.setFillColor(Color(21, 128, 61));
    FloatRect utb = uTotTxt.getLocalBounds();
    uTotTxt.setPosition({std::round(rCountX + (L.rCountW - utb.size.x) / 2.f - utb.position.x), std::round(rFltY + (34.f - utb.size.y) / 2.f - utb.position.y)});
    win.draw(uTotTxt);

    // Header Bảng Người Nhận (43% / 27% / 10% / 20%)
    float rTblY = L.rTblY;
    drawRoundedCard(win, {L.rTblX, rTblY}, {L.rTblW, L.rHdrH}, 6.f, Color(248, 250, 252), 1.f, Color(226, 232, 240));

    auto drawRecipHdr = [&](const wstring& lbl, float xPos) {
        Text h(boldFont, lbl, 14);
        h.setFillColor(Color(71, 85, 105));
        FloatRect hb = h.getLocalBounds();
        h.setPosition({std::round(xPos), std::round(rTblY + (L.rHdrH - hb.size.y) / 2.f - hb.position.y)});
        win.draw(h);
    };
    auto drawRecipCenterHdr = [&](const wstring& lbl, float xStart, float xEnd) {
        Text h(boldFont, lbl, 14);
        h.setFillColor(Color(71, 85, 105));
        FloatRect hb = h.getLocalBounds();
        h.setPosition({std::round(xStart + (xEnd - xStart - hb.size.x) / 2.f - hb.position.x), std::round(rTblY + (L.rHdrH - hb.size.y) / 2.f - hb.position.y)});
        win.draw(h);
    };

    drawRecipHdr(L"EMAIL KH\u00c1CH H\u00c0NG", L.rTblX + 10.f);
    drawRecipHdr(L"H\u1ecc V\u00c0 T\u00caN", L.rDiv1 + 10.f);
    drawRecipCenterHdr(L"SL", L.rDiv2, L.rDiv3);
    drawRecipCenterHdr(L"TR\u1ea0NG TH\u00c1I", L.rDiv3, L.rTblRight);

    // Dòng dữ liệu Người Nhận
    float rRowH = L.rRowH;
    int rStartIdx = (currentRecipientPage - 1) * recipientPageSize;
    int rCount = max(0, min((int)filteredRecipients.size() - rStartIdx, recipientPageSize));

    for (int r = 0; r < rCount; ++r) {
        int actualIdx = rStartIdx + r;
        float curY = rTblY + L.rHdrH + r * rRowH;
        const auto& itm = filteredRecipients[actualIdx];

        bool isSel = (selectedRecipientIndex == actualIdx);
        bool isHov = (hoveredRecipientRow == actualIdx);
        if (isSel) {
            drawRoundedCard(win, {L.rTblX, curY + 2.f}, {L.rTblW, rRowH - 4.f}, 6.f, Color(254, 242, 242), 1.2f, Color(220, 38, 38));
        } else if (isHov) {
            drawRoundedCard(win, {L.rTblX, curY + 2.f}, {L.rTblW, rRowH - 4.f}, 6.f, Color(248, 250, 252));
        } else {
            RectangleShape sep({L.rTblW, 1.f});
            sep.setFillColor(Color(241, 245, 249));
            sep.setPosition({L.rTblX, curY + rRowH - 1.f});
            win.draw(sep);
        }

        // Email & ngày nhận
        float emailMaxW = (L.rDiv1 - L.rTblX) - 20.f;
        wstring emailStr = ellipsizeText(boldFont, s2ws(itm.email), 14, emailMaxW);
        Text emTxt(boldFont, emailStr, 14);
        emTxt.setFillColor(isSel ? Color(185, 28, 28) : Color(23, 32, 51));
        emTxt.setPosition({L.rTblX + 10.f, std::round(curY + 10.f)});
        win.draw(emTxt);

        wstring recDtStr = ellipsizeText(font, L"\u0110\u00e3 nh\u1eadn: " + s2ws(itm.receivedDate), 13, emailMaxW);
        Text recDt(font, recDtStr, 13);
        recDt.setFillColor(Color(100, 116, 139));
        recDt.setPosition({L.rTblX + 10.f, std::round(curY + 32.f)});
        win.draw(recDt);

        // Họ tên
        float nameMaxW = (L.rDiv2 - L.rDiv1) - 20.f;
        wstring nameStr = ellipsizeText(font, itm.fullName, 14, nameMaxW);
        Text nmTxt(font, nameStr, 14);
        nmTxt.setFillColor(isSel ? Color(185, 28, 28) : Color(51, 65, 85));
        FloatRect nmb = nmTxt.getLocalBounds();
        nmTxt.setPosition({L.rDiv1 + 10.f, std::round(curY + (rRowH - nmb.size.y) / 2.f - nmb.position.y)});
        win.draw(nmTxt);

        // Số lượng (căn giữa trong [rDiv2, rDiv3])
        Text qTxt(boldFont, to_wstring(itm.quantity), 14);
        qTxt.setFillColor(isSel ? Color(185, 28, 28) : Color(23, 32, 51));
        FloatRect qb = qTxt.getLocalBounds();
        qTxt.setPosition({std::round(L.rDiv2 + ((L.rDiv3 - L.rDiv2) - qb.size.x) / 2.f - qb.position.x), std::round(curY + (rRowH - qb.size.y) / 2.f - qb.position.y)});
        win.draw(qTxt);

        // Trạng thái: Đã dùng hoặc Chưa dùng (căn giữa trong [rDiv3, rTblRight])
        Color bStBg = itm.isUsed ? Color(220, 252, 231) : Color(224, 242, 254);
        Color bStFg = itm.isUsed ? Color(22, 101, 52) : Color(8, 125, 181);
        wstring stLabel = itm.isUsed ? L"\u2022 \u0110\u00e3 d\u00f9ng" : L"\u2022 Ch\u01b0a d\u00f9ng";
        float stPillW = min(98.f, (L.rTblRight - L.rDiv3) - 8.f);
        float stPillH = 26.f;
        float stPillX = L.rDiv3 + ((L.rTblRight - L.rDiv3) - stPillW) / 2.f;
        float stPillY = curY + (rRowH - stPillH) / 2.f;
        drawRoundedCard(win, {stPillX, stPillY}, {stPillW, stPillH}, 13.f, bStBg);
        Text stTxt(boldFont, stLabel, 13);
        stTxt.setFillColor(bStFg);
        FloatRect stb = stTxt.getLocalBounds();
        stTxt.setPosition({std::round(stPillX + (stPillW - stb.size.x) / 2.f - stb.position.x), std::round(stPillY + (stPillH - stb.size.y) / 2.f - stb.position.y)});
        win.draw(stTxt);
    }

    // Footer Cột Phải
    float rFootY = L.rFootY;
    int usedCnt = 0;
    for (const auto& item : currentRecipients) {
        if (item.isUsed) usedCnt++;
    }
    int totalRec = (int)currentRecipients.size();
    float pct = totalRec > 0 ? ((float)usedCnt * 100.f / totalRec) : 0.f;
    stringstream ssPct;
    ssPct << fixed << setprecision(1) << pct;
    wstring usageStr = L"\u0110\u00e3 s\u1eed d\u1ee5ng: " + to_wstring(usedCnt) + L"/" + to_wstring(totalRec) + L" (" + s2ws(ssPct.str()) + L"%)";
    Text rUsage(font, usageStr, 14);
    rUsage.setFillColor(Color(100, 116, 139));
    FloatRect rub = rUsage.getLocalBounds();
    rUsage.setPosition({rightX + 16.f, std::round(rFootY + (28.f - rub.size.y) / 2.f - rub.position.y)});
    win.draw(rUsage);

    float rPagRight = L.rPagRight;
    if (totalRecipientPages > 1) {
        bool rCanPrev = (currentRecipientPage > 1);
        drawRoundedCard(win, {rPagRight - 88.f, rFootY}, {28.f, 28.f}, 4.f, Color::White, 1.f, Color(226, 232, 240));
        Text rPgPrev(boldFont, L"<", 14);
        rPgPrev.setFillColor(rCanPrev ? Color(100, 116, 139) : Color(203, 213, 225));
        FloatRect rpb = rPgPrev.getLocalBounds();
        rPgPrev.setPosition({std::round(rPagRight - 88.f + (28.f - rpb.size.x) / 2.f - rpb.position.x), std::round(rFootY + (28.f - rpb.size.y) / 2.f - rpb.position.y)});
        win.draw(rPgPrev);

        wstring pStr = to_wstring(currentRecipientPage) + L"/" + to_wstring(totalRecipientPages);
        Text rPgTxt(boldFont, pStr, 14);
        rPgTxt.setFillColor(Color(71, 85, 105));
        FloatRect rptb = rPgTxt.getLocalBounds();
        rPgTxt.setPosition({std::round(rPagRight - 58.f + (28.f - rptb.size.x) / 2.f - rptb.position.x), std::round(rFootY + (28.f - rptb.size.y) / 2.f - rptb.position.y)});
        win.draw(rPgTxt);

        bool rCanNext = (currentRecipientPage < totalRecipientPages);
        drawRoundedCard(win, {rPagRight - 28.f, rFootY}, {28.f, 28.f}, 4.f, Color::White, 1.f, Color(226, 232, 240));
        Text rPgNext(boldFont, L">", 14);
        rPgNext.setFillColor(rCanNext ? Color(100, 116, 139) : Color(203, 213, 225));
        FloatRect rnb = rPgNext.getLocalBounds();
        rPgNext.setPosition({std::round(rPagRight - 28.f + (28.f - rnb.size.x) / 2.f - rnb.position.x), std::round(rFootY + (28.f - rnb.size.y) / 2.f - rnb.position.y)});
        win.draw(rPgNext);
    } else {
        Text rAllLnk(boldFont, L"Hi\u1ec3n th\u1ecb " + to_wstring(filteredRecipients.size()) + L" ng\u01b0\u1eddi nh\u1eadn", 14);
        rAllLnk.setFillColor(Color(8, 125, 181));
        FloatRect alb = rAllLnk.getLocalBounds();
        rAllLnk.setPosition({std::round(rPagRight - alb.size.x - alb.position.x), std::round(rFootY + (28.f - alb.size.y) / 2.f - alb.position.y)});
        win.draw(rAllLnk);
    }

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

        Text pTitle(boldFont, (editingIndex >= 0) ? L"S\u1eeda th\u00f4ng tin voucher" : L"Th\u00eam voucher m\u1edbi", 24);
        pTitle.setFillColor(Color(23, 32, 51));
        pTitle.setPosition({popX + 30.f, popY + 22.f});
        win.draw(pTitle);

        bool f0Act = (popupActiveField == 0);
        Text l1(boldFont, L"M\u00e3 voucher:", 14); l1.setFillColor(Color(71, 85, 105)); l1.setPosition({popX + 30.f, popY + 65.f}); win.draw(l1);
        drawRoundedCard(win, {popX + 30.f, popY + 85.f}, {210.f, 36.f}, 6.f,
                        f0Act ? Color::White : Color(248, 250, 252), f0Act ? 1.8f : 1.f, f0Act ? Color(8, 125, 181) : Color(203, 213, 225));
        Text v1(font, s2ws(popupCode) + (f0Act ? L"|" : L""), 14); v1.setFillColor(Color(23, 32, 51)); v1.setPosition({popX + 40.f, popY + 93.f}); win.draw(v1);

        bool f1Act = (popupActiveField == 1);
        Text l2(boldFont, L"Lo\u1ea1i voucher (Click \u0111\u1ec3 \u0111\u1ed5i):", 14); l2.setFillColor(Color(71, 85, 105)); l2.setPosition({popX + 260.f, popY + 65.f}); win.draw(l2);
        drawRoundedCard(win, {popX + 260.f, popY + 85.f}, {210.f, 36.f}, 6.f,
                        f1Act ? Color::White : Color(248, 250, 252), f1Act ? 1.8f : 1.f, f1Act ? Color(8, 125, 181) : Color(203, 213, 225));
        Text v2(font, to_wstring(popupType) + (popupType == 2 ? L" (Gi\u1ea3m %) \u25bc" : L" (Gi\u1ea3m ti\u1ec1n \u0111) \u25bc"), 14); v2.setFillColor(Color(8, 125, 181)); v2.setPosition({popX + 270.f, popY + 93.f}); win.draw(v2);

        bool f2Act = (popupActiveField == 2);
        Text l3(boldFont, L"Chi ti\u1ebft / M\u00f4 t\u1ea3:", 14); l3.setFillColor(Color(71, 85, 105)); l3.setPosition({popX + 30.f, popY + 130.f}); win.draw(l3);
        drawRoundedCard(win, {popX + 30.f, popY + 150.f}, {popW - 60.f, 36.f}, 6.f,
                        f2Act ? Color::White : Color(248, 250, 252), f2Act ? 1.8f : 1.f, f2Act ? Color(8, 125, 181) : Color(203, 213, 225));
        Text v3(font, popupSub + (f2Act ? L"|" : L""), 14); v3.setFillColor(Color(23, 32, 51)); v3.setPosition({popX + 40.f, popY + 158.f}); win.draw(v3);

        bool f3Act = (popupActiveField == 3);
        Text l4(boldFont, L"Gi\u00e1 tr\u1ecb \u01b0u \u0111\u00e3i (% ho\u1eb7c \u0111):", 14); l4.setFillColor(Color(71, 85, 105)); l4.setPosition({popX + 30.f, popY + 195.f}); win.draw(l4);
        drawRoundedCard(win, {popX + 30.f, popY + 215.f}, {210.f, 36.f}, 6.f,
                        f3Act ? Color::White : Color(248, 250, 252), f3Act ? 1.8f : 1.f, f3Act ? Color(8, 125, 181) : Color(203, 213, 225));
        Text v4(font, s2ws(popupValue) + (f3Act ? L"|" : L""), 14); v4.setFillColor(Color(23, 32, 51)); v4.setPosition({popX + 40.f, popY + 223.f}); win.draw(v4);

        bool f4Act = (popupActiveField == 4);
        Text l5(boldFont, L"\u0110\u01a1n t\u1ed1i thi\u1ec3u (\u0111):", 14); l5.setFillColor(Color(71, 85, 105)); l5.setPosition({popX + 260.f, popY + 195.f}); win.draw(l5);
        drawRoundedCard(win, {popX + 260.f, popY + 215.f}, {210.f, 36.f}, 6.f,
                        f4Act ? Color::White : Color(248, 250, 252), f4Act ? 1.8f : 1.f, f4Act ? Color(8, 125, 181) : Color(203, 213, 225));
        Text v5(font, s2ws(popupMinBill) + (f4Act ? L"|" : L""), 14); v5.setFillColor(Color(23, 32, 51)); v5.setPosition({popX + 270.f, popY + 223.f}); win.draw(v5);

        // Buttons
        drawRoundedCard(win, {popX + 70.f, popY + 270.f}, {160.f, 38.f}, 6.f, Color(21, 128, 61));
        Text sTxt(boldFont, L"L\u01b0u th\u00f4ng tin", 14); sTxt.setFillColor(Color::White); sTxt.setPosition({popX + 105.f, popY + 279.f}); win.draw(sTxt);

        drawRoundedCard(win, {popX + 270.f, popY + 270.f}, {160.f, 38.f}, 6.f, Color(241, 245, 249), 1.f, Color(203, 213, 225));
        Text cTxt(boldFont, L"H\u1ee7y b\u1ecf", 14); cTxt.setFillColor(Color(71, 85, 105)); cTxt.setPosition({popX + 325.f, popY + 279.f}); win.draw(cTxt);
    }

    // ==========================================
    // 8b. POPUP THÊM NGƯỜI NHẬN VOUCHER
    // ==========================================
    if (showAddRecipientPopup) {
        RectangleShape overlay({width, height});
        overlay.setPosition(position);
        overlay.setFillColor(Color(15, 23, 42, 160));
        win.draw(overlay);

        float popW = 480.f;
        float popH = 340.f;
        float popX = position.x + (width - popW) / 2.f;
        float popY = position.y + (height - popH) / 2.f - 20.f;

        drawRoundedCard(win, {popX, popY}, {popW, popH}, 12.f, Color::White);

        Text pTitle(boldFont, L"Th\u00eam ng\u01b0\u1eddi nh\u1eadn voucher", 24);
        pTitle.setFillColor(Color(23, 32, 51));
        pTitle.setPosition({popX + 30.f, popY + 22.f});
        win.draw(pTitle);

        // Field 0: Email
        bool f0Act = (recipientPopupActiveField == 0);
        Text l1(boldFont, L"Email kh\u00e1ch h\u00e0ng:", 14); l1.setFillColor(Color(71, 85, 105)); l1.setPosition({popX + 30.f, popY + 65.f}); win.draw(l1);
        drawRoundedCard(win, {popX + 30.f, popY + 85.f}, {popW - 60.f, 36.f}, 6.f,
                        f0Act ? Color::White : Color(248, 250, 252), f0Act ? 1.8f : 1.f, f0Act ? Color(8, 125, 181) : Color(203, 213, 225));
        wstring dispEmail = popupRecipientEmail.empty() ? (f0Act ? L"|" : L"vd: nguyen.van.a@gmail.com") : (s2ws(popupRecipientEmail) + (f0Act ? L"|" : L""));
        Text v1(font, dispEmail, 14);
        v1.setFillColor(popupRecipientEmail.empty() && !f0Act ? Color(148, 163, 184) : Color(23, 32, 51));
        v1.setPosition({popX + 40.f, popY + 93.f});
        win.draw(v1);

        // Field 1: Họ và tên
        bool f1Act = (recipientPopupActiveField == 1);
        Text l2(boldFont, L"H\u1ecc v\u00e0 t\u00ean kh\u00e1ch h\u00e0ng:", 14); l2.setFillColor(Color(71, 85, 105)); l2.setPosition({popX + 30.f, popY + 130.f}); win.draw(l2);
        drawRoundedCard(win, {popX + 30.f, popY + 150.f}, {popW - 60.f, 36.f}, 6.f,
                        f1Act ? Color::White : Color(248, 250, 252), f1Act ? 1.8f : 1.f, f1Act ? Color(8, 125, 181) : Color(203, 213, 225));
        wstring dispName = popupRecipientName.empty() ? (f1Act ? L"|" : L"vd: Nguy\u1ec5n V\u0103n A") : (popupRecipientName + (f1Act ? L"|" : L""));
        Text v2(font, dispName, 14);
        v2.setFillColor(popupRecipientName.empty() && !f1Act ? Color(148, 163, 184) : Color(23, 32, 51));
        v2.setPosition({popX + 40.f, popY + 158.f});
        win.draw(v2);

        // Field 2: Số lượng voucher
        bool f2Act = (recipientPopupActiveField == 2);
        Text l3(boldFont, L"S\u1ed1 l\u01b0\u1ee3ng voucher c\u1ea5p:", 14); l3.setFillColor(Color(71, 85, 105)); l3.setPosition({popX + 30.f, popY + 195.f}); win.draw(l3);
        drawRoundedCard(win, {popX + 30.f, popY + 215.f}, {200.f, 36.f}, 6.f,
                        f2Act ? Color::White : Color(248, 250, 252), f2Act ? 1.8f : 1.f, f2Act ? Color(8, 125, 181) : Color(203, 213, 225));
        Text v3(font, s2ws(popupRecipientQty) + (f2Act ? L"|" : L""), 14);
        v3.setFillColor(Color(23, 32, 51));
        v3.setPosition({popX + 40.f, popY + 223.f});
        win.draw(v3);

        // Buttons
        drawRoundedCard(win, {popX + 50.f, popY + 272.f}, {170.f, 38.f}, 6.f, Color(21, 128, 61));
        Text sTxt(boldFont, L"Th\u00eam ng\u01b0\u1eddi nh\u1eadn", 14); sTxt.setFillColor(Color::White);
        FloatRect sb = sTxt.getLocalBounds();
        sTxt.setPosition({popX + 50.f + (170.f - sb.size.x) / 2.f - sb.position.x, popY + 272.f + (38.f - sb.size.y) / 2.f - sb.position.y});
        win.draw(sTxt);

        drawRoundedCard(win, {popX + 260.f, popY + 272.f}, {170.f, 38.f}, 6.f, Color(241, 245, 249), 1.f, Color(203, 213, 225));
        Text canTxt(boldFont, L"H\u1ee7y b\u1ecf", 14); canTxt.setFillColor(Color(71, 85, 105));
        FloatRect cb = canTxt.getLocalBounds();
        canTxt.setPosition({popX + 260.f + (170.f - cb.size.x) / 2.f - cb.position.x, popY + 272.f + (38.f - cb.size.y) / 2.f - cb.position.y});
        win.draw(canTxt);
    }

    // ==========================================
    // 8c. POPUP XÁC NHẬN THU HỒI VOUCHER
    // ==========================================
    if (showRevokeConfirmPopup) {
        RectangleShape overlay({width, height});
        overlay.setPosition(position);
        overlay.setFillColor(Color(15, 23, 42, 160));
        win.draw(overlay);

        float popW = 460.f;
        float popH = 250.f;
        float popX = position.x + (width - popW) / 2.f;
        float popY = position.y + (height - popH) / 2.f - 20.f;

        drawRoundedCard(win, {popX, popY}, {popW, popH}, 12.f, Color::White);

        // Tiêu đề
        Text pTitle(boldFont, L"X\u00e1c nh\u1eadn thu h\u1ed3i voucher", 14);
        pTitle.setFillColor(Color(220, 38, 38));
        pTitle.setPosition({popX + 30.f, popY + 22.f});
        win.draw(pTitle);

        Text pSub(font, L"B\u1ea1n c\u00f3 ch\u1eafc ch\u1eafn mu\u1ed1n thu h\u1ed3i voucher c\u1ee7a kh\u00e1ch h\u00e0ng:", 14);
        pSub.setFillColor(Color(71, 85, 105));
        pSub.setPosition({popX + 30.f, popY + 54.f});
        win.draw(pSub);

        // Khung thông tin khách hàng được chọn
        drawRoundedCard(win, {popX + 30.f, popY + 80.f}, {popW - 60.f, 76.f}, 6.f, Color(254, 242, 242), 1.f, Color(254, 202, 202));

        if (selectedRecipientIndex >= 0 && selectedRecipientIndex < (int)filteredRecipients.size()) {
            const auto& targetRec = filteredRecipients[selectedRecipientIndex];
            Text emailTxt(boldFont, s2ws(targetRec.email), 14);
            emailTxt.setFillColor(Color(23, 32, 51));
            emailTxt.setPosition({popX + 44.f, popY + 92.f});
            win.draw(emailTxt);

            wstring detailStr = L"H\u1ecd t\u00ean: " + targetRec.fullName + L"  \u2022  S\u1ed1 l\u01b0\u1ee3ng: " + to_wstring(targetRec.quantity) + L" voucher";
            Text detTxt(font, detailStr, 12.f);
            detTxt.setFillColor(Color(100, 116, 139));
            detTxt.setPosition({popX + 44.f, popY + 118.f});
            win.draw(detTxt);
        }

        // Nút Xác nhận thu hồi
        drawRoundedCard(win, {popX + 40.f, popY + 180.f}, {170.f, 38.f}, 6.f, Color(220, 38, 38));
        Text cfmTxt(boldFont, L"X\u00e1c nh\u1eadn thu h\u1ed3i", 14);
        cfmTxt.setFillColor(Color::White);
        FloatRect cfb = cfmTxt.getLocalBounds();
        cfmTxt.setPosition({popX + 40.f + (170.f - cfb.size.x) / 2.f - cfb.position.x, popY + 180.f + (38.f - cfb.size.y) / 2.f - cfb.position.y});
        win.draw(cfmTxt);

        // Nút Hủy bỏ
        drawRoundedCard(win, {popX + 240.f, popY + 180.f}, {170.f, 38.f}, 6.f, Color(241, 245, 249), 1.f, Color(203, 213, 225));
        Text canTxt(boldFont, L"H\u1ee7y b\u1ecf", 14);
        canTxt.setFillColor(Color(71, 85, 105));
        FloatRect cab = canTxt.getLocalBounds();
        canTxt.setPosition({popX + 240.f + (170.f - cab.size.x) / 2.f - cab.position.x, popY + 180.f + (38.f - cab.size.y) / 2.f - cab.position.y});
        win.draw(canTxt);
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
            drawRoundedCard(win, {notX, notY}, {notW, notH}, 8.f, Color(22, 163, 74, 245), 1.5f, Color(21, 128, 61));
            Text notText(boldFont, s2ws(notificationMessage), 14);
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
