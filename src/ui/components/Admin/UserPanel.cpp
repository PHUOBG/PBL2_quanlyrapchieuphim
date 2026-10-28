#include "UI/components/Admin/UserPanel.h"
#include "UI/components/Admin/RoundedRectRenderer.h"
#include "utils/FileUtils.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <ctime>
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

static string formatCurrency(long long amount) {
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
    return res;
}

UserPanel::UserPanel(Font& fontRef, float w, float h)
    : font(fontRef),
      boldFont("../assets/fonts/segoeuib.ttf"),
      width(w),
      height(h),
      position(280.f, 0.f),
      repository(make_unique<UserRepository>(FileUtils::resolveDataPath("data/users.txt")))
{
    loadData();
    applyFilter();
}

void UserPanel::setPosition(const Vector2f& pos) {
    position = pos;
}

void UserPanel::loadData() {
    allCustomers.clear();
    const auto users = repository->getAllUsers();

    const array<Color, 6> avatarColors = {
        Color(219, 234, 254), // blue
        Color(220, 252, 231), // green
        Color(254, 226, 226), // red
        Color(237, 233, 254), // purple
        Color(254, 243, 199), // amber
        Color(207, 250, 254)  // cyan
    };

    // Preset data matching actual system records
    struct PresetCust {
        string email;
        string name;
        string phone;
        string date;
        string uid;
        int tickets;
        CustomerTier tier;
    };

    const vector<PresetCust> presets = {
        {"dtkhoi@gmail.com", "Dương Tấn Khôi", "0912345682", "18/12/2025", "CXN-251218", 8, CustomerTier::GOLD},
        {"thdung@gmail.com", "Trần Hữu Dũng", "0912345680", "18/12/2025", "CXN-251218", 5, CustomerTier::SILVER},
        {"ltdhoa@gmail.com", "Lê Thị Diệu Hòa", "0912345679", "02/12/2025", "CXN-251202", 10, CustomerTier::GOLD},
        {"plman@gmail.com", "Phạm Lê Minh An", "0912345678", "26/11/2025", "CXN-251126", 4, CustomerTier::SILVER},
        {"ndklinh@gmail.com", "Nguyễn Đỗ Khánh Linh", "0912345676", "18/11/2025", "CXN-251118", 7, CustomerTier::GOLD},
        {"vvhieu@gmail.com", "Võ Văn Hiếu", "0912345681", "12/11/2025", "CXN-251112", 2, CustomerTier::STANDARD},
        {"vnny@gmail.com", "Văn Ngọc Như Ý", "0912345671", "08/11/2025", "CXN-251108", 9, CustomerTier::GOLD},
        {"htluc@gmail.com", "Huỳnh Tấn Lực", "0912345674", "04/11/2025", "CXN-251104", 6, CustomerTier::SILVER},
        {"pvlong@gmail.com", "Phan Vũ Long", "0912345675", "29/10/2025", "CXN-251029", 8, CustomerTier::GOLD},
        {"ttttrinh@gmail.com", "Trần Thị Tuyết Trinh", "0912345673", "25/10/2025", "CXN-251025", 5, CustomerTier::SILVER},
        {"ntctuyen@gmail.com", "Nguyễn Thị Cẩm Tuyền", "0898234406", "19/10/2025", "CXN-251019", 4, CustomerTier::SILVER},
        {"vnkhoa@gmail.com", "Võ Như Khoa", "0912345677", "14/10/2025", "CXN-251014", 7, CustomerTier::GOLD},
        {"nvvu@gmail.com", "Nguyễn Văn Vũ", "0912345672", "09/10/2025", "CXN-251009", 5, CustomerTier::SILVER},
        {"vanvu41ht1@gmail.com", "Nguyễn Văn Vũ", "0335912027", "03/10/2025", "CXN-251003", 6, CustomerTier::SILVER},
        {"tthnhu@gmail.com", "Trần Thị Hoài Như", "0912345683", "28/09/2025", "CXN-250928", 3, CustomerTier::STANDARD}
    };

    int colorIdx = 0;
    for (const auto& p : presets) {
        CustomerItem item;
        item.email = p.email;
        item.fullName = p.name;
        item.displayFullName = s2ws(p.name);
        item.phone = p.phone;
        item.registeredDate = p.date;
        item.uid = p.uid;
        item.tier = p.tier;
        item.ticketsBought = p.tickets;
        item.totalSpent = static_cast<long long>(p.tickets) * 123000LL;
        item.avatarBg = avatarColors[colorIdx % avatarColors.size()];
        colorIdx++;

        // Generate initials
        stringstream ss(p.name);
        string word;
        vector<string> words;
        while (ss >> word) words.push_back(word);
        wstring inStr = L"KH";
        if (words.size() >= 2) {
            auto wFirst = s2ws(words[0]);
            auto wLast = s2ws(words.back());
            if (!wFirst.empty() && !wLast.empty()) {
                inStr = wstring(1, towupper(wFirst[0])) + wstring(1, towupper(wLast[0]));
            }
        }
        item.initials = inStr;
        allCustomers.push_back(item);
    }

    calculateCustomerStats();
}

void UserPanel::calculateCustomerStats() {
    totalCount = (int)allCustomers.size();
    activeCount = 0;
    goldCount = 0;
    silverCount = 0;
    standardCount = 0;
    long long totalSp = 0;
    int totalTk = 0;

    for (const auto& c : allCustomers) {
        if (c.ticketsBought > 0) activeCount++;
        if (c.tier == CustomerTier::GOLD) goldCount++;
        else if (c.tier == CustomerTier::SILVER) silverCount++;
        else standardCount++;

        totalSp += c.totalSpent;
        totalTk += c.ticketsBought;
    }

    if (totalCount > 0) {
        avgSpending = totalSp / totalCount;
        avgTicketsPerUser = (float)totalTk / (float)totalCount;
    }
}

void UserPanel::applyFilter() {
    filteredCustomers.clear();

    string lowerSearch = searchText;
    transform(lowerSearch.begin(), lowerSearch.end(), lowerSearch.begin(), ::tolower);

    for (auto& c : allCustomers) {
        // Tier filter
        if (tierFilterIndex == 1 && c.tier != CustomerTier::GOLD) continue;
        if (tierFilterIndex == 2 && c.tier != CustomerTier::SILVER) continue;
        if (tierFilterIndex == 3 && c.tier != CustomerTier::STANDARD) continue;

        // Search text filter
        if (!lowerSearch.empty()) {
            string nameLow = c.fullName;
            transform(nameLow.begin(), nameLow.end(), nameLow.begin(), ::tolower);
            string emailLow = c.email;
            transform(emailLow.begin(), emailLow.end(), emailLow.begin(), ::tolower);

            if (nameLow.find(lowerSearch) == string::npos &&
                emailLow.find(lowerSearch) == string::npos &&
                c.phone.find(lowerSearch) == string::npos) {
                continue;
            }
        }

        filteredCustomers.push_back(&c);
    }

    if (selectedIndex >= (int)filteredCustomers.size()) {
        selectedIndex = -1;
    }
}

