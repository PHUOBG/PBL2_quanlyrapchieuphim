#include "UI/components/Admin/ShowtimePanel.h"
#include "UI/components/Admin/UITheme.h"
#include "UI/components/Admin/RoundedRectRenderer.h"
#include "utils/FileUtils.h"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <unordered_map>
#include <ctime>
#include <filesystem>
#include <chrono>
#include <cctype>

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

static int parseTimeMinutes(const string& value) {
    if (value == "24:00") return 24 * 60;
    if (value.size() != 5 || value[2] != ':') return -1;
    try {
        if (!isdigit(static_cast<unsigned char>(value[0])) || !isdigit(static_cast<unsigned char>(value[1])) || !isdigit(static_cast<unsigned char>(value[3])) || !isdigit(static_cast<unsigned char>(value[4]))) return -1;
        int h = stoi(value.substr(0, 2)), m = stoi(value.substr(3, 2));
        if (h < 0 || h > 23 || m < 0 || m > 59) return -1;
        return h * 60 + m;
    } catch (...) { return -1; }
}

static string formatTimeMinutes(int minutes) {
    minutes = max(0, min(minutes, 24 * 60));
    ostringstream out; out << setfill('0') << setw(2) << (minutes / 60) << ':' << setw(2) << (minutes % 60);
    return out.str();
}

static bool isSupportedShowDate(const string& date) {
    if (date.size() != 10 || date[4] != '-' || date[7] != '-') return false;
    try {
        int year = stoi(date.substr(0, 4)), month = stoi(date.substr(5, 2)), day = stoi(date.substr(8, 2));
        return year >= 2025 && year <= 2026 && chrono::year_month_day{chrono::year{year}, chrono::month{static_cast<unsigned>(month)}, chrono::day{static_cast<unsigned>(day)}}.ok();
    } catch (...) { return false; }
}

static string todayIsoDate() {
    time_t now = time(nullptr); tm local{};
#if defined(_WIN32)
    localtime_s(&local, &now);
#else
    localtime_r(&now, &local);
#endif
    char date[11]{}; strftime(date, sizeof(date), "%Y-%m-%d", &local); return date;
}

static int nowLocalMinutes() {
    time_t now = time(nullptr); tm local{};
#if defined(_WIN32)
    localtime_s(&local, &now);
#else
    localtime_r(&now, &local);
#endif
    return local.tm_hour * 60 + local.tm_min;
}

ShowtimePanel::ShowtimePanel(Font& fontRef, float w, float h)
    : font(fontRef),
      boldFont("../assets/fonts/segoeuib.ttf"),
      width(w), height(h), position(280.f, 0.f)
{
    selectedDate = todayIsoDate();
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
        L"\u0110\u00e3 k\u1ebft th\u00fac"
    };

    if (reloadTex.loadFromFile("assets/elements/reload.png") ||
        reloadTex.loadFromFile("../assets/elements/reload.png")) {
        reloadTex.setSmooth(true);
        hasReloadTex = true;
    }

    loadShowtimesFromFile();
    applyFilter();
}

void ShowtimePanel::setPosition(Vector2f pos) {
    position = pos;
}

