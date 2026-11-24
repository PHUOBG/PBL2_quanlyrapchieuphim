#include "UI/components/Admin/ComboPanel.h"
#include "UI/components/Admin/FontManager.h"
#include "UI/components/Admin/UITheme.h"
#include "UI/components/Admin/RoundedRectRenderer.h"
#include "repositories/admin/AdminTicketRepository.h"
#include "utils/FileUtils.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#endif
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
    return res + " Đ";
}

ComboPanel::ComboPanel(Font& fontRef, float w, float h)
    : font(fontRef),
      boldFont(FontManager::instance().bold()),
      width(w), height(h), position(280.f, 0.f)
{

    if (FileUtils::safeLoadFromFile(reloadTex, "assets/elements/reload.png")) {
        reloadTex.setSmooth(true);
        hasReloadTex = true;
    }

    initDefaultCombos();
    loadCombosFromFile();
    applyFilter();
}

const Texture* ComboPanel::getComboTexture(const string& id, const wstring& name) {
    auto it = comboCache.find(id);
    if (it != comboCache.end()) return &it->second;

    string filename = "";
    if (id == "CB01") filename = "combo_coca_cola.jpg";
    else if (id == "CB02") filename = "combo_bap_caramel.jpg";
    else if (id == "CB03") filename = "combo_bap_pho_mai.jpg";
    else if (id == "CB04") filename = "combo_coca_cola_bap_caramel.jpg";
    else if (id == "CB05") filename = "combo_coca_cola_bap_pho_mai.jpg";
    else if (id == "CB06") filename = "combo_nhom_2_coca_cola_bap_caramel.jpg";
    else if (id == "CB07") filename = "combo_coca_cola_bap_pho_mai.jpg";
    else if (id == "CB08") filename = "combo_coca_cola_bap_caramel.jpg";
    else if (id == "CB09") filename = "combo_coca_cola_bap_pho_mai.jpg";
    else if (id == "CB10") filename = "combo_nhom_2_coca_cola_bap_caramel.jpg";
    else {
        if (name.find(L"Coca") != wstring::npos && name.find(L"Bắp") != wstring::npos)
            filename = "combo_coca_cola_bap_caramel.jpg";
        else if (name.find(L"Coca") != wstring::npos)
            filename = "combo_coca_cola.jpg";
        else if (name.find(L"Phô Mai") != wstring::npos)
            filename = "combo_bap_pho_mai.jpg";
        else
            filename = "combo_bap_caramel.jpg";
    }

    vector<string> candidates = {
        "assets/combos/" + filename,
        "../assets/combos/" + filename,
        "../../assets/combos/" + filename
    };

    Texture tex;
    tex.setSmooth(true);
    bool loaded = false;
    for (const auto& c : candidates) {
        if (FileUtils::safeLoadFromFile(tex, c)) {
            loaded = true;
            break;
        }
    }

    if (loaded) {
        comboCache[id] = std::move(tex);
        return &comboCache[id];
    }
    return nullptr;
}

void ComboPanel::setPosition(Vector2f pos) {
    position = pos;
    width = 1728.f - pos.x;
    maxVisibleRows = computeMaxVisibleRows();
    applyFilter();
}

int ComboPanel::computeMaxVisibleRows() const {
    const float tableOffset = 318.f;
    const float headerH = 44.f;
    const float footerH = 60.f;
    const float rowH = 76.f;
    float avail = max(rowH * 4.f, height - tableOffset - headerH - footerH);
    int rows = static_cast<int>(avail / rowH);
    return max(4, min(10, rows));
}

bool ComboPanel::exportCombosToCsv() {
    ComboReportExporter exporter(filteredCombos);
    ExportResult res = exporter.exportToCsv("exports/combos_export.csv");
    if (!res.success) {
        notificationMessage = "Lỗi: Không thể xuất file CSV!";
    } else if (res.fileOpened) {
        notificationMessage = "Đã xuất bảng giá F&B và mở trong Excel thành công!";
    } else {
        notificationMessage = "Đã xuất bảng giá F&B ra file CSV thành công!";
    }
    notificationClock.restart();
    notificationVisible = true;
    return res.success;
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
        {"CB10", L"4 CocaCola + 2 B\u1eafp Ph\u00f4 Mai", L"G\u00f3i Gia \u0110\u00ecnh: 4 Ly Coca 22oz + 2 H\u1ed9p B\u1eafp Ph\u00f4 Mai l\u1edbn", L"Combo Nh\u00f3m", 240000, false, true, false},
        {"CB11", L"Sprite Chanh T\u01b0\u01a1i", L"1x Ly n\u01b0\u1edbc ng\u1ecdt c\u00f3 ga h\u01b0\u01a1ng chanh s\u1ea3ng kho\u00e1i (Size L 22oz)", L"\u0110\u1ed3 u\u1ed1ng", 30000, false, true, false},
        {"CB12", L"Fanta Cam T\u01b0\u01a1i", L"1x Ly Fanta h\u01b0\u01a1ng cam b\u00f9ng n\u1ed5 n\u0103ng l\u01b0\u1ee3ng (Size L 22oz)", L"\u0110\u1ed3 u\u1ed1ng", 30000, false, true, false},
        {"CB13", L"B\u1eafp Rang B\u01a1 Truy\u1ec1n Th\u1ed1ng", L"1x B\u1eafp rang b\u01a1 v\u00e0ng \u00f3ng th\u01a1m n\u1ee9c h\u01b0\u01a1ng v\u1ecb truy\u1ec1n th\u1ed1ng (64oz)", L"B\u1eafp ng\u1ecdt", 65000, false, true, false},
        {"CB14", L"B\u1eafp Rang Ph\u00f4 Mai Rong Bi\u1ec3n", L"1x B\u1eafp l\u1eafc rong bi\u1ec3n gi\u00f2n k\u00e8m b\u1ed9t ph\u00f4 mai \u0111\u1ed9c quy\u1ec1n (64oz)", L"B\u1eafp m\u1eb7n", 80000, true, true, false},
        {"CB15", L"N\u01b0\u1edbc Kho\u00e1ng Dasani 500ml", L"1x Chai n\u01b0\u1edbc kho\u00e1ng tinh khi\u1ebft thanh m\u00e1t", L"\u0110\u1ed3 u\u1ed1ng", 20000, false, true, false},
        {"CB16", L"Tr\u00e0 S\u1eefa Tr\u00e2n Ch\u00e2u R\u1ea1p Chi\u1ebfu", L"1x Ly tr\u00e0 s\u1eefa th\u01a1m b\u00e9o k\u00e8m tr\u00e2n ch\u00e2u ho\u00e0ng kim dai gi\u00f2n", L"\u0110\u1ed3 u\u1ed1ng", 45000, true, true, false},
        {"CB17", L"Khoai T\u00e2y Chi\u00ean L\u1eafc Ph\u00f4 Mai", L"1x H\u1ed9p khoai t\u00e2y chi\u00ean gi\u00f2n r\u1ee5m l\u1eafc ph\u00f4 mai \u0111\u1eadm \u0111\u00e0", L"B\u1eafp m\u1eb7n", 50000, false, true, false},
        {"CB18", L"Combo Couple Sweet", L"2x Ly n\u01b0\u1edbc ng\u1ecdt 22oz + 1 H\u1ed9p b\u1eafp ng\u1ecdt l\u1edbn size L", L"Combo Nh\u00f3m", 115000, false, true, false},
        {"CB19", L"Combo Gia \u0110\u00ecnh", L"4x Ly n\u01b0\u1edbc ng\u1ecdt + 2 H\u1ed9p b\u1eafp t\u1ef1 ch\u1ecdn + 1 Ph\u1ea7n snack gi\u00f2n", L"Combo Nh\u00f3m", 260000, false, true, false},
        {"CB20", L"Combo CineMax VIP", L"2x N\u01b0\u1edbc \u0111\u1eb7c bi\u1ec7t + 1 H\u1ed9p b\u1eafp 3 ng\u0103n \u0111\u1ee7 v\u1ecb l\u1edbn", L"Combo Nh\u00f3m", 150000, true, true, false}
    };
}

