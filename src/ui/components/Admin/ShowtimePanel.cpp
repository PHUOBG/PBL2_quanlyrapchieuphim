#include "UI/components/Admin/ShowtimePanel.h"
#include "UI/components/Admin/FontManager.h"
#include "UI/components/Admin/UITheme.h"
#include "UI/components/Admin/RoundedRectRenderer.h"
#include "data-structures/HashTable.h"
#include "utils/FileUtils.h"
#include "services/ShowtimeCleanupService.h"
#include "repositories/admin/AdminTicketRepository.h"
#include "repositories/booking/ShowtimeSeatRepository.h"
#include <unordered_map>
#include <unordered_set>
#include <ctime>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iomanip>

static wstring s2ws(const string& s) {
    return UITheme::utf8ToWstr(s);
}

static string ws2s(const wstring& ws) {
    return UITheme::wstrToUtf8(ws);
}

static string calcEndTimeStr(const string& startTime, int durationMin = 125) {
    int hh = 14, mm = 0;
    size_t colon = startTime.find(':');
    if (colon != string::npos) {
        try {
            hh = stoi(startTime.substr(0, colon));
            mm = stoi(startTime.substr(colon + 1));
        } catch (...) {}
    } else if (!startTime.empty()) {
        try { hh = stoi(startTime); } catch (...) {}
    }
    int totalMin = (hh * 60 + mm + durationMin) % (24 * 60);
    if (totalMin < 0) totalMin += 24 * 60;
    int endH = totalMin / 60;
    int endM = totalMin % 60;
    ostringstream oss;
    oss << setfill('0') << setw(2) << endH << ":" << setfill('0') << setw(2) << endM;
    return oss.str();
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
    return res + " Đ";
}

namespace { string todayIso(); }

ShowtimePanel::ShowtimePanel(Font& fontRef, float w, float h)
    : font(fontRef),
      boldFont(FontManager::instance().bold()),
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

    if (FileUtils::safeLoadFromFile(reloadTex, "assets/elements/reload.png")) {
        reloadTex.setSmooth(true);
        hasReloadTex = true;
    }

    loadShowtimesFromFile();
    selectedDate = todayIso();
    applyFilter();
}

void ShowtimePanel::setPosition(Vector2f pos) {
    position = pos;
    width = 1728.f - pos.x;
}

void ShowtimePanel::initSampleShowtimes() {
    // No sample/demo data: the list always reflects data/showtimes.txt.
    allShowtimes.clear();
}

namespace {
// Lookup caches rebuilt on every (re)load so the panel always reflects the data files.
struct ShowtimeLookups {
    unordered_map<string, int> movieDuration;      // movie_id -> minutes
    unordered_map<string, string> movieIdByTitle;  // normalized title -> movie_id
    unordered_map<string, wstring> movieTitleById;
    unordered_map<string, string> movieStart;      // movie_id -> dd/mm/yyyy
    unordered_map<string, string> movieEnd;        // movie_id -> dd/mm/yyyy
    unordered_map<string, wstring> roomNameById;
    unordered_map<string, int> roomSeats;
    unordered_set<string> ticketedShowtimes;       // showtimes that already have tickets
};
ShowtimeLookups g_lookups;

string normTitle(const wstring& w) {
    string s = ws2s(w);
    string out;
    for (unsigned char ch : s) if (ch != ' ') out += (char)tolower(ch);
    return out;
}

string stripBom(string s) {
    if (s.size() >= 3 && (unsigned char)s[0] == 0xEF && (unsigned char)s[1] == 0xBB && (unsigned char)s[2] == 0xBF) s.erase(0, 3);
    if (!s.empty() && s.back() == '\r') s.pop_back();
    return s;
}

int timeToMin(const string& t) {
    if (t.size() != 5 || t[2] != ':') return -1;
    if (!isdigit((unsigned char)t[0]) || !isdigit((unsigned char)t[1]) || !isdigit((unsigned char)t[3]) || !isdigit((unsigned char)t[4])) return -1;
    int h = stoi(t.substr(0, 2)), m = stoi(t.substr(3, 2));
    if (h > 23 || m > 59) return -1;
    return h * 60 + m;
}

string todayIso() {
    time_t now = time(nullptr);
    tm ltm{};
#if defined(_WIN32)
    localtime_s(&ltm, &now);
#else
    localtime_r(&now, &ltm);
#endif
    char buf[16];
    strftime(buf, sizeof(buf), "%Y-%m-%d", &ltm);
    return buf;
}

string nowHm() {
    time_t now = time(nullptr);
    tm ltm{};
#if defined(_WIN32)
    localtime_s(&ltm, &now);
#else
    localtime_r(&now, &ltm);
#endif
    char buf[8];
    strftime(buf, sizeof(buf), "%H:%M", &ltm);
    return buf;
}

// dd/mm/yyyy -> yyyy-mm-dd (empty on bad input)
string ddmmyyyyToIso(const string& d) {
    if (d.size() != 10 || d[2] != '/' || d[5] != '/') return "";
    return d.substr(6, 4) + "-" + d.substr(3, 2) + "-" + d.substr(0, 2);
}
} // namespace

void ShowtimePanel::loadShowtimesFromFile() {
    g_lookups = ShowtimeLookups();

    // movies.txt: id|title|age|country|lang|genres|duration|release|end|...
    {
        ifstream mf(FileUtils::resolveDataPath("data/movies.txt"), ios::binary);
        string mline;
        bool header = true;
        while (mf.is_open() && getline(mf, mline)) {
            mline = stripBom(mline);
            if (mline.empty()) continue;
            if (header) { header = false; continue; }
            vector<string> col;
            { stringstream mss(mline); string tok; while (getline(mss, tok, '|')) col.push_back(tok); }
            if (col.size() < 9 || col[0].empty()) continue;
            int d = 120;
            try { d = stoi(col[6]); } catch (...) {}
            if (d <= 0) d = 120;
            g_lookups.movieDuration[col[0]] = d;
            g_lookups.movieIdByTitle[normTitle(s2ws(col[1]))] = col[0];
            g_lookups.movieTitleById[col[0]] = s2ws(col[1]);
            g_lookups.movieStart[col[0]] = col[7];
            g_lookups.movieEnd[col[0]] = col[8];
        }
    }

    // rooms.txt: id|name|rows|cols|total
    {
        ifstream rf(FileUtils::resolveDataPath("data/rooms.txt"), ios::binary);
        string rline;
        bool header = true;
        while (rf.is_open() && getline(rf, rline)) {
            rline = stripBom(rline);
            if (rline.empty()) continue;
            if (header) { header = false; continue; }
            vector<string> col;
            { stringstream rss(rline); string tok; while (getline(rss, tok, '|')) col.push_back(tok); }
            if (col.size() < 5 || col[0].empty()) continue;
            g_lookups.roomNameById[col[0]] = s2ws(col[1]);
            int seats = 0;
            try { seats = stoi(col[4]); } catch (...) {}
            g_lookups.roomSeats[col[0]] = seats;
        }
    }

    // Tickets that reference a showtime
    {
        AdminTicketRepository ticketRepo(FileUtils::resolveDataPath("data/tickets.txt"));
        DLL<Ticket> tickets = ticketRepo.loadAll();
        for (int i = 0; i < tickets.getSize(); ++i) g_lookups.ticketedShowtimes.insert(tickets[i].showtimeId);
    }

    // Real booked-seat counts
    ShowtimeSeatRepository seatRepo(FileUtils::resolveDataPath("data/RoomStatusAtShowtime.txt"));

    vector<ShowtimeItem> loaded;
    ifstream f(FileUtils::resolveDataPath("data/showtimes.txt"), ios::binary);
    string line;
    bool isHeader = true;
    while (f.is_open() && getline(f, line)) {
        line = stripBom(line);
        if (line.empty()) continue;
        if (isHeader) { isHeader = false; continue; }
        vector<string> col;
        { stringstream ss(line); string tok; while (getline(ss, tok, '|')) col.push_back(tok); }
        if (col.size() < 6 || col[0].empty()) continue;   // malformed record

        ShowtimeItem item;
        item.id = col[0];
        item.movieId = col[1];
        item.roomId = col[2];
        item.date = col[3];
        item.startTime = col[4];
        try { item.price = stoi(col[5]); } catch (...) { continue; }

        auto rn = g_lookups.roomNameById.find(item.roomId);
        item.roomName = rn != g_lookups.roomNameById.end() ? rn->second : s2ws(item.roomId);
        auto rs = g_lookups.roomSeats.find(item.roomId);
        item.totalSeats = rs != g_lookups.roomSeats.end() && rs->second > 0 ? rs->second : 81;
        item.bookedSeats = seatRepo.getBookedSeats(item.id, item.roomId).getSize();

        {
            auto tt = g_lookups.movieTitleById.find(item.movieId);
            item.movieTitle = tt != g_lookups.movieTitleById.end() ? tt->second : s2ws(item.movieId);
        }
        auto md = g_lookups.movieDuration.find(item.movieId);
        item.endTime = calcEndTimeStr(item.startTime, md != g_lookups.movieDuration.end() ? md->second : 120);
        loaded.push_back(item);
    }
    allShowtimes = loaded;   // exactly what is on disk (no sample fallback)
}

