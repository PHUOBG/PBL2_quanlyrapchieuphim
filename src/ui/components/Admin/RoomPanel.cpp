#include "UI/components/Admin/RoomPanel.h"
#include "UI/components/Admin/UITheme.h"
#include "UI/components/Admin/RoundedRectRenderer.h"
#include "utils/FileUtils.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <cmath>

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

static wstring toUpperVi(const wstring& s) {
    wstring res = s;
    const wstring lower = L"aáàảãạăắằẳẵặâấầẩẫậeéèẻẽẹêếềểễệiíìỉĩịoóòỏõọôốồổỗộơớờởỡợuúùủũụưứừửữựyýỳỷỹỵđ";
    const wstring upper = L"AÁÀẢÃẠĂẮẰẲẴẶÂẤẦẨẪẬEÉÈẺẼẸÊẾỀỂỄỆIÍÌỈĨỊOÓÒỎÕỌÔỐỒỔỖỘƠỚỜỞỠỢUÚÙỦŨỤƯỨỪỬỮỰYÝỲỶỸỴĐ";
    for (size_t i = 0; i < res.size(); ++i) {
        wchar_t c = res[i];
        if (c >= L'a' && c <= L'z') {
            res[i] = (wchar_t)(c - 32);
        } else {
            size_t pos = lower.find(c);
            if (pos != wstring::npos) {
                res[i] = upper[pos];
            }
        }
    }
    return res;
}

RoomPanel::RoomPanel(Font& fontRef, float w, float h)
    : font(fontRef),
      width(w), height(h), position(280.f, 0.f),
      zoomFactor(1.0f)
{
    if (!boldFont.openFromFile("assets/fonts/segoeuib.ttf") &&
        !boldFont.openFromFile("../assets/fonts/segoeuib.ttf") &&
        !boldFont.openFromFile("assets/fonts/Montserrat_SemiBold.ttf") &&
        !boldFont.openFromFile("../assets/fonts/Montserrat_SemiBold.ttf")) {
        boldFont.openFromFile("C:/Windows/Fonts/segoeuib.ttf");
    }

    if (!logoTex.loadFromFile("assets/elements/cinexine_icon.png") &&
        !logoTex.loadFromFile("../assets/elements/cinexine_icon.png") &&
        !logoTex.loadFromFile("assets/elements/icon.png") &&
        !logoTex.loadFromFile("../assets/elements/icon.png")) {
        hasLogo = false;
    } else {
        hasLogo = true;
    }

    if (reloadTex.loadFromFile("assets/elements/reload.png") ||
        reloadTex.loadFromFile("../assets/elements/reload.png")) {
        reloadTex.setSmooth(true);
        hasReloadTex = true;
    }

    initDefaultRooms();
    loadRoomsFromFile();

    if (!rooms.empty()) {
        selectedRoomIndex = 4; // Mặc định hiển thị Phòng 5 như ảnh
        if (selectedRoomIndex >= (int)rooms.size()) selectedRoomIndex = 0;
        inputRoomName = rooms[selectedRoomIndex].name;
        inputRows = to_string(rooms[selectedRoomIndex].rows);
        inputCols = to_string(rooms[selectedRoomIndex].cols);
        inputStatus = rooms[selectedRoomIndex].status;
    }
}

void RoomPanel::setPosition(Vector2f pos) {
    position = pos;
}

void RoomPanel::initDefaultRooms() {
    rooms.clear();
    for (int i = 1; i <= 6; ++i) {
        RoomData r;
        r.id = "PHONG_00" + to_string(i);
        r.name = L"Ph\u00f2ng " + to_wstring(i);
        if (i == 5) {
            r.roomType = L"Laser 4K Atmos - Hi\u1ec7n h\u00e0nh";
            r.status = L"B\u00ecnh th\u01b0\u1eddng (\u0110ang \u0111\u00f3n kh\u00e1ch)";
            r.rows = 6;
            r.cols = 6;
            r.seatMatrix.assign(6, vector<SeatType>(6, SeatType::STANDARD));
            // Cấu hình mẫu theo đúng ảnh cho Phòng 5:
            // Hàng B: ghế 2,3,4,5 là VIP
            r.seatMatrix[1][1] = SeatType::VIP;
            r.seatMatrix[1][2] = SeatType::VIP;
            r.seatMatrix[1][3] = SeatType::VIP;
            r.seatMatrix[1][4] = SeatType::VIP;
            // Hàng E và F: Sweetbox (ghế đôi)
            for (int c = 0; c < 6; ++c) {
                r.seatMatrix[4][c] = SeatType::SWEETBOX;
                r.seatMatrix[5][c] = SeatType::SWEETBOX;
            }
        } else {
            r.roomType = (i % 2 == 0) ? L"IMAX 3D Laser" : L"Digital 2D Dolby 7.1";
            r.status = L"B\u00ecnh th\u01b0\u1eddng (\u0110ang \u0111\u00f3n kh\u00e1ch)";
            r.rows = 6;
            r.cols = 6;
            r.seatMatrix.assign(6, vector<SeatType>(6, SeatType::STANDARD));
            r.seatMatrix[1][1] = SeatType::VIP;
            r.seatMatrix[1][2] = SeatType::VIP;
            r.seatMatrix[1][3] = SeatType::VIP;
            r.seatMatrix[1][4] = SeatType::VIP;
            for (int c = 0; c < 6; ++c) {
                r.seatMatrix[4][c] = SeatType::SWEETBOX;
                r.seatMatrix[5][c] = SeatType::SWEETBOX;
            }
        }
        rooms.push_back(r);
    }
}

void RoomPanel::loadRoomsFromFile() {
    string filePath = FileUtils::resolveDataPath("data/rooms.txt");
    ifstream file(filePath);
    if (!file.is_open()) return;

    string line;
    bool isHeader = true;
    while (getline(file, line)) {
        if (line.empty()) continue;
        if (isHeader) { isHeader = false; continue; }
        stringstream ss(line);
        string id, nameStr, rowsStr, colsStr, totalStr;
        getline(ss, id, '|');
        getline(ss, nameStr, '|');
        getline(ss, rowsStr, '|');
        getline(ss, colsStr, '|');
        getline(ss, totalStr, '|');

        // Tìm room đã có để update rows/cols nếu có
        for (auto& r : rooms) {
            if (r.id == id) {
                r.name = s2ws(nameStr);
                try {
                    int rw = stoi(rowsStr);
                    int cl = stoi(colsStr);
                    if (rw >= 2 && rw <= 12 && cl >= 2 && cl <= 14) {
                        r.rows = rw;
                        r.cols = cl;
                        r.seatMatrix.assign(rw, vector<SeatType>(cl, SeatType::STANDARD));
                        if (rw >= 2 && cl >= 4) {
                            r.seatMatrix[1][1] = SeatType::VIP;
                            r.seatMatrix[1][2] = SeatType::VIP;
                            r.seatMatrix[1][3] = SeatType::VIP;
                            if (cl > 4) r.seatMatrix[1][4] = SeatType::VIP;
                        }
                        if (rw >= 6) {
                            for (int c = 0; c < cl; ++c) {
                                r.seatMatrix[rw - 2][c] = SeatType::SWEETBOX;
                                r.seatMatrix[rw - 1][c] = SeatType::SWEETBOX;
                            }
                        }
                    }
                } catch (...) {}
                break;
            }
        }
    }
}