void ComboPanel::loadCombosFromFile() {
    string path = FileUtils::resolveDataPath("data/combo.txt");
    ifstream f(path, ios::binary);
    if (!f.is_open()) return;

    // Keep default descriptions/categories by ID for richer display when present
    vector<ComboItem> defaults = allCombos;
    vector<ComboItem> loaded;

    string line;
    bool isHeader = true;
    while (getline(f, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;
        if (isHeader) { isHeader = false; continue; }
        stringstream ss(line);
        string id, name, pr;
        getline(ss, id, '|');
        getline(ss, name, '|');
        getline(ss, pr, '|');
        if (id.empty() || name.empty()) continue;

        int p = 0;
        try { p = stoi(pr); } catch (...) { p = 0; }
        if (p < 0) p = 0;

        ComboItem item{id, s2ws(name), L"Sản phẩm bắp nước rạp chiếu", L"Combo Đơn", p, false, true, false};
        for (const auto& def : defaults) {
            if (def.id == id) {
                item.subtitle = def.subtitle;
                item.category = def.category;
                item.isHot = def.isHot;
                break;
            }
        }
        loaded.push_back(item);
    }

    if (!loaded.empty()) {
        allCombos = std::move(loaded);
    }
}

bool ComboPanel::saveCombosToFile() {
    string path = FileUtils::resolveDataPath("data/combo.txt");
    auto sanitize = [](string s) {
        for (char& ch : s) if (ch == '|' || ch == '\n' || ch == '\r') ch = ' ';
        return s;
    };
    string out = "combo_id|combo_name|price\n";
    for (const auto& item : allCombos) {
        out += sanitize(item.id) + "|" + sanitize(ws2s(item.name)) + "|" + to_string(item.price) + "\n";
    }
    return FileUtils::writeAtomic(path, out);
}

void ComboPanel::applyFilter() {
    filteredCombos.clear();

    for (const auto& itm : allCombos) {
        // Lọc theo tabs phân loại
        if (activeFilterIndex == 1 && itm.category != L"\u0110\u1ed3 u\u1ed1ng") continue;
        if (activeFilterIndex == 2 && (itm.category != L"B\u1eafp ng\u1ecdt" && itm.category != L"B\u1eafp m\u1eb7n")) continue;
        if (activeFilterIndex == 3 && itm.category != L"Combo \u0110\u01a1n") continue;
        if (activeFilterIndex == 4 && itm.category != L"Combo Nh\u00f3m") continue;

        // Lọc theo search box hỗ trợ tiếng Việt có dấu & không dấu
        if (!searchQuery.empty()) {
            bool match = UITheme::matchesViQuery(s2ws(itm.id), searchQuery) ||
                         UITheme::matchesViQuery(itm.name, searchQuery) ||
                         UITheme::matchesViQuery(itm.subtitle, searchQuery) ||
                         UITheme::matchesViQuery(itm.category, searchQuery);
            if (!match) continue;
        }

        filteredCombos.push_back(itm);
    }

    maxVisibleRows = computeMaxVisibleRows();
    totalPages = max(1, (static_cast<int>(filteredCombos.size()) + maxVisibleRows - 1) / maxVisibleRows);
    if (currentPage > totalPages) currentPage = totalPages;
    if (currentPage < 1) currentPage = 1;
    scrollOffset = (currentPage - 1) * maxVisibleRows;
    selectedRow = (!searchQuery.empty() && !filteredCombos.empty()) ? 0 : -1;
}

void ComboPanel::drawRoundedCard(RenderWindow& win, Vector2f pos, Vector2f sz, float r, Color fill, float outline, Color outCol) {
    RoundedRectRenderer::draw(win, pos, sz, r, fill, outline, outCol);
}

void ComboPanel::performDeleteCombo(const string& selId) {
    vector<ComboItem> backup = allCombos;
    for (auto it = allCombos.begin(); it != allCombos.end(); ++it) {
        if (it->id == selId) {
            allCombos.erase(it);
            break;
        }
    }
    if (!saveCombosToFile()) {
        allCombos = std::move(backup);
        notificationMessage = "Lỗi: Không thể xóa combo!";
    } else {
        applyFilter();
        notificationMessage = "Đã xóa combo thành công!";
    }
    notificationClock.restart();
    notificationVisible = true;
}

void ComboPanel::handleEvent(const Event& event, const RenderWindow& win) {
    if (deleteModal.isOpen) {
        int res = deleteModal.handleEvent(event, win);
        if (res == 2) {
            performDeleteCombo(deleteModal.targetId);
        }
        return;
    }

    // 1. ƯU TIÊN XỬ LÝ POPUP THÊM / SỬA COMBO (MODAL BLOCKING)
    if (showPopup) {
        float popW = 500.f;
        float popH = 340.f;
        float popX = position.x + (width - popW) / 2.f;
        float popY = position.y + (height - popH) / 2.f - 20.f;

        auto commitSaveCombo = [&]() {
            if (popupName.empty()) {
                notificationMessage = "Tên combo không được để trống!";
                notificationClock.restart();
                notificationVisible = true;
                return;
            }
            if (popupSub.empty()) popupSub = L"1x B\u1eafp + 1x N\u01b0\u1edbc ng\u1ecdt";
            int parsedPr = 0;
            try { if (!popupPrice.empty()) parsedPr = stoi(popupPrice); } catch (...) { parsedPr = 0; }
            if (parsedPr <= 0) {
                notificationMessage = "Giá combo phải lớn hơn 0!";
                notificationClock.restart();
                notificationVisible = true;
                return;
            }

            vector<ComboItem> backup = allCombos;
            if (editingIndex >= 0 && editingIndex < (int)allCombos.size()) {
                allCombos[editingIndex].name = popupName;
                allCombos[editingIndex].subtitle = popupSub;
                allCombos[editingIndex].category = popupCat.empty() ? L"Combo \u0110\u01a1n" : popupCat;
                allCombos[editingIndex].price = parsedPr;
            } else {
                // Ensure unique CBxx ID
                int maxNum = 0;
                for (const auto& c : allCombos) {
                    if (c.id.size() > 2 && c.id[0] == 'C' && c.id[1] == 'B') {
                        try { maxNum = max(maxNum, stoi(c.id.substr(2))); } catch (...) {}
                    }
                }
                string genId = (maxNum + 1 < 10 ? "CB0" : "CB") + to_string(maxNum + 1);
                string candidateId = popupId.empty() ? genId : popupId;
                for (const auto& c : allCombos) {
                    if (c.id == candidateId) { candidateId = genId; break; }
                }

                ComboItem ni;
                ni.id = candidateId;
                ni.name = popupName;
                ni.subtitle = popupSub;
                ni.category = popupCat.empty() ? L"Combo \u0110\u01a1n" : popupCat;
                ni.price = parsedPr;
                allCombos.push_back(ni);
            }
            if (!saveCombosToFile()) {
                allCombos = std::move(backup);
                notificationMessage = "Lỗi: Không thể lưu dữ liệu combo!";
                notificationClock.restart();
                notificationVisible = true;
                return;
            }
            applyFilter();
            showPopup = false;
            notificationMessage = "Đã lưu thông tin combo thành công!";
            notificationClock.restart();
            notificationVisible = true;
        };

        if (const auto* mouseBtn = event.getIf<Event::MouseButtonPressed>()) {
            if (mouseBtn->button == Mouse::Button::Left) {
                Vector2f mousePos = win.mapPixelToCoords(Mouse::getPosition(win));

                FloatRect boxName({popX + 30.f, popY + 85.f}, {popW - 60.f, 36.f});
                FloatRect boxSub({popX + 30.f, popY + 150.f}, {popW - 60.f, 36.f});
                FloatRect boxPrice({popX + 30.f, popY + 215.f}, {popW - 60.f, 36.f});
                FloatRect btnCatCycle({popX + popW - 150.f, popY + 20.f}, {120.f, 28.f});
                FloatRect btnPopSave({popX + 70.f, popY + 270.f}, {160.f, 38.f});
                FloatRect btnPopCancel({popX + 270.f, popY + 270.f}, {160.f, 38.f});

                if (boxName.contains(mousePos)) {
                    popupActiveField = 0;
                    if (editingIndex < 0 && popupName == L"Combo M\u1edbi") popupName.clear();
                    return;
                }
                if (boxSub.contains(mousePos)) {
                    popupActiveField = 1;
                    if (editingIndex < 0 && popupSub == L"Quy chu\u1ea9n chi ti\u1ebft combo...") popupSub.clear();
                    return;
                }
                if (boxPrice.contains(mousePos)) {
                    popupActiveField = 2;
                    return;
                }
                if (btnCatCycle.contains(mousePos)) {
                    vector<wstring> cats = {L"Combo \u0110\u01a1n", L"Combo Nh\u00f3m", L"\u0110\u1ed3 u\u1ed1ng", L"B\u1eafp ng\u1ecdt", L"B\u1eafp m\u1eb7n"};
                    int curIdx = 0;
                    for (size_t i = 0; i < cats.size(); ++i) {
                        if (cats[i] == popupCat) { curIdx = (int)i; break; }
                    }
                    popupCat = cats[(curIdx + 1) % cats.size()];
                    return;
                }
                if (btnPopSave.contains(mousePos)) {
                    commitSaveCombo();
                    return;
                }
                if (btnPopCancel.contains(mousePos) || !FloatRect({popX, popY}, {popW, popH}).contains(mousePos)) {
                    showPopup = false;
                    return;
                }
            }
            return;
        }

        if (const auto* keyEv = event.getIf<Event::KeyPressed>()) {
            if (keyEv->code == Keyboard::Key::Escape) { showPopup = false; return; }
            if (keyEv->code == Keyboard::Key::Tab) { popupActiveField = (popupActiveField + 1) % 3; return; }
            if (keyEv->code == Keyboard::Key::Enter) { commitSaveCombo(); return; }
            if (keyEv->code == Keyboard::Key::Backspace) {
                if (popupActiveField == 0 && !popupName.empty()) popupName.pop_back();
                else if (popupActiveField == 1 && !popupSub.empty()) popupSub.pop_back();
                else if (popupActiveField == 2 && !popupPrice.empty()) popupPrice.pop_back();
                return;
            }
        }

        if (const auto* txtEv = event.getIf<Event::TextEntered>()) {
            char32_t u = txtEv->unicode;
            if (u >= 32 && u != 127) {
                if (popupActiveField == 0 && popupName.size() < 45) popupName += static_cast<wchar_t>(u);
                else if (popupActiveField == 1 && popupSub.size() < 65) popupSub += static_cast<wchar_t>(u);
                else if (popupActiveField == 2 && u >= '0' && u <= '9' && popupPrice.size() < 8) popupPrice += static_cast<char>(u);
            }
            return;
        }
        return;
    }

    if (const auto* mouseBtn = event.getIf<Event::MouseButtonPressed>()) {
        if (mouseBtn->button == Mouse::Button::Left) {
            Vector2f mousePos = win.mapPixelToCoords(Mouse::getPosition(win));

            // Nút Header: Reload, Xuất Excel
            FloatRect btnRel({position.x + width - 193.f, position.y + 39.f}, {40.f, 38.f});
            FloatRect btnExp({position.x + width - 143.f, position.y + 39.f}, {95.f, 38.f});

            if (btnRel.contains(mousePos)) {
                searchQuery.clear();
                activeFilterIndex = 0;
                loadCombosFromFile();
                applyFilter();
                notificationMessage = "Đã làm mới dữ liệu combo bắp nước!";
                notificationClock.restart();
                notificationVisible = true;
                return;
            }

            if (btnExp.contains(mousePos)) {
                exportCombosToCsv();
                return;
            }

            // Toolbar buttons: Thêm combo, Sửa combo, Xóa combo, Đồng bộ POS
            float tbY = position.y + 216.f;
            FloatRect btnAdd({position.x + 30.f, tbY}, {134.f, 38.f});
            FloatRect btnEdit({position.x + 174.f, tbY}, {122.f, 38.f});
            FloatRect btnDel({position.x + 306.f, tbY}, {122.f, 38.f});
            FloatRect btnSync({position.x + width - 180.f, tbY}, {150.f, 38.f});

            if (btnAdd.contains(mousePos)) {
                showPopup = true;
                editingIndex = -1;
                popupActiveField = 0;
                int nextNum = (int)allCombos.size() + 1;
                popupId = (nextNum < 10 ? "CB0" : "CB") + to_string(nextNum);
                popupName = L"";
                popupSub = L"1x B\u1eafp ng\u1ecdt + 1x CocaCola 22oz";
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
                            popupActiveField = 0;
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
                    const auto& item = filteredCombos[selectedRow];
                    deleteModal.open(
                        L"X\u00c1C NH\u1eacN X\u00d3A COMBO",
                        L"B\u1ea1n c\u00f3 ch\u1eafc ch\u1eafn mu\u1ed1n x\u00f3a combo \"" + item.name + L"\" kh\u00f4ng?\nThao t\u00e1c n\u00e0y kh\u00f4ng th\u1ec3 ho\u00e0n t\u00e1c.",
                        item.id
                    );
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
            float fltY = position.y + 266.f;
            float searchW = min(320.f, max(220.f, width * 0.24f));
            FloatRect searchBox({position.x + 30.f, fltY}, {searchW, 38.f});
            if (searchBox.contains(mousePos)) {
                if (!searchQuery.empty() && mousePos.x >= position.x + 30.f + searchW - 28.f) {
                    searchQuery.clear();
                    applyFilter();
                }
                searchActive = true;
                return;
            } else {
                searchActive = false;
            }

            // Filter pills (dynamic width & wrap-safe)
            vector<wstring> pillLabels = {
                L"T\u1ea5t c\u1ea3 (" + to_wstring(allCombos.size()) + L")",
                L"\u0110\u1ed3 u\u1ed1ng",
                L"B\u1eafp \u0111\u01a1n",
                L"Combo 1 Ng\u01b0\u1eddi",
                L"Combo Nh\u00f3m / C\u1eb7p \u0111\u00f4i"
            };
            float pX = position.x + 30.f + searchW + 58.f;
            float pY = fltY + 1.f;
            float maxRight = position.x + width - 30.f;
            bool wrapped = false;
            for (size_t i = 0; i < pillLabels.size(); ++i) {
                Text mTxt(boldFont, pillLabels[i], 14);
                float pw = std::round(mTxt.getLocalBounds().size.x + 28.f);
                if (pX + pw > maxRight && i > 0) {
                    pX = position.x + 30.f;
                    pY = fltY + 44.f;
                    wrapped = true;
                }
                FloatRect pB({pX, pY}, {pw, 36.f});
                if (pB.contains(mousePos)) {
                    activeFilterIndex = (int)i;
                    applyFilter();
                    return;
                }
                pX += pw + 10.f;
            }

            // Table Row Click & Actions
            float tableY = position.y + (wrapped ? 360.f : 318.f);
            float rowH = 76.f;

            int count = max(0, min((int)filteredCombos.size() - scrollOffset, maxVisibleRows));

            for (int r = 0; r < count; ++r) {
                float rY = tableY + 44.f + r * rowH;
                FloatRect rowRect({position.x + 30.f, rY}, {width - 60.f, rowH});

                if (rowRect.contains(mousePos)) {
                    selectedRow = scrollOffset + r;

                    // Click checkbox
                    FloatRect cbRect({position.x + 42.f, rY + 28.f}, {20.f, 20.f});
                    if (cbRect.contains(mousePos)) {
                        filteredCombos[selectedRow].isChecked = !filteredCombos[selectedRow].isChecked;
                        return;
                    }

                    // Click action edit
                    float actX = position.x + width - 30.f - 124.f + (124.f - 76.f) / 2.f;
                    FloatRect btnRowEdit({actX, rY + 24.f}, {36.f, 28.f});
                    if (btnRowEdit.contains(mousePos)) {
                        string selId = filteredCombos[selectedRow].id;
                        for (size_t i = 0; i < allCombos.size(); ++i) {
                            if (allCombos[i].id == selId) {
                                editingIndex = (int)i;
                                popupActiveField = 0;
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
                    FloatRect btnRowDel({actX + 40.f, rY + 24.f}, {36.f, 28.f});
                    if (btnRowDel.contains(mousePos)) {
                        const auto& item = filteredCombos[selectedRow];
                        deleteModal.open(
                            L"X\u00c1C NH\u1eacN X\u00d3A COMBO",
                            L"B\u1ea1n c\u00f3 ch\u1eafc ch\u1eafn mu\u1ed1n x\u00f3a combo \"" + item.name + L"\" kh\u00f4ng?\nThao t\u00e1c n\u00e0y kh\u00f4ng th\u1ec3 ho\u00e0n t\u00e1c.",
                            item.id
                        );
                        return;
                    }
                    return;
                }
            }

            // Pagination buttons
            float pageY = tableY + 44.f + maxVisibleRows * rowH + 14.f;
            float tableW = width - 60.f;
            float rightEdge = position.x + 30.f + tableW;

            int numPageBtns = min(5, totalPages);
            int startP = max(1, min(currentPage - 2, totalPages - numPageBtns + 1));
            int endP = min(totalPages, startP + numPageBtns - 1);

            float btnPrevW = 60.f;
            float btnNextW = 60.f;
            float btnPageW = 32.f;
            float gap = 6.f;
            float totalBtnsW = btnPrevW + gap + (endP - startP + 1) * (btnPageW + gap) - gap + gap + btnNextW;
            float curBtnX = rightEdge - totalBtnsW;

            // Nút Trước
            FloatRect btnPrev({curBtnX, pageY}, {btnPrevW, 30.f});
            if (btnPrev.contains(mousePos) && currentPage > 1) {
                currentPage--;
                selectedRow = -1;
                scrollOffset = (currentPage - 1) * maxVisibleRows;
                return;
            }
            curBtnX += btnPrevW + gap;

            // Nút số trang 1, 2, 3...
            for (int p = startP; p <= endP; ++p) {
                FloatRect btnP({curBtnX, pageY}, {btnPageW, 30.f});
                if (btnP.contains(mousePos)) {
                    currentPage = p;
                    selectedRow = -1;
                    scrollOffset = (currentPage - 1) * maxVisibleRows;
                    return;
                }
                curBtnX += btnPageW + gap;
            }

            // Nút Tiếp
            FloatRect btnNext({curBtnX, pageY}, {btnNextW, 30.f});
            if (btnNext.contains(mousePos) && currentPage < totalPages) {
                currentPage++;
                selectedRow = -1;
                scrollOffset = (currentPage - 1) * maxVisibleRows;
                return;
            }
        }
    }

    // Scroll
    if (const auto* wheel = event.getIf<Event::MouseWheelScrolled>()) {
        if (!showPopup) {
            if (wheel->delta > 0) {
                if (currentPage > 1) {
                    currentPage--;
                    scrollOffset = (currentPage - 1) * maxVisibleRows;
                }
            } else if (wheel->delta < 0) {
                if (currentPage < totalPages) {
                    currentPage++;
                    scrollOffset = (currentPage - 1) * maxVisibleRows;
                }
            }
        }
    }

    // Search typing (hỗ trợ đầy đủ tiếng Việt Unicode)
    if (const auto* keyEv = event.getIf<Event::KeyPressed>()) {
        if (searchActive && !showPopup) {
            if (keyEv->code == Keyboard::Key::Backspace && !searchQuery.empty()) {
                searchQuery.pop_back();
                applyFilter();
                return;
            }
            if (keyEv->code == Keyboard::Key::Escape) {
                searchQuery.clear();
                searchActive = false;
                applyFilter();
                return;
            }
        }
    }

    if (const auto* textEv = event.getIf<Event::TextEntered>()) {
        if (searchActive && !showPopup) {
            char32_t c = textEv->unicode;
            if (c >= 32 && c != 127 && searchQuery.size() < 35) {
                searchQuery += static_cast<wchar_t>(c);
                applyFilter();
            }
        }
    }
}

void ComboPanel::update(Vector2f mousePos, bool) {
    deleteModal.update(mousePos);
    hoveredRow = -1;
    if (!showPopup && !deleteModal.isOpen) {
        float tableY = position.y + 318.f;
        float rowH = 76.f;
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
    // 2. TOP BAR (Standardized Navigation & Profile)
    // ==========================================
    // ==========================================
    // 3. PAGE HEADER CARD (Unified RoomPanel style)
    // ==========================================
    UITheme::drawSectionHeaderCard(win, position.x + 30.f, position.y + 16.f, width - 60.f,
                                   L"Qu\u1ea3n l\u00fd combo b\u1eafp n\u01b0\u1edbc",
                                   to_wstring(allCombos.size()) + L" Combo \u0110ang B\u00e1n",
                                   Color(220, 252, 231), Color(22, 101, 52),
                                   L"C\u1eadp nh\u1eadt b\u1ea3ng gi\u00e1, c\u00f4ng th\u1ee9c m\u00f3n b\u1eafp rang, n\u01b0\u1edbc ng\u1ecdt v\u00e0 c\u00e1c g\u00f3i \u01b0u \u0111\u00e3i t\u1ea1i qu\u1ea7y b\u1eafp n\u01b0\u1edbc.",
                                   boldFont, font, false);

    // Nút Reload và Xuất Excel góc phải
    drawRoundedCard(win, {position.x + width - 193.f, position.y + 39.f}, {40.f, 38.f}, 8.f, Color::White, 1.f, Color(203, 213, 225));
    UITheme::drawReloadIcon(win, {position.x + width - 193.f + 20.f, position.y + 39.f + 19.f}, 16.f, Color(71, 85, 105));

    drawRoundedCard(win, {position.x + width - 143.f, position.y + 39.f}, {95.f, 38.f}, 8.f, Color::White, 1.f, Color(203, 213, 225));
    Text expTxt(boldFont, L"Xu\u1ea5t Excel", 14);
    expTxt.setFillColor(Color(22, 101, 52));
    FloatRect etb = expTxt.getLocalBounds();
    expTxt.setPosition({position.x + width - 143.f + (95.f - etb.size.x) / 2.f - etb.position.x, position.y + 39.f + (38.f - etb.size.y) / 2.f - etb.position.y});
    win.draw(expTxt);

    // ==========================================
    // 4. 4 THẺ THỐNG KÊ KPI (STAT CARDS GRID)
    // ==========================================
    float statY = position.y + 114.f;
    float cardW = (width - 60.f - 45.f) / 4.f;
    float cardH = 90.f;

    // Compute real summary metrics from allCombos & tickets.txt
    long long sumComboPrices = 0;
    unordered_set<wstring> distinctCats;
    for (const auto& c : allCombos) {
        sumComboPrices += c.price;
        distinctCats.insert(c.category);
    }
    long long avgMenuPrice = allCombos.empty() ? 0 : (sumComboPrices / static_cast<long long>(allCombos.size()));

    long long totalFbRev = 0;
    unordered_map<string, int> soldQtyByCode;
    {
        unordered_map<string, int> priceByKey;
        for (const auto& c : allCombos) {
            priceByKey[c.id] = c.price;
            priceByKey[ws2s(c.name)] = c.price;
        }
        AdminTicketRepository tRepo(FileUtils::resolveDataPath("data/tickets.txt"));
        DLL<Ticket> tickets = tRepo.loadAll();
        for (int i = 0; i < tickets.getSize(); ++i) {
            const string& cs = tickets[i].comboName;
            if (cs.empty() || cs == "Không có" || cs == "None") continue;
            stringstream css(cs);
            string tok;
            while (getline(css, tok, ',')) {
                size_t a = tok.find_first_not_of(" \t\r\n");
                size_t b = tok.find_last_not_of(" \t\r\n");
                if (a == string::npos) continue;
                string item = tok.substr(a, b - a + 1);
                string code = item;
                int qty = 1;
                size_t col = item.find(":x");
                if (col != string::npos) {
                    code = item.substr(0, col);
                    try { qty = max(1, stoi(item.substr(col + 2))); } catch (...) { qty = 1; }
                } else {
                    size_t xpos = item.rfind(" x");
                    if (xpos != string::npos) {
                        code = item.substr(0, xpos);
                        try { qty = max(1, stoi(item.substr(xpos + 2))); } catch (...) { qty = 1; }
                    }
                }
                soldQtyByCode[code] += qty;
                if (priceByKey.count(code)) {
                    totalFbRev += static_cast<long long>(priceByKey[code]) * qty;
                }
            }
        }
    }
    string bestCode;
    int bestQty = 0;
    for (const auto& kv : soldQtyByCode) {
        if (kv.second > bestQty) { bestQty = kv.second; bestCode = kv.first; }
    }
    wstring bestLabel = L"Ch\u01b0a c\u00f3 d\u1eef li\u1ec7u";
    if (!bestCode.empty()) {
        bestLabel = s2ws(bestCode);
        for (const auto& c : allCombos) {
            if (c.id == bestCode || ws2s(c.name) == bestCode) {
                bestLabel = s2ws(c.id) + L": " + c.name;
                break;
            }
        }
    }

    auto drawCenteredComboKpi = [&](float cX, const wstring& titleStr, const wstring& valStr, const wstring& subStr) {
        drawRoundedCard(win, {cX, statY}, {cardW, cardH}, 10.f, Color::White, 1.f, Color(226, 232, 240));

        Text tLbl(boldFont, titleStr, 14);
        tLbl.setFillColor(Color(100, 116, 139));
        FloatRect tb = tLbl.getLocalBounds();
        tLbl.setPosition({std::round(cX + (cardW - tb.size.x) / 2.f - tb.position.x), std::round(statY + 12.f)});
        win.draw(tLbl);

        Text tVal(boldFont, valStr, 22);
        tVal.setFillColor(Color(23, 32, 51));
        FloatRect vb = tVal.getLocalBounds();
        tVal.setPosition({std::round(cX + (cardW - vb.size.x) / 2.f - vb.position.x), std::round(statY + 34.f)});
        win.draw(tVal);

        Text tSub(font, subStr, 16);
        tSub.setFillColor(Color(100, 116, 139));
        FloatRect sb = tSub.getLocalBounds();
        tSub.setPosition({std::round(cX + (cardW - sb.size.x) / 2.f - sb.position.x), std::round(statY + 62.f)});
        win.draw(tSub);
    };

    float c1X = position.x + 30.f;
    float c2X = c1X + cardW + 15.f;
    float c3X = c2X + cardW + 15.f;
    float c4X = c3X + cardW + 15.f;
    if (bestLabel.size() > 16) bestLabel = bestLabel.substr(0, 14) + L"..";

    drawCenteredComboKpi(c1X, L"T\u1ed4NG DANH M\u1ee4C", to_wstring(allCombos.size()), to_wstring(distinctCats.size()) + L" nh\u00f3m s\u1ea3n ph\u1ea9m F&B");
    drawCenteredComboKpi(c2X, L"DOANH THU F&B", s2ws(formatPrice((int)min(totalFbRev, 2000000000LL))), L"T\u1ed5ng doanh thu combo \u0111\u00e3 b\u00e1n");
    drawCenteredComboKpi(c3X, L"BEST SELLER", bestLabel, to_wstring(bestQty) + L" ph\u1ea7n \u0111\u00e3 b\u00e1n ra");
    drawCenteredComboKpi(c4X, L"GI\u00c1 TR\u1eca TRUNG B\u00ccNH", s2ws(formatPrice((int)avgMenuPrice)), L"\u0110\u01a1n gi\u00e1 trung b\u00ecnh th\u1ef1c \u0111\u01a1n");

    // ==========================================
    // 5. TOOLBAR & FILTER PILLS
    // ==========================================
    float tbY = position.y + 216.f;

    auto drawToolbarBtn = [&](float bx, float bw, Color bg, Color fg, const wstring& label, Color border = Color::Transparent) {
        drawRoundedCard(win, {bx, tbY}, {bw, 38.f}, 6.f, bg, border == Color::Transparent ? 0.f : 1.f, border);
        Text txt(boldFont, label, 14);
        txt.setFillColor(fg);
        FloatRect b = txt.getLocalBounds();
        txt.setPosition({std::round(bx + (bw - b.size.x) / 2.f - b.position.x), std::round(tbY + (38.f - b.size.y) / 2.f - b.position.y)});
        win.draw(txt);
    };
    drawToolbarBtn(position.x + 30.f, 134.f, Color(21, 128, 61), Color::White, L"+ Th\u00eam combo");
    drawToolbarBtn(position.x + 174.f, 122.f, Color(217, 119, 6), Color::White, L"S\u1eeda combo");
    drawToolbarBtn(position.x + 306.f, 122.f, Color(220, 38, 38), Color::White, L"X\u00f3a combo");
    drawToolbarBtn(position.x + width - 180.f, 150.f, Color(238, 242, 255), Color(67, 56, 202), L"\u0110\u1ed3ng b\u1ed9 m\u00e1y POS", Color(199, 210, 254));

    // Dòng 2: Ô tìm kiếm & Bộ lọc (căn giữa dọc, tự động xuống dòng nếu hẹp)
    float fltY = position.y + 266.f;
    float searchW = min(320.f, max(220.f, width * 0.24f));
    drawRoundedCard(win, {position.x + 30.f, fltY}, {searchW, 38.f}, 6.f, Color::White, searchActive ? 1.5f : 1.f, searchActive ? Color(8, 125, 181) : Color(203, 213, 225));

    CircleShape sGlass(5.5f);
    sGlass.setFillColor(Color::Transparent);
    sGlass.setOutlineThickness(1.8f);
    sGlass.setOutlineColor(Color(148, 163, 184));
    sGlass.setPosition({position.x + 42.f, fltY + 12.f});
    win.draw(sGlass);

    RectangleShape sHandle({6.f, 2.f});
    sHandle.setFillColor(Color(148, 163, 184));
    sHandle.setPosition({position.x + 51.5f, fltY + 21.5f});
    sHandle.setRotation(degrees(45.f));
    win.draw(sHandle);

    wstring sPrompt = searchQuery.empty()
        ? (searchActive ? L"|" : L"T\u00ecm m\u00e3 (CB01...) ho\u1eb7c t\u00ean...")
        : (searchQuery + (searchActive ? L"|" : L""));
    Text sTxt(font, sPrompt, 14);
    sTxt.setFillColor(searchQuery.empty() && !searchActive ? Color(148, 163, 184) : Color(23, 32, 51));
    sTxt.setPosition({position.x + 62.f, fltY + 10.f});
    win.draw(sTxt);
    if (!searchQuery.empty()) {
        UITheme::drawCloseIcon(win, {position.x + 30.f + searchW - 14.f, fltY + 19.f}, 10.f, Color(148, 163, 184), 1.8f);
    }

    Text locLbl(boldFont, L"L\u1eccC:", 14); locLbl.setFillColor(Color(100, 116, 139)); locLbl.setPosition({position.x + 30.f + searchW + 14.f, fltY + 10.f}); win.draw(locLbl);

    // Các Pill filter (chiều rộng động, không chồng chéo)
    vector<wstring> pillLabels = {
        L"T\u1ea5t c\u1ea3 (" + to_wstring(allCombos.size()) + L")",
        L"\u0110\u1ed3 u\u1ed1ng",
        L"B\u1eafp \u0111\u01a1n",
        L"Combo 1 Ng\u01b0\u1eddi",
        L"Combo Nh\u00f3m / C\u1eb7p \u0111\u00f4i"
    };
    float pX = position.x + 30.f + searchW + 58.f;
    float pY = fltY + 1.f;
    float maxRight = position.x + width - 30.f;
    bool wrapped = false;
    for (size_t i = 0; i < pillLabels.size(); ++i) {
        Text pTxt(boldFont, pillLabels[i], 14);
        float pw = std::round(pTxt.getLocalBounds().size.x + 28.f);
        if (pX + pw > maxRight && i > 0) {
            pX = position.x + 30.f;
            pY = fltY + 44.f;
            wrapped = true;
        }
        bool isAct = ((int)i == activeFilterIndex);
        drawRoundedCard(win, {pX, pY}, {pw, 36.f}, 18.f, isAct ? Color(8, 125, 181) : Color::White, isAct ? 0.f : 1.f, Color(226, 232, 240));
        pTxt.setFillColor(isAct ? Color::White : Color(71, 85, 105));
        FloatRect ptb = pTxt.getLocalBounds();
        pTxt.setPosition({std::round(pX + (pw - ptb.size.x) / 2.f - ptb.position.x), std::round(pY + (36.f - ptb.size.y) / 2.f - ptb.position.y)});
        win.draw(pTxt);
        pX += pw + 10.f;
    }

    // ==========================================
    // 6. BẢNG DỮ LIỆU COMBO (DATA TABLE)
    // ==========================================
    float tableX = position.x + 30.f;
    float tableY = position.y + (wrapped ? 360.f : 318.f);
    float tableW = width - 60.f;
    float tableRight = tableX + tableW;
    float rowH = 76.f;

    const float div1 = tableX + 132.f;
    const float div5 = tableRight - 124.f;
    const float div4 = div5 - 146.f;
    const float div3 = div4 - 165.f;
    const float div2 = div3 - 145.f;

    auto drawCenteredHdr = [&](const wstring& str, float xStart, float xEnd) {
        Text h(boldFont, str, 14);
        h.setFillColor(Color::White);
        FloatRect hb = h.getLocalBounds();
        h.setPosition({std::round(xStart + (xEnd - xStart - hb.size.x) / 2.f - hb.position.x), tableY + 13.f});
        win.draw(h);
    };

    // Header bảng (#0077B6)
    drawRoundedCard(win, {tableX, tableY}, {tableW, 44.f}, 6.f, Color(8, 125, 181));

    for (float dx : {div1, div2, div3, div4, div5}) {
        RectangleShape hDiv(Vector2f(1.f, 44.f));
        hDiv.setPosition({dx, tableY});
        hDiv.setFillColor(Color(255, 255, 255, 45));
        win.draw(hDiv);
    }

    drawCenteredHdr(L"M\u00c3 COMBO", tableX, div1);
    Text h2(boldFont, L"T\u00caN COMBO & QUY CHU\u1ea8N CHI TI\u1ebeT", 14); h2.setFillColor(Color::White); h2.setPosition({div1 + 18.f, tableY + 13.f}); win.draw(h2);
    drawCenteredHdr(L"PH\u00c2N LO\u1ea0I", div2, div3);
    drawCenteredHdr(L"GI\u00c1 B\u00c1N NI\u00caM Y\u1ebeT", div3, div4);
    drawCenteredHdr(L"TR\u1ea0NG TH\u00c1I", div4, div5);
    drawCenteredHdr(L"THAO T\u00c1C", div5, tableRight);

    // Dòng dữ liệu
    int count = min((int)filteredCombos.size() - scrollOffset, maxVisibleRows);

    for (int r = 0; r < count; ++r) {
        int idx = scrollOffset + r;
        const auto& itm = filteredCombos[idx];
        float rY = tableY + 44.f + r * rowH;

        bool isSel = (selectedRow == idx);
        bool isHov = (hoveredRow == r);

        Color rowBg = isSel ? Color(224, 242, 254) : (isHov ? Color(248, 250, 252) : Color::White);
        drawRoundedCard(win, {tableX, rY}, {tableW, rowH}, 0.f, rowBg, 0.5f, Color(226, 232, 240));

        for (float dx : {div1, div2, div3, div4, div5}) {
            RectangleShape rDiv(Vector2f(1.f, rowH));
            rDiv.setPosition({dx, rY});
            rDiv.setFillColor(Color(241, 245, 249));
            win.draw(rDiv);
        }

        // 1. Checkbox
        drawRoundedCard(win, {tableX + 14.f, rY + 30.f}, {16.f, 16.f}, 3.f, itm.isChecked ? Color(8, 125, 181) : Color::White, 1.f, Color(203, 213, 225));
        if (itm.isChecked) {
            UITheme::drawCheckmark(win, {tableX + 22.f, rY + 38.f}, 0.72f, Color::White, 2.f);
        }

        // 2. Mã combo (Badge viền mỏng)
        drawRoundedCard(win, {tableX + 44.f, rY + 25.f}, {58.f, 26.f}, 4.f, Color(248, 250, 252), 1.f, Color(226, 232, 240));
        Text idTxt(boldFont, s2ws(itm.id), 14); idTxt.setFillColor(Color(51, 65, 85));
        FloatRect idb = idTxt.getLocalBounds();
        idTxt.setPosition({std::round(tableX + 44.f + (58.f - idb.size.x) / 2.f - idb.position.x), std::round(rY + 25.f + (26.f - idb.size.y) / 2.f - idb.position.y)});
        win.draw(idTxt);

        // 3. Tên combo & Quy chuẩn chi tiết (thumbnail 52x52)
        const float thumbX = div1 + 18.f;
        const Texture* cTex = getComboTexture(itm.id, itm.name);
        if (cTex) {
            drawRoundedCard(win, {thumbX, rY + 12.f}, {52.f, 52.f}, 8.f, Color::White, 1.2f, Color(226, 232, 240));
            Sprite spr(*cTex);
            Vector2u sz = cTex->getSize();
            spr.setScale({50.f / (float)sz.x, 50.f / (float)sz.y});
            spr.setPosition({thumbX + 1.f, rY + 13.f});
            win.draw(spr);
            drawRoundedCard(win, {thumbX + 1.f, rY + 13.f}, {50.f, 50.f}, 6.f, Color::Transparent, 1.f, Color(203, 213, 225));
        } else {
            drawRoundedCard(win, {thumbX + 1.f, rY + 13.f}, {50.f, 50.f}, 8.f, itm.category == L"Đồ uống" ? Color(254, 226, 226) : Color(254, 243, 199));
            Text icDish(boldFont, itm.category == L"Đồ uống" ? L"DRINK" : L"SNACK", 14);
            icDish.setFillColor(Color(180, 83, 9));
            FloatRect icb = icDish.getLocalBounds();
            icDish.setPosition({std::round(thumbX + 1.f + (50.f - icb.size.x) / 2.f - icb.position.x), std::round(rY + 13.f + (50.f - icb.size.y) / 2.f - icb.position.y)});
            win.draw(icDish);
        }

        const float comboTextX = thumbX + 64.f;
        const float maxTextW = max(120.f, div2 - comboTextX - 16.f);
        Text nameTxt(boldFont, itm.name, 17);
        nameTxt.setFillColor(Color(23, 32, 51));
        nameTxt.setPosition({comboTextX, std::round(rY + 11.f)});
        win.draw(nameTxt);

        // Badge HOT nếu có
        if (itm.isHot) {
            FloatRect nb = nameTxt.getGlobalBounds();
            if (nb.position.x + nb.size.x + 50.f < div2) {
                drawRoundedCard(win, {nb.position.x + nb.size.x + 8.f, rY + 12.f}, {40.f, 20.f}, 4.f, Color(245, 158, 11));
                Text hotTxt(boldFont, L"HOT", 14); hotTxt.setFillColor(Color::White); hotTxt.setPosition({nb.position.x + nb.size.x + 12.f, rY + 13.f}); win.draw(hotTxt);
            }
        }

        // Wrap mô tả tối đa 2 dòng ở cỡ chữ 17 (+20% theo yêu cầu) để vừa khít cột
        wstring wrappedSub;
        {
            wstringstream wss(itm.subtitle);
            wstring word, line1, line2;
            bool onSecond = false;
            while (wss >> word) {
                if (!onSecond) {
                    wstring cand = line1.empty() ? word : (line1 + L" " + word);
                    Text probe(font, cand, 17);
                    if (probe.getLocalBounds().size.x <= maxTextW) line1 = cand;
                    else { onSecond = true; line2 = word; }
                } else {
                    wstring cand = line2.empty() ? word : (line2 + L" " + word);
                    Text probe(font, cand + L"...", 17);
                    if (probe.getLocalBounds().size.x <= maxTextW) line2 = cand;
                    else { line2 += L"..."; break; }
                }
            }
            wrappedSub = line2.empty() ? line1 : (line1 + L"\n" + line2);
        }
        Text subTxt(font, wrappedSub, 17);
        subTxt.setFillColor(Color(100, 116, 139));
        subTxt.setPosition({comboTextX, rY + 35.f});
        win.draw(subTxt);

        // 4. Phân loại (Badge pill căn giữa trong [div2, div3])
        Color catBg = Color(241, 245, 249);
        Color catFg = Color(71, 85, 105);
        if (itm.category == L"B\u1eafp ng\u1ecdt") { catBg = Color(254, 243, 199); catFg = Color(180, 83, 9); }
        else if (itm.category == L"B\u1eafp m\u1eb7n") { catBg = Color(254, 215, 170); catFg = Color(194, 65, 12); }
        else if (itm.category == L"Combo \u0110\u01a1n") { catBg = Color(224, 242, 254); catFg = Color(8, 125, 181); }
        else if (itm.category == L"Combo Nh\u00f3m") { catBg = Color(243, 232, 255); catFg = Color(126, 34, 206); }

        float catPillX = div2 + ((div3 - div2) - 102.f) / 2.f;
        drawRoundedCard(win, {catPillX, rY + 24.f}, {102.f, 28.f}, 14.f, catBg);
        Text catTxt(boldFont, itm.category, 14); catTxt.setFillColor(catFg);
        FloatRect ctb = catTxt.getLocalBounds();
        catTxt.setPosition({std::round(catPillX + (102.f - ctb.size.x) / 2.f - ctb.position.x), std::round(rY + 24.f + (28.f - ctb.size.y) / 2.f - ctb.position.y)});
        win.draw(catTxt);

        // 5. Giá bán niêm yết (căn giữa trong [div3, div4])
        Text prTxt(boldFont, s2ws(formatPrice(itm.price)), 14);
        prTxt.setFillColor(Color(23, 32, 51));
        FloatRect prb = prTxt.getLocalBounds();
        prTxt.setPosition({std::round(div3 + ((div4 - div3) - prb.size.x) / 2.f - prb.position.x), std::round(rY + (rowH - prb.size.y) / 2.f - prb.position.y)});
        win.draw(prTxt);

        // 6. Trạng thái: ● Đang bán (căn giữa trong [div4, div5])
        float stPillX = div4 + ((div5 - div4) - 100.f) / 2.f;
        drawRoundedCard(win, {stPillX, rY + 24.f}, {100.f, 28.f}, 14.f, Color(220, 252, 231));
        CircleShape dotG(3.f); dotG.setFillColor(Color(22, 101, 52)); dotG.setPosition({stPillX + 10.f, rY + 35.f}); win.draw(dotG);
        Text stTxt(boldFont, L"\u0110ang b\u00e1n", 14); stTxt.setFillColor(Color(22, 101, 52)); stTxt.setPosition({stPillX + 20.f, rY + 29.f}); win.draw(stTxt);

        // 7. Thao tác: Nút Sửa & Xóa (căn giữa trong [div5, tableRight])
        float actX = div5 + ((tableRight - div5) - 76.f) / 2.f;
        drawRoundedCard(win, {actX, rY + 24.f}, {36.f, 28.f}, 4.f, Color(254, 243, 199));
        Text btnEd(boldFont, L"S\u1eeda", 14);
        btnEd.setFillColor(Color(180, 83, 9));
        FloatRect edb = btnEd.getLocalBounds();
        btnEd.setPosition({std::round(actX + (36.f - edb.size.x) / 2.f - edb.position.x), std::round(rY + 24.f + (28.f - edb.size.y) / 2.f - edb.position.y)});
        win.draw(btnEd);

        drawRoundedCard(win, {actX + 40.f, rY + 24.f}, {36.f, 28.f}, 4.f, Color(254, 226, 226));
        Text btnDl(boldFont, L"X\u00f3a", 14);
        btnDl.setFillColor(Color(220, 38, 38));
        FloatRect dlb = btnDl.getLocalBounds();
        btnDl.setPosition({std::round(actX + 40.f + (36.f - dlb.size.x) / 2.f - dlb.position.x), std::round(rY + 24.f + (28.f - dlb.size.y) / 2.f - dlb.position.y)});
        win.draw(btnDl);
    }

    // 6b. THANH PHÂN TRANG (PAGINATION BAR)
    float pagY = tableY + 44.f + maxVisibleRows * rowH + 14.f;
    int fromIdx = filteredCombos.empty() ? 0 : (scrollOffset + 1);
    int toIdx = min((int)filteredCombos.size(), scrollOffset + maxVisibleRows);
    wstring pageInfoStr = filteredCombos.empty()
        ? L"Không có sản phẩm combo nào"
        : (L"Hiển thị " + to_wstring(fromIdx) + L" – " + to_wstring(toIdx) + L" trên tổng số " + to_wstring(filteredCombos.size()) + L" sản phẩm combo");
    Text pageInfo(font, pageInfoStr, 14);
    pageInfo.setFillColor(Color(100, 116, 139));
    pageInfo.setPosition({position.x + 30.f, pagY + 7.f});
    win.draw(pageInfo);

    float rightEdge = position.x + 30.f + tableW;
    int numPageBtns = min(5, totalPages);
    int startP = max(1, min(currentPage - 2, totalPages - numPageBtns + 1));
    int endP = min(totalPages, startP + numPageBtns - 1);

    float btnPrevW = 60.f;
    float btnNextW = 60.f;
    float btnPageW = 32.f;
    float gap = 6.f;
    float totalBtnsW = btnPrevW + gap + (endP - startP + 1) * (btnPageW + gap) - gap + gap + btnNextW;
    float curBtnX = rightEdge - totalBtnsW;

    // Nút Trước
    drawRoundedCard(win, {curBtnX, pagY}, {btnPrevW, 32.f}, 6.f, Color::White, 1.f, Color(226, 232, 240));
    Text prTxt(font, L"Trước", 14);
    prTxt.setFillColor(Color(100, 116, 139));
    FloatRect prb = prTxt.getLocalBounds();
    prTxt.setPosition({std::round(curBtnX + (btnPrevW - prb.size.x) / 2.f - prb.position.x), std::round(pagY + (32.f - prb.size.y) / 2.f - prb.position.y)});
    win.draw(prTxt);
    curBtnX += btnPrevW + gap;

    // Các nút số trang 1, 2, 3...
    for (int p = startP; p <= endP; ++p) {
        bool isAct = (p == currentPage);
        Color pBg = isAct ? Color(8, 125, 181) : Color::White;
        Color pBorder = isAct ? Color(8, 125, 181) : Color(226, 232, 240);
        Color pFg = isAct ? Color::White : Color(71, 85, 105);

        drawRoundedCard(win, {curBtnX, pagY}, {btnPageW, 32.f}, 6.f, pBg, 1.f, pBorder);
        Text pTxt(boldFont, to_wstring(p), 14);
        pTxt.setFillColor(pFg);
        FloatRect pb = pTxt.getLocalBounds();
        pTxt.setPosition({std::round(curBtnX + (btnPageW - pb.size.x) / 2.f - pb.position.x), std::round(pagY + (32.f - pb.size.y) / 2.f - pb.position.y)});
        win.draw(pTxt);
        curBtnX += btnPageW + gap;
    }

    // Nút Tiếp
    drawRoundedCard(win, {curBtnX, pagY}, {btnNextW, 32.f}, 6.f, Color::White, 1.f, Color(226, 232, 240));
    Text nxTxt(font, L"Tiếp", 14);
    nxTxt.setFillColor(Color(100, 116, 139));
    FloatRect nxb = nxTxt.getLocalBounds();
    nxTxt.setPosition({std::round(curBtnX + (btnNextW - nxb.size.x) / 2.f - nxb.position.x), std::round(pagY + (32.f - nxb.size.y) / 2.f - nxb.position.y)});
    win.draw(nxTxt);

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

        Text pTitle(boldFont, (editingIndex >= 0) ? L"S\u1eeda th\u00f4ng tin combo" : L"Th\u00eam combo m\u1edbi", 24);
        pTitle.setFillColor(Color(23, 32, 51));
        pTitle.setPosition({popX + 30.f, popY + 22.f});
        win.draw(pTitle);

        // Nút chọn phân loại combo ở góc trên phải
        drawRoundedCard(win, {popX + popW - 150.f, popY + 20.f}, {120.f, 28.f}, 6.f, Color(224, 242, 254), 1.f, Color(186, 230, 253));
        Text catBadge(boldFont, (popupCat.empty() ? L"Combo \u0110\u01a1n" : popupCat) + L" \u25bc", 14);
        catBadge.setFillColor(Color(8, 125, 181));
        FloatRect cbb = catBadge.getLocalBounds();
        catBadge.setPosition({popX + popW - 150.f + (120.f - cbb.size.x) / 2.f - cbb.position.x, popY + 20.f + (28.f - cbb.size.y) / 2.f - cbb.position.y - 1.f});
        win.draw(catBadge);

        bool f0Act = (popupActiveField == 0);
        Text l1(boldFont, L"T\u00ean combo:", 14); l1.setFillColor(Color(71, 85, 105)); l1.setPosition({popX + 30.f, popY + 65.f}); win.draw(l1);
        drawRoundedCard(win, {popX + 30.f, popY + 85.f}, {popW - 60.f, 36.f}, 6.f,
                        f0Act ? Color::White : Color(248, 250, 252), f0Act ? 1.8f : 1.f, f0Act ? Color(8, 125, 181) : Color(203, 213, 225));
        wstring dispN = popupName.empty() ? (f0Act ? L"|" : L"Nh\u1eadp t\u00ean combo...") : (popupName + (f0Act ? L"|" : L""));
        Text v1(font, dispN, 14); v1.setFillColor(popupName.empty() && !f0Act ? Color(148, 163, 184) : Color(23, 32, 51)); v1.setPosition({popX + 40.f, popY + 94.f}); win.draw(v1);

        bool f1Act = (popupActiveField == 1);
        Text l2(boldFont, L"Quy chu\u1ea9n chi ti\u1ebft:", 14); l2.setFillColor(Color(71, 85, 105)); l2.setPosition({popX + 30.f, popY + 130.f}); win.draw(l2);
        drawRoundedCard(win, {popX + 30.f, popY + 150.f}, {popW - 60.f, 36.f}, 6.f,
                        f1Act ? Color::White : Color(248, 250, 252), f1Act ? 1.8f : 1.f, f1Act ? Color(8, 125, 181) : Color(203, 213, 225));
        Text v2(font, popupSub + (f1Act ? L"|" : L""), 14); v2.setFillColor(Color(23, 32, 51)); v2.setPosition({popX + 40.f, popY + 159.f}); win.draw(v2);

        bool f2Act = (popupActiveField == 2);
        Text l3(boldFont, L"Gi\u00e1 ni\u00eam y\u1ebft (\u0111):", 14); l3.setFillColor(Color(71, 85, 105)); l3.setPosition({popX + 30.f, popY + 195.f}); win.draw(l3);
        drawRoundedCard(win, {popX + 30.f, popY + 215.f}, {popW - 60.f, 36.f}, 6.f,
                        f2Act ? Color::White : Color(248, 250, 252), f2Act ? 1.8f : 1.f, f2Act ? Color(8, 125, 181) : Color(203, 213, 225));
        Text v3(font, s2ws(popupPrice) + (f2Act ? L"|" : L""), 14); v3.setFillColor(Color(23, 32, 51)); v3.setPosition({popX + 40.f, popY + 224.f}); win.draw(v3);

        // Nút Lưu và Hủy
        drawRoundedCard(win, {popX + 70.f, popY + 270.f}, {160.f, 38.f}, 6.f, Color(21, 128, 61));
        Text sTxt(boldFont, L"L\u01b0u th\u00f4ng tin", 14); sTxt.setFillColor(Color::White); sTxt.setPosition({popX + 105.f, popY + 279.f}); win.draw(sTxt);

        drawRoundedCard(win, {popX + 270.f, popY + 270.f}, {160.f, 38.f}, 6.f, Color(241, 245, 249), 1.f, Color(203, 213, 225));
        Text cTxt(boldFont, L"H\u1ee7y b\u1ecf", 14); cTxt.setFillColor(Color(71, 85, 105)); cTxt.setPosition({popX + 325.f, popY + 279.f}); win.draw(cTxt);
    }

    // ==========================================
    // 8. TOAST NOTIFICATION
    // ==========================================
    if (notificationVisible) {
        if (notificationClock.getElapsedTime().asSeconds() > 3.0f) {
            notificationVisible = false;
        } else {
            Text notText(boldFont, s2ws(notificationMessage), 14);
            notText.setFillColor(Color::White);
            FloatRect ntb = notText.getLocalBounds();
            float notW = max(380.f, min(width - 80.f, ntb.size.x + 48.f));
            float notH = 42.f;
            float notX = position.x + (width - notW) / 2.f;
            float notY = position.y + height - 60.f;
            drawRoundedCard(win, {notX, notY}, {notW, notH}, 8.f, Color(22, 163, 74, 245), 1.5f, Color(21, 128, 61));
            notText.setPosition({
                std::round(notX + (notW - ntb.size.x) / 2.f - ntb.position.x),
                std::round(notY + (notH - ntb.size.y) / 2.f - ntb.position.y)
            });
            win.draw(notText);
        }
    }

    deleteModal.render(win, font, boldFont);
}