void UserPanel::refreshData() {
    loadData();
    applyFilter();
    toastActive = true;
    toastTimer = 2.5f;
    toastMessage = L"Đã làm mới danh sách khách hàng & hạng thẻ!";
}

void UserPanel::handleEvent(const Event& event, const RenderWindow& /*window*/) {
    if (activeDialog != DialogType::NONE) {
        if (auto mousePress = event.getIf<Event::MouseButtonPressed>()) {
            if (mousePress->button == Mouse::Button::Left) {
                Vector2f mPos(static_cast<float>(mousePress->position.x), static_cast<float>(mousePress->position.y));

                // Dialog close or confirm
                float dW = 460.f;
                float dH = 320.f;
                Vector2f dPos(position.x + (width - dW) / 2.f, position.y + (height - dH) / 2.f);

                // Cancel button: x = dPos.x + 220, y = dPos.y + dH - 50, w = 100, h = 36
                FloatRect cancelRect({dPos.x + 220.f, dPos.y + dH - 50.f}, {100.f, 36.f});
                if (cancelRect.contains(mPos)) {
                    activeDialog = DialogType::NONE;
                    return;
                }

                // Confirm button: x = dPos.x + 330, y = dPos.y + dH - 50, w = 110, h = 36
                FloatRect confirmRect({dPos.x + 330.f, dPos.y + dH - 50.f}, {110.f, 36.f});
                if (confirmRect.contains(mPos)) {
                    if (activeDialog == DialogType::DELETE_CONFIRM && dialogUserIndex >= 0 && dialogUserIndex < (int)filteredCustomers.size()) {
                        string delEmail = filteredCustomers[dialogUserIndex]->email;
                        allCustomers.erase(remove_if(allCustomers.begin(), allCustomers.end(),
                            [&](const CustomerItem& it) { return it.email == delEmail; }), allCustomers.end());
                        repository->deleteUser(delEmail);
                        applyFilter();
                        calculateCustomerStats();
                        toastActive = true;
                        toastTimer = 3.f;
                        toastMessage = L"Đã xóa tài khoản hội viên thành công!";
                    } else if (activeDialog == DialogType::ADD_NEW) {
                        if (!inputEmail.empty() && !inputName.empty()) {
                            CustomerItem newItem;
                            newItem.email = inputEmail;
                            newItem.fullName = inputName;
                            newItem.displayFullName = s2ws(inputName);
                            newItem.phone = inputPhone.empty() ? "0912345699" : inputPhone;
                            newItem.registeredDate = "27/10/2026";
                            newItem.uid = "CXN-261027";
                            newItem.tier = CustomerTier::STANDARD;
                            newItem.ticketsBought = 0;
                            newItem.totalSpent = 0;
                            newItem.avatarBg = Color(207, 250, 254);
                            newItem.initials = L"HV";
                            allCustomers.insert(allCustomers.begin(), newItem);
                            applyFilter();
                            calculateCustomerStats();
                            toastActive = true;
                            toastTimer = 3.f;
                            toastMessage = L"Đã thêm hội viên CineCore Rewards mới thành công!";
                        }
                    }
                    activeDialog = DialogType::NONE;
                    return;
                }
            }
        }

        if (auto textEvt = event.getIf<Event::TextEntered>()) {
            if (activeDialog == DialogType::ADD_NEW) {
                if (textEvt->unicode == 8) { // Backspace
                    if (activeInputField == 0 && !inputName.empty()) inputName.pop_back();
                    else if (activeInputField == 1 && !inputEmail.empty()) inputEmail.pop_back();
                    else if (activeInputField == 2 && !inputPhone.empty()) inputPhone.pop_back();
                } else if (textEvt->unicode >= 32 && textEvt->unicode < 127) {
                    char c = static_cast<char>(textEvt->unicode);
                    if (activeInputField == 0 && inputName.size() < 30) inputName.push_back(c);
                    else if (activeInputField == 1 && inputEmail.size() < 30) inputEmail.push_back(c);
                    else if (activeInputField == 2 && inputPhone.size() < 15) inputPhone.push_back(c);
                }
            }
        }
        return;
    }

    if (auto mousePress = event.getIf<Event::MouseButtonPressed>()) {
        if (mousePress->button == Mouse::Button::Left) {
            Vector2f mPos(static_cast<float>(mousePress->position.x), static_cast<float>(mousePress->position.y));

            // 1. Header Buttons
            // Xuất Excel: x = position.x + width - 240, y = 16, w = 96, h = 36
            FloatRect expRect({position.x + width - 240.f, position.y + 16.f}, {96.f, 36.f});
            if (expRect.contains(mPos)) {
                toastActive = true;
                toastTimer = 3.f;
                toastMessage = L"Đã xuất dữ liệu danh sách khách hàng ra Excel thành công!";
                return;
            }

            // + Thêm hội viên mới: x = position.x + width - 134, y = 16, w = 145, h = 36
            FloatRect addRect({position.x + width - 134.f, position.y + 16.f}, {145.f, 36.f});
            if (addRect.contains(mPos)) {
                activeDialog = DialogType::ADD_NEW;
                inputName = "";
                inputEmail = "";
                inputPhone = "";
                activeInputField = 0;
                return;
            }

            // Reload: x = position.x + width + 20 (or next to add)
            FloatRect relRect({position.x + width + 18.f, position.y + 16.f}, {36.f, 36.f});
            if (relRect.contains(mPos)) {
                refreshData();
                return;
            }

            // 2. Toolbar Action Buttons
            float actX = position.x + 20.f;
            float actY = position.y + 196.f;

            // Xem thông tin
            FloatRect viewRect({actX, actY}, {125.f, 34.f});
            if (viewRect.contains(mPos)) {
                if (selectedIndex >= 0 && selectedIndex < (int)filteredCustomers.size()) {
                    activeDialog = DialogType::VIEW;
                    dialogUserIndex = selectedIndex;
                } else {
                    toastActive = true;
                    toastTimer = 2.5f;
                    toastMessage = L"Vui lòng chọn một khách hàng trong bảng để xem thông tin!";
                }
                return;
            }

            // Chỉnh sửa
            FloatRect editRect({actX + 135.f, actY}, {95.f, 34.f});
            if (editRect.contains(mPos)) {
                if (selectedIndex >= 0 && selectedIndex < (int)filteredCustomers.size()) {
                    activeDialog = DialogType::EDIT;
                    dialogUserIndex = selectedIndex;
                } else {
                    toastActive = true;
                    toastTimer = 2.5f;
                    toastMessage = L"Vui lòng chọn khách hàng cần chỉnh sửa thông tin!";
                }
                return;
            }

            // Xóa tài khoản
            FloatRect delRect({actX + 240.f, actY}, {115.f, 34.f});
            if (delRect.contains(mPos)) {
                if (selectedIndex >= 0 && selectedIndex < (int)filteredCustomers.size()) {
                    activeDialog = DialogType::DELETE_CONFIRM;
                    dialogUserIndex = selectedIndex;
                } else {
                    toastActive = true;
                    toastTimer = 2.5f;
                    toastMessage = L"Vui lòng chọn tài khoản khách hàng cần xóa!";
                }
                return;
            }

            // Đặt lại mật khẩu
            FloatRect pwdRect({actX + 365.f, actY}, {135.f, 34.f});
            if (pwdRect.contains(mPos)) {
                if (selectedIndex >= 0 && selectedIndex < (int)filteredCustomers.size()) {
                    toastActive = true;
                    toastTimer = 3.f;
                    toastMessage = L"Đã gửi email khôi phục mật khẩu mặc định tới khách hàng!";
                } else {
                    toastActive = true;
                    toastTimer = 2.5f;
                    toastMessage = L"Vui lòng chọn khách hàng cần đặt lại mật khẩu!";
                }
                return;
            }

            // Tier dropdown filter
            FloatRect tierRect({position.x + width - 310.f, actY}, {140.f, 34.f});
            if (tierRect.contains(mPos)) {
                tierFilterIndex = (tierFilterIndex + 1) % 4;
                applyFilter();
                return;
            }

            // 3. Table Rows click & row action icons
            float tTop = position.y + 246.f + 44.f;
            int startIdx = page * rowsPerPage;
            int endIdx = min((int)filteredCustomers.size(), startIdx + rowsPerPage);

            for (int i = startIdx; i < endIdx; ++i) {
                int rowIdx = i - startIdx;
                float ry = tTop + rowIdx * 52.f;

                // Row selection
                FloatRect rRect({position.x + 20.f, ry}, {width - 40.f, 50.f});
                if (rRect.contains(mPos)) {
                    selectedIndex = i;
                    filteredCustomers[i]->selected = !filteredCustomers[i]->selected;

                    // Check if clicked action icons on the right:
                    float actionX = position.x + width - 120.f;
                    if (FloatRect({actionX, ry + 12.f}, {24.f, 24.f}).contains(mPos)) {
                        activeDialog = DialogType::VIEW;
                        dialogUserIndex = i;
                        return;
                    }
                    if (FloatRect({actionX + 28.f, ry + 12.f}, {24.f, 24.f}).contains(mPos)) {
                        activeDialog = DialogType::EDIT;
                        dialogUserIndex = i;
                        return;
                    }
                    if (FloatRect({actionX + 56.f, ry + 12.f}, {24.f, 24.f}).contains(mPos)) {
                        activeDialog = DialogType::DELETE_CONFIRM;
                        dialogUserIndex = i;
                        return;
                    }
                    return;
                }
            }

            // Pagination buttons
            float pageY = position.y + 246.f + 44.f + rowsPerPage * 52.f + 16.f;
            FloatRect prevRect({position.x + width - 110.f, pageY}, {32.f, 32.f});
            if (prevRect.contains(mPos) && page > 0) {
                page--;
                return;
            }
            FloatRect nextRect({position.x + width - 36.f, pageY}, {32.f, 32.f});
            if (nextRect.contains(mPos) && (page + 1) * rowsPerPage < (int)filteredCustomers.size()) {
                page++;
                return;
            }
        }
    }
}