bool ShowtimePanel::saveShowtimesToFile() {
    string out = "showtime_id|movie_id|room_id|date|time|price\n";
    for (const auto& item : allShowtimes) {
        out += item.id + "|" + item.movieId + "|" + item.roomId + "|" + item.date + "|" + item.startTime + "|" + to_string(item.price) + "\n";
    }
    return FileUtils::writeAtomic(FileUtils::resolveDataPath("data/showtimes.txt"), out);
}
void ShowtimePanel::applyFilter() {
    filteredShowtimes.clear();
    for (const auto& itm : allShowtimes) {
        // Lọc phòng
        if (selectedRoom != L"T\u1ea5t c\u1ea3 ph\u00f2ng" && itm.roomName != selectedRoom) {
            continue;
        }
        // Lọc trạng thái
        if (selectedStatus == L"\u0110\u00e3 h\u1ebft v\u00e9" && itm.bookedSeats < itm.totalSeats) {
            continue;
        }
        if (selectedStatus == L"\u0110ang m\u1edf b\u00e1n" && itm.bookedSeats >= itm.totalSeats) {
            continue;
        }
        if (selectedStatus == L"S\u1eafp chi\u1ebfu" && !(itm.date > todayIso() || (itm.date == todayIso() && itm.startTime > nowHm()))) {
            continue;
        }
        // Tìm kiếm theo từ khóa (Tên phim, Phòng, Giờ chiếu, Mã suất) hỗ trợ tiếng Việt không dấu
        if (!searchQuery.empty()) {
            bool match = UITheme::matchesViQuery(itm.movieTitle, searchQuery) ||
                         UITheme::matchesViQuery(itm.roomName, searchQuery) ||
                         UITheme::matchesViQuery(s2ws(itm.startTime), searchQuery) ||
                         UITheme::matchesViQuery(s2ws(itm.id), searchQuery);
            if (!match) continue;
        }
        filteredShowtimes.push_back(itm);
    }

    totalPages = max(1, (int)((filteredShowtimes.size() + pageSize - 1) / pageSize));
    if (currentPage > totalPages) currentPage = totalPages;
    if (currentPage < 1) currentPage = 1;
}

void ShowtimePanel::autoScheduleShowtimes() {
    // Delegates to the real scheduler: removes expired showtimes (keeping those that have tickets),
    // then generates new non-overlapping ones (movie duration + 15 min cleaning gap) inside each
    // movie's screening period. Existing bookings are never touched.
    const string path = FileUtils::resolveDataPath("data/showtimes.txt");
    ShowtimeCleanupService::maintainShowtimes(path, 6);
    loadShowtimesFromFile();
    currentPage = 1;
    applyFilter();
    notificationMessage = "Đã xếp lịch tự động (giữ nguyên các suất đã có vé)!";
    notificationClock.restart();
    notificationVisible = true;
}
void ShowtimePanel::drawRoundedCard(RenderWindow& win, Vector2f pos, Vector2f sz, float r, Color fill, float outline, Color outCol) {
    RoundedRectRenderer::draw(win, pos, sz, r, fill, outline, outCol);
}

void ShowtimePanel::performDeleteShowtime(const string& delId) {
    allShowtimes.erase(
        remove_if(allShowtimes.begin(), allShowtimes.end(),
                  [&](const ShowtimeItem& s) { return s.id == delId; }),
        allShowtimes.end()
    );
    if (!saveShowtimesToFile()) {
        loadShowtimesFromFile();
        applyFilter();
        notificationMessage = "Không thể lưu thay đổi (lỗi ghi file)!";
        notificationClock.restart();
        notificationVisible = true;
        return;
    }
    applyFilter();
    notificationMessage = "Đã xóa suất chiếu thành công!";
    notificationClock.restart();
    notificationVisible = true;
}