void RoomPanel::saveRoomsToFile() {
    string filePath = FileUtils::resolveDataPath("data/rooms.txt");
    ofstream file(filePath);
    if (!file.is_open()) return;

    file << "room_id|room_name|seat_rows|seat_cols|total_seats\n";
    for (const auto& r : rooms) {
        file << r.id << "|" << ws2s(r.name) << "|" << r.rows << "|" << r.cols << "|" << (r.rows * r.cols) << "\n";
    }
}

void RoomPanel::updateSeatStatistics(int& standard, int& vip, int& sweetbox, int& maintenance) const {
    standard = 0; vip = 0; sweetbox = 0; maintenance = 0;
    if (selectedRoomIndex < 0 || selectedRoomIndex >= (int)rooms.size()) return;
    const auto& matrix = rooms[selectedRoomIndex].seatMatrix;
    for (const auto& row : matrix) {
        for (auto st : row) {
            if (st == SeatType::STANDARD) standard++;
            else if (st == SeatType::VIP) vip++;
            else if (st == SeatType::SWEETBOX) sweetbox++;
            else if (st == SeatType::MAINTENANCE) maintenance++;
        }
    }
}

void RoomPanel::drawRoundedCard(RenderWindow& win, Vector2f pos, Vector2f sz, float r, Color fill, float outline, Color outCol) {
    RoundedRectRenderer::draw(win, pos, sz, r, fill, outline, outCol);
}

void RoomPanel::drawSeat(RenderWindow& win, float x, float y, float w, float h, SeatType type, const string& label, bool hovered) {
    Color bodyColor, cushionColor, textColor;

    switch (type) {
        case SeatType::VIP:
            bodyColor = Color(245, 158, 11);        // Gold amber
            cushionColor = Color(217, 119, 6);
            textColor = Color::White;
            break;
        case SeatType::SWEETBOX:
            bodyColor = Color(30, 58, 95);          // Deep navy
            cushionColor = Color(18, 36, 60);
            textColor = Color::White;
            break;
        case SeatType::MAINTENANCE:
            bodyColor = Color(239, 68, 68);         // Red alert
            cushionColor = Color(185, 28, 28);
            textColor = Color::White;
            break;
        case SeatType::STANDARD:
        default:
            bodyColor = Color(218, 228, 240);       // Soft pastel blue-gray
            cushionColor = Color(148, 163, 184);
            textColor = Color(71, 85, 105);
            break;
    }

    if (hovered) {
        bodyColor = Color(min(255, bodyColor.r + 30), min(255, bodyColor.g + 30), min(255, bodyColor.b + 30));
    }

    // 1. Lưng ghế bo tròn phía trên
    RoundedRectRenderer::draw(win, {x, y}, {w, h * 0.72f}, 6.f, bodyColor, hovered ? 1.5f : 0.f, Color(2, 132, 199));

    // 2. Đệm ngồi bên dưới
    float cushionH = h * 0.20f;
    RoundedRectRenderer::draw(win, {x + 2.f, y + h * 0.78f}, {w - 4.f, cushionH}, 3.f, cushionColor);

    // 3. Số ghế
    Text numText(boldFont, label, (unsigned int)(h * 0.30f));
    numText.setFillColor(textColor);
    FloatRect tb = numText.getLocalBounds();
    numText.setPosition({
        x + (w - tb.size.x) / 2.f - tb.position.x,
        y + (h * 0.68f - tb.size.y) / 2.f - tb.position.y
    });
    win.draw(numText);
}