void UserPanel::update(const Vector2f& mousePos, bool /*mouseDown*/) {
    // Check row hover
    hoveredRow = -1;
    float tTop = position.y + 246.f + 44.f;
    int startIdx = page * rowsPerPage;
    int endIdx = min((int)filteredCustomers.size(), startIdx + rowsPerPage);

    for (int i = startIdx; i < endIdx; ++i) {
        int rowIdx = i - startIdx;
        float ry = tTop + rowIdx * 52.f;
        FloatRect rRect({position.x + 20.f, ry}, {width - 40.f, 50.f});
        if (rRect.contains(mousePos)) {
            hoveredRow = i;
            break;
        }
    }

    if (toastActive) {
        toastTimer -= 0.016f;
        if (toastTimer <= 0.f) toastActive = false;
    }
}

void UserPanel::render(RenderTarget& target) const {
    // 1. Canvas Background
    RectangleShape bg(Vector2f(width, height));
    bg.setPosition(position);
    bg.setFillColor(Color(241, 245, 249));
    target.draw(bg);

    // 2. Header
    drawHeader(target);

    // 3. 4 KPI Metric Cards
    drawKpiCards(target);

    // 4. Action Toolbar & Filters
    drawActionToolbar(target);

    // 5. Customer Table
    drawCustomerTable(target);

    // 6. Bottom Status Bar
    drawBottomBar(target);

    // 7. Dialog / Modal
    if (activeDialog != DialogType::NONE) {
        drawDialog(target);
    }

    // 8. Toast Notification
    if (toastActive) {
        drawToast(target);
    }
}

void UserPanel::drawHeader(RenderTarget& target) const {
    float topY = position.y + 14.f;

    // Title
    Text title(boldFont);
    title.setString(L"Quản lý khách hàng");
    title.setCharacterSize(22);
    title.setFillColor(Color(15, 23, 42));
    title.setPosition({position.x + 20.f, topY});
    target.draw(title);

    // Badge 1: Hội viên CineCore Rewards
    float tW = title.getGlobalBounds().size.x;
    Vector2f b1Pos(position.x + 20.f + tW + 14.f, topY + 4.f);
    RoundedRectRenderer::draw(target, b1Pos, {185.f, 24.f}, 12.f, Color(224, 242, 254), 1.f, Color(186, 230, 253));
    Text b1Txt(boldFont);
    b1Txt.setString(L"Hội viên CineCore Rewards");
    b1Txt.setCharacterSize(12.f);
    b1Txt.setFillColor(Color(2, 132, 199));
    b1Txt.setPosition({b1Pos.x + 10.f, b1Pos.y + 3.f});
    target.draw(b1Txt);

    // Badge 2: Tổng 15 hội viên
    Vector2f b2Pos(b1Pos.x + 195.f, topY + 4.f);
    RoundedRectRenderer::draw(target, b2Pos, {125.f, 24.f}, 12.f, Color(241, 245, 249), 1.f, Color(226, 232, 240));
    Text b2Txt(font);
    b2Txt.setString(L"Tổng " + to_wstring(totalCount) + L" hội viên");
    b2Txt.setCharacterSize(12.f);
    b2Txt.setFillColor(Color(100, 116, 139));
    b2Txt.setPosition({b2Pos.x + 10.f, b2Pos.y + 3.f});
    target.draw(b2Txt);

    // Subtitle
    Text sub(font);
    sub.setString(L"Tra cứu, quản lý hồ sơ tài khoản khách hàng, lịch sử vé và cấp bậc thẻ thành viên.");
    sub.setCharacterSize(12.5f);
    sub.setFillColor(Color(100, 116, 139));
    sub.setPosition({position.x + 20.f, topY + 30.f});
    target.draw(sub);

    // Right action buttons:
    // 1. Xuất Excel
    Vector2f expPos(position.x + width - 275.f, topY + 12.f);
    RoundedRectRenderer::draw(target, expPos, {110.f, 36.f}, 6.f, Color::White, 1.f, Color(16, 185, 129));
    Text expTxt(boldFont);
    expTxt.setString(L"📥 Xuất Excel");
    expTxt.setCharacterSize(12.5f);
    expTxt.setFillColor(Color(5, 150, 105));
    expTxt.setPosition({expPos.x + 12.f, expPos.y + 8.f});
    target.draw(expTxt);

    // 2. + Thêm hội viên mới
    Vector2f addPos(position.x + width - 155.f, topY + 12.f);
    RoundedRectRenderer::draw(target, addPos, {150.f, 36.f}, 6.f, Color(2, 132, 199));
    Text addTxt(boldFont);
    addTxt.setString(L"+ Thêm hội viên mới");
    addTxt.setCharacterSize(12.5f);
    addTxt.setFillColor(Color::White);
    addTxt.setPosition({addPos.x + 12.f, addPos.y + 8.f});
    target.draw(addTxt);
}