void ShowtimePanel::handleEvent(const Event& event, const RenderWindow& win) {
    if (deleteModal.isOpen) {
        int res = deleteModal.handleEvent(event, win);
        if (res == 2) {
            performDeleteShowtime(deleteModal.targetId);
        }
        return;
    }

    // 1. ƯU TIÊN XỬ LÝ POPUP THÊM / SỬA SUẤT CHIẾU (MODAL BLOCKING)
    if (showAddPopup) {
        float popW = 450.f;
        float popH = 300.f;
        float popX = position.x + (width - popW) / 2.f;
        float popY = position.y + (height - popH) / 2.f - 30.f;

        auto commitSavePopup = [&]() {
            auto fail = [&](const string& msg) {
                notificationMessage = msg;
                notificationClock.restart();
                notificationVisible = true;
            };

            // 1. Validate inputs
            auto mit = g_lookups.movieIdByTitle.find(normTitle(popupMovieTitle));
            if (mit == g_lookups.movieIdByTitle.end()) { fail("Không tìm thấy phim này trong danh sách phim!"); return; }
            const string movieId = mit->second;
            const int dur = g_lookups.movieDuration.count(movieId) ? g_lookups.movieDuration[movieId] : 120;

            const int startMin = timeToMin(popupStartTime);
            if (startMin < 0) { fail("Giờ chiếu không hợp lệ (HH:MM)!"); return; }

            int parsedPrice = 0;
            try { parsedPrice = popupPrice.empty() ? 0 : stoi(popupPrice); } catch (...) { parsedPrice = 0; }
            if (parsedPrice <= 0) { fail("Giá vé phải lớn hơn 0!"); return; }

            string rId;
            for (const auto& kv : g_lookups.roomNameById) if (kv.second == popupRoomName) { rId = kv.first; break; }
            if (rId.empty()) { fail("Phòng chiếu không hợp lệ!"); return; }

            const bool isEdit = editingIndex >= 0 && editingIndex < (int)filteredShowtimes.size();
            const string targetId = isEdit ? filteredShowtimes[editingIndex].id : "";
            const string showDate = isEdit ? filteredShowtimes[editingIndex].date : selectedDate;

            if (isEdit && (g_lookups.ticketedShowtimes.count(targetId) || filteredShowtimes[editingIndex].bookedSeats > 0)) {
                fail("Suất chiếu đã có vé, không thể sửa!"); return;
            }

            // 2. Must be inside the movie's screening period
            string from = ddmmyyyyToIso(g_lookups.movieStart[movieId]);
            string to = ddmmyyyyToIso(g_lookups.movieEnd[movieId]);
            if ((!from.empty() && showDate < from) || (!to.empty() && showDate > to)) {
                fail("Ngày chiếu nằm ngoài thời gian công chiếu của phim!"); return;
            }

            // 3. Room must not host overlapping screenings (duration + 15 min cleaning gap)
            const int kGap = 15;
            const int newEnd = startMin + dur + kGap;
            for (const auto& other : allShowtimes) {
                if (other.id == targetId || other.roomId != rId || other.date != showDate) continue;
                int oStart = timeToMin(other.startTime);
                if (oStart < 0) continue;
                int oDur = g_lookups.movieDuration.count(other.movieId) ? g_lookups.movieDuration[other.movieId] : 120;
                int oEnd = oStart + oDur + kGap;
                if (startMin < oEnd && oStart < newEnd) {
                    fail("Trùng lịch với suất " + other.id + " trong cùng phòng!"); return;
                }
            }

            // 4. Apply
            if (isEdit) {
                for (auto& st : allShowtimes) {
                    if (st.id == targetId) {
                        st.movieId = movieId;
                        st.roomId = rId;
                        st.startTime = popupStartTime;
                        st.price = parsedPrice;
                        break;
                    }
                }
            } else {
                // Unique ID: SUATCHIEU_YYYYMMDD_NNNN (next free counter)
                string ymd;
                for (char ch : showDate) if (ch != '-') ymd += ch;
                int maxN = 0;
                const string prefix = "SUATCHIEU_" + ymd + "_";
                for (const auto& s : allShowtimes) {
                    if (s.id.rfind(prefix, 0) == 0) { try { maxN = max(maxN, stoi(s.id.substr(prefix.size()))); } catch (...) {} }
                }
                char idbuf[64];
                snprintf(idbuf, sizeof(idbuf), "%s%04d", prefix.c_str(), maxN + 1);
                ShowtimeItem ni;
                ni.id = idbuf;
                ni.movieId = movieId;
                ni.roomId = rId;
                ni.date = showDate;
                ni.startTime = popupStartTime;
                ni.price = parsedPrice;
                allShowtimes.insert(allShowtimes.begin(), ni);
            }
            if (!saveShowtimesToFile()) {
                fail("Không thể lưu suất chiếu (lỗi ghi file)!");
                loadShowtimesFromFile();
                applyFilter();
                return;
            }
            loadShowtimesFromFile();   // refresh derived fields (title, end time, seat counts)
            currentPage = 1;
            applyFilter();
            showAddPopup = false;
            notificationMessage = isEdit ? "Đã cập nhật suất chiếu thành công!" : "Đã thêm suất chiếu mới thành công!";
            notificationClock.restart();
            notificationVisible = true;
        };
        if (const auto* mouseBtn = event.getIf<Event::MouseButtonPressed>()) {
            if (mouseBtn->button == Mouse::Button::Left) {
                Vector2f mousePos = win.mapPixelToCoords(Mouse::getPosition(win));

                FloatRect boxTitle({popX + 30.f, popY + 80.f}, {popW - 60.f, 36.f});
                FloatRect boxTime({popX + 30.f, popY + 145.f}, {180.f, 36.f});
                FloatRect boxPrice({popX + 230.f, popY + 145.f}, {190.f, 36.f});
                FloatRect btnRoomCycle({popX + popW - 168.f, popY + 20.f}, {138.f, 28.f});
                FloatRect btnPopSave({popX + 60.f, popY + 230.f}, {150.f, 38.f});
                FloatRect btnPopCancel({popX + 240.f, popY + 230.f}, {150.f, 38.f});

                if (boxTitle.contains(mousePos)) {
                    popupActiveField = 0;
                    if (editingIndex < 0 && popupMovieTitle == L"Phim Chi\u1ebfu M\u1edbi") popupMovieTitle.clear();
                    return;
                }
                if (boxTime.contains(mousePos)) {
                    popupActiveField = 1;
                    return;
                }
                if (boxPrice.contains(mousePos)) {
                    popupActiveField = 2;
                    return;
                }
                if (btnRoomCycle.contains(mousePos)) {
                    // Đổi phòng chiếu khi click vào badge phòng ở góc trên popup
                    int curR = 1;
                    for (size_t i = 1; i < roomList.size(); ++i) {
                        if (roomList[i] == popupRoomName) { curR = (int)i; break; }
                    }
                    int nextR = (curR % ((int)roomList.size() - 1)) + 1;
                    popupRoomName = roomList[nextR];
                    return;
                }
                if (btnPopSave.contains(mousePos)) {
                    commitSavePopup();
                    return;
                }
                if (btnPopCancel.contains(mousePos)) {
                    showAddPopup = false;
                    return;
                }
                // Click ngoài popup thì đóng
                if (!FloatRect({popX, popY}, {popW, popH}).contains(mousePos)) {
                    showAddPopup = false;
                    return;
                }
            }
            return;
        }

        if (const auto* keyEv = event.getIf<Event::KeyPressed>()) {
            if (keyEv->code == Keyboard::Key::Escape) {
                showAddPopup = false;
                return;
            }
            if (keyEv->code == Keyboard::Key::Tab) {
                popupActiveField = (popupActiveField + 1) % 3;
                return;
            }
            if (keyEv->code == Keyboard::Key::Enter) {
                commitSavePopup();
                return;
            }
            if (keyEv->code == Keyboard::Key::Backspace) {
                if (popupActiveField == 0 && !popupMovieTitle.empty()) popupMovieTitle.pop_back();
                else if (popupActiveField == 1 && !popupStartTime.empty()) popupStartTime.pop_back();
                else if (popupActiveField == 2 && !popupPrice.empty()) popupPrice.pop_back();
                return;
            }
        }

        if (const auto* txtEv = event.getIf<Event::TextEntered>()) {
            char32_t u = txtEv->unicode;
            if (u >= 32 && u != 127) {
                if (popupActiveField == 0) {
                    if (popupMovieTitle.size() < 45) popupMovieTitle += static_cast<wchar_t>(u);
                } else if (popupActiveField == 1) {
                    if (((u >= '0' && u <= '9') || u == ':') && popupStartTime.size() < 5)
                        popupStartTime += static_cast<char>(u);
                } else if (popupActiveField == 2) {
                    if (u >= '0' && u <= '9' && popupPrice.size() < 8)
                        popupPrice += static_cast<char>(u);
                }
            }
            return;
        }
        return;
    }

    // 2. XỬ LÝ BÀN PHÍM CHO Ô TÌM KIẾM SUẤT CHIẾU
    if (searchFocused) {
        if (const auto* keyEv = event.getIf<Event::KeyPressed>()) {
            if (keyEv->code == Keyboard::Key::Escape) {
                searchQuery.clear();
                searchFocused = false;
                currentPage = 1;
                applyFilter();
                return;
            }
            if (keyEv->code == Keyboard::Key::Backspace) {
                if (!searchQuery.empty()) {
                    searchQuery.pop_back();
                    currentPage = 1;
                    applyFilter();
                }
                return;
            }
        }
        if (const auto* txtEv = event.getIf<Event::TextEntered>()) {
            char32_t u = txtEv->unicode;
            if (u >= 32 && u != 127 && searchQuery.size() < 35) {
                searchQuery += static_cast<wchar_t>(u);
                currentPage = 1;
                applyFilter();
            }
            return;
        }
    }

    // 3. XỬ LÝ CLICK CHUỘT TRÊN MÀN HÌNH CHÍNH
    if (const auto* mouseBtn = event.getIf<Event::MouseButtonPressed>()) {
        if (mouseBtn->button == Mouse::Button::Left) {
            Vector2f mousePos = win.mapPixelToCoords(Mouse::getPosition(win));
            float fCardY = position.y + 114.f;

            // Ưu tiên kiểm tra danh sách Dropdown đang mở trước khi kiểm tra bảng bên dưới
            if (roomDropdownOpen) {
                float ddY = fCardY + 76.f;
                for (size_t i = 0; i < roomList.size(); ++i) {
                    FloatRect itmB({position.x + 310.f, ddY + i * 34.f}, {230.f, 34.f});
                    if (itmB.contains(mousePos)) {
                        selectedRoom = roomList[i];
                        roomDropdownOpen = false;
                        currentPage = 1;
                        applyFilter();
                        return;
                    }
                }
                roomDropdownOpen = false;
                return;
            }

            if (statusDropdownOpen) {
                float ddY = fCardY + 144.f;
                for (size_t i = 0; i < statusList.size(); ++i) {
                    FloatRect itmB({position.x + 50.f, ddY + i * 34.f}, {230.f, 34.f});
                    if (itmB.contains(mousePos)) {
                        selectedStatus = statusList[i];
                        statusDropdownOpen = false;
                        currentPage = 1;
                        applyFilter();
                        return;
                    }
                }
                statusDropdownOpen = false;
                return;
            }

            // Ô tìm kiếm nhanh trong thẻ Filter
            FloatRect searchBoxRect({position.x + 310.f, fCardY + 108.f}, {230.f, 36.f});
            if (searchBoxRect.contains(mousePos)) {
                if (!searchQuery.empty() && mousePos.x >= position.x + 512.f) {
                    searchQuery.clear();
                    currentPage = 1;
                    applyFilter();
                }
                searchFocused = true;
                return;
            } else {
                searchFocused = false;
            }

            // Nút Header: Làm mới, Xếp lịch tự động, + Thêm suất chiếu mới
            float btnY = position.y + 39.f;
            FloatRect btnRef({position.x + width - 488.f, btnY}, {110.f, 38.f});
            FloatRect btnAuto({position.x + width - 368.f, btnY}, {170.f, 38.f});
            FloatRect btnAdd({position.x + width - 188.f, btnY}, {140.f, 38.f});

            if (btnRef.contains(mousePos)) {
                selectedRoom = L"T\u1ea5t c\u1ea3 ph\u00f2ng";
                selectedStatus = L"T\u1ea5t c\u1ea3 tr\u1ea1ng th\u00e1i";
                searchQuery.clear();
                loadShowtimesFromFile();
                applyFilter();
                notificationMessage = "Đã làm mới dữ liệu suất chiếu!";
                notificationClock.restart();
                notificationVisible = true;
                return;
            }

            // Nút Làm mới (Dấu xoay vòng cạnh KPI Summary)
            FloatRect btnReloadKpi({position.x + width - 85.f, fCardY + 51.f}, {48.f, 48.f});
            if (btnReloadKpi.contains(mousePos)) {
                selectedRoom = L"T\u1ea5t c\u1ea3 ph\u00f2ng";
                selectedStatus = L"T\u1ea5t c\u1ea3 tr\u1ea1ng th\u00e1i";
                searchQuery.clear();
                loadShowtimesFromFile();
                applyFilter();
                notificationMessage = "Đã làm mới danh sách suất chiếu!";
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
                popupActiveField = 0;
                popupMovieTitle = L"";
                popupRoomName = (selectedRoom != L"T\u1ea5t c\u1ea3 ph\u00f2ng") ? selectedRoom : L"Ph\u00f2ng Chi\u1ebfu 1";
                popupStartTime = "14:00";
                popupPrice = "70000";
                return;
            }

            // Dropdown Phòng chiếu (khớp tọa độ render: fCardY + 38.f, rộng 230.f)
            FloatRect ddRoomB({position.x + 310.f, fCardY + 38.f}, {230.f, 38.f});
            if (ddRoomB.contains(mousePos)) {
                roomDropdownOpen = !roomDropdownOpen;
                statusDropdownOpen = false;
                return;
            }

            // Dropdown Trạng thái (khớp tọa độ render: fCardY + 108.f, rộng 230.f)
            FloatRect ddStatB({position.x + 50.f, fCardY + 108.f}, {230.f, 36.f});
            if (ddStatB.contains(mousePos)) {
                statusDropdownOpen = !statusDropdownOpen;
                roomDropdownOpen = false;
                return;
            }

            // Thao tác trên từng dòng bảng (Sửa, Xóa)
            float tableY = position.y + 286.f;
            float rowH = 46.f;
            int startIdx = (currentPage - 1) * pageSize;
            int count = max(0, min((int)filteredShowtimes.size() - startIdx, pageSize));

            for (int r = 0; r < count; ++r) {
                float rY = tableY + 44.f + r * rowH;
                float tblX = position.x + 30.f;
                float tblW = width - 60.f;
                float div5 = tblX + tblW * 0.87f;
                float actX = div5 + ((tblX + tblW - div5) - 78.f) / 2.f;

                FloatRect btnEdit({actX, rY + 8.f}, {36.f, 26.f});
                FloatRect btnDel({actX + 42.f, rY + 8.f}, {36.f, 26.f});

                if (btnEdit.contains(mousePos)) {
                    int realIdx = startIdx + r;
                    editingIndex = realIdx;
                    popupActiveField = 0;
                    popupMovieTitle = filteredShowtimes[realIdx].movieTitle;
                    popupRoomName = filteredShowtimes[realIdx].roomName;
                    popupStartTime = filteredShowtimes[realIdx].startTime;
                    popupPrice = to_string(filteredShowtimes[realIdx].price);
                    showAddPopup = true;
                    return;
                }

                if (btnDel.contains(mousePos)) {
                    int realIdx = startIdx + r;
                    const auto& item = filteredShowtimes[realIdx];
                    if (g_lookups.ticketedShowtimes.count(item.id) || item.bookedSeats > 0) {
                        notificationMessage = "Suất chiếu đã có vé, không thể xóa!";
                        notificationClock.restart();
                        notificationVisible = true;
                        return;
                    }
                    deleteModal.open(
                        L"X\u00c1C NH\u1eacN X\u00d3A SU\u1ea4T CHI\u1ebeU",
                        L"B\u1ea1n c\u00f3 ch\u1eafc ch\u1eafn mu\u1ed1n x\u00f3a su\u1ea5t chi\u1ebfu \"" + item.movieTitle + L" (" + s2ws(item.startTime) + L")\" kh\u00f4ng?\nThao t\u00e1c n\u00e0y kh\u00f4ng th\u1ec3 ho\u00e0n t\u00e1c.",
                        item.id
                    );
                    return;
                }
            }

            // Phân trang
            float pagY = position.y + 800.f;
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
        }
    }
}

