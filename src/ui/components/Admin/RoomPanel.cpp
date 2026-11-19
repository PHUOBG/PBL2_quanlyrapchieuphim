#include "UI/components/Admin/RoomPanel.h"
#include "UI/components/Admin/FontManager.h"
#include "UI/components/Admin/UITheme.h"
#include "UI/components/Admin/RoundedRectRenderer.h"
#include "utils/FileUtils.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <cmath>
#include <ctime>
#include <unordered_map>

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
      boldFont(FontManager::instance().bold()),
      width(w), height(h), position(280.f, 0.f),
      zoomFactor(1.0f)
{

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
    width = 1728.f - pos.x;
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
    ifstream file(filePath, ios::binary);
    if (!file.is_open()) return;

    // Load movie titles & durations for live room status calculation
    unordered_map<string, string> movieTitleById;
    unordered_map<string, int> movieDurById;
    {
        ifstream mf(FileUtils::resolveDataPath("data/movies.txt"), ios::binary);
        string mline; bool hdr = true;
        while (mf.is_open() && getline(mf, mline)) {
            if (!mline.empty() && mline.back() == '\r') mline.pop_back();
            if (mline.empty()) continue;
            if (hdr) { hdr = false; continue; }
            stringstream mss(mline);
            string id, title, tmp, durStr;
            getline(mss, id, '|');
            getline(mss, title, '|');
            for (int k = 0; k < 4; ++k) getline(mss, tmp, '|');
            getline(mss, durStr, '|');
            if (!id.empty()) {
                movieTitleById[id] = title;
                int d = 120;
                try { d = stoi(durStr); } catch (...) {}
                movieDurById[id] = d > 0 ? d : 120;
            }
        }
    }

    // Current local date (YYYY-MM-DD) & minutes since midnight
    time_t now = time(nullptr);
    tm ltm{};
#if defined(_WIN32)
    localtime_s(&ltm, &now);
#else
    localtime_r(&now, &ltm);
#endif
    char todayBuf[16];
    strftime(todayBuf, sizeof(todayBuf), "%Y-%m-%d", &ltm);
    const string todayStr(todayBuf);
    const int nowMin = ltm.tm_hour * 60 + ltm.tm_min;

    struct RoomScheduleInfo {
        bool isPlaying = false;
        string playingTitle;
        int nextStartMin = 999999;
        string nextTimeStr;
        int upcomingCount = 0;
    };
    unordered_map<string, RoomScheduleInfo> schedByRoom;
    {
        ifstream sf(FileUtils::resolveDataPath("data/showtimes.txt"), ios::binary);
        string sline; bool hdr = true;
        while (sf.is_open() && getline(sf, sline)) {
            if (!sline.empty() && sline.back() == '\r') sline.pop_back();
            if (sline.empty()) continue;
            if (hdr) { hdr = false; continue; }
            stringstream sss(sline);
            string sid, mid, rid, sdate, stime;
            getline(sss, sid, '|');
            getline(sss, mid, '|');
            getline(sss, rid, '|');
            getline(sss, sdate, '|');
            getline(sss, stime, '|');
            if (rid.empty() || sdate.empty() || stime.size() < 5) continue;
            if (sdate < todayStr) continue;
            int sh = 0, sm = 0;
            try { sh = stoi(stime.substr(0, 2)); sm = stoi(stime.substr(3, 2)); } catch (...) { continue; }
            int startMin = sh * 60 + sm;
            int dur = movieDurById.count(mid) ? movieDurById[mid] : 120;
            auto& info = schedByRoom[rid];
            if (sdate == todayStr && nowMin >= startMin && nowMin < startMin + dur) {
                info.isPlaying = true;
                info.playingTitle = movieTitleById.count(mid) ? movieTitleById[mid] : mid;
            } else if (sdate > todayStr || (sdate == todayStr && startMin > nowMin)) {
                info.upcomingCount++;
                if (sdate == todayStr && startMin < info.nextStartMin) {
                    info.nextStartMin = startMin;
                    info.nextTimeStr = stime;
                }
            }
        }
    }

    auto buildMatrix = [](int rw, int cl) {
        vector<vector<SeatType>> mat(rw, vector<SeatType>(cl, SeatType::STANDARD));
        if (rw >= 2 && cl >= 4) {
            mat[1][1] = SeatType::VIP;
            mat[1][2] = SeatType::VIP;
            mat[1][3] = SeatType::VIP;
            if (cl > 4) mat[1][4] = SeatType::VIP;
        }
        if (rw >= 6) {
            for (int c = 0; c < cl; ++c) {
                mat[rw - 2][c] = SeatType::SWEETBOX;
                mat[rw - 1][c] = SeatType::SWEETBOX;
            }
        }
        return mat;
    };

    vector<RoomData> loaded;
    string line;
    bool isHeader = true;
    while (getline(file, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;
        if (isHeader) { isHeader = false; continue; }
        stringstream ss(line);
        string id, nameStr, rowsStr, colsStr, totalStr;
        getline(ss, id, '|');
        getline(ss, nameStr, '|');
        getline(ss, rowsStr, '|');
        getline(ss, colsStr, '|');
        getline(ss, totalStr, '|');
        if (id.empty()) continue;

        int rw = 6, cl = 6;
        try {
            int rParsed = stoi(rowsStr);
            int cParsed = stoi(colsStr);
            if (rParsed >= 1 && rParsed <= 10 && cParsed >= 1 && cParsed <= 10) {
                rw = rParsed;
                cl = cParsed;
            }
        } catch (...) {}

        RoomData r;
        r.id = id;
        r.name = s2ws(nameStr.empty() ? id : nameStr);
        r.rows = rw;
        r.cols = cl;
        r.seatMatrix = buildMatrix(rw, cl);

        // Preserve roomType if matching an existing default entry
        r.roomType = L"Digital 2D Dolby 7.1";
        for (const auto& existing : rooms) {
            if (existing.id == id) { r.roomType = existing.roomType; break; }
        }

        // Compute live status from showtimes.txt
        auto sit = schedByRoom.find(id);
        if (sit != schedByRoom.end() && sit->second.isPlaying) {
            r.status = L"\u0110ang chi\u1ebfu: " + s2ws(sit->second.playingTitle);
        } else if (sit != schedByRoom.end() && !sit->second.nextTimeStr.empty()) {
            r.status = L"S\u1eafp chi\u1ebfu (" + s2ws(sit->second.nextTimeStr) + L")";
        } else if (sit != schedByRoom.end() && sit->second.upcomingCount > 0) {
            r.status = L"B\u00ecnh th\u01b0\u1eddng (" + to_wstring(sit->second.upcomingCount) + L" su\u1ea5t s\u1eafp t\u1edbi)";
        } else {
            r.status = L"Tr\u1ed1ng (Ch\u01b0a c\u00f3 su\u1ea5t chi\u1ebfu)";
        }

        loaded.push_back(std::move(r));
    }

    if (!loaded.empty()) {
        rooms = std::move(loaded);
    }
}

bool RoomPanel::saveRoomsToFile() {
    string filePath = FileUtils::resolveDataPath("data/rooms.txt");
    auto sanitize = [](string s) {
        for (char& ch : s) if (ch == '|' || ch == '\n' || ch == '\r') ch = ' ';
        return s;
    };
    string out = "room_id|room_name|seat_rows|seat_cols|total_seats\n";
    for (const auto& r : rooms) {
        out += sanitize(r.id) + "|" + sanitize(ws2s(r.name)) + "|" + to_string(r.rows) + "|" + to_string(r.cols) + "|" + to_string(r.rows * r.cols) + "\n";
    }
    return FileUtils::writeAtomic(filePath, out);
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
            bodyColor = Color(220, 38, 38);         // Red alert
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
    RoundedRectRenderer::draw(win, {x, y}, {w, h * 0.72f}, 6.f, bodyColor, hovered ? 1.5f : 0.f, Color(8, 125, 181));

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
            FloatRect btnRefresh({position.x + width - 470.f, position.y + 38.f}, {110.f, 38.f});
            FloatRect btnAdd({position.x + width - 350.f, position.y + 38.f}, {170.f, 38.f});
            FloatRect btnDelete({position.x + width - 170.f, position.y + 38.f}, {145.f, 38.f});

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
            FloatRect zoomOutBtn({position.x + width - 165.f, position.y + 184.f}, {28.f, 26.f});
            FloatRect zoomInBtn({position.x + width - 68.f, position.y + 184.f}, {28.f, 26.f});
            if (zoomOutBtn.contains(mousePos)) {
                zoomFactor = max(0.7f, zoomFactor - 0.1f);
                return;
            }
            if (zoomInBtn.contains(mousePos)) {
                zoomFactor = min(1.4f, zoomFactor + 0.1f);
                return;
            }

            float cardY = position.y + 164.f;
            float leftCardW = 370.f;
            float cardH = height - 230.f;

            // Ưu tiên kiểm tra danh sách dropdown đang mở trước để không click xuyên xuống các ô bên dưới
            if (roomDropdownOpen) {
                float ddY = cardY + 122.f;
                for (size_t i = 0; i < rooms.size(); ++i) {
                    FloatRect itemB({position.x + 50.f, ddY + i * 36.f}, {leftCardW - 40.f, 36.f});
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
                return;
            }

            if (statusDropdownOpen) {
                const wstring statusOpts[2] = {
                    L"B\u00ecnh th\u01b0\u1eddng (\u0110ang \u0111\u00f3n kh\u00e1ch)",
                    L"\u0110ang b\u1ea3o tr\u00ec k\u1ef9 thu\u1eadt"
                };
                float ddY = cardY + 434.f;
                for (int i = 0; i < 2; ++i) {
                    FloatRect itmB({position.x + 50.f, ddY + i * 36.f}, {leftCardW - 40.f, 36.f});
                    if (itmB.contains(mousePos)) {
                        inputStatus = statusOpts[i];
                        statusDropdownOpen = false;
                        return;
                    }
                }
                statusDropdownOpen = false;
                return;
            }

            if (seatTypeDropdownOpen) {
                SeatType types[4] = {SeatType::STANDARD, SeatType::VIP, SeatType::SWEETBOX, SeatType::MAINTENANCE};
                float ddY = cardY + 508.f;
                for (int i = 0; i < 4; ++i) {
                    FloatRect itmB({position.x + 50.f, ddY + i * 36.f}, {leftCardW - 40.f, 36.f});
                    if (itmB.contains(mousePos)) {
                        currentAssignType = types[i];
                        seatTypeDropdownOpen = false;
                        return;
                    }
                }
                seatTypeDropdownOpen = false;
                return;
            }

            // 4. Form Cột trái: Click chọn dropdown phòng chiếu
            FloatRect ddRoomBounds({position.x + 50.f, cardY + 82.f}, {leftCardW - 40.f, 38.f});
            if (ddRoomBounds.contains(mousePos)) {
                roomDropdownOpen = !roomDropdownOpen;
                statusDropdownOpen = false;
                seatTypeDropdownOpen = false;
                activeInputBox = -1;
                return;
            }

            // Stepper - / + cho Số lượng hàng ghế & Số lượng cột ghế (giới hạn 1..10)
            auto applyGridSize = [&](int newR, int newC) {
                newR = max(1, min(10, newR));
                newC = max(1, min(10, newC));
                inputRows = to_string(newR);
                inputCols = to_string(newC);
                if (selectedRoomIndex >= 0 && selectedRoomIndex < (int)rooms.size()) {
                    auto& r = rooms[selectedRoomIndex];
                    vector<vector<SeatType>> nextMat(newR, vector<SeatType>(newC, SeatType::STANDARD));
                    for (int rr = 0; rr < min(newR, r.rows); ++rr) {
                        for (int cc = 0; cc < min(newC, r.cols); ++cc) {
                            nextMat[rr][cc] = r.seatMatrix[rr][cc];
                        }
                    }
                    r.rows = newR;
                    r.cols = newC;
                    r.seatMatrix = std::move(nextMat);
                }
            };
            float stepperW = leftCardW - 40.f;
            FloatRect btnRowMinus({position.x + 56.f, cardY + 233.f}, {38.f, 38.f});
            FloatRect btnRowPlus({position.x + 50.f + stepperW - 44.f, cardY + 233.f}, {38.f, 38.f});
            FloatRect btnColMinus({position.x + 56.f, cardY + 315.f}, {38.f, 38.f});
            FloatRect btnColPlus({position.x + 50.f + stepperW - 44.f, cardY + 315.f}, {38.f, 38.f});
            int curR = 6, curC = 6;
            try { curR = stoi(inputRows); } catch (...) {}
            try { curC = stoi(inputCols); } catch (...) {}
            if (btnRowMinus.contains(mousePos)) { applyGridSize(curR - 1, curC); activeInputBox = -1; return; }
            if (btnRowPlus.contains(mousePos))  { applyGridSize(curR + 1, curC); activeInputBox = -1; return; }
            if (btnColMinus.contains(mousePos)) { applyGridSize(curR, curC - 1); activeInputBox = -1; return; }
            if (btnColPlus.contains(mousePos))  { applyGridSize(curR, curC + 1); activeInputBox = -1; return; }

            // Click ô Trạng thái phòng chiếu (chuyển đổi Bình thường / Đang bảo trì)
            FloatRect boxStatus({position.x + 50.f, cardY + 394.f}, {leftCardW - 40.f, 38.f});
            if (boxStatus.contains(mousePos)) {
                statusDropdownOpen = !statusDropdownOpen;
                roomDropdownOpen = false;
                seatTypeDropdownOpen = false;
                activeInputBox = -1;
                return;
            }

            // Click ô loại ghế cần gán
            FloatRect ddSeatTypeBounds({position.x + 50.f, cardY + 468.f}, {leftCardW - 40.f, 38.f});
            if (ddSeatTypeBounds.contains(mousePos)) {
                seatTypeDropdownOpen = !seatTypeDropdownOpen;
                roomDropdownOpen = false;
                statusDropdownOpen = false;
                activeInputBox = -1;
                return;
            }

            // Click ô Input Name
            FloatRect boxName({position.x + 50.f, cardY + 154.f}, {leftCardW - 40.f, 38.f});
            if (boxName.contains(mousePos)) { activeInputBox = 0; return; }
            activeInputBox = -1;

            // Nút LƯU và HỦY BỎ (khớp tọa độ render: actBtnY = cardY + cardH - 58.f)
            float actBtnY = cardY + cardH - 58.f;
            FloatRect btnSave({position.x + 50.f, actBtnY}, {155.f, 40.f});
            FloatRect btnCancel({position.x + 225.f, actBtnY}, {155.f, 40.f});

            if (btnSave.contains(mousePos)) {
                if (selectedRoomIndex >= 0 && selectedRoomIndex < (int)rooms.size()) {
                    auto& r = rooms[selectedRoomIndex];
                    RoomData backup = r;
                    if (inputRoomName.empty()) {
                        notificationMessage = "Tên phòng chiếu không được để trống!";
                        notificationClock.restart();
                        notificationVisible = true;
                        return;
                    }
                    int nr = r.rows, nc = r.cols;
                    try {
                        nr = stoi(inputRows);
                        nc = stoi(inputCols);
                    } catch (...) {
                        nr = -1;
                    }
                    if (nr < 1 || nr > 10 || nc < 1 || nc > 10) {
                        notificationMessage = "Số hàng và số cột ghế chỉ được từ 1 đến 10!";
                        notificationClock.restart();
                        notificationVisible = true;
                        return;
                    }
                    r.name = inputRoomName;
                    if (!inputStatus.empty()) r.status = inputStatus;
                    if (nr != r.rows || nc != r.cols) {
                        r.rows = nr;
                        r.cols = nc;
                        r.seatMatrix.assign(nr, vector<SeatType>(nc, SeatType::STANDARD));
                    }
                    if (!saveRoomsToFile()) {
                        r = backup;
                        notificationMessage = "Lỗi: Không thể lưu cấu hình phòng chiếu!";
                    } else {
                        notificationMessage = "Đã lưu cấu hình phòng chiếu thành công!";
                    }
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
                    inputStatus = rooms[selectedRoomIndex].status;
                    notificationMessage = "Đã hoàn tác thay đổi!";
                    notificationClock.restart();
                    notificationVisible = true;
                }
                return;
            }

            // 5. Cột phải: Click trực tiếp vào ghế để đổi loại ghế
            if (selectedRoomIndex >= 0 && selectedRoomIndex < (int)rooms.size()) {
                auto& r = rooms[selectedRoomIndex];
                float rightX = position.x + 420.f;
                float rightCardW = width - 450.f;
                float gridStartY = cardY + 128.f;
                float seatW = 42.f * zoomFactor;
                float seatH = 42.f * zoomFactor;
                float gapX = 16.f * zoomFactor;
                float gapY = 12.f * zoomFactor;

                // Căn giữa sơ đồ trong khung phải
                float totalW = r.cols * seatW + (r.cols - 1) * gapX;
                float gridStartX = rightX + (rightCardW - totalW) / 2.f;

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

    // Xử lý phím điều hướng và xóa khi đang active input box
    if (const auto* keyEv = event.getIf<Event::KeyPressed>()) {
        if (activeInputBox >= 0) {
            if (keyEv->code == Keyboard::Key::Tab) {
                activeInputBox = (activeInputBox + 1) % 3;
                return;
            } else if (keyEv->code == Keyboard::Key::Escape) {
                activeInputBox = -1;
                return;
            } else if (keyEv->code == Keyboard::Key::Backspace) {
                if (activeInputBox == 0 && !inputRoomName.empty()) inputRoomName.pop_back();
                else if (activeInputBox == 1 && !inputRows.empty()) inputRows.pop_back();
                else if (activeInputBox == 2 && !inputCols.empty()) inputCols.pop_back();
                return;
            }
        }
    }

    // Xử lý gõ phím khi đang active input box
    if (const auto* textEv = event.getIf<Event::TextEntered>()) {
        char32_t c = textEv->unicode;
        if (c < 32 || c == 127) return;
        if (activeInputBox == 0) { // Tên phòng
            if (inputRoomName.size() < 28) inputRoomName.push_back((wchar_t)c);
        } else if (activeInputBox == 1) { // Hàng ghế (1-10)
            if (c >= '0' && c <= '9') {
                string next = inputRows + static_cast<char>(c);
                try {
                    int val = stoi(next);
                    if (val >= 1 && val <= 10 && next.size() <= 2 && !(inputRows.empty() && c == '0')) {
                        inputRows = next;
                    }
                } catch (...) {}
            }
        } else if (activeInputBox == 2) { // Cột ghế (1-10)
            if (c >= '0' && c <= '9') {
                string next = inputCols + static_cast<char>(c);
                try {
                    int val = stoi(next);
                    if (val >= 1 && val <= 10 && next.size() <= 2 && !(inputCols.empty() && c == '0')) {
                        inputCols = next;
                    }
                } catch (...) {}
            }
        }
    }
}

void RoomPanel::update(Vector2f mousePos, bool) {
    hoveredSeatRow = -1;
    hoveredSeatCol = -1;

    if (selectedRoomIndex >= 0 && selectedRoomIndex < (int)rooms.size()) {
        const auto& r = rooms[selectedRoomIndex];
        float seatW = 42.f * zoomFactor;
        float seatH = 42.f * zoomFactor;
        float gapX = 16.f * zoomFactor;
        float gapY = 12.f * zoomFactor;

        float totalW = r.cols * seatW + (r.cols - 1) * gapX;
        float rightX = position.x + 420.f;
        float rightCardW = width - 450.f;
        float gridStartX = rightX + (rightCardW - totalW) / 2.f;
        float gridStartY = position.y + 164.f + 128.f;

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
    // ==========================================
    // 3. PAGE HEADER CARD (Y = 56.f)
    // ==========================================
    drawRoundedCard(win, {position.x + 30.f, position.y + 16.f}, {width - 60.f, 84.f}, 12.f, Color::White, 1.f, Color(226, 232, 240));

    // Logo CineXine trong ô bo tròn (thay thế 2 gạch theo yêu cầu)
    drawRoundedCard(win, {position.x + 48.f, position.y + 32.f}, {52.f, 52.f}, 12.f, Color(224, 242, 254));
    if (hasLogo) {
        Sprite logoSpr(logoTex);
        float targetS = 36.f;
        float sX = targetS / (float)logoTex.getSize().x;
        float sY = targetS / (float)logoTex.getSize().y;
        logoSpr.setScale({sX, sY});
        logoSpr.setPosition({position.x + 48.f + (52.f - targetS) / 2.f, position.y + 32.f + (52.f - targetS) / 2.f});
        win.draw(logoSpr);
    }

    Text title(boldFont, L"Qu\u1ea3n l\u00fd ph\u00f2ng chi\u1ebfu", 24);
    title.setFillColor(Color(23, 32, 51));
    title.setPosition({position.x + 115.f, position.y + 30.f});
    win.draw(title);

    // Badge "Hoạt động ổn định" (đồng bộ màu xanh dương với các chức năng khác)
    FloatRect tbBounds = title.getGlobalBounds();
    Text gTxt(boldFont, L"Ho\u1ea1t \u0111\u1ed9ng \u1ed5n \u0111\u1ecbnh", 14);
    gTxt.setFillColor(Color(8, 125, 181));
    FloatRect gtb = gTxt.getLocalBounds();
    float dotRadius = 3.5f;
    float dotGap = 6.f;
    float padH = 12.f;
    float badgeW = padH * 2.f + dotRadius * 2.f + dotGap + gtb.size.x;
    float badgeH = 26.f;
    float badgeX = tbBounds.position.x + tbBounds.size.x + 14.f;
    float badgeY = position.y + 32.f;
    drawRoundedCard(win, {badgeX, badgeY}, {badgeW, badgeH}, 13.f, Color(224, 242, 254));

    float contentW = dotRadius * 2.f + dotGap + gtb.size.x;
    float startX = badgeX + (badgeW - contentW) / 2.f;
    CircleShape gDot(dotRadius);
    gDot.setFillColor(Color(8, 125, 181));
    gDot.setPosition({startX, badgeY + (badgeH - dotRadius * 2.f) / 2.f});
    win.draw(gDot);

    gTxt.setPosition({
        startX + dotRadius * 2.f + dotGap - gtb.position.x,
        badgeY + (badgeH - gtb.size.y) / 2.f - gtb.position.y
    });
    win.draw(gTxt);

    Text subTitle(font, L"H\u1ec7 th\u1ed1ng c\u1ea5u h\u00ecnh s\u01a1 \u0111\u1ed3 gh\u1ebf v\u00e0 gi\u00e1m s\u00e1t tr\u1ea1ng th\u00e1i ph\u00f2ng chi\u1ebfu t\u1ea1i c\u1ee5m r\u1ea1p \u0110\u00e0 N\u1eb5ng", 17);
    subTitle.setFillColor(Color(100, 116, 139));
    subTitle.setPosition({std::round(position.x + 115.f), std::round(position.y + 61.f)});
    win.draw(subTitle);

    // 3 nút góc phải: Làm mới, Thêm phòng chiếu, Xóa phòng chiếu
    float btnY = position.y + 38.f;
    // Nút 1: Làm mới (vector refresh icon sắc nét)
    float headRefX = position.x + width - 470.f;
    drawRoundedCard(win, {headRefX, btnY}, {110.f, 38.f}, 8.f, Color::White, 1.2f, Color(203, 213, 225));
    UITheme::drawReloadIcon(win, {headRefX + 22.f, btnY + 19.f}, 16.f, Color(51, 65, 85));
    Text refTxt(boldFont, L"L\u00e0m m\u1edbi", 14);
    refTxt.setFillColor(Color(51, 65, 85));
    refTxt.setPosition({headRefX + 38.f, btnY + 9.f});
    win.draw(refTxt);

    // Nút 2: + Thêm phòng chiếu
    drawRoundedCard(win, {position.x + width - 350.f, btnY}, {170.f, 38.f}, 8.f, Color(8, 125, 181));
    Text addTxt(boldFont, L"+ Th\u00eam ph\u00f2ng chi\u1ebfu", 14);
    addTxt.setFillColor(Color::White);
    FloatRect addb = addTxt.getLocalBounds();
    addTxt.setPosition({
        position.x + width - 350.f + (170.f - addb.size.x) / 2.f - addb.position.x,
        btnY + (38.f - addb.size.y) / 2.f - addb.position.y
    });
    win.draw(addTxt);

    // Nút 3: Xóa phòng
    drawRoundedCard(win, {position.x + width - 170.f, btnY}, {140.f, 38.f}, 8.f, Color(220, 38, 38));
    Text delTxt(boldFont, L"X\u00f3a ph\u00f2ng", 14);
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
    drawRoundedCard(win, {position.x + 30.f, position.y + 116.f}, {280.f, 36.f}, 8.f, Color::White, 1.f, Color(226, 232, 240));
    Text secHeadTxt(boldFont, L"C\u1ea5u h\u00ecnh & S\u01a1 \u0111\u1ed3 gh\u1ebf tr\u1ef1c quan", 14);
    secHeadTxt.setFillColor(Color(8, 125, 181));
    FloatRect shb = secHeadTxt.getLocalBounds();
    secHeadTxt.setPosition({
        position.x + 30.f + (280.f - shb.size.x) / 2.f - shb.position.x,
        position.y + 116.f + (36.f - shb.size.y) / 2.f - shb.position.y
    });
    win.draw(secHeadTxt);

    // ==========================================
    // 5. CỘT TRÁI: FORM "THÔNG TIN PHÒNG CHIẾU"
    // ==========================================
    float cardY = position.y + 164.f;
    float leftCardW = 370.f;
    float cardH = height - 230.f;
    drawRoundedCard(win, {position.x + 30.f, cardY}, {leftCardW, cardH}, 14.f, Color::White, 1.f, Color(226, 232, 240));

    // Tiêu đề form & Badge "BÌNH THƯỜNG" (cân đối, sắc nét)
    Text formTitle(boldFont, L"Th\u00f4ng tin ph\u00f2ng chi\u1ebfu", 18);
    formTitle.setFillColor(Color(23, 32, 51));
    formTitle.setPosition({position.x + 50.f, cardY + 18.f});
    win.draw(formTitle);

    Text stBadge(boldFont, L"B\u00ccNH TH\u01af\u1edcNG", 11);
    stBadge.setFillColor(Color(4, 120, 87));
    FloatRect sbb = stBadge.getLocalBounds();
    float sBadgeW = sbb.size.x + 22.f;
    float sBadgeH = 22.f;
    float sBadgeX = position.x + 30.f + leftCardW - 20.f - sBadgeW;
    float sBadgeY = cardY + 19.f;
    drawRoundedCard(win, {sBadgeX, sBadgeY}, {sBadgeW, sBadgeH}, 11.f, Color(209, 250, 229));
    stBadge.setPosition({
        std::round(sBadgeX + (sBadgeW - sbb.size.x) / 2.f - sbb.position.x),
        std::round(sBadgeY + (sBadgeH - sbb.size.y) / 2.f - sbb.position.y - 0.5f)
    });
    win.draw(stBadge);

    // 1. Chọn phòng chiếu
    Text lbl1(boldFont, L"Ch\u1ecdn ph\u00f2ng chi\u1ebfu:", 14); lbl1.setFillColor(Color(51, 65, 85)); lbl1.setPosition({position.x + 50.f, cardY + 60.f}); win.draw(lbl1);
    drawRoundedCard(win, {position.x + 50.f, cardY + 82.f}, {leftCardW - 40.f, 38.f}, 8.f, Color(248, 250, 252), 1.f, Color(203, 213, 225));
    wstring currRoomLabel = (selectedRoomIndex >= 0 && selectedRoomIndex < (int)rooms.size())
        ? rooms[selectedRoomIndex].name + L" (" + rooms[selectedRoomIndex].roomType + L")"
        : L"Ch\u1ecdn ph\u00f2ng...";
    Text currRoomTxt(boldFont, currRoomLabel, 14); currRoomTxt.setFillColor(Color(23, 32, 51)); currRoomTxt.setPosition({position.x + 62.f, cardY + 91.f}); win.draw(currRoomTxt);
    Text arr1(boldFont, L"\u25bc", 14); arr1.setFillColor(Color(100, 116, 139)); arr1.setPosition({position.x + leftCardW - 15.f, cardY + 93.f}); win.draw(arr1);

    // 2. Tên phòng chiếu
    Text lbl2(boldFont, L"T\u00ean ph\u00f2ng chi\u1ebfu:", 14); lbl2.setFillColor(Color(51, 65, 85)); lbl2.setPosition({position.x + 50.f, cardY + 132.f}); win.draw(lbl2);
    drawRoundedCard(win, {position.x + 50.f, cardY + 154.f}, {leftCardW - 40.f, 38.f}, 8.f, Color(248, 250, 252), (activeInputBox == 0) ? 1.5f : 1.f, (activeInputBox == 0) ? Color(8, 125, 181) : Color(203, 213, 225));
    Text nameInputTxt(boldFont, inputRoomName + ((activeInputBox == 0) ? L"|" : L""), 14); nameInputTxt.setFillColor(Color(23, 32, 51)); nameInputTxt.setPosition({position.x + 62.f, cardY + 163.f}); win.draw(nameInputTxt);

    // Helper vẽ Stepper [ - |  N  HÀNG/CỘT GHẾ  | + ]
    auto drawStepper = [&](float topY, Color dotColor, const wstring& title, const wstring& rangeHint,
                           const string& valStr, const wstring& unitLabel) {
        float boxW = leftCardW - 40.f;
        float boxX = position.x + 50.f;
        CircleShape dot(4.f);
        dot.setFillColor(dotColor);
        dot.setPosition({boxX, topY + 5.f});
        win.draw(dot);

        Text sLbl(boldFont, title, 14);
        sLbl.setFillColor(Color(23, 32, 51));
        sLbl.setPosition({boxX + 14.f, topY});
        win.draw(sLbl);

        Text sHint(font, rangeHint, 12);
        sHint.setFillColor(Color(100, 116, 139));
        FloatRect hb = sHint.getLocalBounds();
        sHint.setPosition({boxX + boxW - hb.size.x - hb.position.x, topY + 2.f});
        win.draw(sHint);

        float ctrlY = topY + 22.f;
        drawRoundedCard(win, {boxX, ctrlY}, {boxW, 48.f}, 12.f, Color(248, 250, 252), 1.f, Color(226, 232, 240));

        // Nút -
        drawRoundedCard(win, {boxX + 6.f, ctrlY + 5.f}, {38.f, 38.f}, 8.f, Color::White, 1.f, Color(226, 232, 240));
        Text mTxt(boldFont, L"-", 18);
        mTxt.setFillColor(Color(51, 65, 85));
        FloatRect mb = mTxt.getLocalBounds();
        mTxt.setPosition({boxX + 6.f + (38.f - mb.size.x) / 2.f - mb.position.x, ctrlY + 5.f + (38.f - mb.size.y) / 2.f - mb.position.y - 1.f});
        win.draw(mTxt);

        // Nút +
        float plusX = boxX + boxW - 44.f;
        drawRoundedCard(win, {plusX, ctrlY + 5.f}, {38.f, 38.f}, 8.f, Color::White, 1.f, Color(226, 232, 240));
        Text pTxt(boldFont, L"+", 18);
        pTxt.setFillColor(Color(51, 65, 85));
        FloatRect pb = pTxt.getLocalBounds();
        pTxt.setPosition({plusX + (38.f - pb.size.x) / 2.f - pb.position.x, ctrlY + 5.f + (38.f - pb.size.y) / 2.f - pb.position.y - 1.f});
        win.draw(pTxt);

        // Số ở giữa & nhãn nhỏ
        Text vTxt(boldFont, s2ws(valStr), 18);
        vTxt.setFillColor(Color(23, 32, 51));
        FloatRect vb = vTxt.getLocalBounds();
        vTxt.setPosition({boxX + (boxW - vb.size.x) / 2.f - vb.position.x, ctrlY + 6.f});
        win.draw(vTxt);

        Text uTxt(font, unitLabel, 10);
        uTxt.setFillColor(Color(100, 116, 139));
        FloatRect ub = uTxt.getLocalBounds();
        uTxt.setPosition({boxX + (boxW - ub.size.x) / 2.f - ub.position.x, ctrlY + 30.f});
        win.draw(uTxt);
    };

    // 3. Số lượng hàng ghế (A - J) & Số lượng cột ghế (1 - 10)
    drawStepper(cardY + 206.f, Color(8, 125, 181), L"S\u1ed1 l\u01b0\u1ee3ng h\u00e0ng gh\u1ebf", L"H\u00e0ng (A - J)", inputRows, L"H\u00c0NG GH\u1ebe");
    drawStepper(cardY + 288.f, Color(99, 102, 241), L"S\u1ed1 l\u01b0\u1ee3ng c\u1ed9t gh\u1ebf", L"C\u1ed9t (1 - 10)", inputCols, L"C\u1ed8T GH\u1ebe");

    // 4. Trạng thái phòng chiếu
    Text lbl5(boldFont, L"Tr\u1ea1ng th\u00e1i ph\u00f2ng chi\u1ebfu:", 14); lbl5.setFillColor(Color(51, 65, 85)); lbl5.setPosition({position.x + 50.f, cardY + 372.f}); win.draw(lbl5);
    drawRoundedCard(win, {position.x + 50.f, cardY + 394.f}, {leftCardW - 40.f, 38.f}, 8.f, Color(248, 250, 252), statusDropdownOpen ? 1.5f : 1.f, statusDropdownOpen ? Color(8, 125, 181) : Color(203, 213, 225));
    Text stTxt(boldFont, inputStatus.empty() ? L"B\u00ecnh th\u01b0\u1eddng (\u0110ang \u0111\u00f3n kh\u00e1ch)" : inputStatus, 14); stTxt.setFillColor(Color(23, 32, 51)); stTxt.setPosition({position.x + 62.f, cardY + 403.f}); win.draw(stTxt);
    Text arr2(boldFont, L"\u25bc", 14); arr2.setFillColor(Color(100, 116, 139)); arr2.setPosition({position.x + leftCardW - 15.f, cardY + 405.f}); win.draw(arr2);

    // 5. Chọn loại ghế để gán / chỉnh sửa
    Text lbl6(boldFont, L"Ch\u1ecdn lo\u1ea1i gh\u1ebf \u0111\u1ec3 g\u00e1n / ch\u1ec9nh s\u1eeda:", 14); lbl6.setFillColor(Color(51, 65, 85)); lbl6.setPosition({position.x + 50.f, cardY + 446.f}); win.draw(lbl6);
    drawRoundedCard(win, {position.x + 50.f, cardY + 468.f}, {leftCardW - 40.f, 38.f}, 8.f, Color(248, 250, 252), seatTypeDropdownOpen ? 1.5f : 1.f, seatTypeDropdownOpen ? Color(8, 125, 181) : Color(203, 213, 225));

    wstring seatLabel = L"Gh\u1ebf VIP (M\u00e0u V\u00e0ng / Cao c\u1ea5p)";
    if (currentAssignType == SeatType::STANDARD) seatLabel = L"Gh\u1ebf Th\u01b0\u1eddng (M\u00e0u Xanh Nh\u1ea1t)";
    else if (currentAssignType == SeatType::SWEETBOX) seatLabel = L"Gh\u1ebf \u0110\u00f4i Sweetbox (M\u00e0u Xanh \u0110\u1eadm)";
    else if (currentAssignType == SeatType::MAINTENANCE) seatLabel = L"Gh\u1ebf b\u1ea3o tr\u00ec (T\u1ea1m ng\u1eebng)";

    Text seatTypeTxt(boldFont, seatLabel, 14); seatTypeTxt.setFillColor(Color(23, 32, 51)); seatTypeTxt.setPosition({position.x + 62.f, cardY + 477.f}); win.draw(seatTypeTxt);
    Text arr3(boldFont, L"\u25bc", 14); arr3.setFillColor(Color(100, 116, 139)); arr3.setPosition({position.x + leftCardW - 15.f, cardY + 479.f}); win.draw(arr3);

    // Mẹo hướng dẫn (chia thành 2 dòng gọn gàng, không bị tràn ra ngoài khung)
    Text hintTxt(font, L"* M\u1eb9o: Click tr\u1ef1c ti\u1ebfp v\u00e0o gh\u1ebf tr\u00ean s\u01a1 \u0111\u1ed3 b\u00ean ph\u1ea3i\n  \u0111\u1ec3 chuy\u1ec3n \u0111\u1ed5i lo\u1ea1i gh\u1ebf t\u1ee9c th\u00ec.", 13);
    hintTxt.setFillColor(Color(100, 116, 139));
    hintTxt.setPosition({position.x + 50.f, cardY + 516.f});
    win.draw(hintTxt);

    // Thẻ Thống kê loại ghế (đồng nhất cỡ chữ 14 cho toàn bộ nhãn & số lượng)
    int cntStd = 0, cntVip = 0, cntSwt = 0, cntMnt = 0;
    updateSeatStatistics(cntStd, cntVip, cntSwt, cntMnt);
    int totalSeats = cntStd + cntVip + cntSwt + cntMnt;

    float statCardY = cardY + 558.f;
    float statCardW = leftCardW - 40.f;
    float statRightX = position.x + 50.f + statCardW - 14.f;
    drawRoundedCard(win, {position.x + 50.f, statCardY}, {statCardW, 110.f}, 10.f, Color(241, 245, 249));

    Text stHead(boldFont, L"Th\u1ed1ng k\u00ea lo\u1ea1i gh\u1ebf", 14); stHead.setFillColor(Color(23, 32, 51)); stHead.setPosition({position.x + 64.f, statCardY + 11.f}); win.draw(stHead);
    Text stTotal(font, to_wstring(totalSeats) + L" gh\u1ebf", 14); stTotal.setFillColor(Color(8, 125, 181));
    stTotal.setPosition({statRightX - stTotal.getLocalBounds().size.x, statCardY + 11.f}); win.draw(stTotal);

    // Ghế Thường
    CircleShape c1(3.5f); c1.setFillColor(Color(148, 163, 184)); c1.setPosition({position.x + 64.f, statCardY + 41.f}); win.draw(c1);
    Text c1T(font, L"Gh\u1ebf Th\u01b0\u1eddng", 14); c1T.setFillColor(Color(71, 85, 105)); c1T.setPosition({position.x + 78.f, statCardY + 36.f}); win.draw(c1T);
    Text c1V(font, to_wstring(cntStd) + L" gh\u1ebf", 14); c1V.setFillColor(Color(51, 65, 85));
    c1V.setPosition({statRightX - c1V.getLocalBounds().size.x, statCardY + 36.f}); win.draw(c1V);

    // Ghế VIP
    CircleShape c2(3.5f); c2.setFillColor(Color(245, 158, 11)); c2.setPosition({position.x + 64.f, statCardY + 64.f}); win.draw(c2);
    Text c2T(font, L"Gh\u1ebf VIP", 14); c2T.setFillColor(Color(71, 85, 105)); c2T.setPosition({position.x + 78.f, statCardY + 59.f}); win.draw(c2T);
    Text c2V(font, to_wstring(cntVip) + L" gh\u1ebf", 14); c2V.setFillColor(Color(217, 119, 6));
    c2V.setPosition({statRightX - c2V.getLocalBounds().size.x, statCardY + 59.f}); win.draw(c2V);

    // Ghế Sweetbox
    CircleShape c3(3.5f); c3.setFillColor(Color(30, 58, 95)); c3.setPosition({position.x + 64.f, statCardY + 87.f}); win.draw(c3);
    Text c3T(font, L"Gh\u1ebf \u0110\u00f4i Sweetbox", 14); c3T.setFillColor(Color(71, 85, 105)); c3T.setPosition({position.x + 78.f, statCardY + 82.f}); win.draw(c3T);
    Text c3V(font, to_wstring(cntSwt / 2) + L" c\u1eb7p", 14); c3V.setFillColor(Color(30, 58, 95));
    c3V.setPosition({statRightX - c3V.getLocalBounds().size.x, statCardY + 82.f}); win.draw(c3V);

    // 2 Nút cuối: LƯU và HỦY BỎ
    float actBtnY = cardY + cardH - 58.f;
    float actBtnW = 155.f;
    float actBtnH = 40.f;

    // Nút LƯU
    drawRoundedCard(win, {position.x + 50.f, actBtnY}, {actBtnW, actBtnH}, 8.f, Color(8, 125, 181));
    Text svTxt(boldFont, L"L\u01afU", 14);
    svTxt.setFillColor(Color::White);
    FloatRect svb = svTxt.getLocalBounds();
    svTxt.setPosition({
        position.x + 50.f + (actBtnW - svb.size.x) / 2.f - svb.position.x,
        actBtnY + (actBtnH - svb.size.y) / 2.f - svb.position.y
    });
    win.draw(svTxt);

    // Nút HỦY BỎ
    drawRoundedCard(win, {position.x + 225.f, actBtnY}, {actBtnW, actBtnH}, 8.f, Color(241, 245, 249), 1.f, Color(203, 213, 225));
    Text clTxt(boldFont, L"H\u1ee6Y B\u1ece", 14);
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

    // Header sơ đồ: SƠ ĐỒ PHÒNG X
    wstring roomHead = (selectedRoomIndex >= 0 && selectedRoomIndex < (int)rooms.size())
        ? L"S\u01a0 \u0110\u1ed2 " + toUpperVi(rooms[selectedRoomIndex].name)
        : L"S\u01a0 \u0110\u1ed2 PH\u00d2NG CHI\u1ebeU";

    Text head1(boldFont, roomHead, 14);
    head1.setFillColor(Color(23, 32, 51));
    head1.setPosition({rightX + 24.f, cardY + 22.f});
    win.draw(head1);

    // Điều khiển Zoom: [ - ] 100% [ + ] (nền xanh nhạt)
    drawRoundedCard(win, {rightX + rightW - 135.f, cardY + 18.f}, {115.f, 28.f}, 14.f, Color(224, 242, 254), 1.f, Color(186, 230, 253));
    Text zMinus(boldFont, L"-", 14); zMinus.setFillColor(Color(8, 125, 181)); zMinus.setPosition({rightX + rightW - 124.f, cardY + 20.f}); win.draw(zMinus);
    int zoomPercent = (int)(zoomFactor * 100.f + 0.5f);
    Text zVal(boldFont, to_wstring(zoomPercent) + L"%", 14); zVal.setFillColor(Color(18, 59, 93)); zVal.setPosition({rightX + rightW - 95.f, cardY + 24.f}); win.draw(zVal);
    Text zPlus(boldFont, L"+", 14); zPlus.setFillColor(Color(8, 125, 181)); zPlus.setPosition({rightX + rightW - 40.f, cardY + 22.f}); win.draw(zPlus);

    // Khối MÀN HÌNH CHIẾU DẠNG CONG (Curved Cinema Screen)
    {
        const float arcW = min(rightW - 140.f, 680.f);
        const float arcLeft = rightX + (rightW - arcW) / 2.f;
        const float arcTopY = cardY + 72.f;
        const float sag = 18.f;
        const float thick = 14.f;
        const float glowH = 26.f;
        const int segs = 48;

        VertexArray glowStrip(PrimitiveType::TriangleStrip, (segs + 1) * 2);
        VertexArray rimStrip(PrimitiveType::TriangleStrip, (segs + 1) * 2);
        VertexArray screenStrip(PrimitiveType::TriangleStrip, (segs + 1) * 2);

        for (int i = 0; i <= segs; ++i) {
            float t = static_cast<float>(i) / static_cast<float>(segs);
            float u = 2.f * t - 1.f; // -1 .. +1
            float x = arcLeft + t * arcW;
            float y = arcTopY + sag * (u * u);

            // Soft projection glow underneath the curved screen
            glowStrip[i * 2].position = {x, y + thick};
            glowStrip[i * 2].color = Color(56, 189, 248, 38);
            glowStrip[i * 2 + 1].position = {x, y + thick + glowH};
            glowStrip[i * 2 + 1].color = Color(255, 255, 255, 0);

            // Darker 3D bottom rim
            rimStrip[i * 2].position = {x, y + thick - 2.f};
            rimStrip[i * 2].color = Color(7, 89, 133);
            rimStrip[i * 2 + 1].position = {x, y + thick + 2.5f};
            rimStrip[i * 2 + 1].color = Color(7, 89, 133);

            // Main curved blue screen ribbon (slightly lighter in center)
            uint8_t g = static_cast<uint8_t>(138 - 18 * (u * u));
            uint8_t b = static_cast<uint8_t>(204 - 22 * (u * u));
            screenStrip[i * 2].position = {x, y};
            screenStrip[i * 2].color = Color(14, g, b);
            screenStrip[i * 2 + 1].position = {x, y + thick};
            screenStrip[i * 2 + 1].color = Color(2, static_cast<uint8_t>(g - 18), static_cast<uint8_t>(b - 18));
        }

        win.draw(glowStrip);
        win.draw(rimStrip);
        win.draw(screenStrip);

        Text scrTxt(boldFont, L"M \u00c0 N   H \u00cc N H   C H I \u1ebe U", 14);
        scrTxt.setFillColor(Color::White);
        FloatRect stb = scrTxt.getLocalBounds();
        scrTxt.setPosition({
            arcLeft + (arcW - stb.size.x) / 2.f - stb.position.x,
            arcTopY + (thick - stb.size.y) / 2.f - stb.position.y - 0.5f
        });
        win.draw(scrTxt);
    }

    // ==========================================
    // MA TRẬN GHẾ
    // ==========================================
    if (selectedRoomIndex >= 0 && selectedRoomIndex < (int)rooms.size()) {
        const auto& r = rooms[selectedRoomIndex];
        float seatW = 42.f * zoomFactor;
        float seatH = 42.f * zoomFactor;
        float gapX = 16.f * zoomFactor;
        float gapY = 12.f * zoomFactor;

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
    Text lg1T(boldFont, L"Gh\u1ebf Th\u01b0\u1eddng", 14); lg1T.setFillColor(Color(23, 32, 51)); lg1T.setPosition({rightX + 64.f, legY + 6.f}); win.draw(lg1T);
    Text lg1P(font, L"100.000\u0111", 14); lg1P.setFillColor(Color(100, 116, 139)); lg1P.setPosition({rightX + 64.f, legY + 22.f}); win.draw(lg1P);

    // 2. Ghế VIP Gold (140.000đ)
    drawRoundedCard(win, {rightX + 24.f + legW, legY}, {legW - 10.f, 44.f}, 8.f, Color(254, 243, 199), 1.f, Color(253, 230, 138));
    drawSeat(win, rightX + 34.f + legW, legY + 8.f, 22.f, 24.f, SeatType::VIP, "", false);
    Text lg2T(boldFont, L"Gh\u1ebf VIP (Gold)", 14); lg2T.setFillColor(Color(180, 83, 9)); lg2T.setPosition({rightX + 64.f + legW, legY + 6.f}); win.draw(lg2T);
    Text lg2P(font, L"140.000\u0111", 14); lg2P.setFillColor(Color(217, 119, 6)); lg2P.setPosition({rightX + 64.f + legW, legY + 22.f}); win.draw(lg2P);

    // 3. Ghế Đôi Sweetbox (220.000đ/cặp)
    drawRoundedCard(win, {rightX + 24.f + legW * 2.f, legY}, {legW - 10.f, 44.f}, 8.f, Color(238, 242, 255), 1.f, Color(199, 210, 254));
    drawSeat(win, rightX + 34.f + legW * 2.f, legY + 8.f, 22.f, 24.f, SeatType::SWEETBOX, "", false);
    Text lg3T(boldFont, L"Gh\u1ebf \u0110\u00f4i Sweetbox", 14); lg3T.setFillColor(Color(30, 58, 95)); lg3T.setPosition({rightX + 64.f + legW * 2.f, legY + 6.f}); win.draw(lg3T);
    Text lg3P(font, L"220.000\u0111 / c\u1eb7p", 14); lg3P.setFillColor(Color(79, 70, 229)); lg3P.setPosition({rightX + 64.f + legW * 2.f, legY + 22.f}); win.draw(lg3P);

    // 4. Ghế bảo trì (Tạm ngừng bán)
    drawRoundedCard(win, {rightX + 24.f + legW * 3.f, legY}, {legW - 10.f, 44.f}, 8.f, Color(254, 242, 242), 1.f, Color(254, 202, 202));
    drawSeat(win, rightX + 34.f + legW * 3.f, legY + 8.f, 22.f, 24.f, SeatType::MAINTENANCE, "", false);
    Text lg4T(boldFont, L"Gh\u1ebf b\u1ea3o tr\u00ec", 14); lg4T.setFillColor(Color(185, 28, 28)); lg4T.setPosition({rightX + 64.f + legW * 3.f, legY + 6.f}); win.draw(lg4T);
    Text lg4P(font, L"T\u1ea1m ng\u1eebng b\u00e1n", 14); lg4P.setFillColor(Color(220, 38, 38)); lg4P.setPosition({rightX + 64.f + legW * 3.f, legY + 22.f}); win.draw(lg4P);

    // ==========================================
    // 7. POPUP / DROPDOWN MENUS (Render trên cùng)
    // ==========================================
    if (roomDropdownOpen) {
        float ddY = cardY + 122.f;
        drawRoundedCard(win, {position.x + 50.f, ddY}, {leftCardW - 40.f, (float)rooms.size() * 36.f}, 8.f, Color::White, 1.5f, Color(8, 125, 181));
        for (size_t i = 0; i < rooms.size(); ++i) {
            float iY = ddY + i * 36.f;
            if (i == (size_t)selectedRoomIndex) {
                RoundedRectRenderer::draw(win, {position.x + 52.f, iY}, {leftCardW - 44.f, 35.f}, 4.f, Color(224, 242, 254));
            }
            Text rItm(font, rooms[i].name + L" - " + rooms[i].roomType, 14);
            rItm.setFillColor((i == (size_t)selectedRoomIndex) ? Color(8, 125, 181) : Color(23, 32, 51));
            rItm.setPosition({position.x + 62.f, iY + 9.f});
            win.draw(rItm);
        }
    }

    if (statusDropdownOpen) {
        float ddY = cardY + 434.f;
        drawRoundedCard(win, {position.x + 50.f, ddY}, {leftCardW - 40.f, 72.f}, 8.f, Color::White, 1.5f, Color(8, 125, 181));
        const wstring statusOpts[2] = {
            L"B\u00ecnh th\u01b0\u1eddng (\u0110ang \u0111\u00f3n kh\u00e1ch)",
            L"\u0110ang b\u1ea3o tr\u00ec k\u1ef9 thu\u1eadt"
        };
        bool isNormal = inputStatus.empty() || inputStatus.find(L"B\u00ecnh th\u01b0\u1eddng") != wstring::npos;
        for (int i = 0; i < 2; ++i) {
            float iY = ddY + i * 36.f;
            bool sel = (i == 0) ? isNormal : !isNormal;
            if (sel) {
                RoundedRectRenderer::draw(win, {position.x + 52.f, iY}, {leftCardW - 44.f, 35.f}, 4.f, Color(224, 242, 254));
            }
            Text sItm(font, statusOpts[i], 14);
            sItm.setFillColor(sel ? Color(8, 125, 181) : Color(23, 32, 51));
            sItm.setPosition({position.x + 62.f, iY + 9.f});
            win.draw(sItm);
        }
    }

    if (seatTypeDropdownOpen) {
        float ddY = cardY + 508.f;
        drawRoundedCard(win, {position.x + 50.f, ddY}, {leftCardW - 40.f, 144.f}, 8.f, Color::White, 1.5f, Color(8, 125, 181));
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
            Text tItm(font, typeNames[i], 14);
            tItm.setFillColor((types[i] == currentAssignType) ? Color(8, 125, 181) : Color(23, 32, 51));
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
            float notY = position.y + 35.f; // Nằm ở giữa trên cao, cực kỳ rõ ràng và dễ thấy
            drawRoundedCard(win, {notX, notY}, {notW, notH}, 8.f, Color(22, 163, 74, 245), 1.5f, Color(21, 128, 61));
            Text notText(boldFont, s2ws(notificationMessage), 14);
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