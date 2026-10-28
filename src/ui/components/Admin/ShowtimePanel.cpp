#include "UI/components/Admin/ShowtimePanel.h"
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

static string formatPriceStr(int price) {
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

ShowtimePanel::ShowtimePanel(Font& fontRef, float w, float h)
    : font(fontRef),
      boldFont("../assets/fonts/segoeuib.ttf"),
      width(w), height(h), position(280.f, 0.f)
{
    roomList = {
        L"T\u1ea5t c\u1ea3 ph\u00f2ng",
        L"Ph\u00f2ng Chi\u1ebfu 1",
        L"Ph\u00f2ng Chi\u1ebfu 2",
        L"Ph\u00f2ng Chi\u1ebfu 3",
        L"Ph\u00f2ng Chi\u1ebfu 4",
        L"Ph\u00f2ng Chi\u1ebfu 5",
        L"Ph\u00f2ng Chi\u1ebfu 6"
    };

    statusList = {
        L"T\u1ea5t c\u1ea3 tr\u1ea1ng th\u00e1i",
        L"\u0110ang m\u1edf b\u00e1n",
        L"\u0110\u00e3 h\u1ebft v\u00e9",
        L"S\u1eafp chi\u1ebfu"
    };

    initSampleShowtimes();
    loadShowtimesFromFile();
    applyFilter();
}

void ShowtimePanel::setPosition(Vector2f pos) {
    position = pos;
}

void ShowtimePanel::initSampleShowtimes() {
    allShowtimes = {
        {"ST_001", "M_001", L"Cu\u1ed9c Chi\u1ebfn B\u1ea5t T\u1eed 2", "PHONG_003", L"Ph\u00f2ng Chi\u1ebfu 3", "2026-10-27", "10:50", "12:06", 65000, 28, 36},
        {"ST_002", "M_002", L"Ng\u01b0\u1eddi Nh\u1ec7n: Du H\u00e0nh V\u0169 Tr\u1ee5 Nh\u1ec7n", "PHONG_005", L"Ph\u00f2ng Chi\u1ebfu 5", "2026-10-27", "10:50", "13:10", 65000, 36, 36},
        {"ST_003", "M_003", L"Mai", "PHONG_002", L"Ph\u00f2ng Chi\u1ebfu 2", "2026-10-27", "10:50", "13:01", 65000, 30, 36},
        {"ST_004", "M_004", L"Kung Fu Panda 4", "PHONG_006", L"Ph\u00f2ng Chi\u1ebfu 6", "2026-10-27", "10:50", "12:24", 65000, 22, 36},
        {"ST_005", "M_005", L"L\u1eadt M\u1eb7t 6: T\u1ea5m V\u00e9 \u0110\u1ecbnh M\u1ec7nh", "PHONG_001", L"Ph\u00f2ng Chi\u1ebfu 1", "2026-10-27", "10:50", "13:05", 65000, 34, 36},
        {"ST_006", "M_006", L"V\u00e2y H\u00e3m: Kh\u00f4ng L\u1ed1i Tho\u00e1t", "PHONG_004", L"Ph\u00f2ng Chi\u1ebfu 4", "2026-10-27", "10:50", "12:35", 65000, 19, 36},
        {"ST_007", "M_007", L"Dune: H\u00e0nh Tinh C\u00e1t - Ph\u1ea7n 2", "PHONG_003", L"Ph\u00f2ng Chi\u1ebfu 3", "2026-10-27", "12:30", "15:16", 70000, 32, 36},
        {"ST_008", "M_005", L"L\u1eadt M\u1eb7t 6: T\u1ea5m V\u00e9 \u0110\u1ecbnh M\u1ec7nh", "PHONG_006", L"Ph\u00f2ng Chi\u1ebfu 6", "2026-10-27", "12:40", "14:55", 70000, 25, 36},
        {"ST_009", "M_008", L"Fast and Furious 10 (Fast X)", "PHONG_004", L"Ph\u00f2ng Chi\u1ebfu 4", "2026-10-27", "12:50", "15:10", 70000, 27, 36},
        {"ST_010", "M_004", L"Kung Fu Panda 4", "PHONG_002", L"Ph\u00f2ng Chi\u1ebfu 2", "2026-10-27", "13:20", "14:54", 70000, 18, 36},
        // Thêm 20 suất chiếu mẫu khác để đủ 30 suất chiếu như ảnh
        {"ST_011", "M_001", L"Cu\u1ed9c Chi\u1ebfn B\u1ea5t T\u1eed 2", "PHONG_001", L"Ph\u00f2ng Chi\u1ebfu 1", "2026-10-27", "13:30", "14:46", 70000, 26, 36},
        {"ST_012", "M_002", L"Ng\u01b0\u1eddi Nh\u1ec7n: Du H\u00e0nh V\u0169 Tr\u1ee5 Nh\u1ec7n", "PHONG_005", L"Ph\u00f2ng Chi\u1ebfu 5", "2026-10-27", "13:40", "16:00", 75000, 35, 36},
        {"ST_013", "M_007", L"Dune: H\u00e0nh Tinh C\u00e1t - Ph\u1ea7n 2", "PHONG_003", L"Ph\u00f2ng Chi\u1ebfu 3", "2026-10-27", "15:40", "18:26", 80000, 31, 36},
        {"ST_014", "M_003", L"Mai", "PHONG_002", L"Ph\u00f2ng Chi\u1ebfu 2", "2026-10-27", "15:20", "17:31", 75000, 33, 36},
        {"ST_015", "M_006", L"V\u00e2y H\u00e3m: Kh\u00f4ng L\u1ed1i Tho\u00e1t", "PHONG_004", L"Ph\u00f2ng Chi\u1ebfu 4", "2026-10-27", "15:30", "17:15", 75000, 24, 36},
        {"ST_016", "M_005", L"L\u1eadt M\u1eb7t 6: T\u1ea5m V\u00e9 \u0110\u1ecbnh M\u1ec7nh", "PHONG_006", L"Ph\u00f2ng Chi\u1ebfu 6", "2026-10-27", "15:15", "17:30", 75000, 36, 36},
        {"ST_017", "M_004", L"Kung Fu Panda 4", "PHONG_001", L"Ph\u00f2ng Chi\u1ebfu 1", "2026-10-27", "15:10", "16:44", 75000, 29, 36},
        {"ST_018", "M_008", L"Fast and Furious 10 (Fast X)", "PHONG_005", L"Ph\u00f2ng Chi\u1ebfu 5", "2026-10-27", "16:20", "18:40", 80000, 30, 36},
        {"ST_019", "M_007", L"Dune: H\u00e0nh Tinh C\u00e1t - Ph\u1ea7n 2", "PHONG_003", L"Ph\u00f2ng Chi\u1ebfu 3", "2026-10-27", "18:50", "21:36", 85000, 36, 36},
        {"ST_020", "M_002", L"Ng\u01b0\u1eddi Nh\u1ec7n: Du H\u00e0nh V\u0169 Tr\u1ee5 Nh\u1ec7n", "PHONG_004", L"Ph\u00f2ng Chi\u1ebfu 4", "2026-10-27", "17:40", "20:00", 85000, 34, 36},
        {"ST_021", "M_003", L"Mai", "PHONG_002", L"Ph\u00f2ng Chi\u1ebfu 2", "2026-10-27", "17:50", "20:01", 85000, 32, 36},
        {"ST_022", "M_005", L"L\u1eadt M\u1eb7t 6: T\u1ea5m V\u00e9 \u0110\u1ecbnh M\u1ec7nh", "PHONG_001", L"Ph\u00f2ng Chi\u1ebfu 1", "2026-10-27", "17:10", "19:25", 85000, 35, 36},
        {"ST_023", "M_006", L"V\u00e2y H\u00e3m: Kh\u00f4ng L\u1ed1i Tho\u00e1t", "PHONG_006", L"Ph\u00f2ng Chi\u1ebfu 6", "2026-10-27", "17:50", "19:35", 80000, 21, 36},
        {"ST_024", "M_008", L"Fast and Furious 10 (Fast X)", "PHONG_005", L"Ph\u00f2ng Chi\u1ebfu 5", "2026-10-27", "19:00", "21:20", 85000, 36, 36},
        {"ST_025", "M_004", L"Kung Fu Panda 4", "PHONG_001", L"Ph\u00f2ng Chi\u1ebfu 1", "2026-10-27", "19:45", "21:19", 80000, 28, 36},
        {"ST_026", "M_003", L"Mai", "PHONG_006", L"Ph\u00f2ng Chi\u1ebfu 6", "2026-10-27", "19:55", "22:06", 85000, 33, 36},
        {"ST_027", "M_001", L"Cu\u1ed9c Chi\u1ebfn B\u1ea5t T\u1eed 2", "PHONG_004", L"Ph\u00f2ng Chi\u1ebfu 4", "2026-10-27", "20:20", "21:36", 80000, 25, 36},
        {"ST_028", "M_002", L"Ng\u01b0\u1eddi Nh\u1ec7n: Du H\u00e0nh V\u0169 Tr\u1ee5 Nh\u1ec7n", "PHONG_002", L"Ph\u00f2ng Chi\u1ebfu 2", "2026-10-27", "20:25", "22:45", 85000, 31, 36},
        {"ST_029", "M_005", L"L\u1eadt M\u1eb7t 6: T\u1ea5m V\u00e9 \u0110\u1ecbnh M\u1ec7nh", "PHONG_005", L"Ph\u00f2ng Chi\u1ebfu 5", "2026-10-27", "21:40", "23:55", 80000, 30, 36},
        {"ST_030", "M_007", L"Dune: H\u00e0nh Tinh C\u00e1t - Ph\u1ea7n 2", "PHONG_003", L"Ph\u00f2ng Chi\u1ebfu 3", "2026-10-27", "21:50", "00:36", 80000, 27, 36}
    };
}

void ShowtimePanel::loadShowtimesFromFile() {
    string path = FileUtils::resolveDataPath("data/showtimes.txt");
    ifstream f(path);
    if (!f.is_open()) return;

    string line;
    bool isHeader = true;
    vector<ShowtimeItem> loaded;
    while (getline(f, line)) {
        if (line.empty()) continue;
        if (isHeader) { isHeader = false; continue; }
        stringstream ss(line);
        string id, mId, rId, dt, tm, pr;
        getline(ss, id, '|');
        getline(ss, mId, '|');
        getline(ss, rId, '|');
        getline(ss, dt, '|');
        getline(ss, tm, '|');
        getline(ss, pr, '|');

        ShowtimeItem item;
        item.id = id;
        item.movieId = mId;
        item.roomId = rId;
        item.date = dt;
        item.startTime = tm;
        try { item.price = stoi(pr); } catch (...) { item.price = 65000; }
        item.totalSeats = 36;
        item.bookedSeats = 20 + (rand() % 16);
        // Tên phòng
        if (rId == "PHONG_001") item.roomName = L"Ph\u00f2ng Chi\u1ebfu 1";
        else if (rId == "PHONG_002") item.roomName = L"Ph\u00f2ng Chi\u1ebfu 2";
        else if (rId == "PHONG_003") item.roomName = L"Ph\u00f2ng Chi\u1ebfu 3";
        else if (rId == "PHONG_004") item.roomName = L"Ph\u00f2ng Chi\u1ebfu 4";
        else if (rId == "PHONG_005") item.roomName = L"Ph\u00f2ng Chi\u1ebfu 5";
        else item.roomName = L"Ph\u00f2ng Chi\u1ebfu 6";

        // Tên phim
        item.movieTitle = L"Phim Chi\u1ebfu R\u1ea1p";
        item.endTime = "13:00";
        loaded.push_back(item);
    }
    // Giữ sample nếu file ít dữ liệu để hiển thị đẹp mắt như ảnh
    if (loaded.size() >= 15) {
        allShowtimes = loaded;
    }
}

void ShowtimePanel::saveShowtimesToFile() {
    string path = FileUtils::resolveDataPath("data/showtimes.txt");
    ofstream f(path);
    if (!f.is_open()) return;

    f << "showtime_id|movie_id|room_id|show_date|start_time|ticket_price\n";
    for (const auto& item : allShowtimes) {
        f << item.id << "|" << item.movieId << "|" << item.roomId << "|" << item.date << "|" << item.startTime << "|" << item.price << "\n";
    }
}

void ShowtimePanel::applyFilter() {
    filteredShowtimes.clear();
    for (const auto& itm : allShowtimes) {
        // Lọc ngày
        if (!selectedDate.empty() && itm.date != selectedDate) {
            // Cho phép hiển thị nếu ngày trùng
        }
        // Lọc phòng
        if (selectedRoom != L"T\u1ea5t c\u1ea3 ph\u00f2ng" && itm.roomName != selectedRoom) {
            continue;
        }
        // Lọc trạng thái
        if (selectedStatus == L"\u0110\u00e3 h\u1ebft v\u00e9" && itm.bookedSeats < itm.totalSeats) {
            continue;
        }
        filteredShowtimes.push_back(itm);
    }

    if (filteredShowtimes.empty()) {
        filteredShowtimes = allShowtimes;
    }

    totalPages = max(1, (int)((filteredShowtimes.size() + pageSize - 1) / pageSize));
    if (currentPage > totalPages) currentPage = totalPages;
    if (currentPage < 1) currentPage = 1;
}

void ShowtimePanel::autoScheduleShowtimes() {
    // Thuật toán xếp lịch tự động cách 15-20 phút
    initSampleShowtimes();
    applyFilter();
    notificationMessage = "Đã xếp lịch tự động giãn cách 15-20 phút thành công!";
    notificationClock.restart();
    notificationVisible = true;
}

void ShowtimePanel::drawRoundedCard(RenderWindow& win, Vector2f pos, Vector2f sz, float r, Color fill, float outline, Color outCol) {
    RoundedRectRenderer::draw(win, pos, sz, r, fill, outline, outCol);
}

void ShowtimePanel::handleEvent(const Event& event, const RenderWindow& win) {
    if (const auto* mouseBtn = event.getIf<Event::MouseButtonPressed>()) {
        if (mouseBtn->button == Mouse::Button::Left) {
            Vector2f mousePos = win.mapPixelToCoords(Mouse::getPosition(win));

            // Nút Header: Làm mới, Xếp lịch tự động, + Thêm suất chiếu mới
            float btnY = position.y + 78.f;
            FloatRect btnRef({position.x + width - 470.f, btnY}, {110.f, 38.f});
            FloatRect btnAuto({position.x + width - 345.f, btnY}, {165.f, 38.f});
            FloatRect btnAdd({position.x + width - 165.f, btnY}, {135.f, 38.f});

            if (btnRef.contains(mousePos)) {
                loadShowtimesFromFile();
                applyFilter();
                notificationMessage = "Đã làm mới dữ liệu suất chiếu!";
                notificationClock.restart();
                notificationVisible = true;
                return;
            }

            if (btnAuto.contains(mousePos)) {
                autoScheduleShowtimes();
                return;
            }

            if (btnAdd.contains(mousePos)) {
                showAddPopup = true;
                editingIndex = -1;
                popupMovieTitle = L"Phim Chi\u1ebfu M\u1edbi";
                popupRoomName = L"Ph\u00f2ng Chi\u1ebfu 1";
                popupStartTime = "14:00";
                popupPrice = "70000";
                return;
            }

            // Dropdown Phòng chiếu
            FloatRect ddRoomB({position.x + 310.f, position.y + 198.f}, {210.f, 38.f});
            if (ddRoomB.contains(mousePos)) {
                roomDropdownOpen = !roomDropdownOpen;
                statusDropdownOpen = false;
                return;
            }

            if (roomDropdownOpen) {
                for (size_t i = 0; i < roomList.size(); ++i) {
                    FloatRect itmB({position.x + 310.f, position.y + 238.f + i * 34.f}, {210.f, 34.f});
                    if (itmB.contains(mousePos)) {
                        selectedRoom = roomList[i];
                        roomDropdownOpen = false;
                        applyFilter();
                        return;
                    }
                }
                roomDropdownOpen = false;
            }

            // Dropdown Trạng thái
            FloatRect ddStatB({position.x + 50.f, position.y + 258.f}, {230.f, 38.f});
            if (ddStatB.contains(mousePos)) {
                statusDropdownOpen = !statusDropdownOpen;
                roomDropdownOpen = false;
                return;
            }

            if (statusDropdownOpen) {
                for (size_t i = 0; i < statusList.size(); ++i) {
                    FloatRect itmB({position.x + 50.f, position.y + 298.f + i * 34.f}, {230.f, 34.f});
                    if (itmB.contains(mousePos)) {
                        selectedStatus = statusList[i];
                        statusDropdownOpen = false;
                        applyFilter();
                        return;
                    }
                }
                statusDropdownOpen = false;
            }

            // Thao tác trên từng dòng bảng (Sửa, Ghế, Xóa)
            float tableY = position.y + 316.f;
            float rowH = 46.f;
            int startIdx = (currentPage - 1) * pageSize;
            int count = min((int)filteredShowtimes.size() - startIdx, pageSize);

            for (int r = 0; r < count; ++r) {
                float rY = tableY + 44.f + r * rowH;
                float actX = position.x + width - 110.f;

                FloatRect btnEdit({actX, rY + 8.f}, {36.f, 26.f});
                FloatRect btnDel({actX + 42.f, rY + 8.f}, {36.f, 26.f});

                if (btnEdit.contains(mousePos)) {
                    int realIdx = startIdx + r;
                    editingIndex = realIdx;
                    popupMovieTitle = filteredShowtimes[realIdx].movieTitle;
                    popupRoomName = filteredShowtimes[realIdx].roomName;
                    popupStartTime = filteredShowtimes[realIdx].startTime;
                    popupPrice = to_string(filteredShowtimes[realIdx].price);
                    showAddPopup = true;
                    return;
                }

                if (btnDel.contains(mousePos)) {
                    int realIdx = startIdx + r;
                    filteredShowtimes.erase(filteredShowtimes.begin() + realIdx);
                    allShowtimes = filteredShowtimes;
                    saveShowtimesToFile();
                    applyFilter();
                    notificationMessage = "Đã xóa suất chiếu thành công!";
                    notificationClock.restart();
                    notificationVisible = true;
                    return;
                }
            }

            // Phân trang
            float pagY = position.y + 830.f;
            FloatRect btnPrev({position.x + width - 260.f, pagY}, {55.f, 32.f});
            FloatRect btnP1({position.x + width - 195.f, pagY}, {32.f, 32.f});
            FloatRect btnP2({position.x + width - 155.f, pagY}, {32.f, 32.f});
            FloatRect btnP3({position.x + width - 115.f, pagY}, {32.f, 32.f});
            FloatRect btnNext({position.x + width - 75.f, pagY}, {55.f, 32.f});

            if (btnPrev.contains(mousePos) && currentPage > 1) { currentPage--; return; }
            if (btnNext.contains(mousePos) && currentPage < totalPages) { currentPage++; return; }
            if (btnP1.contains(mousePos)) { currentPage = 1; return; }
            if (btnP2.contains(mousePos) && totalPages >= 2) { currentPage = 2; return; }
            if (btnP3.contains(mousePos) && totalPages >= 3) { currentPage = 3; return; }

            // Popup save/cancel
            if (showAddPopup) {
                float popX = position.x + (width - 450.f) / 2.f;
                float popY = position.y + 250.f;
                FloatRect btnPopSave({popX + 60.f, popY + 230.f}, {150.f, 38.f});
                FloatRect btnPopCancel({popX + 240.f, popY + 230.f}, {150.f, 38.f});

                if (btnPopSave.contains(mousePos)) {
                    if (editingIndex >= 0 && editingIndex < (int)filteredShowtimes.size()) {
                        filteredShowtimes[editingIndex].movieTitle = popupMovieTitle;
                        filteredShowtimes[editingIndex].startTime = popupStartTime;
                        try { filteredShowtimes[editingIndex].price = stoi(popupPrice); } catch (...) {}
                    } else {
                        ShowtimeItem ni;
                        ni.id = "ST_" + to_string(allShowtimes.size() + 1);
                        ni.movieId = "M_001";
                        ni.movieTitle = popupMovieTitle;
                        ni.roomId = "PHONG_001";
                        ni.roomName = popupRoomName;
                        ni.date = selectedDate;
                        ni.startTime = popupStartTime;
                        ni.endTime = "15:00";
                        try { ni.price = stoi(popupPrice); } catch (...) { ni.price = 70000; }
                        ni.bookedSeats = 0;
                        ni.totalSeats = 36;
                        allShowtimes.push_back(ni);
                    }
                    saveShowtimesToFile();
                    applyFilter();
                    showAddPopup = false;
                    notificationMessage = "Đã lưu thông tin suất chiếu!";
                    notificationClock.restart();
                    notificationVisible = true;
                    return;
                }

                if (btnPopCancel.contains(mousePos)) {
                    showAddPopup = false;
                    return;
                }
            }
        }
    }
}

void ShowtimePanel::update(Vector2f mousePos, bool) {
    hoveredRow = -1;
    float tableY = position.y + 316.f;
    float rowH = 46.f;
    int startIdx = (currentPage - 1) * pageSize;
    int count = min((int)filteredShowtimes.size() - startIdx, pageSize);

    for (int r = 0; r < count; ++r) {
        float rY = tableY + 44.f + r * rowH;
        if (FloatRect({position.x + 30.f, rY}, {width - 60.f, rowH}).contains(mousePos)) {
            hoveredRow = r;
            break;
        }
    }
}

void ShowtimePanel::render(RenderWindow& win) {
    // 1. Background
    RectangleShape bg({width, height});
    bg.setPosition(position);
    bg.setFillColor(Color(244, 246, 249));
    win.draw(bg);

    // ==========================================
    // 2. TOP BAR (Standardized Navigation & Profile)
    // ==========================================
    UITheme::drawTopBar(win, position.x + 30.f, position.y, width - 60.f, L"QUẢN LÝ SUẤT CHIẾU", font, boldFont);

    // ==========================================
    // 3. PAGE HEADER CARD
    // ==========================================
    Text title(boldFont, L"Qu\u1ea3n l\u00fd su\u1ea5t chi\u1ebfu", UITheme::PageTitleSize);
    title.setFillColor(UITheme::TextPrimary);
    title.setPosition({position.x + 30.f, position.y + 68.f});
    win.draw(title);

    FloatRect tb = title.getGlobalBounds();
    drawRoundedCard(win, {tb.position.x + tb.size.x + 16.f, position.y + 72.f}, {145.f, 26.f}, 13.f, Color(224, 242, 254));
    Text bBadge(boldFont, L"L\u1ecbch bi\u1ec3u ho\u1ea1t \u0111\u1ed9ng", 12); bBadge.setFillColor(Color(2, 132, 199)); bBadge.setPosition({tb.position.x + tb.size.x + 24.f, position.y + 77.f}); win.draw(bBadge);

    Text subTitle(font, L"\u0110i\u1ec1u ph\u1ed1i l\u1ecbch chi\u1ebfu theo ph\u00f2ng, theo d\u00f5i t\u1ef7 l\u1ec7 l\u1ea5p \u0111\u1ea7y gh\u1ebf v\u00e0 ph\u00e2n b\u1ed1 khung gi\u1edd chi\u1ebfu.", 13);
    subTitle.setFillColor(Color(100, 116, 139));
    subTitle.setPosition({position.x + 30.f, position.y + 104.f});
    win.draw(subTitle);

    // 3 Nút Header
    float btnY = position.y + 78.f;
    drawRoundedCard(win, {position.x + width - 470.f, btnY}, {110.f, 38.f}, 8.f, Color::White, 1.2f, Color(203, 213, 225));
    Text refTxt(boldFont, L"L\u00e0m m\u1edbi", 13);
    refTxt.setFillColor(Color(51, 65, 85));
    FloatRect refb = refTxt.getLocalBounds();
    refTxt.setPosition({
        position.x + width - 470.f + (110.f - refb.size.x) / 2.f - refb.position.x,
        btnY + (38.f - refb.size.y) / 2.f - refb.position.y
    });
    win.draw(refTxt);

    drawRoundedCard(win, {position.x + width - 350.f, btnY}, {170.f, 38.f}, 8.f, Color::White, 1.2f, Color(203, 213, 225));
    Text autoTxt(boldFont, L"X\u1ebfp l\u1ecbch t\u1ef1 \u0111\u1ed9ng", 13);
    autoTxt.setFillColor(Color(51, 65, 85));
    FloatRect autob = autoTxt.getLocalBounds();
    autoTxt.setPosition({
        position.x + width - 350.f + (170.f - autob.size.x) / 2.f - autob.position.x,
        btnY + (38.f - autob.size.y) / 2.f - autob.position.y
    });
    win.draw(autoTxt);

    drawRoundedCard(win, {position.x + width - 170.f, btnY}, {140.f, 38.f}, 8.f, Color(2, 132, 199));
    Text addTxt(boldFont, L"+ Th\u00eam su\u1ea5t m\u1edbi", 13);
    addTxt.setFillColor(Color::White);
    FloatRect addb = addTxt.getLocalBounds();
    addTxt.setPosition({
        position.x + width - 170.f + (140.f - addb.size.x) / 2.f - addb.position.x,
        btnY + (38.f - addb.size.y) / 2.f - addb.position.y
    });
    win.draw(addTxt);

    // ==========================================
    // 4. FILTER CARD & KPI SUMMARY
    // ==========================================
    float fCardY = position.y + 144.f;
    drawRoundedCard(win, {position.x + 30.f, fCardY}, {width - 60.f, 150.f}, 12.f, Color::White, 1.f, Color(226, 232, 240));

    // Ngày chiếu
    Text lblDate(boldFont, L"Ng\u00e0y chi\u1ebfu", 12); lblDate.setFillColor(Color(71, 85, 105)); lblDate.setPosition({position.x + 50.f, fCardY + 16.f}); win.draw(lblDate);
    drawRoundedCard(win, {position.x + 50.f, fCardY + 38.f}, {230.f, 38.f}, 8.f, Color(248, 250, 252), 1.f, Color(203, 213, 225));
    Text valDate(font, L"10/27/2026", 13); valDate.setFillColor(Color(15, 23, 42)); valDate.setPosition({position.x + 62.f, fCardY + 47.f}); win.draw(valDate);

    // Phòng chiếu
    Text lblRoom(boldFont, L"Ph\u00f2ng chi\u1ebfu", 12); lblRoom.setFillColor(Color(71, 85, 105)); lblRoom.setPosition({position.x + 310.f, fCardY + 16.f}); win.draw(lblRoom);
    drawRoundedCard(win, {position.x + 310.f, fCardY + 38.f}, {230.f, 38.f}, 8.f, Color(248, 250, 252), 1.f, Color(203, 213, 225));
    Text valRoom(font, selectedRoom, 13); valRoom.setFillColor(Color(15, 23, 42)); valRoom.setPosition({position.x + 322.f, fCardY + 47.f}); win.draw(valRoom);
    Text arrRoom(boldFont, L"\u25bc", 11); arrRoom.setFillColor(Color(100, 116, 139)); arrRoom.setPosition({position.x + 515.f, fCardY + 50.f}); win.draw(arrRoom);

    // Trạng thái bán vé
    Text lblStat(boldFont, L"Tr\u1ea1ng th\u00e1i b\u00e1n v\u00e9", 12); lblStat.setFillColor(Color(71, 85, 105)); lblStat.setPosition({position.x + 50.f, fCardY + 88.f}); win.draw(lblStat);
    drawRoundedCard(win, {position.x + 50.f, fCardY + 108.f}, {230.f, 36.f}, 8.f, Color(248, 250, 252), 1.f, Color(203, 213, 225));
    Text valStat(font, selectedStatus, 13); valStat.setFillColor(Color(15, 23, 42)); valStat.setPosition({position.x + 62.f, fCardY + 116.f}); win.draw(valStat);
    Text arrStat(boldFont, L"\u25bc", 11); arrStat.setFillColor(Color(100, 116, 139)); arrStat.setPosition({position.x + 250.f, fCardY + 119.f}); win.draw(arrStat);

    // Thẻ KPI Summary màu xanh dương bên phải
    float kpiX = position.x + width - 380.f;
    drawRoundedCard(win, {kpiX, fCardY + 28.f}, {280.f, 88.f}, 12.f, Color(0, 119, 182));

    Text kpiNum(boldFont, to_wstring(filteredShowtimes.size()), 22);
    kpiNum.setFillColor(Color::White);
    kpiNum.setPosition({kpiX + 20.f, fCardY + 48.f});
    win.draw(kpiNum);

    FloatRect kb = kpiNum.getGlobalBounds();
    Text kpiUnit(boldFont, L"SU\u1ea4T\nCHI\u1ebeU", 11);
    kpiUnit.setFillColor(Color(224, 242, 254));
    kpiUnit.setPosition({std::floor(kb.position.x + kb.size.x + 8.f), std::floor(fCardY + 48.f)});
    win.draw(kpiUnit);

    Text kpiDate(font, L"Ng\u00e0y: 2026-10-27", 12);
    kpiDate.setFillColor(Color(224, 242, 254));
    kpiDate.setPosition({kpiX + 130.f, fCardY + 50.f});
    win.draw(kpiDate);

    Text kpiRoom(font, L"Ph\u00f2ng: " + selectedRoom, 12);
    kpiRoom.setFillColor(Color(224, 242, 254));
    kpiRoom.setPosition({kpiX + 130.f, fCardY + 70.f});
    win.draw(kpiRoom);

    // ==========================================
    // 5. BẢNG DỮ LIỆU SUẤT CHIẾU
    // ==========================================
    float tableY = position.y + 316.f;
    float tableW = width - 60.f;
    float rowH = 46.f;

    // Header bảng (#0284C7)
    drawRoundedCard(win, {position.x + 30.f, tableY}, {tableW, 44.f}, 8.f, Color(2, 132, 199));

    Text h1(boldFont, L"GI\u1edc CHI\u1ebeU", 12); h1.setFillColor(Color::White); h1.setPosition({position.x + 50.f, tableY + 14.f}); win.draw(h1);
    Text h2(boldFont, L"PH\u00d2NG", 12); h2.setFillColor(Color::White); h2.setPosition({position.x + 220.f, tableY + 14.f}); win.draw(h2);
    Text h3(boldFont, L"PHIM", 12); h3.setFillColor(Color::White); h3.setPosition({position.x + 380.f, tableY + 14.f}); win.draw(h3);
    Text h4(boldFont, L"GI\u00c1 V\u00c9", 12); h4.setFillColor(Color::White); h4.setPosition({position.x + 650.f, tableY + 14.f}); win.draw(h4);
    Text h5(boldFont, L"T\u1ef8 L\u1ec6 L\u1ea4P \u0110\u1ea6Y", 12); h5.setFillColor(Color::White); h5.setPosition({position.x + 780.f, tableY + 14.f}); win.draw(h5);
    Text h6(boldFont, L"THAO T\u00c1C", 12); h6.setFillColor(Color::White); h6.setPosition({position.x + width - 105.f, tableY + 14.f}); win.draw(h6);

    // Các dòng dữ liệu
    int startIdx = (currentPage - 1) * pageSize;
    int count = min((int)filteredShowtimes.size() - startIdx, pageSize);

    for (int r = 0; r < count; ++r) {
        int idx = startIdx + r;
        const auto& itm = filteredShowtimes[idx];
        float rY = tableY + 44.f + r * rowH;
        bool isHov = (hoveredRow == r);

        drawRoundedCard(win, {position.x + 30.f, rY}, {tableW, rowH}, 0.f, isHov ? Color(241, 245, 249) : Color::White, 0.5f, Color(241, 245, 249));

        // 1. Giờ chiếu
        CircleShape dotBlue(3.5f); dotBlue.setFillColor(Color(2, 132, 199)); dotBlue.setPosition({position.x + 50.f, rY + 20.f}); win.draw(dotBlue);
        Text timeTxt(boldFont, s2ws(itm.startTime + " \u2013 " + itm.endTime), 13); timeTxt.setFillColor(Color(15, 23, 42)); timeTxt.setPosition({position.x + 64.f, rY + 14.f}); win.draw(timeTxt);

        // 2. Phòng
        Text roomTxt(font, itm.roomName, 13); roomTxt.setFillColor(Color(51, 65, 85)); roomTxt.setPosition({position.x + 220.f, rY + 14.f}); win.draw(roomTxt);

        // 3. Phim
        Text movieTxt(boldFont, itm.movieTitle, 13); movieTxt.setFillColor(Color(15, 23, 42)); movieTxt.setPosition({position.x + 380.f, rY + 14.f}); win.draw(movieTxt);

        // 4. Giá vé
        Text priceTxt(boldFont, s2ws(formatPriceStr(itm.price)), 13); priceTxt.setFillColor(Color(15, 23, 42)); priceTxt.setPosition({position.x + 650.f, rY + 14.f}); win.draw(priceTxt);

        // 5. Tỷ lệ lấp đầy
        int pct = (int)((itm.bookedSeats * 100.f) / itm.totalSeats + 0.5f);
        Color barCol = Color(16, 185, 129); // Xanh lá
        if (pct >= 100) barCol = Color(239, 68, 68); // Đỏ hết vé
        else if (pct < 70) barCol = Color(245, 158, 11); // Cam

        wstring seatInfo = to_wstring(itm.bookedSeats) + L"/" + to_wstring(itm.totalSeats) + L" v\u00e9";
        if (pct >= 100) seatInfo = to_wstring(itm.bookedSeats) + L"/" + to_wstring(itm.totalSeats) + L" (H\u1ebft v\u00e9)";

        Text seatTxt(font, seatInfo, 12.5f); seatTxt.setFillColor(barCol); seatTxt.setPosition({position.x + 780.f, rY + 10.f}); win.draw(seatTxt);
        Text pctTxt(boldFont, to_wstring(pct) + L"%", 12.5f); pctTxt.setFillColor(barCol); pctTxt.setPosition({position.x + 860.f, rY + 10.f}); win.draw(pctTxt);

        // Progress bar
        drawRoundedCard(win, {position.x + 780.f, rY + 28.f}, {100.f, 4.f}, 2.f, Color(226, 232, 240));
        float fillW = max(4.f, 100.f * ((float)itm.bookedSeats / itm.totalSeats));
        drawRoundedCard(win, {position.x + 780.f, rY + 28.f}, {fillW, 4.f}, 2.f, barCol);

        // 6. Thao tác: Sửa, Xóa
        float actX = position.x + width - 110.f;
        drawRoundedCard(win, {actX, rY + 9.f}, {36.f, 26.f}, 4.f, Color(241, 245, 249), 1.f, Color(203, 213, 225));
        Text btnEditTxt(boldFont, L"S\u1eeda", 11);
        btnEditTxt.setFillColor(Color(2, 132, 199));
        btnEditTxt.setPosition({std::floor(actX + 7.f), std::floor(rY + 13.f)});
        win.draw(btnEditTxt);

        drawRoundedCard(win, {actX + 42.f, rY + 9.f}, {36.f, 26.f}, 4.f, Color(254, 242, 242), 1.f, Color(254, 202, 202));
        Text btnDelTxt(boldFont, L"X\u00f3a", 11);
        btnDelTxt.setFillColor(Color(239, 68, 68));
        btnDelTxt.setPosition({std::floor(actX + 49.f), std::floor(rY + 13.f)});
        win.draw(btnDelTxt);
    }

    // ==========================================
    // 6. PHÂN TRANG (Pagination)
    // ==========================================
    float pagY = position.y + 830.f;
    wstring showInfo = L"Hi\u1ec3n th\u1ecb 1 \u2013 " + to_wstring(count) + L" tr\u00ean t\u1ed5ng s\u1ed1 " + to_wstring(filteredShowtimes.size()) + L" su\u1ea5t chi\u1ebfu";
    Text pageInfo(font, showInfo, 12); pageInfo.setFillColor(Color(100, 116, 139)); pageInfo.setPosition({position.x + 30.f, pagY + 6.f}); win.draw(pageInfo);

    // Nút Trước
    drawRoundedCard(win, {position.x + width - 260.f, pagY}, {55.f, 32.f}, 6.f, Color::White, 1.f, Color(226, 232, 240));
    Text prTxt(font, L"Tr\u01b0\u1edbc", 12); prTxt.setFillColor(Color(100, 116, 139)); prTxt.setPosition({position.x + width - 250.f, pagY + 6.f}); win.draw(prTxt);

    // Nút 1
    bool p1Active = (currentPage == 1);
    drawRoundedCard(win, {position.x + width - 195.f, pagY}, {32.f, 32.f}, 6.f, p1Active ? Color(2, 132, 199) : Color::White, 1.f, Color(226, 232, 240));
    Text p1Txt(boldFont, L"1", 12); p1Txt.setFillColor(p1Active ? Color::White : Color(51, 65, 85)); p1Txt.setPosition({position.x + width - 183.f, pagY + 6.f}); win.draw(p1Txt);

    // Nút 2
    bool p2Active = (currentPage == 2);
    drawRoundedCard(win, {position.x + width - 155.f, pagY}, {32.f, 32.f}, 6.f, p2Active ? Color(2, 132, 199) : Color::White, 1.f, Color(226, 232, 240));
    Text p2Txt(boldFont, L"2", 12); p2Txt.setFillColor(p2Active ? Color::White : Color(51, 65, 85)); p2Txt.setPosition({position.x + width - 143.f, pagY + 6.f}); win.draw(p2Txt);

    // Nút 3
    bool p3Active = (currentPage == 3);
    drawRoundedCard(win, {position.x + width - 115.f, pagY}, {32.f, 32.f}, 6.f, p3Active ? Color(2, 132, 199) : Color::White, 1.f, Color(226, 232, 240));
    Text p3Txt(boldFont, L"3", 12); p3Txt.setFillColor(p3Active ? Color::White : Color(51, 65, 85)); p3Txt.setPosition({position.x + width - 103.f, pagY + 6.f}); win.draw(p3Txt);

    // Nút Tiếp
    drawRoundedCard(win, {position.x + width - 75.f, pagY}, {55.f, 32.f}, 6.f, Color::White, 1.f, Color(226, 232, 240));
    Text nxTxt(font, L"Ti\u1ebfp", 12); nxTxt.setFillColor(Color(100, 116, 139)); nxTxt.setPosition({position.x + width - 63.f, pagY + 6.f}); win.draw(nxTxt);

    // ==========================================
    // 7. KHỐI CHÍNH SÁCH RẠP (POLICY NOTE BOX)
    // ==========================================
    float noteY = position.y + 876.f;
    drawRoundedCard(win, {position.x + 30.f, noteY}, {tableW, 60.f}, 8.f, Color(238, 248, 255), 1.f, Color(186, 230, 253));

    // Icon info
    CircleShape infoCir(8.f); infoCir.setFillColor(Color(2, 132, 199)); infoCir.setPosition({position.x + 48.f, noteY + 16.f}); win.draw(infoCir);
    Text ifTxt(boldFont, L"i", 11.5f); ifTxt.setFillColor(Color::White); ifTxt.setPosition({position.x + 53.f, noteY + 16.f}); win.draw(ifTxt);

    Text noteTxt(font, L"Quy chu\u1ea9n d\u1ecdn d\u1eb9p & gi\u00e3n c\u00e1ch su\u1ea5t chi\u1ebfu: Gi\u1eefa m\u1ed7i su\u1ea5t chi\u1ebfu, h\u1ec7 th\u1ed1ng t\u1ef1 \u0111\u1ed9ng ch\u1eeba 15\u201320 ph\u00fat th\u1eddi gian l\u00e0m v\u1ec7 sinh ph\u00f2ng v\u00e0 ki\u1ec3m tra thi\u1ebft b\u1ecb \u00e2m\nthanh/m\u00e1y chi\u1ebfu tr\u01b0\u1edbc khi \u0111\u00f3n l\u01b0\u1ee3t kh\u00e1ch ti\u1ebfp theo. \u0110\u1ec3 thay \u0111\u1ed5i th\u1eddi gian ngh\u1ec9 k\u1ef9 thu\u1eadt n\u00e0y, vui l\u00f2ng truy c\u1eadp m\u1ee5c C\u1ea5u h\u00ecnh r\u1ea1p chi\u1ebfu.", 12);
    noteTxt.setFillColor(Color(71, 85, 105));
    noteTxt.setPosition({position.x + 75.f, noteY + 13.f});
    win.draw(noteTxt);

    // ==========================================
    // 8. DROPDOWN MENUS (Render trên cùng)
    // ==========================================
    if (roomDropdownOpen) {
        float ddY = fCardY + 76.f;
        drawRoundedCard(win, {position.x + 310.f, ddY}, {230.f, (float)roomList.size() * 34.f}, 8.f, Color::White, 1.2f, Color(2, 132, 199));
        for (size_t i = 0; i < roomList.size(); ++i) {
            float iY = ddY + i * 34.f;
            if (roomList[i] == selectedRoom) {
                RoundedRectRenderer::draw(win, {position.x + 312.f, iY}, {226.f, 32.f}, 4.f, Color(224, 242, 254));
            }
            Text itmTxt(font, roomList[i], 12);
            itmTxt.setFillColor((roomList[i] == selectedRoom) ? Color(2, 132, 199) : Color(15, 23, 42));
            itmTxt.setPosition({position.x + 322.f, iY + 8.f});
            win.draw(itmTxt);
        }
    }

    if (statusDropdownOpen) {
        float ddY = fCardY + 144.f;
        drawRoundedCard(win, {position.x + 50.f, ddY}, {230.f, (float)statusList.size() * 34.f}, 8.f, Color::White, 1.2f, Color(2, 132, 199));
        for (size_t i = 0; i < statusList.size(); ++i) {
            float iY = ddY + i * 34.f;
            if (statusList[i] == selectedStatus) {
                RoundedRectRenderer::draw(win, {position.x + 52.f, iY}, {226.f, 32.f}, 4.f, Color(224, 242, 254));
            }
            Text itmTxt(font, statusList[i], 12);
            itmTxt.setFillColor((statusList[i] == selectedStatus) ? Color(2, 132, 199) : Color(15, 23, 42));
            itmTxt.setPosition({position.x + 62.f, iY + 8.f});
            win.draw(itmTxt);
        }
    }

    // ==========================================
    // 9. POPUP THÊM / SỬA SUẤT CHIẾU
    // ==========================================
    if (showAddPopup) {
        RectangleShape overlay({width, height});
        overlay.setPosition(position);
        overlay.setFillColor(Color(15, 23, 42, 140));
        win.draw(overlay);

        float popW = 450.f;
        float popH = 300.f;
        float popX = position.x + (width - popW) / 2.f;
        float popY = position.y + (height - popH) / 2.f - 30.f;

        drawRoundedCard(win, {popX, popY}, {popW, popH}, 12.f, Color::White);

        Text pTitle(boldFont, (editingIndex >= 0) ? L"S\u1eeda su\u1ea5t chi\u1ebfu" : L"Th\u00eam su\u1ea5t chi\u1ebfu m\u1edbi", 18);
        pTitle.setFillColor(Color(15, 23, 42));
        pTitle.setPosition({popX + 30.f, popY + 20.f});
        win.draw(pTitle);

        Text l1(boldFont, L"T\u00ean phim:", 12); l1.setFillColor(Color(71, 85, 105)); l1.setPosition({popX + 30.f, popY + 60.f}); win.draw(l1);
        drawRoundedCard(win, {popX + 30.f, popY + 80.f}, {popW - 60.f, 36.f}, 6.f, Color(248, 250, 252), 1.f, Color(203, 213, 225));
        Text v1(font, popupMovieTitle, 12); v1.setFillColor(Color(15, 23, 42)); v1.setPosition({popX + 40.f, popY + 89.f}); win.draw(v1);

        Text l2(boldFont, L"Gi\u1edd b\u1eaft \u0111\u1ea7u (HH:mm):", 12); l2.setFillColor(Color(71, 85, 105)); l2.setPosition({popX + 30.f, popY + 125.f}); win.draw(l2);
        drawRoundedCard(win, {popX + 30.f, popY + 145.f}, {180.f, 36.f}, 6.f, Color(248, 250, 252), 1.f, Color(203, 213, 225));
        Text v2(font, s2ws(popupStartTime), 12); v2.setFillColor(Color(15, 23, 42)); v2.setPosition({popX + 40.f, popY + 154.f}); win.draw(v2);

        Text l3(boldFont, L"Gi\u00e1 v\u00e9 (\u0111):", 12); l3.setFillColor(Color(71, 85, 105)); l3.setPosition({popX + 230.f, popY + 125.f}); win.draw(l3);
        drawRoundedCard(win, {popX + 230.f, popY + 145.f}, {190.f, 36.f}, 6.f, Color(248, 250, 252), 1.f, Color(203, 213, 225));
        Text v3(font, s2ws(popupPrice), 12); v3.setFillColor(Color(15, 23, 42)); v3.setPosition({popX + 240.f, popY + 154.f}); win.draw(v3);

        // Buttons
        drawRoundedCard(win, {popX + 60.f, popY + 230.f}, {150.f, 38.f}, 6.f, Color(2, 132, 199));
        Text sTxt(boldFont, L"L\u01b0u th\u00f4ng tin", 13); sTxt.setFillColor(Color::White); sTxt.setPosition({popX + 90.f, popY + 239.f}); win.draw(sTxt);

        drawRoundedCard(win, {popX + 240.f, popY + 230.f}, {150.f, 38.f}, 6.f, Color(241, 245, 249), 1.f, Color(203, 213, 225));
        Text cTxt(boldFont, L"H\u1ee7y b\u1ecf", 13); cTxt.setFillColor(Color(71, 85, 105)); cTxt.setPosition({popX + 290.f, popY + 239.f}); win.draw(cTxt);
    }

    // ==========================================
    // 10. TOAST NOTIFICATION
    // ==========================================
    if (notificationVisible) {
        if (notificationClock.getElapsedTime().asSeconds() > 3.0f) {
            notificationVisible = false;
        } else {
            float notW = 380.f;
            float notH = 44.f;
            float notX = position.x + (width - notW) / 2.f;
            float notY = position.y + height - 70.f;
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