void ShowtimePanel::loadShowtimesFromFile() {
    allShowtimes.clear();
    const string path = FileUtils::resolveDataPath("data/showtimes.txt");
    ifstream f(path);
    if (!f.is_open()) return;

    unordered_map<string, wstring> movieTitles;
    unordered_map<string, int> movieDurations;
    ifstream movies(FileUtils::resolveDataPath("data/movies.txt"));
    string line;
    getline(movies, line);
    while (getline(movies, line)) {
        stringstream row(line); string id, title, age, country, language, genres, duration;
        getline(row, id, '|'); getline(row, title, '|'); getline(row, age, '|'); getline(row, country, '|'); getline(row, language, '|'); getline(row, genres, '|'); getline(row, duration, '|');
        if (!id.empty()) { movieTitles[id] = s2ws(title); try { movieDurations[id] = stoi(duration); } catch (...) { movieDurations[id] = 0; } }
    }

    unordered_map<string, wstring> roomNames;
    unordered_map<string, int> roomCapacities;
    const wstring previousRoom = selectedRoom;
    roomList.clear(); roomList.push_back(L"Tất cả phòng");
    ifstream rooms(FileUtils::resolveDataPath("data/rooms.txt"));
    getline(rooms, line);
    while (getline(rooms, line)) {
        stringstream row(line); string id, name, rowCount, colCount;
        getline(row, id, '|'); getline(row, name, '|'); getline(row, rowCount, '|'); getline(row, colCount, '|');
        if (id.empty()) continue;
        roomNames[id] = s2ws(name);
        roomList.push_back(s2ws(name));
        try { roomCapacities[id] = stoi(rowCount) * stoi(colCount); } catch (...) { roomCapacities[id] = 0; }
    }
    if (find(roomList.begin(), roomList.end(), previousRoom) == roomList.end()) selectedRoom = roomList.front();

    unordered_map<string, int> bookedByShowtime;
    ifstream tickets(FileUtils::resolveDataPath("data/tickets.txt"));
    getline(tickets, line);
    while (getline(tickets, line)) {
        stringstream row(line); string id, showtimeId, title, date, time, room, seats;
        getline(row,id,'|'); getline(row,showtimeId,'|'); getline(row,title,'|'); getline(row,date,'|'); getline(row,time,'|'); getline(row,room,'|'); getline(row,seats,'|');
        if (showtimeId.empty() || seats.empty() || seats == "Khong co" || seats == "Không có") continue;
        int count = 1;
        for (char c : seats) if (c == ',') ++count;
        bookedByShowtime[showtimeId] += count;
    }

    getline(f, line); // schema header
    while (getline(f, line)) {
        if (line.empty()) continue;
        stringstream row(line); string id, movieId, roomId, date, start, price;
        getline(row,id,'|'); getline(row,movieId,'|'); getline(row,roomId,'|'); getline(row,date,'|'); getline(row,start,'|'); getline(row,price,'|');
        if (id.empty() || movieId.empty() || roomId.empty() || date.empty() || start.empty()) continue;
        ShowtimeItem item;
        item.id=id; item.movieId=movieId; item.roomId=roomId; item.date=date; item.startTime=start;
        item.movieTitle = movieTitles.count(movieId) ? movieTitles[movieId] : s2ws(movieId);
        item.roomName = roomNames.count(roomId) ? roomNames[roomId] : s2ws(roomId);
        try { item.price=stoi(price); } catch (...) { continue; }
        item.totalSeats = roomCapacities.count(roomId) ? roomCapacities[roomId] : 0;
        item.bookedSeats = bookedByShowtime[id];
        const int startMinutes = parseTimeMinutes(start);
        const int duration = movieDurations.count(movieId) ? movieDurations[movieId] : 0;
        item.endTime = startMinutes >= 0 && duration > 0 ? formatTimeMinutes(startMinutes + duration) : start;
        allShowtimes.push_back(std::move(item));
    }
}

bool ShowtimePanel::saveShowtimesToFile() {
    string path = FileUtils::resolveDataPath("data/showtimes.txt");
    const string tempPath = path + ".tmp";
    const string backupPath = path + ".bak";
    ofstream f(tempPath, ios::trunc);
    if (!f.is_open()) return false;

    f << "showtime_id|movie_id|room_id|show_date|start_time|ticket_price\n";
    for (const auto& item : allShowtimes) {
        f << item.id << "|" << item.movieId << "|" << item.roomId << "|" << item.date << "|" << item.startTime << "|" << item.price << "\n";
    }
    f.flush();
    const bool written = f.good();
    f.close();
    if (!written) { filesystem::remove(tempPath); return false; }
    error_code ec;
    filesystem::remove(backupPath, ec);
    ec.clear();
    const bool hadOriginal = filesystem::exists(path, ec) && !ec;
    if (hadOriginal) {
        filesystem::rename(path, backupPath, ec);
        if (ec) { filesystem::remove(tempPath); return false; }
    }
    filesystem::rename(tempPath, path, ec);
    if (ec) {
        if (hadOriginal) { error_code restoreError; filesystem::rename(backupPath, path, restoreError); }
        filesystem::remove(tempPath);
        return false;
    }
    if (hadOriginal) filesystem::remove(backupPath, ec);
    return true;
}