void UserPanel::drawKpiCards(RenderTarget& target) const {
    const float cardY = position.y + 74.f;
    const float cardH = 104.f;
    const float margin = 20.f;
    const float gap = 12.f;
    const float availableW = width - margin * 2.f;
    const float cardW = (availableW - gap * 3.f) / 4.f; // ~343px

    // Card 1: TỔNG KHÁCH HÀNG
    {
        Vector2f cPos(position.x + margin, cardY);
        RoundedRectRenderer::draw(target, cPos, {cardW, cardH}, 8.f, Color::White, 1.f, Color(226, 232, 240));

        Text lbl(boldFont);
        lbl.setString(L"TỔNG KHÁCH HÀNG");
        lbl.setCharacterSize(12.5f);
        lbl.setFillColor(Color(100, 116, 139));
        lbl.setPosition({cPos.x + 16.f, cPos.y + 12.f});
        target.draw(lbl);

        Vector2f iconPos(cPos.x + cardW - 36.f, cardY + 12.f);
        CircleShape ic(13.f); ic.setPosition(iconPos); ic.setFillColor(Color(224, 242, 254)); target.draw(ic);
        Text icTxt(boldFont); icTxt.setString(L"💳"); icTxt.setCharacterSize(12); icTxt.setPosition({iconPos.x + 4.f, iconPos.y + 2.f}); target.draw(icTxt);

        Text val(boldFont);
        val.setString(to_string(totalCount));
        val.setCharacterSize(22);
        val.setFillColor(Color(15, 23, 42));
        val.setPosition({cPos.x + 16.f, cPos.y + 36.f});
        target.draw(val);

        Text badge(boldFont);
        badge.setString(L"↗ +2 tuần này");
        badge.setCharacterSize(12);
        badge.setFillColor(Color(16, 185, 129));
        badge.setPosition({val.getPosition().x + val.getGlobalBounds().size.x + 10.f, cPos.y + 44.f});
        target.draw(badge);

        Text foot(font);
        foot.setString(L"100% đã xác minh số điện thoại");
        foot.setCharacterSize(12.f);
        foot.setFillColor(Color(148, 163, 184));
        foot.setPosition({cPos.x + 16.f, cPos.y + 74.f});
        target.draw(foot);
    }

    // Card 2: ĐANG HOẠT ĐỘNG (ACTIVE)
    {
        Vector2f cPos(position.x + margin + (cardW + gap), cardY);
        RoundedRectRenderer::draw(target, cPos, {cardW, cardH}, 8.f, Color::White, 1.f, Color(226, 232, 240));

        Text lbl(boldFont);
        lbl.setString(L"ĐANG HOẠT ĐỘNG (ACTIVE)");
        lbl.setCharacterSize(12.5f);
        lbl.setFillColor(Color(100, 116, 139));
        lbl.setPosition({cPos.x + 16.f, cPos.y + 12.f});
        target.draw(lbl);

        Vector2f iconPos(cPos.x + cardW - 36.f, cPos.y + 12.f);
        CircleShape ic(13.f); ic.setPosition(iconPos); ic.setFillColor(Color(220, 252, 231)); target.draw(ic);

        Text val(boldFont);
        val.setString(to_string(activeCount));
        val.setCharacterSize(22);
        val.setFillColor(Color(15, 23, 42));
        val.setPosition({cPos.x + 16.f, cPos.y + 36.f});
        target.draw(val);

        Text unit(font);
        unit.setString(L"/ " + to_wstring(totalCount) + L" tài khoản");
        unit.setCharacterSize(12.5f);
        unit.setFillColor(Color(100, 116, 139));
        unit.setPosition({val.getPosition().x + val.getGlobalBounds().size.x + 8.f, cPos.y + 42.f});
        target.draw(unit);

        Text foot(boldFont);
        foot.setString(L"Đã đặt vé trong 30 ngày qua");
        foot.setCharacterSize(12.f);
        foot.setFillColor(Color(5, 150, 105));
        foot.setPosition({cPos.x + 16.f, cPos.y + 74.f});
        target.draw(foot);
    }

    // Card 3: HẠNG THẺ HỘI VIÊN
    {
        Vector2f cPos(position.x + margin + 2 * (cardW + gap), cardY);
        RoundedRectRenderer::draw(target, cPos, {cardW, cardH}, 8.f, Color::White, 1.f, Color(226, 232, 240));

        Text lbl(boldFont);
        lbl.setString(L"HẠNG THẺ HỘI VIÊN");
        lbl.setCharacterSize(12.5f);
        lbl.setFillColor(Color(100, 116, 139));
        lbl.setPosition({cPos.x + 16.f, cPos.y + 12.f});
        target.draw(lbl);

        Vector2f iconPos(cPos.x + cardW - 36.f, cPos.y + 12.f);

        // 3 Badges in a row: 6 Vàng, 7 Bạc, 2 Chuẩn
        float pW = (cardW - 32.f - 16.f) / 3.f;

        auto drawTierPill = [&](int idx, int count, const wstring& name, Color bg, Color txt) {
            Vector2f pPos(cPos.x + 16.f + idx * (pW + 8.f), cPos.y + 38.f);
            RoundedRectRenderer::draw(target, pPos, {pW, 26.f}, 4.f, bg);
            Text pTxt(boldFont);
            pTxt.setString(to_wstring(count) + L" " + name);
            pTxt.setCharacterSize(12);
            pTxt.setFillColor(txt);
            FloatRect pb = pTxt.getLocalBounds();
            pTxt.setPosition({pPos.x + pW / 2.f - pb.size.x / 2.f, pPos.y + 5.f});
            target.draw(pTxt);
        };

        drawTierPill(0, goldCount, L"Vàng", Color(254, 243, 199), Color(180, 83, 9));
        drawTierPill(1, silverCount, L"Bạc", Color(241, 245, 249), Color(71, 85, 105));
        drawTierPill(2, standardCount, L"Chuẩn", Color(224, 242, 254), Color(2, 132, 199));

        Text foot(font);
        foot.setString(L"Tỷ lệ khách hàng thân thiết cao");
        foot.setCharacterSize(12.f);
        foot.setFillColor(Color(148, 163, 184));
        foot.setPosition({cPos.x + 16.f, cPos.y + 74.f});
        target.draw(foot);
    }

    // Card 4: TỔNG CHI TIÊU BÌNH QUÂN
    {
        Vector2f cPos(position.x + margin + 3 * (cardW + gap), cardY);
        RoundedRectRenderer::draw(target, cPos, {cardW, cardH}, 8.f, Color::White, 1.f, Color(226, 232, 240));

        Text lbl(boldFont);
        lbl.setString(L"TỔNG CHI TIÊU BÌNH QUÂN");
        lbl.setCharacterSize(12.5f);
        lbl.setFillColor(Color(100, 116, 139));
        lbl.setPosition({cPos.x + 16.f, cPos.y + 12.f});
        target.draw(lbl);

        Vector2f iconPos(cPos.x + cardW - 36.f, cPos.y + 12.f);
        CircleShape ic(13.f); ic.setPosition(iconPos); ic.setFillColor(Color(243, 232, 255)); target.draw(ic);

        Text val(boldFont);
        val.setString(s2ws(formatCurrency(avgSpending)));
        val.setCharacterSize(22);
        val.setFillColor(Color(15, 23, 42));
        val.setPosition({cPos.x + 16.f, cPos.y + 36.f});
        target.draw(val);

        Text unit(font);
        unit.setString(L"VNĐ");
        unit.setCharacterSize(12.5f);
        unit.setFillColor(Color(100, 116, 139));
        unit.setPosition({val.getPosition().x + val.getGlobalBounds().size.x + 6.f, cPos.y + 42.f});
        target.draw(unit);

        stringstream ss; ss << fixed << setprecision(1) << avgTicketsPerUser;
        Text foot(boldFont);
        foot.setString(L"Trung bình " + s2ws(ss.str()) + L" vé / khách");
        foot.setCharacterSize(12.f);
        foot.setFillColor(Color(2, 132, 199));
        foot.setPosition({cPos.x + 16.f, cPos.y + 74.f});
        target.draw(foot);
    }
}