void ShowtimePanel::update(Vector2f mousePos, bool) {
    deleteModal.update(mousePos);
    hoveredRow = -1;
    if (deleteModal.isOpen) return;
    float tableY = position.y + 286.f;
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
    // ==========================================
    // 3. PAGE HEADER CARD (Unified RoomPanel style)
    // ==========================================
    UITheme::drawSectionHeaderCard(win, position.x + 30.f, position.y + 16.f, width - 60.f,
                                   L"Qu\u1ea3n l\u00fd su\u1ea5t chi\u1ebfu",
                                   L"L\u1ecbch bi\u1ec3u ho\u1ea1t \u0111\u1ed9ng",
                                   Color(224, 242, 254), Color(8, 125, 181),
                                   L"\u0110i\u1ec1u ph\u1ed1i l\u1ecbch chi\u1ebfu theo ph\u00f2ng, theo d\u00f5i t\u1ef7 l\u1ec7 l\u1ea5p \u0111\u1ea7y gh\u1ebf v\u00e0 ph\u00e2n b\u1ed1 khung gi\u1edd chi\u1ebfu.",
                                   boldFont, font, false);

    // 3 Nút Header
    float btnY = position.y + 39.f;
    float headRefX = position.x + width - 488.f;
    drawRoundedCard(win, {headRefX, btnY}, {110.f, 38.f}, 8.f, Color::White, 1.2f, Color(203, 213, 225));
    UITheme::drawReloadIcon(win, {headRefX + 22.f, btnY + 19.f}, 16.f, Color(51, 65, 85));
    Text refTxt(boldFont, L"L\u00e0m m\u1edbi", 14);
    refTxt.setFillColor(Color(51, 65, 85));
    refTxt.setPosition({headRefX + 38.f, btnY + 9.f});
    win.draw(refTxt);

    drawRoundedCard(win, {position.x + width - 368.f, btnY}, {170.f, 38.f}, 8.f, Color::White, 1.2f, Color(203, 213, 225));
    Text autoTxt(boldFont, L"X\u1ebfp l\u1ecbch t\u1ef1 \u0111\u1ed9ng", 14);
    autoTxt.setFillColor(Color(51, 65, 85));
    FloatRect autob = autoTxt.getLocalBounds();
    autoTxt.setPosition({
        position.x + width - 368.f + (170.f - autob.size.x) / 2.f - autob.position.x,
        btnY + (38.f - autob.size.y) / 2.f - autob.position.y
    });
    win.draw(autoTxt);

    drawRoundedCard(win, {position.x + width - 188.f, btnY}, {140.f, 38.f}, 8.f, Color(8, 125, 181));
    Text addTxt(boldFont, L"+ Th\u00eam su\u1ea5t m\u1edbi", 14);
    addTxt.setFillColor(Color::White);
    FloatRect addb = addTxt.getLocalBounds();
    addTxt.setPosition({
        position.x + width - 188.f + (140.f - addb.size.x) / 2.f - addb.position.x,
        btnY + (38.f - addb.size.y) / 2.f - addb.position.y
    });
    win.draw(addTxt);

    // ==========================================
    // 4. FILTER CARD & KPI SUMMARY
    // ==========================================
    float fCardY = position.y + 114.f;
    drawRoundedCard(win, {position.x + 30.f, fCardY}, {width - 60.f, 166.f}, 12.f, Color::White, 1.f, Color(226, 232, 240));

    // Ngày chiếu
    Text lblDate(boldFont, L"Ng\u00e0y chi\u1ebfu", 14); lblDate.setFillColor(Color(71, 85, 105)); lblDate.setPosition({position.x + 50.f, fCardY + 16.f}); win.draw(lblDate);
    drawRoundedCard(win, {position.x + 50.f, fCardY + 38.f}, {230.f, 38.f}, 8.f, Color(248, 250, 252), 1.f, Color(203, 213, 225));
    Text valDate(font, L"10/27/2026", 14); valDate.setFillColor(Color(23, 32, 51)); valDate.setPosition({position.x + 62.f, fCardY + 47.f}); win.draw(valDate);

    // Phòng chiếu
    Text lblRoom(boldFont, L"Ph\u00f2ng chi\u1ebfu", 14); lblRoom.setFillColor(Color(71, 85, 105)); lblRoom.setPosition({position.x + 310.f, fCardY + 16.f}); win.draw(lblRoom);
    drawRoundedCard(win, {position.x + 310.f, fCardY + 38.f}, {230.f, 38.f}, 8.f, Color(248, 250, 252), 1.f, Color(203, 213, 225));
    Text valRoom(font, selectedRoom, 14); valRoom.setFillColor(Color(23, 32, 51)); valRoom.setPosition({position.x + 322.f, fCardY + 47.f}); win.draw(valRoom);
    Text arrRoom(boldFont, L"\u25bc", 14); arrRoom.setFillColor(Color(100, 116, 139)); arrRoom.setPosition({position.x + 515.f, fCardY + 50.f}); win.draw(arrRoom);

    // Trạng thái bán vé
    Text lblStat(boldFont, L"Tr\u1ea1ng th\u00e1i b\u00e1n v\u00e9", 14); lblStat.setFillColor(Color(71, 85, 105)); lblStat.setPosition({position.x + 50.f, fCardY + 88.f}); win.draw(lblStat);
    drawRoundedCard(win, {position.x + 50.f, fCardY + 108.f}, {230.f, 36.f}, 8.f, Color(248, 250, 252), 1.f, Color(203, 213, 225));
    Text valStat(font, selectedStatus, 14); valStat.setFillColor(Color(23, 32, 51)); valStat.setPosition({position.x + 62.f, fCardY + 116.f}); win.draw(valStat);
    Text arrStat(boldFont, L"\u25bc", 14); arrStat.setFillColor(Color(100, 116, 139)); arrStat.setPosition({position.x + 250.f, fCardY + 119.f}); win.draw(arrStat);

    // Ô tìm kiếm nhanh suất chiếu (Tên phim, Phòng, Giờ)
    Text lblSearch(boldFont, L"T\u00ecm ki\u1ebfm su\u1ea5t chi\u1ebfu", 14); lblSearch.setFillColor(Color(71, 85, 105)); lblSearch.setPosition({position.x + 310.f, fCardY + 88.f}); win.draw(lblSearch);
    drawRoundedCard(win, {position.x + 310.f, fCardY + 108.f}, {230.f, 36.f}, 8.f,
                    searchFocused ? Color::White : Color(248, 250, 252),
                    searchFocused ? 1.8f : 1.f,
                    searchFocused ? Color(8, 125, 181) : Color(203, 213, 225));
    wstring dispSearch = searchQuery.empty()
        ? (searchFocused ? L"|" : L"T\u00ecm phim, ph\u00f2ng, gi\u1edd...")
        : (searchQuery + (searchFocused ? L"|" : L""));
    Text valSearch(font, dispSearch, 14);
    valSearch.setFillColor(searchQuery.empty() && !searchFocused ? Color(148, 163, 184) : Color(23, 32, 51));
    valSearch.setPosition({position.x + 322.f, fCardY + 117.f});
    win.draw(valSearch);
    if (!searchQuery.empty()) {
        UITheme::drawCloseIcon(win, {position.x + 522.f, fCardY + 126.f}, 10.f, Color(148, 163, 184), 1.8f);
    }

    // Thẻ KPI Summary: Tách làm 2 ô màu khác nhau bên trong 1 ô card bao ngoài
    float kpiOuterW = 365.f;
    float kpiOuterH = 94.f;
    float kpiOuterX = position.x + width - 470.f;
    float kpiOuterY = fCardY + 28.f;

    // 1. Ô bao ngoài (Khung card chính)
    drawRoundedCard(win, {kpiOuterX, kpiOuterY}, {kpiOuterW, kpiOuterH}, 12.f, Color(248, 250, 252), 1.2f, Color(226, 232, 240));

    // 2. Ô trái (Màu 1 - Xanh dương đại dương #0284C7): Thống kê số lượng suất chiếu
    float b1X = kpiOuterX + 7.f;
    float b1Y = kpiOuterY + 7.f;
    float b1W = 136.f;
    float b1H = 80.f;
    drawRoundedCard(win, {b1X, b1Y}, {b1W, b1H}, 8.f, Color(8, 125, 181));

    Text kpiNum(boldFont, to_wstring(filteredShowtimes.size()), 24);
    kpiNum.setFillColor(Color::White);
    FloatRect nb = kpiNum.getLocalBounds();

    Text kpiUnit(boldFont, L"SU\u1ea4T\nCHI\u1ebeU", 14);
    kpiUnit.setFillColor(Color(224, 242, 254));
    FloatRect ub = kpiUnit.getLocalBounds();

    float totalW1 = nb.size.x + 14.f + ub.size.x;
    float startX1 = b1X + (b1W - totalW1) / 2.f;
    kpiNum.setPosition({std::floor(startX1 - nb.position.x), std::floor(b1Y + (b1H - nb.size.y) / 2.f - nb.position.y)});
    kpiUnit.setPosition({std::floor(startX1 + nb.size.x + 14.f - ub.position.x), std::floor(b1Y + (b1H - ub.size.y) / 2.f - ub.position.y)});
    win.draw(kpiNum);
    win.draw(kpiUnit);

    // 3. Ô phải (Màu 2 - Xám nhạt thanh lịch #E8EEF5): Thông tin ngày & phòng chiếu
    float b2X = b1X + b1W + 6.f;
    float b2Y = kpiOuterY + 7.f;
    float b2W = kpiOuterW - 14.f - b1W - 6.f;
    float b2H = 80.f;
    drawRoundedCard(win, {b2X, b2Y}, {b2W, b2H}, 8.f, Color(232, 238, 245), 1.f, Color(203, 213, 225));

    float textPadX = b2X + 12.f;

    Text kpiDateLbl(font, L"Ng\u00e0y: ", 16);
    kpiDateLbl.setFillColor(Color(100, 116, 139));
    kpiDateLbl.setPosition({std::floor(textPadX), std::floor(b2Y + 15.f)});
    win.draw(kpiDateLbl);

    Text kpiDateVal(boldFont, s2ws(selectedDate), 16);
    kpiDateVal.setFillColor(Color(23, 32, 51));
    kpiDateVal.setPosition({std::floor(textPadX + kpiDateLbl.getGlobalBounds().size.x + 4.f), std::floor(b2Y + 15.f)});
    win.draw(kpiDateVal);

    Text kpiRoomLbl(font, L"Ph\u00f2ng: ", 16);
    kpiRoomLbl.setFillColor(Color(100, 116, 139));
    kpiRoomLbl.setPosition({std::floor(textPadX), std::floor(b2Y + 45.f)});
    win.draw(kpiRoomLbl);

    Text kpiRoomVal(boldFont, selectedRoom, 16);
    kpiRoomVal.setFillColor(Color(8, 125, 181));
    kpiRoomVal.setPosition({std::floor(textPadX + kpiRoomLbl.getGlobalBounds().size.x + 4.f), std::floor(b2Y + 45.f)});
    win.draw(kpiRoomVal);

    // 4. Nút Làm mới (Dấu xoay vòng reload)
    float refBtnX = position.x + width - 85.f;
    float refBtnY = fCardY + 51.f;
    float refBtnW = 48.f;
    float refBtnH = 48.f;
    drawRoundedCard(win, {refBtnX, refBtnY}, {refBtnW, refBtnH}, 8.f, Color(8, 125, 181));
    UITheme::drawReloadIcon(win, {refBtnX + refBtnW / 2.f, refBtnY + refBtnH / 2.f}, 18.f, Color::White);

    // ==========================================
    // 5. BẢNG DỮ LIỆU SUẤT CHIẾU
    // ==========================================
    float tableX = position.x + 30.f;
    float tableY = position.y + 286.f;
    float tableW = width - 60.f;
    float tableRight = tableX + tableW;
    float rowH = 46.f;

    const float div1 = tableX + tableW * 0.14f;
    const float div2 = tableX + tableW * 0.28f;
    const float div3 = tableX + tableW * 0.54f;
    const float div4 = tableX + tableW * 0.65f;
    const float div5 = tableX + tableW * 0.87f;

    // Header bảng (#0284C7)
    drawRoundedCard(win, {tableX, tableY}, {tableW, 44.f}, 8.f, Color(8, 125, 181));

    for (float dx : {div1, div2, div3, div4, div5}) {
        RectangleShape hDiv(Vector2f(1.f, 44.f));
        hDiv.setPosition({dx, tableY});
        hDiv.setFillColor(Color(255, 255, 255, 45));
        win.draw(hDiv);
    }

    auto centerColText = [&](Text& txt, float colStart, float colEnd, float y) {
        FloatRect b = txt.getLocalBounds();
        txt.setPosition({std::round(colStart + ((colEnd - colStart) - b.size.x) / 2.f - b.position.x), y});
        win.draw(txt);
    };

    Text h1(boldFont, L"GI\u1edc CHI\u1ebeU", 14); h1.setFillColor(Color::White); centerColText(h1, tableX, div1, tableY + 14.f);
    Text h2(boldFont, L"PH\u00d2NG", 14); h2.setFillColor(Color::White); centerColText(h2, div1, div2, tableY + 14.f);
    Text h3(boldFont, L"PHIM", 14); h3.setFillColor(Color::White); centerColText(h3, div2, div3, tableY + 14.f);
    Text h4(boldFont, L"GI\u00c1 V\u00c9", 14); h4.setFillColor(Color::White); centerColText(h4, div3, div4, tableY + 14.f);
    Text h5(boldFont, L"T\u1ef6 L\u1ec6 L\u1ea4P \u0110\u1ea6Y", 14); h5.setFillColor(Color::White); centerColText(h5, div4, div5, tableY + 14.f);

    Text h6(boldFont, L"THAO T\u00c1C", 14); h6.setFillColor(Color::White);
    centerColText(h6, div5, tableRight, tableY + 14.f);

    // Các dòng dữ liệu
    int startIdx = (currentPage - 1) * pageSize;
    int count = min((int)filteredShowtimes.size() - startIdx, pageSize);

    for (int r = 0; r < count; ++r) {
        int idx = startIdx + r;
        const auto& itm = filteredShowtimes[idx];
        float rY = tableY + 44.f + r * rowH;
        bool isHov = (hoveredRow == r);

        drawRoundedCard(win, {tableX, rY}, {tableW, rowH}, 0.f, isHov ? Color(241, 245, 249) : Color::White, 0.5f, Color(241, 245, 249));

        for (float dx : {div1, div2, div3, div4, div5}) {
            RectangleShape rDiv(Vector2f(1.f, rowH));
            rDiv.setPosition({dx, rY});
            rDiv.setFillColor(Color(241, 245, 249));
            win.draw(rDiv);
        }

        // 1. Giờ chiếu (căn giữa trong cột [tableX, div1])
        Text timeTxt(boldFont, s2ws(itm.startTime + " \u2013 " + itm.endTime), 14);
        timeTxt.setFillColor(Color(23, 32, 51));
        FloatRect tb = timeTxt.getLocalBounds();
        float dotGap = 7.f;
        float dotDiameter = 7.f;
        float groupW = dotDiameter + dotGap + tb.size.x;
        float groupStartX = std::round(tableX + ((div1 - tableX) - groupW) / 2.f);
        CircleShape dotBlue(3.5f); dotBlue.setFillColor(Color(8, 125, 181)); dotBlue.setPosition({groupStartX, rY + 20.f}); win.draw(dotBlue);
        timeTxt.setPosition({groupStartX + dotDiameter + dotGap - tb.position.x, rY + 14.f}); win.draw(timeTxt);

        // 2. Phòng (căn giữa trong cột [div1, div2])
        Text roomTxt(font, itm.roomName, 14); roomTxt.setFillColor(Color(51, 65, 85)); centerColText(roomTxt, div1, div2, rY + 14.f);

        // 3. Phim
        Text movieTxt(boldFont, itm.movieTitle, 14); movieTxt.setFillColor(Color(23, 32, 51)); movieTxt.setPosition({div2 + 24.f, rY + 14.f}); win.draw(movieTxt);

        // 4. Giá vé (căn giữa trong cột [div3, div4])
        Text priceTxt(boldFont, s2ws(formatPriceStr(itm.price)), 14); priceTxt.setFillColor(Color(23, 32, 51)); centerColText(priceTxt, div3, div4, rY + 14.f);

        // 5. Tỷ lệ lấp đầy
        int pct = (int)((itm.bookedSeats * 100.f) / itm.totalSeats + 0.5f);
        Color barCol = Color(21, 128, 61); // Xanh lá
        if (pct >= 100) barCol = Color(220, 38, 38); // Đỏ hết vé
        else if (pct < 70) barCol = Color(245, 158, 11); // Cam

        wstring seatInfo = to_wstring(itm.bookedSeats) + L"/" + to_wstring(itm.totalSeats) + L" v\u00e9";
        if (pct >= 100) seatInfo = to_wstring(itm.bookedSeats) + L"/" + to_wstring(itm.totalSeats) + L" (H\u1ebft v\u00e9)";

        float barX = div4 + 24.f;
        float barW = (div5 - div4) - 48.f;
        Text seatTxt(font, seatInfo, 14); seatTxt.setFillColor(barCol); seatTxt.setPosition({barX, rY + 9.f}); win.draw(seatTxt);

        Text pctTxt(boldFont, to_wstring(pct) + L"%", 14); pctTxt.setFillColor(barCol);
        FloatRect ptb = pctTxt.getLocalBounds();
        pctTxt.setPosition({barX + barW - ptb.size.x - ptb.position.x, rY + 9.f});
        win.draw(pctTxt);

        // Progress bar
        drawRoundedCard(win, {barX, rY + 29.f}, {barW, 4.5f}, 2.25f, Color(226, 232, 240));
        float fillW = max(4.f, barW * ((float)itm.bookedSeats / itm.totalSeats));
        drawRoundedCard(win, {barX, rY + 29.f}, {fillW, 4.5f}, 2.25f, barCol);

        // 6. Thao tác: Sửa, Xóa (căn giữa trong [div5, tableRight])
        float actX = div5 + ((tableRight - div5) - 78.f) / 2.f;
        drawRoundedCard(win, {actX, rY + 9.f}, {36.f, 26.f}, 4.f, Color(241, 245, 249), 1.f, Color(203, 213, 225));
        Text btnEditTxt(boldFont, L"S\u1eeda", 14);
        btnEditTxt.setFillColor(Color(8, 125, 181));
        btnEditTxt.setPosition({std::floor(actX + 7.f), std::floor(rY + 13.f)});
        win.draw(btnEditTxt);

        drawRoundedCard(win, {actX + 42.f, rY + 9.f}, {36.f, 26.f}, 4.f, Color(254, 242, 242), 1.f, Color(254, 202, 202));
        Text btnDelTxt(boldFont, L"X\u00f3a", 14);
        btnDelTxt.setFillColor(Color(220, 38, 38));
        btnDelTxt.setPosition({std::floor(actX + 49.f), std::floor(rY + 13.f)});
        win.draw(btnDelTxt);
    }

    // ==========================================
    // 6. PHÂN TRANG (Pagination)
    // ==========================================
    float pagY = position.y + 800.f;
    wstring showInfo = L"Hi\u1ec3n th\u1ecb 1 \u2013 " + to_wstring(count) + L" tr\u00ean t\u1ed5ng s\u1ed1 " + to_wstring(filteredShowtimes.size()) + L" su\u1ea5t chi\u1ebfu";
    Text pageInfo(font, showInfo, 14); pageInfo.setFillColor(Color(100, 116, 139)); pageInfo.setPosition({position.x + 30.f, pagY + 6.f}); win.draw(pageInfo);

    // Nút Trước
    drawRoundedCard(win, {position.x + width - 260.f, pagY}, {55.f, 32.f}, 6.f, Color::White, 1.f, Color(226, 232, 240));
    Text prTxt(font, L"Tr\u01b0\u1edbc", 14); prTxt.setFillColor(Color(100, 116, 139));
    {
        FloatRect b = prTxt.getLocalBounds();
        prTxt.setPosition({position.x + width - 260.f + (55.f - b.size.x) / 2.f - b.position.x, pagY + (32.f - b.size.y) / 2.f - b.position.y});
    }
    win.draw(prTxt);

    // Nút 1
    bool p1Active = (currentPage == 1);
    drawRoundedCard(win, {position.x + width - 195.f, pagY}, {32.f, 32.f}, 6.f, p1Active ? Color(8, 125, 181) : Color::White, 1.f, Color(226, 232, 240));
    Text p1Txt(boldFont, L"1", 14); p1Txt.setFillColor(p1Active ? Color::White : Color(51, 65, 85)); p1Txt.setPosition({position.x + width - 183.f, pagY + 6.f}); win.draw(p1Txt);

    // Nút 2
    bool p2Active = (currentPage == 2);
    drawRoundedCard(win, {position.x + width - 155.f, pagY}, {32.f, 32.f}, 6.f, p2Active ? Color(8, 125, 181) : Color::White, 1.f, Color(226, 232, 240));
    Text p2Txt(boldFont, L"2", 14); p2Txt.setFillColor(p2Active ? Color::White : Color(51, 65, 85)); p2Txt.setPosition({position.x + width - 143.f, pagY + 6.f}); win.draw(p2Txt);

    // Nút 3
    bool p3Active = (currentPage == 3);
    drawRoundedCard(win, {position.x + width - 115.f, pagY}, {32.f, 32.f}, 6.f, p3Active ? Color(8, 125, 181) : Color::White, 1.f, Color(226, 232, 240));
    Text p3Txt(boldFont, L"3", 14); p3Txt.setFillColor(p3Active ? Color::White : Color(51, 65, 85)); p3Txt.setPosition({position.x + width - 103.f, pagY + 6.f}); win.draw(p3Txt);

    // Nút Tiếp
    drawRoundedCard(win, {position.x + width - 75.f, pagY}, {55.f, 32.f}, 6.f, Color::White, 1.f, Color(226, 232, 240));
    Text nxTxt(font, L"Ti\u1ebfp", 14); nxTxt.setFillColor(Color(100, 116, 139));
    {
        FloatRect b = nxTxt.getLocalBounds();
        nxTxt.setPosition({position.x + width - 75.f + (55.f - b.size.x) / 2.f - b.position.x, pagY + (32.f - b.size.y) / 2.f - b.position.y});
    }
    win.draw(nxTxt);

    // ==========================================
    // 7. KHỐI CHÍNH SÁCH RẠP (POLICY NOTE BOX)
    // ==========================================
    float noteY = position.y + 846.f;
    drawRoundedCard(win, {position.x + 30.f, noteY}, {tableW, 60.f}, 8.f, Color(238, 248, 255), 1.f, Color(186, 230, 253));

    // Icon info
    CircleShape infoCir(8.f); infoCir.setFillColor(Color(8, 125, 181)); infoCir.setPosition({position.x + 48.f, noteY + 16.f}); win.draw(infoCir);
    Text ifTxt(boldFont, L"i", 14); ifTxt.setFillColor(Color::White); ifTxt.setPosition({position.x + 53.f, noteY + 16.f}); win.draw(ifTxt);

    Text noteTxt(font, L"Quy chu\u1ea9n d\u1ecdn d\u1eb9p & gi\u00e3n c\u00e1ch su\u1ea5t chi\u1ebfu: Gi\u1eefa m\u1ed7i su\u1ea5t chi\u1ebfu, h\u1ec7 th\u1ed1ng t\u1ef1 \u0111\u1ed9ng ch\u1eeba 15\u201320 ph\u00fat th\u1eddi gian l\u00e0m v\u1ec7 sinh ph\u00f2ng v\u00e0 ki\u1ec3m tra thi\u1ebft b\u1ecb \u00e2m\nthanh/m\u00e1y chi\u1ebfu tr\u01b0\u1edbc khi \u0111\u00f3n l\u01b0\u1ee3t kh\u00e1ch ti\u1ebfp theo. \u0110\u1ec3 thay \u0111\u1ed5i th\u1eddi gian ngh\u1ec9 k\u1ef9 thu\u1eadt n\u00e0y, vui l\u00f2ng truy c\u1eadp m\u1ee5c C\u1ea5u h\u00ecnh r\u1ea1p chi\u1ebfu.", 14);
    noteTxt.setFillColor(Color(71, 85, 105));
    noteTxt.setPosition({position.x + 75.f, noteY + 13.f});
    win.draw(noteTxt);

    // ==========================================
    // 8. DROPDOWN MENUS (Render trên cùng)
    // ==========================================
    if (roomDropdownOpen) {
        float ddY = fCardY + 76.f;
        drawRoundedCard(win, {position.x + 310.f, ddY}, {230.f, (float)roomList.size() * 34.f}, 8.f, Color::White, 1.2f, Color(8, 125, 181));
        for (size_t i = 0; i < roomList.size(); ++i) {
            float iY = ddY + i * 34.f;
            if (roomList[i] == selectedRoom) {
                RoundedRectRenderer::draw(win, {position.x + 312.f, iY}, {226.f, 32.f}, 4.f, Color(224, 242, 254));
            }
            Text itmTxt(font, roomList[i], 14);
            itmTxt.setFillColor((roomList[i] == selectedRoom) ? Color(8, 125, 181) : Color(23, 32, 51));
            itmTxt.setPosition({position.x + 322.f, iY + 8.f});
            win.draw(itmTxt);
        }
    }

    if (statusDropdownOpen) {
        float ddY = fCardY + 144.f;
        drawRoundedCard(win, {position.x + 50.f, ddY}, {230.f, (float)statusList.size() * 34.f}, 8.f, Color::White, 1.2f, Color(8, 125, 181));
        for (size_t i = 0; i < statusList.size(); ++i) {
            float iY = ddY + i * 34.f;
            if (statusList[i] == selectedStatus) {
                RoundedRectRenderer::draw(win, {position.x + 52.f, iY}, {226.f, 32.f}, 4.f, Color(224, 242, 254));
            }
            Text itmTxt(font, statusList[i], 14);
            itmTxt.setFillColor((statusList[i] == selectedStatus) ? Color(8, 125, 181) : Color(23, 32, 51));
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

        Text pTitle(boldFont, (editingIndex >= 0) ? L"S\u1eeda su\u1ea5t chi\u1ebfu" : L"Th\u00eam su\u1ea5t chi\u1ebfu m\u1edbi", 24);
        pTitle.setFillColor(Color(23, 32, 51));
        pTitle.setPosition({popX + 30.f, popY + 20.f});
        win.draw(pTitle);

        // Nút chọn nhanh Phòng chiếu ở góc trên phải của Popup
        drawRoundedCard(win, {popX + popW - 168.f, popY + 20.f}, {138.f, 28.f}, 6.f, Color(224, 242, 254), 1.f, Color(186, 230, 253));
        Text rBadge(boldFont, popupRoomName + L" \u25bc", 14);
        rBadge.setFillColor(Color(8, 125, 181));
        FloatRect rbb = rBadge.getLocalBounds();
        rBadge.setPosition({
            std::round(popX + popW - 168.f + (138.f - rbb.size.x) / 2.f - rbb.position.x),
            std::round(popY + 20.f + (28.f - rbb.size.y) / 2.f - rbb.position.y - 0.5f)
        });
        win.draw(rBadge);

        // Trường 0: Tên phim
        bool f0Act = (popupActiveField == 0);
        Text l1(boldFont, L"T\u00ean phim:", 14); l1.setFillColor(Color(71, 85, 105)); l1.setPosition({popX + 30.f, popY + 60.f}); win.draw(l1);
        drawRoundedCard(win, {popX + 30.f, popY + 80.f}, {popW - 60.f, 36.f}, 6.f,
                        f0Act ? Color::White : Color(248, 250, 252),
                        f0Act ? 1.8f : 1.f,
                        f0Act ? Color(8, 125, 181) : Color(203, 213, 225));
        wstring dispTitle = popupMovieTitle.empty()
            ? (f0Act ? L"|" : L"Nh\u1eadp t\u00ean phim chi\u1ebfu...")
            : (popupMovieTitle + (f0Act ? L"|" : L""));
        Text v1(font, dispTitle, 14);
        v1.setFillColor(popupMovieTitle.empty() && !f0Act ? Color(148, 163, 184) : Color(23, 32, 51));
        v1.setPosition({popX + 40.f, popY + 89.f});
        win.draw(v1);

        // Trường 1: Giờ bắt đầu
        bool f1Act = (popupActiveField == 1);
        Text l2(boldFont, L"Gi\u1edd b\u1eaft \u0111\u1ea7u (HH:mm):", 14); l2.setFillColor(Color(71, 85, 105)); l2.setPosition({popX + 30.f, popY + 125.f}); win.draw(l2);
        drawRoundedCard(win, {popX + 30.f, popY + 145.f}, {180.f, 36.f}, 6.f,
                        f1Act ? Color::White : Color(248, 250, 252),
                        f1Act ? 1.8f : 1.f,
                        f1Act ? Color(8, 125, 181) : Color(203, 213, 225));
        Text v2(font, s2ws(popupStartTime) + (f1Act ? L"|" : L""), 14);
        v2.setFillColor(Color(23, 32, 51));
        v2.setPosition({popX + 40.f, popY + 154.f});
        win.draw(v2);

        // Trường 2: Giá vé
        bool f2Act = (popupActiveField == 2);
        Text l3(boldFont, L"Gi\u00e1 v\u00e9 (\u0111):", 14); l3.setFillColor(Color(71, 85, 105)); l3.setPosition({popX + 230.f, popY + 125.f}); win.draw(l3);
        drawRoundedCard(win, {popX + 230.f, popY + 145.f}, {190.f, 36.f}, 6.f,
                        f2Act ? Color::White : Color(248, 250, 252),
                        f2Act ? 1.8f : 1.f,
                        f2Act ? Color(8, 125, 181) : Color(203, 213, 225));
        Text v3(font, s2ws(popupPrice) + (f2Act ? L"|" : L""), 14);
        v3.setFillColor(Color(23, 32, 51));
        v3.setPosition({popX + 240.f, popY + 154.f});
        win.draw(v3);

        // Gợi ý phím tắt
        Text hintTxt(font, L"Nh\u1ea5n Tab \u0111\u1ec3 chuy\u1ec3n \u00f4 \u2022 Enter \u0111\u1ec3 l\u01b0u \u2022 Esc \u0111\u1ec3 h\u1ee7y", 14);
        hintTxt.setFillColor(Color(148, 163, 184));
        hintTxt.setPosition({popX + 30.f, popY + 195.f});
        win.draw(hintTxt);

        // Buttons
        drawRoundedCard(win, {popX + 60.f, popY + 230.f}, {150.f, 38.f}, 6.f, Color(8, 125, 181));
        Text sTxt(boldFont, L"L\u01b0u th\u00f4ng tin", 14); sTxt.setFillColor(Color::White); sTxt.setPosition({popX + 90.f, popY + 239.f}); win.draw(sTxt);

        drawRoundedCard(win, {popX + 240.f, popY + 230.f}, {150.f, 38.f}, 6.f, Color(241, 245, 249), 1.f, Color(203, 213, 225));
        Text cTxt(boldFont, L"H\u1ee7y b\u1ecf", 14); cTxt.setFillColor(Color(71, 85, 105)); cTxt.setPosition({popX + 290.f, popY + 239.f}); win.draw(cTxt);
    }

    // ==========================================
    // 10. TOAST NOTIFICATION
    // ==========================================
    if (notificationVisible) {
        if (notificationClock.getElapsedTime().asSeconds() > 3.0f) {
            notificationVisible = false;
        } else {
            Text notText(boldFont, s2ws(notificationMessage), 14);
            notText.setFillColor(Color::White);
            FloatRect ntb = notText.getLocalBounds();
            float notW = max(380.f, min(width - 80.f, ntb.size.x + 48.f));
            float notH = 44.f;
            float notX = position.x + (width - notW) / 2.f;
            float notY = position.y + height - 70.f;
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