void RoomPanel::handleEvent(const Event& event, const RenderWindow& win) {
    if (const auto* mouseBtn = event.getIf<Event::MouseButtonPressed>()) {
        if (mouseBtn->button == Mouse::Button::Left) {
            Vector2f mousePos = win.mapPixelToCoords(Mouse::getPosition(win));

            // Header Buttons
            FloatRect btnRefresh({position.x + width - 470.f, position.y + 78.f}, {110.f, 38.f});
            FloatRect btnAdd({position.x + width - 350.f, position.y + 78.f}, {170.f, 38.f});
            FloatRect btnDelete({position.x + width - 170.f, position.y + 78.f}, {145.f, 38.f});

            if (btnRefresh.contains(mousePos)) {
                loadRoomsFromFile();
                if (selectedRoomIndex >= 0 && selectedRoomIndex < (int)rooms.size()) {
                    inputRoomName = rooms[selectedRoomIndex].name;
                    inputRows = to_string(rooms[selectedRoomIndex].rows);
                    inputCols = to_string(rooms[selectedRoomIndex].cols);
                }
                activeInputBox = -1;
                zoomFactor = 1.0f;
                notificationMessage = "Đã làm mới dữ liệu & khôi phục cấu hình phòng!";
                notificationClock.restart();
                notificationVisible = true;
                return;
            }

            if (btnAdd.contains(mousePos)) {
                int nextId = (int)rooms.size() + 1;
                RoomData nr;
                nr.id = "PHONG_00" + to_string(nextId);
                nr.name = L"Ph\u00f2ng " + to_wstring(nextId);
                nr.roomType = L"Standard 2D Dolby 7.1";
                nr.status = L"B\u00ecnh th\u01b0\u1eddng (\u0110ang \u0111\u00f3n kh\u00e1ch)";
                nr.rows = 6;
                nr.cols = 6;
                nr.seatMatrix.assign(6, vector<SeatType>(6, SeatType::STANDARD));
                rooms.push_back(nr);
                selectedRoomIndex = (int)rooms.size() - 1;
                inputRoomName = nr.name;
                inputRows = "6";
                inputCols = "6";
                saveRoomsToFile();
                notificationMessage = "Đã thêm phòng chiếu mới!";
                notificationClock.restart();
                notificationVisible = true;
                return;
            }

            if (btnDelete.contains(mousePos)) {
                if (rooms.size() > 1) {
                    rooms.erase(rooms.begin() + selectedRoomIndex);
                    if (selectedRoomIndex >= (int)rooms.size()) selectedRoomIndex = (int)rooms.size() - 1;
                    inputRoomName = rooms[selectedRoomIndex].name;
                    inputRows = to_string(rooms[selectedRoomIndex].rows);
                    inputCols = to_string(rooms[selectedRoomIndex].cols);
                    saveRoomsToFile();
                    notificationMessage = "Đã xóa phòng chiếu thành công!";
                    notificationClock.restart();
                    notificationVisible = true;
                }
                return;
            }

            // 3. Zoom controls
            FloatRect zoomOutBtn({position.x + width - 165.f, position.y + 224.f}, {28.f, 26.f});
            FloatRect zoomInBtn({position.x + width - 68.f, position.y + 224.f}, {28.f, 26.f});
            if (zoomOutBtn.contains(mousePos)) {
                zoomFactor = max(0.7f, zoomFactor - 0.1f);
                return;
            }
            if (zoomInBtn.contains(mousePos)) {
                zoomFactor = min(1.4f, zoomFactor + 0.1f);
                return;
            }

            // 4. Form Cột trái: Click chọn dropdown phòng chiếu
            FloatRect ddRoomBounds({position.x + 50.f, position.y + 282.f}, {320.f, 38.f});
            if (ddRoomBounds.contains(mousePos)) {
                roomDropdownOpen = !roomDropdownOpen;
                statusDropdownOpen = false;
                seatTypeDropdownOpen = false;
                return;
            }

            if (roomDropdownOpen) {
                for (size_t i = 0; i < rooms.size(); ++i) {
                    FloatRect itemB({position.x + 50.f, position.y + 322.f + i * 36.f}, {320.f, 36.f});
                    if (itemB.contains(mousePos)) {
                        selectedRoomIndex = (int)i;
                        inputRoomName = rooms[i].name;
                        inputRows = to_string(rooms[i].rows);
                        inputCols = to_string(rooms[i].cols);
                        inputStatus = rooms[i].status;
                        roomDropdownOpen = false;
                        return;
                    }
                }
                roomDropdownOpen = false;
            }

            // Click ô loại ghế cần gán
            FloatRect ddSeatTypeBounds({position.x + 50.f, position.y + 512.f}, {320.f, 38.f});
            if (ddSeatTypeBounds.contains(mousePos)) {
                seatTypeDropdownOpen = !seatTypeDropdownOpen;
                roomDropdownOpen = false;
                statusDropdownOpen = false;
                return;
            }

            if (seatTypeDropdownOpen) {
                SeatType types[4] = {SeatType::STANDARD, SeatType::VIP, SeatType::SWEETBOX, SeatType::MAINTENANCE};
                for (int i = 0; i < 4; ++i) {
                    FloatRect itmB({position.x + 50.f, position.y + 552.f + i * 36.f}, {320.f, 36.f});
                    if (itmB.contains(mousePos)) {
                        currentAssignType = types[i];
                        seatTypeDropdownOpen = false;
                        return;
                    }
                }
                seatTypeDropdownOpen = false;
            }

            // Click ô Input Name / Rows / Cols
            FloatRect boxName({position.x + 50.f, position.y + 354.f}, {320.f, 38.f});
            FloatRect boxRows({position.x + 50.f, position.y + 424.f}, {150.f, 38.f});
            FloatRect boxCols({position.x + 220.f, position.y + 424.f}, {150.f, 38.f});

            if (boxName.contains(mousePos)) { activeInputBox = 0; return; }
            if (boxRows.contains(mousePos)) { activeInputBox = 1; return; }
            if (boxCols.contains(mousePos)) { activeInputBox = 2; return; }
            activeInputBox = -1;

            // Nút LƯU và HỦY BỎ
            FloatRect btnSave({position.x + 50.f, position.y + 790.f}, {150.f, 40.f});
            FloatRect btnCancel({position.x + 220.f, position.y + 790.f}, {150.f, 40.f});

            if (btnSave.contains(mousePos)) {
                if (selectedRoomIndex >= 0 && selectedRoomIndex < (int)rooms.size()) {
                    auto& r = rooms[selectedRoomIndex];
                    if (!inputRoomName.empty()) r.name = inputRoomName;
                    try {
                        int nr = stoi(inputRows);
                        int nc = stoi(inputCols);
                        if (nr >= 2 && nr <= 10 && nc >= 2 && nc <= 12) {
                            if (nr != r.rows || nc != r.cols) {
                                r.rows = nr;
                                r.cols = nc;
                                r.seatMatrix.assign(nr, vector<SeatType>(nc, SeatType::STANDARD));
                            }
                        }
                    } catch (...) {}
                    saveRoomsToFile();
                    notificationMessage = "Đã lưu cấu hình phòng chiếu thành công!";
                    notificationClock.restart();
                    notificationVisible = true;
                }
                return;
            }

            if (btnCancel.contains(mousePos)) {
                if (selectedRoomIndex >= 0 && selectedRoomIndex < (int)rooms.size()) {
                    inputRoomName = rooms[selectedRoomIndex].name;
                    inputRows = to_string(rooms[selectedRoomIndex].rows);
                    inputCols = to_string(rooms[selectedRoomIndex].cols);
                    notificationMessage = "Đã hoàn tác thay đổi!";
                    notificationClock.restart();
                    notificationVisible = true;
                }
                return;
            }

            // 5. Cột phải: Click trực tiếp vào ghế để đổi loại ghế
            if (selectedRoomIndex >= 0 && selectedRoomIndex < (int)rooms.size()) {
                auto& r = rooms[selectedRoomIndex];
                float gridStartX = position.x + 440.f + 50.f;
                float gridStartY = position.y + 360.f;
                float seatW = 46.f * zoomFactor;
                float seatH = 46.f * zoomFactor;
                float gapX = 18.f * zoomFactor;
                float gapY = 16.f * zoomFactor;

                // Căn giữa sơ đồ trong khung phải
                float totalW = r.cols * seatW + (r.cols - 1) * gapX;
                float rightCardW = width - 470.f;
                gridStartX = position.x + 440.f + (rightCardW - totalW) / 2.f;

                for (int row = 0; row < r.rows; ++row) {
                    for (int col = 0; col < r.cols; ++col) {
                        float sX = gridStartX + col * (seatW + gapX);
                        float sY = gridStartY + row * (seatH + gapY);
                        FloatRect seatBounds({sX, sY}, {seatW, seatH});

                        if (seatBounds.contains(mousePos)) {
                            // Đổi loại ghế theo loại đang chọn hoặc cycle
                            if (r.seatMatrix[row][col] == currentAssignType) {
                                // Nếu click lại cùng loại thì chuyển sang thường
                                r.seatMatrix[row][col] = SeatType::STANDARD;
                            } else {
                                r.seatMatrix[row][col] = currentAssignType;
                            }
                            return;
                        }
                    }
                }
            }
        }
    }

    // Xử lý gõ phím khi đang active input box
    if (const auto* textEv = event.getIf<Event::TextEntered>()) {
        char32_t c = textEv->unicode;
        if (activeInputBox == 0) { // Tên phòng
            if (c == 8) { // Backspace
                if (!inputRoomName.empty()) inputRoomName.pop_back();
            } else if (c >= 32 && c != 127) {
                if (inputRoomName.size() < 24) inputRoomName.push_back((wchar_t)c);
            }
        } else if (activeInputBox == 1) { // Hàng ghế (chỉ số 2-10)
            if (c == 8) {
                if (!inputRows.empty()) inputRows.pop_back();
            } else if (c >= '2' && c <= '9') {
                if (inputRows.size() < 2) inputRows.push_back((char)c);
            }
        } else if (activeInputBox == 2) { // Cột ghế (chỉ số 2-12)
            if (c == 8) {
                if (!inputCols.empty()) inputCols.pop_back();
            } else if (c >= '2' && c <= '9') {
                if (inputCols.size() < 2) inputCols.push_back((char)c);
            }
        }
    }
}