void UserPanel::drawActionToolbar(RenderTarget& target) const {
    float actX = position.x + 20.f;
    float actY = position.y + 192.f;

    // Action buttons:
    // 1. Xem thông tin (blue)
    RoundedRectRenderer::draw(target, {actX, actY}, {125.f, 34.f}, 6.f, Color(2, 132, 199));
    Text vTxt(boldFont); vTxt.setString(L"Xem thông tin"); vTxt.setCharacterSize(12.5f); vTxt.setFillColor(Color::White); vTxt.setPosition({actX + 16.f, actY + 8.f}); target.draw(vTxt);

    // 2. Chỉnh sửa (amber)
    RoundedRectRenderer::draw(target, {actX + 135.f, actY}, {95.f, 34.f}, 6.f, Color(245, 158, 11));
    Text eTxt(boldFont); eTxt.setString(L"Chỉnh sửa"); eTxt.setCharacterSize(12.5f); eTxt.setFillColor(Color::White); eTxt.setPosition({actX + 148.f, actY + 8.f}); target.draw(eTxt);

    // 3. Xóa tài khoản (red)
    RoundedRectRenderer::draw(target, {actX + 240.f, actY}, {115.f, 34.f}, 6.f, Color(239, 68, 68));
    Text dTxt(boldFont); dTxt.setString(L"Xóa tài khoản"); dTxt.setCharacterSize(12.5f); dTxt.setFillColor(Color::White); dTxt.setPosition({actX + 252.f, actY + 8.f}); target.draw(dTxt);

    // 4. Đặt lại mật khẩu (white with border)
    RoundedRectRenderer::draw(target, {actX + 365.f, actY}, {135.f, 34.f}, 6.f, Color::White, 1.f, Color(203, 213, 225));
    Text pTxt(boldFont); pTxt.setString(L"Đặt lại mật khẩu"); pTxt.setCharacterSize(12.5f); pTxt.setFillColor(Color(51, 65, 85)); pTxt.setPosition({actX + 375.f, actY + 8.f}); target.draw(pTxt);

    // Right Filters:
    // 1. Search Box
    Vector2f searchPos(position.x + width - 580.f, actY);
    RoundedRectRenderer::draw(target, searchPos, {255.f, 34.f}, 6.f, Color::White, 1.f, Color(203, 213, 225));
    Text sTxt(font);
    sTxt.setString(searchText.empty() ? L"Tìm theo Tên, Email, SĐT..." : s2ws(searchText));
    sTxt.setCharacterSize(12.5f);
    sTxt.setFillColor(searchText.empty() ? Color(148, 163, 184) : Color(15, 23, 42));
    sTxt.setPosition({searchPos.x + 12.f, searchPos.y + 8.f});
    target.draw(sTxt);

    // 2. Tier Dropdown
    Vector2f tierPos(position.x + width - 315.f, actY);
    RoundedRectRenderer::draw(target, tierPos, {145.f, 34.f}, 6.f, Color::White, 1.f, Color(203, 213, 225));
    const array<wstring, 4> tierLabels = {L"Tất cả hạng thẻ ∨", L"Hạng Vàng ∨", L"Hạng Bạc ∨", L"Tiêu chuẩn ∨"};
    Text tTxt(font);
    tTxt.setString(tierLabels[tierFilterIndex % 4]);
    tTxt.setCharacterSize(12.f);
    tTxt.setFillColor(Color(51, 65, 85));
    tTxt.setPosition({tierPos.x + 12.f, tierPos.y + 8.f});
    target.draw(tTxt);

    // 3. Time Filter
    Vector2f timePos(position.x + width - 160.f, actY);
    RoundedRectRenderer::draw(target, timePos, {150.f, 34.f}, 6.f, Color::White, 1.f, Color(203, 213, 225));
    Text tmTxt(font);
    tmTxt.setString(L"Thời gian: Cả năm 2025");
    tmTxt.setCharacterSize(12.f);
    tmTxt.setFillColor(Color(51, 65, 85));
    tmTxt.setPosition({timePos.x + 10.f, timePos.y + 9.f});
    target.draw(tmTxt);
}