void ShowtimePanel::applyFilter() {
    filteredShowtimes.clear();
    for (const auto& itm : allShowtimes) {
        // Lọc ngày
        if (!selectedDate.empty() && itm.date != selectedDate) continue;
        // Lọc phòng
        if (selectedRoom != L"T\u1ea5t c\u1ea3 ph\u00f2ng" && itm.roomName != selectedRoom) {
            continue;
        }
        // Lọc trạng thái
        const bool soldOut = itm.totalSeats > 0 && itm.bookedSeats >= itm.totalSeats;
        const int endMinutes = parseTimeMinutes(itm.endTime);
        const bool ended = itm.date < todayIsoDate() || (itm.date == todayIsoDate() && endMinutes >= 0 && endMinutes <= nowLocalMinutes());
        if (selectedStatus == L"\u0110\u00e3 h\u1ebft v\u00e9" && !soldOut) continue;
        if (selectedStatus == L"\u0110ang m\u1edf b\u00e1n" && (soldOut || ended)) continue;
        if (selectedStatus == L"\u0110ã k\u1ebft thúc" && !ended) continue;
        filteredShowtimes.push_back(itm);
    }

    totalPages = max(1, (int)((filteredShowtimes.size() + pageSize - 1) / pageSize));
    if (currentPage > totalPages) currentPage = totalPages;
    if (currentPage < 1) currentPage = 1;
}