void RoomPanel::update(Vector2f mousePos, bool) {
    hoveredSeatRow = -1;
    hoveredSeatCol = -1;

    if (selectedRoomIndex >= 0 && selectedRoomIndex < (int)rooms.size()) {
        const auto& r = rooms[selectedRoomIndex];
        float seatW = 46.f * zoomFactor;
        float seatH = 46.f * zoomFactor;
        float gapX = 18.f * zoomFactor;
        float gapY = 16.f * zoomFactor;

        float totalW = r.cols * seatW + (r.cols - 1) * gapX;
        float rightCardW = width - 470.f;
        float gridStartX = position.x + 440.f + (rightCardW - totalW) / 2.f;
        float gridStartY = position.y + 360.f;

        for (int row = 0; row < r.rows; ++row) {
            for (int col = 0; col < r.cols; ++col) {
                float sX = gridStartX + col * (seatW + gapX);
                float sY = gridStartY + row * (seatH + gapY);
                if (FloatRect({sX, sY}, {seatW, seatH}).contains(mousePos)) {
                    hoveredSeatRow = row;
                    hoveredSeatCol = col;
                    break;
                }
            }
        }
    }
}

void RoomPanel::render(RenderWindow& win) {
    // 1. Canvas Background (#F4F6F9)
    RectangleShape bg({width, height});
    bg.setPosition(position);
    bg.setFillColor(Color(244, 246, 249));
    win.draw(bg);

    // ==========================================
    // 2. TOP BAR (Standardized Navigation & Profile)
    // ==========================================
    UITheme::drawTopBar(win, position.x + 30.f, position.y, width - 60.f, L"QUẢN LÝ PHÒNG CHIẾU", font, boldFont);

    // ==========================================
    // 3. PAGE HEADER CARD (Y = 56.f)
    // ==========================================
    drawRoundedCard(win, {position.x + 30.f, position.y + 56.f}, {width - 60.f, 84.f}, 12.f, Color::White, 1.f, Color(226, 232, 240));

    // Logo CineXine trong ô bo tròn (thay thế 2 gạch theo yêu cầu)
    drawRoundedCard(win, {position.x + 48.f, position.y + 72.f}, {52.f, 52.f}, 12.f, Color(224, 242, 254));
    if (hasLogo) {
        Sprite logoSpr(logoTex);
        float targetS = 36.f;
        float sX = targetS / (float)logoTex.getSize().x;
        float sY = targetS / (float)logoTex.getSize().y;
        logoSpr.setScale({sX, sY});
        logoSpr.setPosition({position.x + 48.f + (52.f - targetS) / 2.f, position.y + 72.f + (52.f - targetS) / 2.f});
        win.draw(logoSpr);
    }

    Text title(boldFont, L"Qu\u1ea3n l\u00fd ph\u00f2ng chi\u1ebfu", 22);
    title.setFillColor(Color(15, 23, 42));
    title.setPosition({position.x + 115.f, position.y + 70.f});
    win.draw(title);

    // Badge "Hoạt động ổn định"
    FloatRect tbBounds = title.getGlobalBounds();
    Text gTxt(boldFont, L"Ho\u1ea1t \u0111\u1ed9ng \u1ed5n \u0111\u1ecbnh", 11.5f);
    gTxt.setFillColor(Color(22, 101, 52));
    FloatRect gtb = gTxt.getLocalBounds();
    float dotRadius = 3.5f;
    float dotGap = 6.f;
    float padH = 12.f;
    float badgeW = padH * 2.f + dotRadius * 2.f + dotGap + gtb.size.x;
    float badgeH = 24.f;
    float badgeX = tbBounds.position.x + tbBounds.size.x + 14.f;
    float badgeY = position.y + 73.f;
    drawRoundedCard(win, {badgeX, badgeY}, {badgeW, badgeH}, 12.f, Color(220, 252, 231));

    float contentW = dotRadius * 2.f + dotGap + gtb.size.x;
    float startX = badgeX + (badgeW - contentW) / 2.f;
    CircleShape gDot(dotRadius);
    gDot.setFillColor(Color(22, 101, 52));
    gDot.setPosition({startX, badgeY + (badgeH - dotRadius * 2.f) / 2.f});
    win.draw(gDot);

    gTxt.setPosition({
        startX + dotRadius * 2.f + dotGap - gtb.position.x,
        badgeY + (badgeH - gtb.size.y) / 2.f - gtb.position.y
    });
    win.draw(gTxt);

    Text subTitle(font, L"H\u1ec7 th\u1ed1ng c\u1ea5u h\u00ecnh s\u01a1 \u0111\u1ed3 gh\u1ebf v\u00e0 gi\u00e1m s\u00e1t tr\u1ea1ng th\u00e1i ph\u00f2ng chi\u1ebfu t\u1ea1i c\u1ee5m r\u1ea1p \u0110\u00e0 N\u1eb5ng", 13);
    subTitle.setFillColor(Color(100, 116, 139));
    subTitle.setPosition({position.x + 115.f, position.y + 102.f});
    win.draw(subTitle);

    // 3 nút góc phải: Làm mới, Thêm phòng chiếu, Xóa phòng chiếu
    float btnY = position.y + 78.f;
    // Nút 1: Làm mới
    float headRefX = position.x + width - 470.f;
    drawRoundedCard(win, {headRefX, btnY}, {110.f, 38.f}, 8.f, Color::White, 1.2f, Color(203, 213, 225));
    if (hasReloadTex) {
        Sprite spHead(reloadTex);
        spHead.setColor(Color(51, 65, 85));
        spHead.setScale({16.f / 500.f, 16.f / 500.f});
        spHead.setPosition({headRefX + 14.f, btnY + 11.f});
        win.draw(spHead);
    }
    Text refTxt(boldFont, L"L\u00e0m m\u1edbi", 13);
    refTxt.setFillColor(Color(51, 65, 85));
    refTxt.setPosition({headRefX + (hasReloadTex ? 36.f : 24.f), btnY + 9.f});
    win.draw(refTxt);

    // Nút 2: + Thêm phòng chiếu
    drawRoundedCard(win, {position.x + width - 350.f, btnY}, {170.f, 38.f}, 8.f, Color(2, 132, 199));
    Text addTxt(boldFont, L"+ Th\u00eam ph\u00f2ng chi\u1ebfu", 13);
    addTxt.setFillColor(Color::White);
    FloatRect addb = addTxt.getLocalBounds();
    addTxt.setPosition({
        position.x + width - 350.f + (170.f - addb.size.x) / 2.f - addb.position.x,
        btnY + (38.f - addb.size.y) / 2.f - addb.position.y
    });
    win.draw(addTxt);

    // Nút 3: Xóa phòng
    drawRoundedCard(win, {position.x + width - 170.f, btnY}, {140.f, 38.f}, 8.f, Color(220, 38, 38));
    Text delTxt(boldFont, L"X\u00f3a ph\u00f2ng", 13);
    delTxt.setFillColor(Color::White);
    FloatRect delb = delTxt.getLocalBounds();
    delTxt.setPosition({
        position.x + width - 170.f + (140.f - delb.size.x) / 2.f - delb.position.x,
        btnY + (38.f - delb.size.y) / 2.f - delb.position.y
    });
    win.draw(delTxt);

    // ==========================================
    // 4. SECTION HEADER (Thanh tiêu đề tĩnh, không click nhầm)
    // ==========================================
    drawRoundedCard(win, {position.x + 30.f, position.y + 156.f}, {280.f, 36.f}, 8.f, Color::White, 1.f, Color(226, 232, 240));
    Text secHeadTxt(boldFont, L"C\u1ea5u h\u00ecnh & S\u01a1 \u0111\u1ed3 gh\u1ebf tr\u1ef1c quan", 13);
    secHeadTxt.setFillColor(Color(2, 132, 199));
    FloatRect shb = secHeadTxt.getLocalBounds();
    secHeadTxt.setPosition({
        position.x + 30.f + (280.f - shb.size.x) / 2.f - shb.position.x,
        position.y + 156.f + (36.f - shb.size.y) / 2.f - shb.position.y
    });
    win.draw(secHeadTxt);

    // ==========================================
    // 5. CỘT TRÁI: FORM "THÔNG TIN PHÒNG CHIẾU"
    // ==========================================
    float cardY = position.y + 204.f;
    float leftCardW = 370.f;
    float cardH = height - 230.f;
    drawRoundedCard(win, {position.x + 30.f, cardY}, {leftCardW, cardH}, 14.f, Color::White, 1.f, Color(226, 232, 240));

    // Tiêu đề form & Badge "BÌNH THƯỜNG" (cân đối, sắc nét)
    Text formTitle(boldFont, L"Th\u00f4ng tin ph\u00f2ng chi\u1ebfu", 16);
    formTitle.setFillColor(Color(15, 23, 42));
    formTitle.setPosition({position.x + 50.f, cardY + 20.f});
    win.draw(formTitle);

    float sBadgeW = 104.f;
    float sBadgeH = 26.f;
    float sBadgeX = position.x + 30.f + leftCardW - 20.f - sBadgeW;
    float sBadgeY = cardY + 18.f;
    drawRoundedCard(win, {sBadgeX, sBadgeY}, {sBadgeW, sBadgeH}, 13.f, Color(209, 250, 229));
    Text stBadge(boldFont, L"B\u00ccNH TH\u01af\u1edcNG", 12);
    stBadge.setFillColor(Color(4, 120, 87));
    FloatRect sbb = stBadge.getLocalBounds();
    stBadge.setPosition({
        std::floor(sBadgeX + (sBadgeW - sbb.size.x) / 2.f - sbb.position.x + 0.5f),
        std::floor(sBadgeY + (sBadgeH - sbb.size.y) / 2.f - sbb.position.y + 0.5f)
    });
    win.draw(stBadge);

    // 1. Chọn phòng chiếu
    Text lbl1(boldFont, L"Ch\u1ecdn ph\u00f2ng chi\u1ebfu:", 13); lbl1.setFillColor(Color(51, 65, 85)); lbl1.setPosition({position.x + 50.f, cardY + 58.f}); win.draw(lbl1);
    drawRoundedCard(win, {position.x + 50.f, cardY + 80.f}, {leftCardW - 40.f, 38.f}, 8.f, Color(248, 250, 252), 1.f, Color(203, 213, 225));
    wstring currRoomLabel = (selectedRoomIndex >= 0 && selectedRoomIndex < (int)rooms.size())
        ? rooms[selectedRoomIndex].name + L" (" + rooms[selectedRoomIndex].roomType + L")"
        : L"Ch\u1ecdn ph\u00f2ng...";
    Text currRoomTxt(font, currRoomLabel, 12); currRoomTxt.setFillColor(Color(15, 23, 42)); currRoomTxt.setPosition({position.x + 62.f, cardY + 89.f}); win.draw(currRoomTxt);
    Text arr1(boldFont, L"\u25bc", 11); arr1.setFillColor(Color(100, 116, 139)); arr1.setPosition({position.x + leftCardW - 15.f, cardY + 91.f}); win.draw(arr1);

    // 2. Tên phòng chiếu
    Text lbl2(boldFont, L"T\u00ean ph\u00f2ng chi\u1ebfu:", 13); lbl2.setFillColor(Color(51, 65, 85)); lbl2.setPosition({position.x + 50.f, cardY + 130.f}); win.draw(lbl2);
    drawRoundedCard(win, {position.x + 50.f, cardY + 152.f}, {leftCardW - 40.f, 38.f}, 8.f, Color(248, 250, 252), (activeInputBox == 0) ? 1.5f : 1.f, (activeInputBox == 0) ? Color(2, 132, 199) : Color(203, 213, 225));
    Text nameInputTxt(boldFont, inputRoomName + ((activeInputBox == 0) ? L"|" : L""), 13); nameInputTxt.setFillColor(Color(15, 23, 42)); nameInputTxt.setPosition({position.x + 62.f, cardY + 161.f}); win.draw(nameInputTxt);

    // 3. Số lượng hàng & cột ghế
    Text lbl3(boldFont, L"S\u1ed1 l\u01b0\u1ee3ng h\u00e0ng gh\u1ebf:", 13); lbl3.setFillColor(Color(51, 65, 85)); lbl3.setPosition({position.x + 50.f, cardY + 202.f}); win.draw(lbl3);
    drawRoundedCard(win, {position.x + 50.f, cardY + 224.f}, {155.f, 38.f}, 8.f, Color(248, 250, 252), (activeInputBox == 1) ? 1.5f : 1.f, (activeInputBox == 1) ? Color(2, 132, 199) : Color(203, 213, 225));
    Text rowsTxt(boldFont, s2ws(inputRows) + ((activeInputBox == 1) ? L"|" : L""), 13); rowsTxt.setFillColor(Color(15, 23, 42)); rowsTxt.setPosition({position.x + 62.f, cardY + 233.f}); win.draw(rowsTxt);

    Text lbl4(boldFont, L"S\u1ed1 l\u01b0\u1ee3ng c\u1ed9t gh\u1ebf:", 13); lbl4.setFillColor(Color(51, 65, 85)); lbl4.setPosition({position.x + 225.f, cardY + 202.f}); win.draw(lbl4);
    drawRoundedCard(win, {position.x + 225.f, cardY + 224.f}, {155.f, 38.f}, 8.f, Color(248, 250, 252), (activeInputBox == 2) ? 1.5f : 1.f, (activeInputBox == 2) ? Color(2, 132, 199) : Color(203, 213, 225));
    Text colsTxt(boldFont, s2ws(inputCols) + ((activeInputBox == 2) ? L"|" : L""), 13); colsTxt.setFillColor(Color(15, 23, 42)); colsTxt.setPosition({position.x + 237.f, cardY + 233.f}); win.draw(colsTxt);

    // 4. Trạng thái phòng chiếu
    Text lbl5(boldFont, L"Tr\u1ea1ng th\u00e1i ph\u00f2ng chi\u1ebfu:", 13); lbl5.setFillColor(Color(51, 65, 85)); lbl5.setPosition({position.x + 50.f, cardY + 274.f}); win.draw(lbl5);
    drawRoundedCard(win, {position.x + 50.f, cardY + 296.f}, {leftCardW - 40.f, 38.f}, 8.f, Color(248, 250, 252), 1.f, Color(203, 213, 225));
    Text stTxt(font, inputStatus.empty() ? L"B\u00ecnh th\u01b0\u1eddng (\u0110ang \u0111\u00f3n kh\u00e1ch)" : inputStatus, 12); stTxt.setFillColor(Color(15, 23, 42)); stTxt.setPosition({position.x + 62.f, cardY + 305.f}); win.draw(stTxt);
    Text arr2(boldFont, L"\u25bc", 11); arr2.setFillColor(Color(100, 116, 139)); arr2.setPosition({position.x + leftCardW - 15.f, cardY + 307.f}); win.draw(arr2);

    // 5. Chọn loại ghế để gán / chỉnh sửa
    Text lbl6(boldFont, L"Ch\u1ecdn lo\u1ea1i gh\u1ebf \u0111\u1ec3 g\u00e1n / ch\u1ec9nh s\u1eeda:", 13); lbl6.setFillColor(Color(51, 65, 85)); lbl6.setPosition({position.x + 50.f, cardY + 346.f}); win.draw(lbl6);
    drawRoundedCard(win, {position.x + 50.f, cardY + 368.f}, {leftCardW - 40.f, 38.f}, 8.f, Color(248, 250, 252), 1.2f, Color(2, 132, 199));

    wstring seatLabel = L"Gh\u1ebf VIP (M\u00e0u V\u00e0ng / Cao c\u1ea5p)";
    if (currentAssignType == SeatType::STANDARD) seatLabel = L"Gh\u1ebf Th\u01b0\u1eddng (M\u00e0u Xanh Nh\u1ea1t)";
    else if (currentAssignType == SeatType::SWEETBOX) seatLabel = L"Gh\u1ebf \u0110\u00f4i Sweetbox (M\u00e0u Xanh \u0110\u1eadm)";
    else if (currentAssignType == SeatType::MAINTENANCE) seatLabel = L"Gh\u1ebf b\u1ea3o tr\u00ec (T\u1ea1m ng\u1eebng)";

    Text seatTypeTxt(boldFont, seatLabel, 12); seatTypeTxt.setFillColor(Color(2, 132, 199)); seatTypeTxt.setPosition({position.x + 62.f, cardY + 377.f}); win.draw(seatTypeTxt);
    Text arr3(boldFont, L"\u25bc", 11); arr3.setFillColor(Color(2, 132, 199)); arr3.setPosition({position.x + leftCardW - 15.f, cardY + 379.f}); win.draw(arr3);

    // Mẹo hướng dẫn (chia thành 2 dòng gọn gàng, không bị tràn ra ngoài khung)
    Text hintTxt(font, L"* M\u1eb9o: Click tr\u1ef1c ti\u1ebfp v\u00e0o gh\u1ebf tr\u00ean s\u01a1 \u0111\u1ed3 b\u00ean ph\u1ea3i\n  \u0111\u1ec3 chuy\u1ec3n \u0111\u1ed5i lo\u1ea1i gh\u1ebf t\u1ee9c th\u00ec.", 11.5f);
    hintTxt.setFillColor(Color(100, 116, 139));
    hintTxt.setPosition({position.x + 50.f, cardY + 414.f});
    win.draw(hintTxt);

    // Thẻ Thống kê loại ghế
    int cntStd = 0, cntVip = 0, cntSwt = 0, cntMnt = 0;
    updateSeatStatistics(cntStd, cntVip, cntSwt, cntMnt);
    int totalSeats = cntStd + cntVip + cntSwt + cntMnt;

    float statCardY = cardY + 452.f;
    drawRoundedCard(win, {position.x + 50.f, statCardY}, {leftCardW - 40.f, 105.f}, 10.f, Color(241, 245, 249));

    Text stHead(boldFont, L"Th\u1ed1ng k\u00ea lo\u1ea1i gh\u1ebf", 12.5f); stHead.setFillColor(Color(15, 23, 42)); stHead.setPosition({position.x + 64.f, statCardY + 12.f}); win.draw(stHead);
    Text stTotal(boldFont, to_wstring(totalSeats) + L" gh\u1ebf", 12.5f); stTotal.setFillColor(Color(2, 132, 199)); stTotal.setPosition({position.x + leftCardW - 35.f, statCardY + 12.f}); win.draw(stTotal);

    // Ghế Thường
    CircleShape c1(3.5f); c1.setFillColor(Color(148, 163, 184)); c1.setPosition({position.x + 64.f, statCardY + 38.f}); win.draw(c1);
    Text c1T(font, L"Gh\u1ebf Th\u01b0\u1eddng", 12); c1T.setFillColor(Color(71, 85, 105)); c1T.setPosition({position.x + 78.f, statCardY + 33.f}); win.draw(c1T);
    Text c1V(boldFont, to_wstring(cntStd) + L" gh\u1ebf", 12); c1V.setFillColor(Color(51, 65, 85)); c1V.setPosition({position.x + leftCardW - 35.f, statCardY + 33.f}); win.draw(c1V);

    // Ghế VIP
    CircleShape c2(3.5f); c2.setFillColor(Color(245, 158, 11)); c2.setPosition({position.x + 64.f, statCardY + 58.f}); win.draw(c2);
    Text c2T(font, L"Gh\u1ebf VIP", 12); c2T.setFillColor(Color(71, 85, 105)); c2T.setPosition({position.x + 78.f, statCardY + 53.f}); win.draw(c2T);
    Text c2V(boldFont, to_wstring(cntVip) + L" gh\u1ebf", 12); c2V.setFillColor(Color(245, 158, 11)); c2V.setPosition({position.x + leftCardW - 35.f, statCardY + 53.f}); win.draw(c2V);

    // Ghế Sweetbox
    CircleShape c3(3.5f); c3.setFillColor(Color(30, 58, 95)); c3.setPosition({position.x + 64.f, statCardY + 78.f}); win.draw(c3);
    Text c3T(font, L"Gh\u1ebf \u0110\u00f4i Sweetbox", 12); c3T.setFillColor(Color(71, 85, 105)); c3T.setPosition({position.x + 78.f, statCardY + 73.f}); win.draw(c3T);
    Text c3V(boldFont, to_wstring(cntSwt) + L" gh\u1ebf (" + to_wstring(cntSwt / 2) + L" c\u1eb7p)", 12); c3V.setFillColor(Color(30, 58, 95)); c3V.setPosition({position.x + leftCardW - 75.f, statCardY + 73.f}); win.draw(c3V);

    // 2 Nút cuối: LƯU và HỦY BỎ
    float actBtnY = cardY + cardH - 58.f;
    float actBtnW = 155.f;
    float actBtnH = 40.f;

    // Nút LƯU
    drawRoundedCard(win, {position.x + 50.f, actBtnY}, {actBtnW, actBtnH}, 8.f, Color(2, 132, 199));
    Text svTxt(boldFont, L"L\u01afU", 13);
    svTxt.setFillColor(Color::White);
    FloatRect svb = svTxt.getLocalBounds();
    svTxt.setPosition({
        position.x + 50.f + (actBtnW - svb.size.x) / 2.f - svb.position.x,
        actBtnY + (actBtnH - svb.size.y) / 2.f - svb.position.y
    });
    win.draw(svTxt);

    // Nút HỦY BỎ
    drawRoundedCard(win, {position.x + 225.f, actBtnY}, {actBtnW, actBtnH}, 8.f, Color(241, 245, 249), 1.f, Color(203, 213, 225));
    Text clTxt(boldFont, L"H\u1ee6Y B\u1ece", 13);
    clTxt.setFillColor(Color(71, 85, 105));
    FloatRect clb = clTxt.getLocalBounds();
    clTxt.setPosition({
        position.x + 225.f + (actBtnW - clb.size.x) / 2.f - clb.position.x,
        actBtnY + (actBtnH - clb.size.y) / 2.f - clb.position.y
    });
    win.draw(clTxt);

    // ==========================================
    // 6. CỘT PHẢI: KHUNG "SƠ ĐỒ PHÒNG CHIẾU"
    // ==========================================
    float rightX = position.x + 420.f;
    float rightW = width - 450.f;
    drawRoundedCard(win, {rightX, cardY}, {rightW, cardH}, 14.f, Color::White, 1.f, Color(226, 232, 240));

    // Header sơ đồ: SƠ ĐỒ PHÒNG X | Ma trận: ...
    wstring roomHead = (selectedRoomIndex >= 0 && selectedRoomIndex < (int)rooms.size())
        ? L"S\u01a0 \u0110\u1ed2 " + toUpperVi(rooms[selectedRoomIndex].name)
        : L"S\u01a0 \u0110\u1ed2 PH\u00d2NG CHI\u1ebeU";

    Text head1(boldFont, roomHead, 14);
    head1.setFillColor(Color(15, 23, 42));
    head1.setPosition({rightX + 24.f, cardY + 22.f});
    win.draw(head1);

    FloatRect h1b = head1.getGlobalBounds();
    Text head2(font, L" | Ma tr\u1eadn: " + to_wstring(rooms[selectedRoomIndex].rows) + L" H\u00e0ng x " + to_wstring(rooms[selectedRoomIndex].cols) + L" Gh\u1ebf (Kh\u1ea9u \u0111\u1ed9 r\u1ea1p 24m)", 12);
    head2.setFillColor(Color(100, 116, 139));
    head2.setPosition({h1b.position.x + h1b.size.x, cardY + 24.f});
    win.draw(head2);

    // Điều khiển Zoom: [ - ] 100% [ + ]
    drawRoundedCard(win, {rightX + rightW - 135.f, cardY + 18.f}, {115.f, 28.f}, 14.f, Color(241, 245, 249), 1.f, Color(226, 232, 240));
    Text zMinus(boldFont, L"-", 16); zMinus.setFillColor(Color(71, 85, 105)); zMinus.setPosition({rightX + rightW - 124.f, cardY + 20.f}); win.draw(zMinus);
    int zoomPercent = (int)(zoomFactor * 100.f + 0.5f);
    Text zVal(boldFont, to_wstring(zoomPercent) + L"%", 11); zVal.setFillColor(Color(15, 23, 42)); zVal.setPosition({rightX + rightW - 95.f, cardY + 24.f}); win.draw(zVal);
    Text zPlus(boldFont, L"+", 14); zPlus.setFillColor(Color(71, 85, 105)); zPlus.setPosition({rightX + rightW - 40.f, cardY + 22.f}); win.draw(zPlus);

    // Khối MÀN HÌNH (Cinema Screen)
    float screenW = rightW - 80.f;
    drawRoundedCard(win, {rightX + 40.f, cardY + 68.f}, {screenW, 30.f}, 6.f, Color(224, 238, 252));
    Text scrTxt(boldFont, L"M \u00c0 N   H \u00cc N H", 12);
    scrTxt.setFillColor(Color(2, 132, 199));
    FloatRect stb = scrTxt.getLocalBounds();
    scrTxt.setPosition({
        rightX + 40.f + (screenW - stb.size.x) / 2.f - stb.position.x,
        cardY + 68.f + (30.f - stb.size.y) / 2.f - stb.position.y
    });
    win.draw(scrTxt);

    // ==========================================
    // MA TRẬN GHẾ
    // ==========================================
    if (selectedRoomIndex >= 0 && selectedRoomIndex < (int)rooms.size()) {
        const auto& r = rooms[selectedRoomIndex];
        float seatW = 46.f * zoomFactor;
        float seatH = 46.f * zoomFactor;
        float gapX = 18.f * zoomFactor;
        float gapY = 16.f * zoomFactor;

        float totalW = r.cols * seatW + (r.cols - 1) * gapX;
        float gridStartX = rightX + (rightW - totalW) / 2.f;
        float gridStartY = cardY + 128.f;

        for (int row = 0; row < r.rows; ++row) {
            char rowChar = (char)('A' + row);
            string rowStr(1, rowChar);

            // Ký hiệu hàng bên trái
            Text lChar(boldFont, rowStr, (unsigned int)(13 * zoomFactor));
            lChar.setFillColor(Color(71, 85, 105));
            lChar.setPosition({gridStartX - 32.f * zoomFactor, gridStartY + row * (seatH + gapY) + seatH * 0.25f});
            win.draw(lChar);

            // Các ghế trong hàng
            for (int col = 0; col < r.cols; ++col) {
                float sX = gridStartX + col * (seatW + gapX);
                float sY = gridStartY + row * (seatH + gapY);
                bool isHov = (hoveredSeatRow == row && hoveredSeatCol == col);
                SeatType st = r.seatMatrix[row][col];
                drawSeat(win, sX, sY, seatW, seatH, st, to_string(col + 1), isHov);
            }

            // Ký hiệu hàng bên phải
            Text rChar(boldFont, rowStr, (unsigned int)(13 * zoomFactor));
            rChar.setFillColor(Color(71, 85, 105));
            rChar.setPosition({gridStartX + totalW + 18.f * zoomFactor, gridStartY + row * (seatH + gapY) + seatH * 0.25f});
            win.draw(rChar);
        }
    }

    // ==========================================
    // CHÚ THÍCH & BẢNG GIÁ (LEGEND BAR Ở ĐÁY)
    // ==========================================
    float legY = cardY + cardH - 62.f;
    float legW = (rightW - 60.f) / 4.f;

    // 1. Ghế Thường (100.000đ)
    drawRoundedCard(win, {rightX + 24.f, legY}, {legW - 10.f, 44.f}, 8.f, Color(241, 245, 249), 1.f, Color(226, 232, 240));
    drawSeat(win, rightX + 34.f, legY + 8.f, 22.f, 24.f, SeatType::STANDARD, "", false);
    Text lg1T(boldFont, L"Gh\u1ebf Th\u01b0\u1eddng", 12); lg1T.setFillColor(Color(15, 23, 42)); lg1T.setPosition({rightX + 64.f, legY + 6.f}); win.draw(lg1T);
    Text lg1P(font, L"100.000\u0111", 12); lg1P.setFillColor(Color(100, 116, 139)); lg1P.setPosition({rightX + 64.f, legY + 22.f}); win.draw(lg1P);

    // 2. Ghế VIP Gold (140.000đ)
    drawRoundedCard(win, {rightX + 24.f + legW, legY}, {legW - 10.f, 44.f}, 8.f, Color(254, 243, 199), 1.f, Color(253, 230, 138));
    drawSeat(win, rightX + 34.f + legW, legY + 8.f, 22.f, 24.f, SeatType::VIP, "", false);
    Text lg2T(boldFont, L"Gh\u1ebf VIP (Gold)", 12); lg2T.setFillColor(Color(180, 83, 9)); lg2T.setPosition({rightX + 64.f + legW, legY + 6.f}); win.draw(lg2T);
    Text lg2P(font, L"140.000\u0111", 12); lg2P.setFillColor(Color(217, 119, 6)); lg2P.setPosition({rightX + 64.f + legW, legY + 22.f}); win.draw(lg2P);

    // 3. Ghế Đôi Sweetbox (220.000đ/cặp)
    drawRoundedCard(win, {rightX + 24.f + legW * 2.f, legY}, {legW - 10.f, 44.f}, 8.f, Color(238, 242, 255), 1.f, Color(199, 210, 254));
    drawSeat(win, rightX + 34.f + legW * 2.f, legY + 8.f, 22.f, 24.f, SeatType::SWEETBOX, "", false);
    Text lg3T(boldFont, L"Gh\u1ebf \u0110\u00f4i Sweetbox", 12); lg3T.setFillColor(Color(30, 58, 95)); lg3T.setPosition({rightX + 64.f + legW * 2.f, legY + 6.f}); win.draw(lg3T);
    Text lg3P(font, L"220.000\u0111 / c\u1eb7p", 12); lg3P.setFillColor(Color(79, 70, 229)); lg3P.setPosition({rightX + 64.f + legW * 2.f, legY + 22.f}); win.draw(lg3P);

    // 4. Ghế bảo trì (Tạm ngừng bán)
    drawRoundedCard(win, {rightX + 24.f + legW * 3.f, legY}, {legW - 10.f, 44.f}, 8.f, Color(254, 242, 242), 1.f, Color(254, 202, 202));
    drawSeat(win, rightX + 34.f + legW * 3.f, legY + 8.f, 22.f, 24.f, SeatType::MAINTENANCE, "", false);
    Text lg4T(boldFont, L"Gh\u1ebf b\u1ea3o tr\u00ec", 12); lg4T.setFillColor(Color(185, 28, 28)); lg4T.setPosition({rightX + 64.f + legW * 3.f, legY + 6.f}); win.draw(lg4T);
    Text lg4P(font, L"T\u1ea1m ng\u1eebng b\u00e1n", 12); lg4P.setFillColor(Color(239, 68, 68)); lg4P.setPosition({rightX + 64.f + legW * 3.f, legY + 22.f}); win.draw(lg4P);

    // ==========================================
    // 7. POPUP / DROPDOWN MENUS (Render trên cùng)
    // ==========================================
    if (roomDropdownOpen) {
        float ddY = cardY + 120.f;
        drawRoundedCard(win, {position.x + 50.f, ddY}, {leftCardW - 40.f, (float)rooms.size() * 36.f}, 8.f, Color::White, 1.5f, Color(2, 132, 199));
        for (size_t i = 0; i < rooms.size(); ++i) {
            float iY = ddY + i * 36.f;
            if (i == (size_t)selectedRoomIndex) {
                RoundedRectRenderer::draw(win, {position.x + 52.f, iY}, {leftCardW - 44.f, 35.f}, 4.f, Color(224, 242, 254));
            }
            Text rItm(boldFont, rooms[i].name + L" - " + rooms[i].roomType, 12);
            rItm.setFillColor((i == (size_t)selectedRoomIndex) ? Color(2, 132, 199) : Color(15, 23, 42));
            rItm.setPosition({position.x + 62.f, iY + 9.f});
            win.draw(rItm);
        }
    }

    if (seatTypeDropdownOpen) {
        float ddY = cardY + 408.f;
        drawRoundedCard(win, {position.x + 50.f, ddY}, {leftCardW - 40.f, 144.f}, 8.f, Color::White, 1.5f, Color(2, 132, 199));
        wstring typeNames[4] = {
            L"Gh\u1ebf Th\u01b0\u1eddng (M\u00e0u Xanh Nh\u1ea1t)",
            L"Gh\u1ebf VIP (M\u00e0u V\u00e0ng / Cao c\u1ea5p)",
            L"Gh\u1ebf \u0110\u00f4i Sweetbox (M\u00e0u Xanh \u0110\u1eadm)",
            L"Gh\u1ebf b\u1ea3o tr\u00ec (T\u1ea1m ng\u1eebng)"
        };
        SeatType types[4] = {SeatType::STANDARD, SeatType::VIP, SeatType::SWEETBOX, SeatType::MAINTENANCE};
        for (int i = 0; i < 4; ++i) {
            float iY = ddY + i * 36.f;
            if (types[i] == currentAssignType) {
                RoundedRectRenderer::draw(win, {position.x + 52.f, iY}, {leftCardW - 44.f, 35.f}, 4.f, Color(224, 242, 254));
            }
            Text tItm(boldFont, typeNames[i], 12);
            tItm.setFillColor((types[i] == currentAssignType) ? Color(2, 132, 199) : Color(15, 23, 42));
            tItm.setPosition({position.x + 62.f, iY + 9.f});
            win.draw(tItm);
        }
    }

    // ==========================================
    // 8. TOAST NOTIFICATION (nếu có)
    // ==========================================
    if (notificationVisible) {
        if (notificationClock.getElapsedTime().asSeconds() > 2.5f) {
            notificationVisible = false;
        } else {
            float notW = 400.f;
            float notH = 44.f;
            float notX = position.x + (width - notW) / 2.f;
            float notY = position.y + 75.f; // Nằm ở giữa trên cao, cực kỳ rõ ràng và dễ thấy
            drawRoundedCard(win, {notX, notY}, {notW, notH}, 8.f, Color(15, 23, 42, 245), 1.5f, Color(2, 132, 199));
            Text notText(boldFont, s2ws(notificationMessage), 12);
            notText.setFillColor(Color::White);
            FloatRect ntb = notText.getLocalBounds();
            notText.setPosition({
                std::floor(notX + (notW - ntb.size.x) / 2.f - ntb.position.x),
                std::floor(notY + (notH - ntb.size.y) / 2.f - ntb.position.y)
            });
            win.draw(notText);
        }
    }
}