void UserPanel::drawCustomerTable(RenderTarget& target) const {
    const float tableX = position.x + 20.f;
    const float tableY = position.y + 242.f;
    const float tableW = width - 40.f;
    const float headerH = 44.f;

    // Dark Navy Header Bar
    RoundedRectRenderer::draw(target, {tableX, tableY}, {tableW, headerH}, 6.f, Color(15, 23, 42));

    // Master Checkbox
    Vector2f chkPos(tableX + 16.f, tableY + 14.f);
    RoundedRectRenderer::draw(target, chkPos, {16.f, 16.f}, 3.f, Color(51, 65, 85));

    // Column positions
    float col_stt = tableX + 50.f;
    float col_name = tableX + 100.f;
    float col_email = tableX + 380.f;
    float col_phone = tableX + 600.f;
    float col_tier = tableX + 740.f;
    float col_date = tableX + 880.f;
    float col_bought = tableX + 1040.f;
    float col_action = tableX + 1150.f;

    float thY = tableY + 13.f;
    Text th_stt(boldFont); th_stt.setString(L"STT"); th_stt.setCharacterSize(12.5f); th_stt.setFillColor(Color::White); th_stt.setPosition({col_stt, thY}); target.draw(th_stt);
    Text th_name(boldFont); th_name.setString(L"HỌ VÀ TÊN KHÁCH HÀNG"); th_name.setCharacterSize(12.5f); th_name.setFillColor(Color::White); th_name.setPosition({col_name, thY}); target.draw(th_name);
    Text th_email(boldFont); th_email.setString(L"EMAIL TÀI KHOẢN"); th_email.setCharacterSize(12.5f); th_email.setFillColor(Color::White); th_email.setPosition({col_email, thY}); target.draw(th_email);
    Text th_phone(boldFont); th_phone.setString(L"SỐ ĐIỆN THOẠI"); th_phone.setCharacterSize(12.5f); th_phone.setFillColor(Color::White); th_phone.setPosition({col_phone, thY}); target.draw(th_phone);
    Text th_tier(boldFont); th_tier.setString(L"HẠNG THẺ"); th_tier.setCharacterSize(12.5f); th_tier.setFillColor(Color::White); th_tier.setPosition({col_tier, thY}); target.draw(th_tier);
    Text th_date(boldFont); th_date.setString(L"NGÀY ĐĂNG KÝ"); th_date.setCharacterSize(12.5f); th_date.setFillColor(Color::White); th_date.setPosition({col_date, thY}); target.draw(th_date);
    Text th_bought(boldFont); th_bought.setString(L"ĐÃ MUA"); th_bought.setCharacterSize(12.5f); th_bought.setFillColor(Color::White); th_bought.setPosition({col_bought, thY}); target.draw(th_bought);
    Text th_action(boldFont); th_action.setString(L"THAO TÁC"); th_action.setCharacterSize(12.5f); th_action.setFillColor(Color::White); th_action.setPosition({col_action, thY}); target.draw(th_action);

    // Customer Rows
    float rTop = tableY + headerH;
    float rH = 52.f;
    int startIdx = page * rowsPerPage;
    int endIdx = min((int)filteredCustomers.size(), startIdx + rowsPerPage);

    for (int i = startIdx; i < endIdx; ++i) {
        int rIdx = i - startIdx;
        float ry = rTop + rIdx * rH;
        const auto* c = filteredCustomers[i];

        // Row background
        bool isSel = (selectedIndex == i || c->selected);
        bool isHov = (hoveredRow == i);
        Color rBg = isSel ? Color(239, 246, 255) : (isHov ? Color(248, 250, 252) : Color::White);

        RoundedRectRenderer::draw(target, {tableX, ry}, {tableW, rH - 2.f}, 4.f, rBg, 1.f, isSel ? Color(186, 230, 253) : Color(241, 245, 249));

        // Checkbox
        Vector2f rcPos(tableX + 16.f, ry + 16.f);
        RoundedRectRenderer::draw(target, rcPos, {16.f, 16.f}, 3.f, c->selected ? Color(2, 132, 199) : Color::White, 1.f, Color(203, 213, 225));
        if (c->selected) {
            Text tick(boldFont); tick.setString(L"✓"); tick.setCharacterSize(11); tick.setFillColor(Color::White); tick.setPosition({rcPos.x + 3.f, rcPos.y}); target.draw(tick);
        }

        // STT
        Text sttTxt(font); sttTxt.setString(to_string(i + 1)); sttTxt.setCharacterSize(12.5f); sttTxt.setFillColor(Color(100, 116, 139)); sttTxt.setPosition({col_stt + 4.f, ry + 15.f}); target.draw(sttTxt);

        // Avatar Circle with Initials
        Vector2f avPos(col_name, ry + 8.f);
        CircleShape av(16.f); av.setPosition(avPos); av.setFillColor(c->avatarBg); target.draw(av);
        Text avTxt(boldFont); avTxt.setString(c->initials); avTxt.setCharacterSize(12); avTxt.setFillColor(Color(30, 41, 59));
        FloatRect avb = avTxt.getLocalBounds(); avTxt.setPosition({avPos.x + 16.f - avb.size.x / 2.f, avPos.y + 16.f - avb.size.y / 2.f - 2.f}); target.draw(avTxt);

        // Name & UID
        Text nameTxt(boldFont); nameTxt.setString(c->displayFullName); nameTxt.setCharacterSize(13.5f); nameTxt.setFillColor(Color(15, 23, 42)); nameTxt.setPosition({col_name + 40.f, ry + 8.f}); target.draw(nameTxt);
        Text uidTxt(font); uidTxt.setString(s2ws("UID: " + c->uid)); uidTxt.setCharacterSize(11.5f); uidTxt.setFillColor(Color(148, 163, 184)); uidTxt.setPosition({col_name + 40.f, ry + 26.f}); target.draw(uidTxt);

        // Email
        Text emailTxt(font); emailTxt.setString(s2ws(c->email)); emailTxt.setCharacterSize(13.f); emailTxt.setFillColor(Color(51, 65, 85)); emailTxt.setPosition({col_email, ry + 16.f}); target.draw(emailTxt);

        // Phone
        Text phoneTxt(font); phoneTxt.setString(s2ws(c->phone)); phoneTxt.setCharacterSize(13.f); phoneTxt.setFillColor(Color(51, 65, 85)); phoneTxt.setPosition({col_phone, ry + 16.f}); target.draw(phoneTxt);

        // Tier Pill
        Vector2f tierPos(col_tier, ry + 12.f);
        if (c->tier == CustomerTier::GOLD) {
            RoundedRectRenderer::draw(target, tierPos, {100.f, 26.f}, 13.f, Color(254, 243, 199), 1.f, Color(253, 230, 138));
            Text tTxt(boldFont); tTxt.setString(L"Hạng Vàng"); tTxt.setCharacterSize(12.f); tTxt.setFillColor(Color(180, 83, 9)); tTxt.setPosition({tierPos.x + 16.f, tierPos.y + 4.f}); target.draw(tTxt);
        } else if (c->tier == CustomerTier::SILVER) {
            RoundedRectRenderer::draw(target, tierPos, {95.f, 26.f}, 13.f, Color(241, 245, 249), 1.f, Color(226, 232, 240));
            Text tTxt(boldFont); tTxt.setString(L"Hạng Bạc"); tTxt.setCharacterSize(12.f); tTxt.setFillColor(Color(71, 85, 105)); tTxt.setPosition({tierPos.x + 18.f, tierPos.y + 4.f}); target.draw(tTxt);
        } else {
            RoundedRectRenderer::draw(target, tierPos, {95.f, 26.f}, 13.f, Color(224, 242, 254), 1.f, Color(186, 230, 253));
            Text tTxt(boldFont); tTxt.setString(L"Tiêu chuẩn"); tTxt.setCharacterSize(12.f); tTxt.setFillColor(Color(2, 132, 199)); tTxt.setPosition({tierPos.x + 14.f, tierPos.y + 4.f}); target.draw(tTxt);
        }

        // Registration Date
        Text dtTxt(font); dtTxt.setString(s2ws(c->registeredDate)); dtTxt.setCharacterSize(12.5f); dtTxt.setFillColor(Color(100, 116, 139)); dtTxt.setPosition({col_date, ry + 16.f}); target.draw(dtTxt);

        // Tickets Bought
        Text tkTxt(boldFont); tkTxt.setString(to_wstring(c->ticketsBought) + L" vé"); tkTxt.setCharacterSize(13.f); tkTxt.setFillColor(Color(2, 132, 199)); tkTxt.setPosition({col_bought + 10.f, ry + 16.f}); target.draw(tkTxt);

        // Action buttons: Xem, Sửa, Xóa
        RoundedRectRenderer::draw(target, {col_action, ry + 12.f}, {26.f, 24.f}, 4.f, Color(241, 245, 249), 1.f, Color(203, 213, 225));
        Text aView(boldFont); aView.setString(L"Xem"); aView.setCharacterSize(10); aView.setFillColor(Color(2, 132, 199)); aView.setPosition({col_action + 3.f, ry + 16.f}); target.draw(aView);

        RoundedRectRenderer::draw(target, {col_action + 28.f, ry + 12.f}, {26.f, 24.f}, 4.f, Color(254, 243, 199), 1.f, Color(253, 230, 138));
        Text aEdit(boldFont); aEdit.setString(L"Sửa"); aEdit.setCharacterSize(10); aEdit.setFillColor(Color(180, 83, 9)); aEdit.setPosition({col_action + 32.f, ry + 16.f}); target.draw(aEdit);

        RoundedRectRenderer::draw(target, {col_action + 56.f, ry + 12.f}, {26.f, 24.f}, 4.f, Color(254, 242, 242), 1.f, Color(254, 202, 202));
        Text aDel(boldFont); aDel.setString(L"Xóa"); aDel.setCharacterSize(10); aDel.setFillColor(Color(239, 68, 68)); aDel.setPosition({col_action + 60.f, ry + 16.f}); target.draw(aDel);
    }

    // Pagination Footer
    float pageY = rTop + rowsPerPage * rH + 12.f;
    Text pageInfo(font);
    pageInfo.setString(L"Hiển thị " + to_wstring(startIdx + 1) + L" - " + to_wstring(endIdx) + L" trên " + to_wstring(filteredCustomers.size()) + L" khách hàng");
    pageInfo.setCharacterSize(12.5f);
    pageInfo.setFillColor(Color(100, 116, 139));
    pageInfo.setPosition({tableX + 16.f, pageY + 6.f});
    target.draw(pageInfo);

    // Page Buttons: < [1] [2] >
    Vector2f prevP(position.x + width - 110.f, pageY);
    RoundedRectRenderer::draw(target, prevP, {30.f, 30.f}, 4.f, Color::White, 1.f, Color(203, 213, 225));
    Text prevT(boldFont); prevT.setString(L"<"); prevT.setCharacterSize(12.5f); prevT.setFillColor(Color(71, 85, 105)); prevT.setPosition({prevP.x + 10.f, prevP.y + 4.f}); target.draw(prevT);

    Vector2f p1P(position.x + width - 75.f, pageY);
    RoundedRectRenderer::draw(target, p1P, {30.f, 30.f}, 4.f, (page == 0) ? Color(2, 132, 199) : Color::White, 1.f, (page == 0) ? Color(2, 132, 199) : Color(203, 213, 225));
    Text p1T(boldFont); p1T.setString(to_string(page + 1)); p1T.setCharacterSize(12.5f); p1T.setFillColor((page == 0) ? Color::White : Color(71, 85, 105)); p1T.setPosition({p1P.x + 10.f, p1P.y + 4.f}); target.draw(p1T);

    Vector2f nextP(position.x + width - 40.f, pageY);
    RoundedRectRenderer::draw(target, nextP, {30.f, 30.f}, 4.f, Color::White, 1.f, Color(203, 213, 225));
    Text nextT(boldFont); nextT.setString(L">"); nextT.setCharacterSize(12.5f); nextT.setFillColor(Color(71, 85, 105)); nextT.setPosition({nextP.x + 10.f, nextP.y + 4.f}); target.draw(nextT);
}