void ShowtimePanel::autoScheduleShowtimes() {
    if (!isSupportedShowDate(selectedDate)) {
        notificationMessage = "Chọn ngày hợp lệ trong khoảng 2025-2026 trước khi xếp lịch.";
        notificationClock.restart(); notificationVisible = true; return;
    }
    const int start = parseTimeMinutes(scheduleStartTime), end = parseTimeMinutes(scheduleEndTime);
    int buffer = -1;
    try { if (!scheduleBreakMinutes.empty() && all_of(scheduleBreakMinutes.begin(), scheduleBreakMinutes.end(), [](unsigned char c) { return isdigit(c); })) buffer = stoi(scheduleBreakMinutes); } catch (...) { buffer = -1; }
    if (start < 0 || end < 0 || end <= start || buffer < 0 || buffer > 180) {
        notificationMessage = "Giờ bắt đầu/kết thúc hoặc thời gian nghỉ không hợp lệ.";
        notificationClock.restart(); notificationVisible = true; return;
    }
    if (any_of(allShowtimes.begin(), allShowtimes.end(), [&](const ShowtimeItem& s) { return s.date == selectedDate; })) {
        notificationMessage = "Ngày này đã có lịch. Hãy chọn ngày chưa có suất chiếu.";
        notificationClock.restart(); notificationVisible = true; return;
    }

    struct MovieForSchedule { string id; string title; int duration; };
    struct RoomForSchedule { string id; string name; int capacity; };
    vector<MovieForSchedule> moviesToSchedule;
    vector<RoomForSchedule> roomsToSchedule;
    string line;
    ifstream movieFile(FileUtils::resolveDataPath("data/movies.txt"));
    getline(movieFile, line);
    while (getline(movieFile, line)) {
        stringstream row(line); vector<string> c; string value;
        while (getline(row, value, '|')) c.push_back(value);
        if (c.size() < 14 || c[0].empty() || c[13] != "Đang chiếu") continue;
        int duration = 0; try { duration = stoi(c[6]); } catch (...) { duration = 0; }
        if (duration > 0 && duration <= 360) moviesToSchedule.push_back({c[0], c[1], duration});
    }
    ifstream roomFile(FileUtils::resolveDataPath("data/rooms.txt"));
    getline(roomFile, line);
    while (getline(roomFile, line)) {
        stringstream row(line); string id, name, rows, cols;
        getline(row, id, '|'); getline(row, name, '|'); getline(row, rows, '|'); getline(row, cols, '|');
        int capacity = 0; try { capacity = stoi(rows) * stoi(cols); } catch (...) { capacity = 0; }
        if (!id.empty() && !name.empty() && capacity > 0) roomsToSchedule.push_back({id, name, capacity});
    }
    if (moviesToSchedule.empty() || roomsToSchedule.empty()) {
        notificationMessage = moviesToSchedule.empty() ? "Không có phim Đang chiếu đủ thời lượng." : "Không có phòng chiếu hợp lệ.";
        notificationClock.restart(); notificationVisible = true; return;
    }

    vector<ShowtimeItem> generated;
    int counter = static_cast<int>(allShowtimes.size()) + 1;
    for (const auto& room : roomsToSchedule) {
        int cursor = start;
        for (const auto& movie : moviesToSchedule) {
            const int finish = cursor + movie.duration;
            if (finish > end) break;
            ShowtimeItem item;
            ostringstream showtimeId; showtimeId << "SUATCHIEU_" << selectedDate.substr(0, 4) << selectedDate.substr(5, 2) << selectedDate.substr(8, 2) << '_' << setfill('0') << setw(4) << counter++;
            item.id = showtimeId.str(); item.movieId = movie.id; item.movieTitle = s2ws(movie.title); item.roomId = room.id; item.roomName = s2ws(room.name);
            item.date = selectedDate; item.startTime = formatTimeMinutes(cursor); item.endTime = formatTimeMinutes(finish);
            const int hour = cursor / 60;
            item.price = hour < 12 ? 65000 : (hour < 17 ? 70000 : (hour < 20 ? 75000 : 80000));
            item.totalSeats = room.capacity; item.bookedSeats = 0;
            generated.push_back(std::move(item)); cursor = finish + buffer;
        }
    }
    if (generated.empty()) {
        notificationMessage = "Không phim nào vừa trong khung giờ đã chọn.";
        notificationClock.restart(); notificationVisible = true; return;
    }
    allShowtimes.insert(allShowtimes.end(), generated.begin(), generated.end());
    if (!saveShowtimesToFile()) {
        loadShowtimesFromFile(); applyFilter();
        notificationMessage = "Không thể ghi lịch chiếu vào data/showtimes.txt.";
        notificationClock.restart(); notificationVisible = true; return;
    }
    applyFilter();
    notificationMessage = "Đã xếp " + to_string(generated.size()) + " suất cho " + to_string(roomsToSchedule.size()) + " phòng.";
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

            if (showSchedulePopup) {
                const float popX = position.x + (width - 520.f) / 2.f, popY = position.y + (height - 340.f) / 2.f;
                const FloatRect save({popX + 40.f, popY + 270.f}, {200.f, 40.f});
                const FloatRect cancel({popX + 280.f, popY + 270.f}, {200.f, 40.f});
                if (save.contains(mousePos)) { showSchedulePopup = false; autoScheduleShowtimes(); return; }
                if (cancel.contains(mousePos)) { showSchedulePopup = false; return; }
                const FloatRect fields[] = {{ {popX + 30.f, popY + 92.f}, {460.f, 38.f} }, {{popX + 30.f, popY + 160.f}, {460.f, 38.f}}, {{popX + 30.f, popY + 228.f}, {460.f, 38.f}}};
                for (int i = 0; i < 3; ++i) if (fields[i].contains(mousePos)) { scheduleActiveField = i; return; }
                return;
            }

            dateInputActive = FloatRect({position.x + 50.f, position.y + 198.f}, {210.f, 38.f}).contains(mousePos);
            if (dateInputActive) return;

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

            // Nút Làm mới (Dấu xoay vòng cạnh KPI Summary)
            float fCardY_ev = position.y + 144.f;
            FloatRect btnReloadKpi({position.x + width - 85.f, fCardY_ev + 51.f}, {48.f, 48.f});
            if (btnReloadKpi.contains(mousePos)) {
                loadShowtimesFromFile();
                applyFilter();
                notificationMessage = "Đã làm mới danh sách suất chiếu!";
                notificationClock.restart();
                notificationVisible = true;
                return;
            }

            if (btnAuto.contains(mousePos)) {
                showSchedulePopup = true;
                scheduleActiveField = 0;
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
                    const string removedId = filteredShowtimes[realIdx].id;
                    allShowtimes.erase(remove_if(allShowtimes.begin(), allShowtimes.end(), [&](const ShowtimeItem& x) { return x.id == removedId; }), allShowtimes.end());
                    if (!saveShowtimesToFile()) {
                        loadShowtimesFromFile(); applyFilter();
                        notificationMessage = "Không thể lưu tệp lịch chiếu.";
                        notificationClock.restart(); notificationVisible = true;
                        return;
                    }
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
                        const string editId = filteredShowtimes[editingIndex].id;
                        auto actual = find_if(allShowtimes.begin(), allShowtimes.end(), [&](const ShowtimeItem& x) { return x.id == editId; });
                        if (actual == allShowtimes.end()) { notificationMessage = "Không tìm thấy suất chiếu cần sửa."; notificationVisible = true; notificationClock.restart(); return; }
                        actual->startTime = popupStartTime;
                        try { actual->price = stoi(popupPrice); } catch (...) { notificationMessage = "Giá vé không hợp lệ."; notificationVisible = true; notificationClock.restart(); return; }
                    } else {
                        notificationMessage = "Thêm suất chiếu chưa được hỗ trợ vì form chưa chọn mã phim và phòng hợp lệ.";
                        notificationVisible = true; notificationClock.restart(); return;
                    }
                    if (!saveShowtimesToFile()) {
                        loadShowtimesFromFile(); applyFilter();
                        notificationMessage = "Không thể lưu tệp lịch chiếu.";
                        notificationClock.restart(); notificationVisible = true;
                        return;
                    }
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

    if (showSchedulePopup) {
        if (const auto* key = event.getIf<Event::KeyPressed>(); key && key->code == Keyboard::Key::Backspace) {
            string* fields[] = {&scheduleStartTime, &scheduleEndTime, &scheduleBreakMinutes};
            if (scheduleActiveField >= 0 && scheduleActiveField < 3 && !fields[scheduleActiveField]->empty()) fields[scheduleActiveField]->pop_back();
        }
        if (const auto* text = event.getIf<Event::TextEntered>()) {
            const char32_t c = text->unicode;
            if (scheduleActiveField == 0 || scheduleActiveField == 1) {
                string* field = scheduleActiveField == 0 ? &scheduleStartTime : &scheduleEndTime;
                if (c >= '0' && c <= '9' && field->size() < 5) field->push_back(static_cast<char>(c));
                else if (c == ':' && field->size() == 2) field->push_back(':');
            } else if (scheduleActiveField == 2 && c >= '0' && c <= '9' && scheduleBreakMinutes.size() < 3) scheduleBreakMinutes.push_back(static_cast<char>(c));
        }
        return;
    }
    if (dateInputActive) {
        if (const auto* key = event.getIf<Event::KeyPressed>(); key && key->code == Keyboard::Key::Backspace && !selectedDate.empty()) selectedDate.pop_back();
        if (const auto* text = event.getIf<Event::TextEntered>()) {
            const char32_t c = text->unicode;
            if (((c >= '0' && c <= '9') || c == '-') && selectedDate.size() < 10) selectedDate.push_back(static_cast<char>(c));
        }
        if (const auto* key = event.getIf<Event::KeyPressed>(); key && key->code == Keyboard::Key::Enter) {
            dateInputActive = false; currentPage = 1; applyFilter();
        }
        return;
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
    Text valDate(font, s2ws(selectedDate), 13); valDate.setFillColor(Color(15, 23, 42)); valDate.setPosition({position.x + 62.f, fCardY + 47.f}); win.draw(valDate);

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

    // Thẻ KPI Summary: Tách làm 2 ô màu khác nhau bên trong 1 ô card bao ngoài
    float kpiOuterW = 285.f;
    float kpiOuterH = 94.f;
    float kpiOuterX = position.x + width - 390.f;
    float kpiOuterY = fCardY + 28.f;

    // 1. Ô bao ngoài (Khung card chính)
    drawRoundedCard(win, {kpiOuterX, kpiOuterY}, {kpiOuterW, kpiOuterH}, 12.f, Color(248, 250, 252), 1.2f, Color(226, 232, 240));

    // 2. Ô trái (Màu 1 - Xanh dương đại dương #0284C7): Thống kê số lượng suất chiếu
    float b1X = kpiOuterX + 7.f;
    float b1Y = kpiOuterY + 7.f;
    float b1W = 126.f;
    float b1H = 80.f;
    drawRoundedCard(win, {b1X, b1Y}, {b1W, b1H}, 8.f, Color(2, 132, 199));

    Text kpiNum(boldFont, to_wstring(filteredShowtimes.size()), 24);
    kpiNum.setFillColor(Color::White);
    FloatRect nb = kpiNum.getLocalBounds();

    Text kpiUnit(boldFont, L"SU\u1ea4T\nCHI\u1ebeU", 11);
    kpiUnit.setFillColor(Color(224, 242, 254));
    FloatRect ub = kpiUnit.getLocalBounds();

    float totalW1 = nb.size.x + 8.f + ub.size.x;
    float startX1 = b1X + (b1W - totalW1) / 2.f;
    kpiNum.setPosition({std::floor(startX1 - nb.position.x), std::floor(b1Y + (b1H - nb.size.y) / 2.f - nb.position.y)});
    kpiUnit.setPosition({std::floor(startX1 + nb.size.x + 8.f - ub.position.x), std::floor(b1Y + (b1H - ub.size.y) / 2.f - ub.position.y)});
    win.draw(kpiNum);
    win.draw(kpiUnit);

    // 3. Ô phải (Màu 2 - Xanh đen Navy đậm #0F172A): Thông tin ngày & phòng chiếu
    float b2X = b1X + b1W + 6.f;
    float b2Y = kpiOuterY + 7.f;
    float b2W = 140.f;
    float b2H = 80.f;
    drawRoundedCard(win, {b2X, b2Y}, {b2W, b2H}, 8.f, Color(15, 23, 42));

    float textPadX = b2X + 12.f;

    Text kpiDateLbl(font, L"Ng\u00e0y: ", 11.5f);
    kpiDateLbl.setFillColor(Color(148, 163, 184));
    kpiDateLbl.setPosition({std::floor(textPadX), std::floor(b2Y + 16.f)});
    win.draw(kpiDateLbl);

    Text kpiDateVal(boldFont, s2ws(selectedDate), 11.5f);
    kpiDateVal.setFillColor(Color::White);
    kpiDateVal.setPosition({std::floor(textPadX + kpiDateLbl.getGlobalBounds().size.x), std::floor(b2Y + 16.f)});
    win.draw(kpiDateVal);

    Text kpiRoomLbl(font, L"Ph\u00f2ng: ", 11.5f);
    kpiRoomLbl.setFillColor(Color(148, 163, 184));
    kpiRoomLbl.setPosition({std::floor(textPadX), std::floor(b2Y + 44.f)});
    win.draw(kpiRoomLbl);

    Text kpiRoomVal(boldFont, selectedRoom, 11.5f);
    kpiRoomVal.setFillColor(Color(56, 189, 248));
    kpiRoomVal.setPosition({std::floor(textPadX + kpiRoomLbl.getGlobalBounds().size.x), std::floor(b2Y + 44.f)});
    win.draw(kpiRoomVal);

    // 4. Nút Làm mới (Dấu xoay vòng reload)
    float refBtnX = position.x + width - 85.f;
    float refBtnY = fCardY + 51.f;
    float refBtnW = 48.f;
    float refBtnH = 48.f;
    drawRoundedCard(win, {refBtnX, refBtnY}, {refBtnW, refBtnH}, 8.f, Color(2, 132, 199));

    if (hasReloadTex) {
        Sprite spReload(reloadTex);
        spReload.setScale({26.f / 500.f, 26.f / 500.f});
        spReload.setPosition({refBtnX + 11.f, refBtnY + 11.f});
        win.draw(spReload);
    }

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

    if (showSchedulePopup) {
        RectangleShape overlay({width, height}); overlay.setPosition(position); overlay.setFillColor(Color(15, 23, 42, 150)); win.draw(overlay);
        const float popX = position.x + (width - 520.f) / 2.f, popY = position.y + (height - 340.f) / 2.f;
        drawRoundedCard(win, {popX, popY}, {520.f, 340.f}, 12.f, Color::White, 1.f, Color(203, 213, 225));
        Text title(boldFont, L"Xếp lịch tự động", 18); title.setFillColor(Color(15, 23, 42)); title.setPosition({popX + 30.f, popY + 20.f}); win.draw(title);
        const wstring labels[] = {L"Bắt đầu (HH:MM)", L"Kết thúc (HH:MM, 24:00 được phép)", L"Nghỉ giữa hai suất (phút)"};
        const wstring values[] = {s2ws(scheduleStartTime), s2ws(scheduleEndTime), s2ws(scheduleBreakMinutes)};
        for (int i = 0; i < 3; ++i) {
            const float y = popY + 62.f + i * 68.f;
            Text label(font, labels[i], 12); label.setFillColor(Color(71, 85, 105)); label.setPosition({popX + 30.f, y}); win.draw(label);
            drawRoundedCard(win, {popX + 30.f, y + 20.f}, {460.f, 38.f}, 6.f, Color(248, 250, 252), 1.f, scheduleActiveField == i ? Color(2, 132, 199) : Color(203, 213, 225));
            Text value(font, values[i], 13); value.setFillColor(Color(15, 23, 42)); value.setPosition({popX + 42.f, y + 29.f}); win.draw(value);
        }
        drawRoundedCard(win, {popX + 40.f, popY + 270.f}, {200.f, 40.f}, 7.f, Color(2, 132, 199));
        Text save(boldFont, L"Tạo lịch", 13); save.setFillColor(Color::White); save.setPosition({popX + 110.f, popY + 281.f}); win.draw(save);
        drawRoundedCard(win, {popX + 280.f, popY + 270.f}, {200.f, 40.f}, 7.f, Color(241, 245, 249), 1.f, Color(203, 213, 225));
        Text cancel(boldFont, L"Hủy", 13); cancel.setFillColor(Color(71, 85, 105)); cancel.setPosition({popX + 360.f, popY + 281.f}); win.draw(cancel);
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