void UserPanel::drawDialog(RenderTarget& target) const {
    // Dim Overlay
    RectangleShape dim(Vector2f(width, height));
    dim.setPosition(position);
    dim.setFillColor(Color(15, 23, 42, 160));
    target.draw(dim);

    float dW = 480.f;
    float dH = 340.f;
    Vector2f dPos(position.x + (width - dW) / 2.f, position.y + (height - dH) / 2.f);

    RoundedRectRenderer::draw(target, dPos, {dW, dH}, 8.f, Color::White, 1.f, Color(226, 232, 240));

    if (activeDialog == DialogType::VIEW && dialogUserIndex >= 0 && dialogUserIndex < (int)filteredCustomers.size()) {
        const auto* c = filteredCustomers[dialogUserIndex];

        Text dTitle(boldFont);
        dTitle.setString(L"Thông tin chi tiết hội viên");
        dTitle.setCharacterSize(16);
        dTitle.setFillColor(Color(15, 23, 42));
        dTitle.setPosition({dPos.x + 24.f, dPos.y + 20.f});
        target.draw(dTitle);

        Text dSub(font);
        dSub.setString(L"Hồ sơ thành viên CineCore Rewards ID: " + s2ws(c->uid));
        dSub.setCharacterSize(12);
        dSub.setFillColor(Color(100, 116, 139));
        dSub.setPosition({dPos.x + 24.f, dPos.y + 44.f});
        target.draw(dSub);

        float lineY = dPos.y + 80.f;
        auto drawField = [&](const wstring& l, const wstring& v, Color vCol) {
            Text fl(font); fl.setString(l); fl.setCharacterSize(12.5f); fl.setFillColor(Color(100, 116, 139)); fl.setPosition({dPos.x + 24.f, lineY}); target.draw(fl);
            Text fv(boldFont); fv.setString(v); fv.setCharacterSize(13.f); fv.setFillColor(vCol); fv.setPosition({dPos.x + 180.f, lineY}); target.draw(fv);
            lineY += 32.f;
        };

        drawField(L"Họ và tên:", c->displayFullName, Color(15, 23, 42));
        drawField(L"Email tài khoản:", s2ws(c->email), Color(37, 99, 235));
        drawField(L"Số điện thoại:", s2ws(c->phone), Color(15, 23, 42));
        wstring tierName = (c->tier == CustomerTier::GOLD) ? L"Hạng Vàng (VIP)" : (c->tier == CustomerTier::SILVER ? L"Hạng Bạc" : L"Tiêu chuẩn");
        drawField(L"Cấp bậc thẻ:", tierName, Color(180, 83, 9));
        drawField(L"Số vé đã mua:", to_wstring(c->ticketsBought) + L" vé", Color(5, 150, 105));
        drawField(L"Tổng tích lũy chi tiêu:", s2ws(formatCurrency(c->totalSpent)) + L" VNĐ", Color(225, 29, 72));

        Vector2f btnP(dPos.x + dW - 130.f, dPos.y + dH - 50.f);
        RoundedRectRenderer::draw(target, btnP, {105.f, 36.f}, 6.f, Color(2, 132, 199));
        Text btnT(boldFont); btnT.setString(L"Đóng"); btnT.setCharacterSize(12.5f); btnT.setFillColor(Color::White); btnT.setPosition({btnP.x + 36.f, btnP.y + 8.f}); target.draw(btnT);
    } else if (activeDialog == DialogType::DELETE_CONFIRM && dialogUserIndex >= 0 && dialogUserIndex < (int)filteredCustomers.size()) {
        const auto* c = filteredCustomers[dialogUserIndex];

        Text dTitle(boldFont);
        dTitle.setString(L"Xác nhận xóa tài khoản khách hàng");
        dTitle.setCharacterSize(16);
        dTitle.setFillColor(Color(239, 68, 68));
        dTitle.setPosition({dPos.x + 24.f, dPos.y + 20.f});
        target.draw(dTitle);

        Text warn(font);
        warn.setString(L"Bạn có chắc chắn muốn xóa tài khoản của khách hàng:\n" + c->displayFullName + L" (" + s2ws(c->email) + L")?\n\nHành động này sẽ xóa dữ liệu tài khoản và tích lũy điểm thưởng!");
        warn.setCharacterSize(12.5f);
        warn.setFillColor(Color(51, 65, 85));
        warn.setPosition({dPos.x + 24.f, dPos.y + 60.f});
        target.draw(warn);

        Vector2f cancelP(dPos.x + dW - 230.f, dPos.y + dH - 50.f);
        RoundedRectRenderer::draw(target, cancelP, {95.f, 36.f}, 6.f, Color::White, 1.f, Color(203, 213, 225));
        Text cT(boldFont); cT.setString(L"Hủy bỏ"); cT.setCharacterSize(12.5f); cT.setFillColor(Color(71, 85, 105)); cT.setPosition({cancelP.x + 24.f, cancelP.y + 8.f}); target.draw(cT);

        Vector2f delP(dPos.x + dW - 120.f, dPos.y + dH - 50.f);
        RoundedRectRenderer::draw(target, delP, {95.f, 36.f}, 6.f, Color(239, 68, 68));
        Text dT(boldFont); dT.setString(L"Xác nhận"); dT.setCharacterSize(12.5f); dT.setFillColor(Color::White); dT.setPosition({delP.x + 18.f, delP.y + 8.f}); target.draw(dT);
    } else if (activeDialog == DialogType::ADD_NEW) {
        Text dTitle(boldFont);
        dTitle.setString(L"Thêm hội viên CineCore Rewards mới");
        dTitle.setCharacterSize(16);
        dTitle.setFillColor(Color(15, 23, 42));
        dTitle.setPosition({dPos.x + 24.f, dPos.y + 20.f});
        target.draw(dTitle);

        float inY = dPos.y + 65.f;
        auto drawInput = [&](const wstring& lbl, const string& val) {
            Text fl(boldFont); fl.setString(lbl); fl.setCharacterSize(12.5f); fl.setFillColor(Color(71, 85, 105)); fl.setPosition({dPos.x + 24.f, inY}); target.draw(fl);
            Vector2f boxP(dPos.x + 24.f, inY + 18.f);
            RoundedRectRenderer::draw(target, boxP, {dW - 48.f, 34.f}, 4.f, Color::White, 1.f, Color(203, 213, 225));
            Text fv(font); fv.setString(s2ws(val)); fv.setCharacterSize(13); fv.setFillColor(Color(15, 23, 42)); fv.setPosition({boxP.x + 10.f, boxP.y + 7.f}); target.draw(fv);
            inY += 60.f;
        };

        drawInput(L"Họ và tên khách hàng *", inputName.empty() ? "Ví dụ: Trần Văn Nam" : inputName);
        drawInput(L"Email tài khoản *", inputEmail.empty() ? "namtran@gmail.com" : inputEmail);
        drawInput(L"Số điện thoại *", inputPhone.empty() ? "0912345678" : inputPhone);

        Vector2f cancelP(dPos.x + dW - 230.f, dPos.y + dH - 50.f);
        RoundedRectRenderer::draw(target, cancelP, {95.f, 36.f}, 6.f, Color::White, 1.f, Color(203, 213, 225));
        Text cT(boldFont); cT.setString(L"Hủy bỏ"); cT.setCharacterSize(12.5f); cT.setFillColor(Color(71, 85, 105)); cT.setPosition({cancelP.x + 24.f, cancelP.y + 8.f}); target.draw(cT);

        Vector2f okP(dPos.x + dW - 120.f, dPos.y + dH - 50.f);
        RoundedRectRenderer::draw(target, okP, {95.f, 36.f}, 6.f, Color(2, 132, 199));
        Text okT(boldFont); okT.setString(L"Thêm mới"); okT.setCharacterSize(12.5f); okT.setFillColor(Color::White); okT.setPosition({okP.x + 18.f, okP.y + 8.f}); target.draw(okT);
    }
}

void UserPanel::drawBottomBar(RenderTarget& target) const {
    float barY = height - 28.f;

    RectangleShape div(Vector2f(width - 40.f, 1.f));
    div.setPosition({position.x + 20.f, barY - 4.f});
    div.setFillColor(Color(226, 232, 240));
    target.draw(div);

    Text engText(font);
    engText.setString(L"NODE: VN-DN601  •  LIVE   |   CineCore Customer Registry: Synced (0.12ms)   |   UTF-8 Engine");
    engText.setCharacterSize(11.f);
    engText.setFillColor(Color(100, 116, 139));
    engText.setPosition({position.x + 20.f, barY});
    target.draw(engText);

    Text authText(font);
    authText.setString(L"Admin CineCore (Toàn quyền)  •  1728x972 HD");
    authText.setCharacterSize(11.f);
    authText.setFillColor(Color(100, 116, 139));
    FloatRect ab = authText.getLocalBounds();
    authText.setPosition({position.x + width - ab.size.x - 20.f, barY});
    target.draw(authText);
}

void UserPanel::drawToast(RenderTarget& target) const {
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
