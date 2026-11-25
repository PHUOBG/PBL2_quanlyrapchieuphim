#include "UI/components/Admin/MoviePanel.h"
#include "UI/components/Admin/FontManager.h"
#include "UI/components/Admin/UITheme.h"
#include "utils/FileUtils.h"
#include "repositories/admin/AdminTicketRepository.h"
#include "repositories/booking/ShowtimeSeatRepository.h"
#include <algorithm>
#include <cwctype>
#include <fstream>
#include <sstream>
#include <unordered_set>
#include <iostream>

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

MoviePanel::MoviePanel(Font& fontRef, float w, float h)
    : font(fontRef),
      boldFont(FontManager::instance().bold()),
      width(w), height(h), position(280.f, 0.f)
{
    repository = make_unique<AdminMovieRepository>(FileUtils::resolveDataPath("data/movies.txt"));
    refreshData();
}

const Texture* MoviePanel::getPosterTexture(const string& rawPath, const string& movieId) {
    string key = rawPath.empty() ? movieId : rawPath;
    if (key.empty()) return nullptr;
    auto it = posterCache.find(key);
    if (it != posterCache.end()) return &it->second;

    Texture tex;
    tex.setSmooth(true);

    vector<string> candidates;
    if (!rawPath.empty()) {
        candidates.push_back(rawPath);
        candidates.push_back(FileUtils::resolveDataPath(rawPath));
        auto p = rawPath.find_last_of("/\\");
        string fname = (p == string::npos) ? rawPath : rawPath.substr(p + 1);
        candidates.push_back("assets/posters/" + fname);
        candidates.push_back("../assets/posters/" + fname);
        candidates.push_back("../../assets/posters/" + fname);
    }

    if (movieId == "#PBL-001") {
        candidates.push_back("assets/posters/latmat6-tamvedinhmenh.jpg");
        candidates.push_back("../assets/posters/latmat6-tamvedinhmenh.jpg");
    } else if (movieId == "#PBL-002") {
        candidates.push_back("assets/posters/spiderman.jpg");
        candidates.push_back("../assets/posters/spiderman.jpg");
    } else if (movieId == "#PBL-003") {
        candidates.push_back("assets/posters/vayhamkhongloithoat.jpg");
        candidates.push_back("../assets/posters/vayhamkhongloithoat.jpg");
    } else if (movieId == "#PBL-004") {
        candidates.push_back("assets/posters/PBl04.jpg");
        candidates.push_back("../assets/posters/PBl04.jpg");
    } else if (movieId == "#PBL-005") {
        candidates.push_back("assets/posters/cuocchienbatu.jpg");
        candidates.push_back("../assets/posters/cuocchienbatu.jpg");
    } else if (movieId == "#PBL-006") {
        candidates.push_back("assets/posters/dune-part-two.png");
        candidates.push_back("../assets/posters/dune-part-two.png");
    } else if (movieId == "#PBL-007") {
        candidates.push_back("assets/posters/mai.png");
        candidates.push_back("../assets/posters/mai.png");
    } else if (movieId == "#PBL-008") {
        candidates.push_back("assets/posters/panda4.jpg");
        candidates.push_back("../assets/posters/panda4.jpg");
        candidates.push_back("assets/posters/inside-out.png");
        candidates.push_back("../assets/posters/inside-out.png");
    } else if (movieId == "#PBL-009") {
        candidates.push_back("assets/posters/nha-ba-toi-mot-phong.jpg");
        candidates.push_back("../assets/posters/nha-ba-toi-mot-phong.jpg");
        candidates.push_back("assets/posters/Nhà ba tôi một phòng.jpg");
        candidates.push_back("../assets/posters/Nhà ba tôi một phòng.jpg");
    } else if (movieId == "#PBL-010") {
        candidates.push_back("assets/posters/nghi-he-so-nghi-huu.jpg");
        candidates.push_back("../assets/posters/nghi-he-so-nghi-huu.jpg");
        candidates.push_back("assets/posters/nghỉ hè sợ nghỉ hưu.jpg");
        candidates.push_back("../assets/posters/nghỉ hè sợ nghỉ hưu.jpg");
    } else if (movieId == "#PBL-011") {
        candidates.push_back("assets/posters/mui-pho.jpg");
        candidates.push_back("../assets/posters/mui-pho.jpg");
        candidates.push_back("assets/posters/Mùi phở.jpg");
        candidates.push_back("../assets/posters/Mùi phở.jpg");
    } else if (movieId == "#PBL-012") {
        candidates.push_back("assets/posters/len-huong.jpg");
        candidates.push_back("../assets/posters/len-huong.jpg");
        candidates.push_back("assets/posters/Lên Hương.jpg");
        candidates.push_back("../assets/posters/Lên Hương.jpg");
    } else if (movieId == "#PBL-013") {
        candidates.push_back("assets/posters/shin-ky-nghi-yeu-quai.jpg");
        candidates.push_back("../assets/posters/shin-ky-nghi-yeu-quai.jpg");
        candidates.push_back("assets/posters/kỳ nghỉ siêu quái của tớ.jpg");
        candidates.push_back("../assets/posters/kỳ nghỉ siêu quái của tớ.jpg");
    } else if (movieId == "#PBL-014") {
        candidates.push_back("assets/posters/heo-5-mong.jpg");
        candidates.push_back("../assets/posters/heo-5-mong.jpg");
        candidates.push_back("assets/posters/Heo 5 móng.jpg");
        candidates.push_back("../assets/posters/Heo 5 móng.jpg");
    } else if (movieId == "#PBL-015") {
        candidates.push_back("assets/posters/chi-chi-em-em-3.jpg");
        candidates.push_back("../assets/posters/chi-chi-em-em-3.jpg");
        candidates.push_back("assets/posters/chị chị em em 3.jpg");
        candidates.push_back("../assets/posters/chị chị em em 3.jpg");
    } else if (movieId == "#PBL-016") {
        candidates.push_back("assets/posters/bo-sua-bay.jpg");
        candidates.push_back("../assets/posters/bo-sua-bay.jpg");
        candidates.push_back("assets/posters/bò sữa bay.jpg");
        candidates.push_back("../assets/posters/bò sữa bay.jpg");
    } else if (movieId == "#PBL-017") {
        candidates.push_back("assets/posters/tho-oi.jpg");
        candidates.push_back("../assets/posters/tho-oi.jpg");
        candidates.push_back("assets/posters/Thỏ ơi.jpg");
        candidates.push_back("../assets/posters/Thỏ ơi.jpg");
    } else if (movieId == "#PBL-018") {
        candidates.push_back("assets/posters/trai-buon-nguoi.jpg");
        candidates.push_back("../assets/posters/trai-buon-nguoi.jpg");
        candidates.push_back("assets/posters/trại buôn người.jpg");
        candidates.push_back("../assets/posters/trại buôn người.jpg");
    }

    bool loaded = false;
    for (const auto& c : candidates) {
        if (!c.empty() && (FileUtils::safeLoadFromFile(tex, c) || FileUtils::safeLoadFromFile(tex, s2ws(c)))) {
            loaded = true;
            break;
        }
    }

    if (loaded) {
        posterCache[key] = std::move(tex);
        return &posterCache[key];
    }
    return nullptr;
}

void MoviePanel::setPosition(Vector2f pos) {
    position = pos;
    width = 1728.f - pos.x;
}

void MoviePanel::rebuildMovieSearchTrie() {
    titleTrie = make_unique<Trie>();
    normalizedTitleToIndices.clear();

    for (size_t i = 0; i < allMovies.size(); ++i) {
        if (allMovies[i].size() < 2) continue;
        string normTitle = UITheme::normalizeViUtf8(allMovies[i][1]);
        if (normTitle.empty()) continue;

        auto indexKey = [&](const string& key) {
            if (key.empty()) return;
            titleTrie->insert(key);
            auto& idxList = normalizedTitleToIndices[key];
            if (find(idxList.begin(), idxList.end(), static_cast<int>(i)) == idxList.end()) {
                idxList.push_back(static_cast<int>(i));
            }
        };

        // Lập chỉ mục tiền tố cho cả tên phim đầy đủ và từng hậu tố bắt đầu từ đầu từ
        indexKey(normTitle);
        for (size_t pos = 0; pos < normTitle.size(); ++pos) {
            if (normTitle[pos] == ' ' && pos + 1 < normTitle.size() && normTitle[pos + 1] != ' ') {
                indexKey(normTitle.substr(pos + 1));
            }
        }
    }
}

void MoviePanel::refreshData() {
    if (!repository) return;
    repository->loadFromFile();
    allMovies = repository->getAllData();
    rebuildMovieSearchTrie();
    applyFilter();
}

void MoviePanel::applyFilter() {
    filteredIndices.clear();

    unordered_set<int> trieMatchedIndices;
    bool useTrieFilter = false;
    if (!searchQuery.empty() && titleTrie) {
        string normQuery = UITheme::normalizeViUtf8(ws2s(searchQuery));
        size_t first = normQuery.find_first_not_of(" \t\r\n");
        if (first != string::npos) {
            size_t last = normQuery.find_last_not_of(" \t\r\n");
            normQuery = normQuery.substr(first, last - first + 1);
            useTrieFilter = true;

            StringArray suggestions = titleTrie->getSuggestions(normQuery, 200);
            for (int k = 0; k < suggestions.size; ++k) {
                const char* sug = suggestions.get(k);
                if (!sug) continue;
                auto it = normalizedTitleToIndices.find(sug);
                if (it != normalizedTitleToIndices.end()) {
                    for (int idx : it->second) {
                        trieMatchedIndices.insert(idx);
                    }
                }
            }
        }
    }

    for (size_t i = 0; i < allMovies.size(); ++i) {
        const auto& row = allMovies[i];
        if (row.size() < 8) continue;

        string idStr = row[0];
        string title = row[1];
        string country = row[3];
        string genres = row[5];
        string director = (row.size() > 9) ? row[9] : "";
        string actors = (row.size() > 10) ? row[10] : "";
        string status = (row.size() >= 14) ? row[13] : "Đang chiếu";
        if (status.empty()) status = "Đang chiếu";

        // 1. Tìm kiếm tiền tố bằng Trie (hỗ trợ tiếng Việt có dấu & không dấu)
        if (useTrieFilter && trieMatchedIndices.count(static_cast<int>(i)) == 0) {
            continue;
        }

        // 2. Lọc thể loại (hỗ trợ so khớp chuẩn hóa tiếng Việt)
        if (selectedGenre != "all") {
            if (!UITheme::matchesViQuery(s2ws(genres), s2ws(selectedGenre))) continue;
        }

        // 3. Lọc trạng thái
        if (selectedStatus != "all") {
            if (selectedStatus == "dang_chieu" && !UITheme::matchesViQuery(s2ws(status), L"Đang chiếu")) continue;
            if (selectedStatus == "sap_chieu" && !UITheme::matchesViQuery(s2ws(status), L"Sắp chiếu")) continue;
        }

        filteredIndices.push_back(static_cast<int>(i));
    }

    totalPages = max(1, static_cast<int>((filteredIndices.size() + moviesPerPage - 1) / moviesPerPage));
    if (currentPage > totalPages) currentPage = totalPages;
    if (currentPage < 1) currentPage = 1;
    scrollOffset = (currentPage - 1) * moviesPerPage;
    if (!searchQuery.empty() && !filteredIndices.empty()) {
        selectedRow = 0;
    } else if (selectedRow >= static_cast<int>(filteredIndices.size())) {
        selectedRow = -1;
    }
}

void MoviePanel::showToast(const wstring& msg, Color col) {
    notificationMessage = msg;
    notificationColor = col;
    notificationVisible = true;
    notificationClock.restart();
}

void MoviePanel::openAddModal() {
    currentPopup = ADD;
    editingRecordIndex = -1;
    inputTitle = L"";
    inputGenres = L"Hành động, Phiêu lưu";
    inputFormat = L"2D / Atmos";
    inputDirector = L"";
    inputCountry = L"Việt Nam";
    inputDuration = L"120";
    inputReleaseDate = L"28/04/2024";
    inputStatus = L"Đang chiếu";
    inputPosterPath = L"../assets/posters/godzilla-x-kong.png";
    posterPresetIdx = 0;
    activeInputBox = 0;
}

void MoviePanel::openEditModal(int recordIndex) {
    if (recordIndex < 0 || recordIndex >= static_cast<int>(allMovies.size())) return;
    currentPopup = EDIT;
    editingRecordIndex = recordIndex;

    const auto& row = allMovies[recordIndex];
    inputTitle = s2ws(row.size() > 1 ? row[1] : "");
    inputCountry = s2ws(row.size() > 3 ? row[3] : "Việt Nam");
    string fmtVal = (row.size() > 4) ? row[4] : "";
    if (fmtVal.find("2D") != string::npos || fmtVal.find("IMAX") != string::npos ||
        fmtVal.find("4DX") != string::npos || fmtVal.find("Lồng tiếng") != string::npos) {
        inputFormat = s2ws(fmtVal);
    } else {
        inputFormat = L"2D / Atmos";
    }
    inputGenres = s2ws(row.size() > 5 ? row[5] : "");
    inputDuration = s2ws(row.size() > 6 ? row[6] : "120");
    inputReleaseDate = s2ws(row.size() > 7 ? row[7] : "");
    inputDirector = s2ws(row.size() > 9 ? row[9] : "");
    inputPosterPath = s2ws(row.size() > 12 ? row[12] : "");
    inputStatus = s2ws(row.size() >= 14 ? row[13] : "Đang chiếu");
    if (inputStatus.empty()) inputStatus = L"Đang chiếu";
    activeInputBox = 0;
}

void MoviePanel::openDeleteModal(int recordIndex) {
    if (recordIndex < 0 || recordIndex >= static_cast<int>(allMovies.size())) return;
    currentPopup = DELETE_CONFIRM;
    editingRecordIndex = recordIndex;
}

void MoviePanel::closePopupModal() {
    currentPopup = NONE;
    editingRecordIndex = -1;
    activeInputBox = -1;
}

void MoviePanel::saveMovie() {
    if (inputTitle.empty()) {
        showToast(L"Vui lòng nhập tên phim!", Color(220, 38, 38));
        return;
    }

    // Validate positive duration
    string durStr = ws2s(inputDuration);
    int durVal = 0;
    try { durVal = stoi(durStr); } catch (...) { durVal = 0; }
    if (durVal <= 0 || durVal > 600) {
        showToast(L"Thời lượng phim phải là số nguyên dương (1 - 600 phút)!", Color(220, 38, 38));
        return;
    }

    // Validate DD/MM/YYYY release date (calendar-valid, and <= end_date)
    auto parseDdMmYyyy = [](const string& d, int& yyyymmdd) -> bool {
        if (d.size() != 10 || d[2] != '/' || d[5] != '/') return false;
        for (int i = 0; i < 10; ++i) {
            if (i == 2 || i == 5) continue;
            if (!isdigit(static_cast<unsigned char>(d[i]))) return false;
        }
        int day = stoi(d.substr(0, 2));
        int mon = stoi(d.substr(3, 2));
        int yr  = stoi(d.substr(6, 4));
        if (mon < 1 || mon > 12 || yr < 1900 || yr > 2100) return false;
        int dim[] = {31,28,31,30,31,30,31,31,30,31,30,31};
        if ((yr % 4 == 0 && yr % 100 != 0) || (yr % 400 == 0)) dim[1] = 29;
        if (day < 1 || day > dim[mon - 1]) return false;
        yyyymmdd = yr * 10000 + mon * 100 + day;
        return true;
    };

    string relStr = ws2s(inputReleaseDate);
    int relNum = 0;
    if (!parseDdMmYyyy(relStr, relNum)) {
        showToast(L"Ngày phát hành không hợp lệ (DD/MM/YYYY)!", Color(220, 38, 38));
        return;
    }

    string endStr = "30/12/2030";
    if (currentPopup == EDIT && editingRecordIndex >= 0 && editingRecordIndex < static_cast<int>(allMovies.size())) {
        if (allMovies[editingRecordIndex].size() > 8 && !allMovies[editingRecordIndex][8].empty()) {
            endStr = allMovies[editingRecordIndex][8];
        }
    }
    int endNum = 0;
    if (parseDdMmYyyy(endStr, endNum) && relNum > endNum) {
        showToast(L"Ngày phát hành không được lớn hơn ngày kết thúc chiếu!", Color(220, 38, 38));
        return;
    }

    auto sanitize = [](string s) {
        for (char& ch : s) if (ch == '|' || ch == '\n' || ch == '\r') ch = ' ';
        return s;
    };

    if (currentPopup == ADD) {
        int maxNum = static_cast<int>(allMovies.size());
        for (const auto& r : allMovies) {
            if (!r.empty() && r[0].size() > 5 && r[0].rfind("#PBL-", 0) == 0) {
                try {
                    int n = stoi(r[0].substr(5));
                    if (n > maxNum) maxNum = n;
                } catch (...) {}
            }
        }
        string newId = "#PBL-" + to_string(maxNum + 1);
        if (newId.size() < 8) newId.insert(5, string(8 - newId.size(), '0'));

        string posterStr = sanitize(ws2s(inputPosterPath));
        if (posterStr.empty()) posterStr = "../assets/posters/godzilla-x-kong.png";
        string fmtStr = sanitize(ws2s(inputFormat));
        if (fmtStr.empty()) fmtStr = "2D / Atmos";

        vector<string> newRow = {
            newId,
            sanitize(ws2s(inputTitle)),
            "T16",
            sanitize(ws2s(inputCountry)),
            fmtStr,
            sanitize(ws2s(inputGenres)),
            to_string(durVal),
            relStr,
            endStr,
            sanitize(ws2s(inputDirector)),
            "Diễn viên chính",
            "Mô tả phim...",
            posterStr,
            sanitize(ws2s(inputStatus))
        };
        repository->addRecord(newRow);
        if (!repository->saveToFile()) {
            repository->loadFromFile();
            showToast(L"Lỗi: Không thể lưu dữ liệu phim!", Color(220, 38, 38));
            return;
        }
        selectedGenre = "all";
        selectedStatus = "all";
        searchQuery.clear();
        refreshData();
        currentPage = totalPages;
        selectedRow = filteredIndices.empty() ? -1 : static_cast<int>(filteredIndices.size()) - 1;
        closePopupModal();
        showToast(L"Thêm phim mới thành công!");
    }
    else if (currentPopup == EDIT) {
        if (editingRecordIndex >= 0 && editingRecordIndex < static_cast<int>(allMovies.size())) {
            auto row = allMovies[editingRecordIndex];
            while (row.size() < 14) row.push_back("");
            row[1] = sanitize(ws2s(inputTitle));
            row[3] = sanitize(ws2s(inputCountry));
            if (!inputFormat.empty()) row[4] = sanitize(ws2s(inputFormat));
            row[5] = sanitize(ws2s(inputGenres));
            row[6] = to_string(durVal);
            row[7] = relStr;
            row[9] = sanitize(ws2s(inputDirector));
            if (!inputPosterPath.empty()) row[12] = sanitize(ws2s(inputPosterPath));
            row[13] = sanitize(ws2s(inputStatus));

            repository->updateRecord(editingRecordIndex, row);
            if (!repository->saveToFile()) {
                repository->loadFromFile();
                showToast(L"Lỗi: Không thể lưu thay đổi phim!", Color(220, 38, 38));
                return;
            }
            refreshData();
            closePopupModal();
            showToast(L"Cập nhật thông tin phim thành công!");
        }
    }
}

void MoviePanel::confirmDelete() {
    if (editingRecordIndex >= 0 && editingRecordIndex < static_cast<int>(allMovies.size())) {
        const string movieId = allMovies[editingRecordIndex].empty() ? "" : allMovies[editingRecordIndex][0];

        // Check if any showtime for this movie already has tickets or booked seats
        unordered_set<string> ticketedShowtimes;
        {
            AdminTicketRepository ticketRepo(FileUtils::resolveDataPath("data/tickets.txt"));
            DLL<Ticket> tickets = ticketRepo.loadAll();
            for (int i = 0; i < tickets.getSize(); ++i) {
                ticketedShowtimes.insert(tickets[i].showtimeId);
            }
        }
        ShowtimeSeatRepository seatRepo(FileUtils::resolveDataPath("data/RoomStatusAtShowtime.txt"));

        string stPath = FileUtils::resolveDataPath("data/showtimes.txt");
        vector<string> keptShowtimeLines;
        string stHeader = "showtime_id|movie_id|room_id|date|time|price";
        bool hasShowtimeFile = false;
        {
            ifstream sf(stPath, ios::binary);
            if (sf.is_open()) {
                hasShowtimeFile = true;
                string line;
                if (getline(sf, line)) {
                    if (!line.empty() && line.back() == '\r') line.pop_back();
                    if (!line.empty()) stHeader = line;
                }
                while (getline(sf, line)) {
                    if (!line.empty() && line.back() == '\r') line.pop_back();
                    if (line.empty()) continue;
                    stringstream ss(line);
                    string sid, mid, rid;
                    getline(ss, sid, '|');
                    getline(ss, mid, '|');
                    getline(ss, rid, '|');
                    if (mid == movieId) {
                        if (ticketedShowtimes.count(sid) || seatRepo.getBookedSeats(sid, rid).getSize() > 0) {
                            closePopupModal();
                            showToast(L"Không thể xóa phim vì đã có vé đặt cho suất chiếu của phim này!", Color(220, 38, 38));
                            return;
                        }
                        // Unbooked showtime for deleted movie -> prune so showtimes stay consistent
                        continue;
                    }
                    keptShowtimeLines.push_back(line);
                }
            }
        }

        repository->deleteRecord(editingRecordIndex);
        if (!repository->saveToFile()) {
            repository->loadFromFile();
            showToast(L"Lỗi: Không thể xóa phim (lỗi ghi file)!", Color(220, 38, 38));
            return;
        }
        if (hasShowtimeFile) {
            string out = stHeader + "\n";
            for (const auto& l : keptShowtimeLines) out += l + "\n";
            FileUtils::writeAtomic(stPath, out);
        }

        selectedRow = -1;
        refreshData();
        closePopupModal();
        selectedRow = -1;
        showToast(L"Đã xóa phim và các suất chiếu chưa có vé!");
    }
}

// Vector Icon Helpers
void MoviePanel::drawPlayIcon(RenderTarget& target, Vector2f pos, Color col) {
    UITheme::drawPlayIcon(target, {pos.x + 5.f, pos.y + 5.5f}, 12.f, col);
}

void MoviePanel::drawCalendarIcon(RenderTarget& target, Vector2f pos, Color col) {
    UITheme::drawCalendarIcon(target, {pos.x + 7.f, pos.y + 6.5f}, 15.f, col);
}

void MoviePanel::drawClockIcon(RenderTarget& target, Vector2f pos, Color col) {
    UITheme::drawClockIcon(target, {pos.x + 7.f, pos.y + 7.f}, 7.5f, col);
}

void MoviePanel::drawFilmIcon(RenderTarget& target, Vector2f pos, Color col) {
    UITheme::drawFilmIcon(target, {pos.x + 7.5f, pos.y + 6.f}, 16.f, col);
}

void MoviePanel::drawSearchIcon(RenderTarget& target, Vector2f pos, Color col) {
    CircleShape c(5.f, 14);
    c.setFillColor(Color::Transparent);
    c.setOutlineThickness(1.5f);
    c.setOutlineColor(col);
    c.setPosition(pos);
    target.draw(c);

    RectangleShape stem({5.f, 1.8f});
    stem.setFillColor(col);
    stem.setRotation(degrees(45.f));
    stem.setPosition({pos.x + 8.f, pos.y + 8.f});
    target.draw(stem);
}

void MoviePanel::drawPencilIcon(RenderTarget& target, Vector2f pos, Color col) {
    RectangleShape b({9.f, 3.f});
    b.setFillColor(col);
    b.setRotation(degrees(-45.f));
    b.setPosition({pos.x + 2.f, pos.y + 8.f});
    target.draw(b);

    ConvexShape tip(3);
    tip.setPoint(0, {pos.x, pos.y + 11.f});
    tip.setPoint(1, {pos.x + 3.f, pos.y + 11.f});
    tip.setPoint(2, {pos.x, pos.y + 8.f});
    tip.setFillColor(col);
    target.draw(tip);
}

void MoviePanel::drawTrashIcon(RenderTarget& target, Vector2f pos, Color col) {
    RectangleShape handle({4.f, 1.6f});
    handle.setFillColor(col);
    handle.setPosition({pos.x + 5.f, pos.y});
    target.draw(handle);

    RectangleShape top({14.f, 1.8f});
    top.setFillColor(col);
    top.setPosition({pos.x, pos.y + 1.8f});
    target.draw(top);

    RoundedRectRenderer::draw(target, {pos.x + 2.f, pos.y + 4.f}, {10.f, 10.f}, 1.5f, Color::Transparent, 1.4f, col);

    RectangleShape bar1({1.4f, 5.5f});
    bar1.setFillColor(col);
    bar1.setPosition({pos.x + 4.8f, pos.y + 6.2f});
    target.draw(bar1);

    RectangleShape bar2({1.4f, 5.5f});
    bar2.setFillColor(col);
    bar2.setPosition({pos.x + 7.8f, pos.y + 6.2f});
    target.draw(bar2);
}

void MoviePanel::drawPlusIcon(RenderTarget& target, Vector2f pos, Color col) {
    RectangleShape h({10.f, 2.f});
    h.setFillColor(col);
    h.setPosition({pos.x, pos.y + 4.f});
    target.draw(h);
    RectangleShape v({2.f, 10.f});
    v.setFillColor(col);
    v.setPosition({pos.x + 4.f, pos.y});
    target.draw(v);
}

void MoviePanel::drawReloadIcon(RenderTarget& target, Vector2f pos, Color col) {
    CircleShape arc(6.f, 20);
    arc.setFillColor(Color::Transparent);
    arc.setOutlineThickness(1.6f);
    arc.setOutlineColor(col);
    arc.setPosition(pos);
    target.draw(arc);

    ConvexShape arrow(3);
    arrow.setPoint(0, {pos.x + 10.f, pos.y - 1.f});
    arrow.setPoint(1, {pos.x + 13.f, pos.y + 3.f});
    arrow.setPoint(2, {pos.x + 8.f, pos.y + 3.f});
    arrow.setFillColor(col);
    target.draw(arrow);
}

void MoviePanel::drawBellIcon(RenderTarget& target, Vector2f pos, Color col) {
    RoundedRectRenderer::draw(target, {pos.x + 2.f, pos.y + 2.f}, {11.f, 8.f}, 3.5f, Color::Transparent, 1.5f, col);
    RectangleShape btm({15.f, 1.8f});
    btm.setFillColor(col);
    btm.setPosition({pos.x, pos.y + 10.f});
    target.draw(btm);
    CircleShape clapper(1.8f, 8);
    clapper.setFillColor(col);
    clapper.setPosition({pos.x + 5.5f, pos.y + 11.5f});
    target.draw(clapper);
}

void MoviePanel::drawFunnelIcon(RenderTarget& target, Vector2f pos, Color col) {
    ConvexShape top(3);
    top.setPoint(0, {pos.x, pos.y});
    top.setPoint(1, {pos.x + 14.f, pos.y});
    top.setPoint(2, {pos.x + 7.f, pos.y + 8.f});
    top.setFillColor(col);
    target.draw(top);

    RectangleShape stem({2.6f, 6.5f});
    stem.setFillColor(col);
    stem.setPosition({pos.x + 5.7f, pos.y + 7.5f});
    target.draw(stem);
}

void MoviePanel::drawCheckmark(RenderTarget& target, Vector2f pos, Color col) {
    RectangleShape c1({5.5f, 2.2f});
    c1.setOrigin({0.f, 1.1f});
    c1.setFillColor(col);
    c1.setRotation(degrees(45.f));
    c1.setPosition({pos.x + 1.f, pos.y + 5.f});
    target.draw(c1);

    RectangleShape c2({10.f, 2.2f});
    c2.setOrigin({0.f, 1.1f});
    c2.setFillColor(col);
    c2.setRotation(degrees(-48.f));
    c2.setPosition({pos.x + 4.5f, pos.y + 8.6f});
    target.draw(c2);
}

void MoviePanel::drawDownArrow(RenderTarget& target, Vector2f pos, Color col) {
    ConvexShape a(3);
    a.setPoint(0, {pos.x, pos.y});
    a.setPoint(1, {pos.x + 7.f, pos.y});
    a.setPoint(2, {pos.x + 3.5f, pos.y + 4.f});
    a.setFillColor(col);
    target.draw(a);
}

void MoviePanel::handleEvent(const Event& event, const RenderWindow& window) {
    // Caret blinking
    if (caretClock.getElapsedTime().asSeconds() >= 0.5f) {
        caretVisible = !caretVisible;
        caretClock.restart();
    }

    // 1. ƯU TIÊN XỬ LÝ POPUP MODAL (ADD / EDIT / DELETE_CONFIRM)
    if (currentPopup != NONE) {
        const float popupW = 680.f;
        const float popupH = (currentPopup == DELETE_CONFIRM) ? 240.f : 590.f;
        const float popupX = (1728.f - popupW) / 2.f;
        const float popupY = (972.f - popupH) / 2.f;

        if (const auto* mouseBtn = event.getIf<Event::MouseButtonPressed>()) {
            if (mouseBtn->button == Mouse::Button::Left) {
                Vector2f mouse = window.mapPixelToCoords(Mouse::getPosition(window));
                if (currentPopup == DELETE_CONFIRM) {
                    FloatRect cancelBtn({popupX + 40.f, popupY + 160.f}, {280.f, 44.f});
                    FloatRect okBtn({popupX + 360.f, popupY + 160.f}, {280.f, 44.f});
                    if (cancelBtn.contains(mouse)) { closePopupModal(); return; }
                    if (okBtn.contains(mouse)) { confirmDelete(); return; }
                    if (!FloatRect({popupX, popupY}, {popupW, popupH}).contains(mouse)) { closePopupModal(); return; }
                } else {
                    const float leftX = popupX + 40.f;
                    const float rightX = popupX + 360.f;
                    const float startY = popupY + 75.f;
                    const float gapY = 68.f;

                    for (int c = 0; c < 4; ++c) {
                        FloatRect lb({leftX, startY + c * gapY + 20.f}, {280.f, 40.f});
                        if (lb.contains(mouse)) { activeInputBox = c; return; }
                        if (c < 3) {
                            FloatRect rb({rightX, startY + c * gapY + 20.f}, {280.f, 40.f});
                            if (rb.contains(mouse)) { activeInputBox = c + 4; return; }
                        } else {
                            // Status pick selector: Đang chiếu vs Sắp chiếu
                            FloatRect pickPlaying({rightX, startY + 3 * gapY + 20.f}, {135.f, 40.f});
                            FloatRect pickUpcoming({rightX + 145.f, startY + 3 * gapY + 20.f}, {135.f, 40.f});
                            if (pickPlaying.contains(mouse)) { inputStatus = L"Đang chiếu"; activeInputBox = -1; return; }
                            if (pickUpcoming.contains(mouse)) { inputStatus = L"Sắp chiếu"; activeInputBox = -1; return; }
                        }
                    }

                    // Poster input + preset picker
                    float posterY = startY + 4 * gapY + 20.f;
                    FloatRect posterBox({leftX, posterY}, {490.f, 40.f});
                    FloatRect posterPickBtn({leftX + 500.f, posterY}, {100.f, 40.f});
                    if (posterBox.contains(mouse)) { activeInputBox = 8; return; }
                    if (posterPickBtn.contains(mouse)) {
                        static const wchar_t* kPresets[] = {
                            L"../assets/posters/godzilla-x-kong.png",
                            L"../assets/posters/inside-out.png",
                            L"../assets/posters/spirited-away.png",
                            L"../assets/posters/blue-beetle.png",
                            L"../assets/posters/dao-pho-va-piano.png",
                            L"../assets/posters/bo-gia-remastered.png",
                            L"../assets/posters/lat-mat-48h.png",
                            L"../assets/posters/tu-chien-tren-khong.png"
                        };
                        posterPresetIdx = (posterPresetIdx + 1) % 8;
                        inputPosterPath = kPresets[posterPresetIdx];
                        activeInputBox = 8;
                        return;
                    }

                    FloatRect cancelBtn({leftX, popupY + 516.f}, {280.f, 44.f});
                    FloatRect saveBtn({rightX, popupY + 516.f}, {280.f, 44.f});
                    if (cancelBtn.contains(mouse)) { closePopupModal(); return; }
                    if (saveBtn.contains(mouse)) { saveMovie(); return; }
                }
            }
            return;
        }

        if (const auto* keyEvent = event.getIf<Event::KeyPressed>()) {
            if (keyEvent->code == Keyboard::Key::Escape) {
                closePopupModal();
                return;
            }
            if (keyEvent->code == Keyboard::Key::Enter) {
                if (currentPopup == DELETE_CONFIRM) confirmDelete();
                else saveMovie();
                return;
            }
            if (keyEvent->code == Keyboard::Key::Tab) {
                if (currentPopup != DELETE_CONFIRM) {
                    activeInputBox = (activeInputBox + 1) % 9;
                    if (activeInputBox == 7) activeInputBox = 8;
                }
                return;
            }
            if (keyEvent->code == Keyboard::Key::Backspace) {
                if (activeInputBox == 0 && !inputTitle.empty()) inputTitle.pop_back();
                else if (activeInputBox == 1 && !inputGenres.empty()) inputGenres.pop_back();
                else if (activeInputBox == 2 && !inputFormat.empty()) inputFormat.pop_back();
                else if (activeInputBox == 3 && !inputDirector.empty()) inputDirector.pop_back();
                else if (activeInputBox == 4 && !inputCountry.empty()) inputCountry.pop_back();
                else if (activeInputBox == 5 && !inputDuration.empty()) inputDuration.pop_back();
                else if (activeInputBox == 6 && !inputReleaseDate.empty()) inputReleaseDate.pop_back();
                else if (activeInputBox == 8 && !inputPosterPath.empty()) inputPosterPath.pop_back();
                return;
            }
        }
        if (const auto* textEvent = event.getIf<Event::TextEntered>()) {
            char32_t uc = textEvent->unicode;
            if (uc >= 32 && uc != 127) {
                if (activeInputBox == 0 && inputTitle.size() < 60) inputTitle.push_back((wchar_t)uc);
                else if (activeInputBox == 1 && inputGenres.size() < 60) inputGenres.push_back((wchar_t)uc);
                else if (activeInputBox == 2 && inputFormat.size() < 30) inputFormat.push_back((wchar_t)uc);
                else if (activeInputBox == 3 && inputDirector.size() < 40) inputDirector.push_back((wchar_t)uc);
                else if (activeInputBox == 4 && inputCountry.size() < 30) inputCountry.push_back((wchar_t)uc);
                else if (activeInputBox == 5 && inputDuration.size() < 10) inputDuration.push_back((wchar_t)uc);
                else if (activeInputBox == 6 && inputReleaseDate.size() < 20) inputReleaseDate.push_back((wchar_t)uc);
                else if (activeInputBox == 8 && inputPosterPath.size() < 100) inputPosterPath.push_back((wchar_t)uc);
            }
            return;
        }
        return;
    }

    // 2. XỬ LÝ CLICK CHUỘT TRÊN MÀN HÌNH CHÍNH QUẢN LÝ PHIM
    if (const auto* mouseBtn = event.getIf<Event::MouseButtonPressed>()) {
        if (mouseBtn->button == Mouse::Button::Left) {
            Vector2f mouse = window.mapPixelToCoords(Mouse::getPosition(window));
            const float startX = position.x + 30.f;
            const float contentW = max(1200.f, width - 60.f);
            const float searchW = max(320.f, contentW - 884.f);
            const float toolRightX = startX + searchW + 15.f;
            const float toolY = 220.f;

            // Ưu tiên kiểm tra danh sách Dropdown đang mở trước khi kiểm tra bảng bên dưới
            if (genreDropdownOpen) {
                vector<string> genres = {"Tất cả thể loại", "Hành động", "Hoạt hình", "Tội phạm", "Viễn tưởng", "Tâm lý", "Hài", "Kinh dị", "Gia đình"};
                for (size_t g = 0; g < genres.size(); ++g) {
                    FloatRect gb({toolRightX, toolY + 46.f + g * 36.f}, {160.f, 36.f});
                    if (gb.contains(mouse)) {
                        selectedGenre = (g == 0) ? "all" : genres[g];
                        genreDropdownOpen = false;
                        applyFilter();
                        return;
                    }
                }
                genreDropdownOpen = false;
                return;
            }

            if (statusDropdownOpen) {
                vector<pair<string, string>> statuses = {{"all", "Trạng thái: Tất cả"}, {"dang_chieu", "Đang chiếu"}, {"sap_chieu", "Sắp chiếu"}};
                for (size_t s = 0; s < statuses.size(); ++s) {
                    FloatRect sb({toolRightX + 175.f, toolY + 46.f + s * 36.f}, {160.f, 36.f});
                    if (sb.contains(mouse)) {
                        selectedStatus = statuses[s].first;
                        statusDropdownOpen = false;
                        applyFilter();
                        return;
                    }
                }
                statusDropdownOpen = false;
                return;
            }

            FloatRect searchBox({startX, toolY}, {searchW, 44.f});
            FloatRect genreBtn({toolRightX, toolY}, {160.f, 44.f});
            FloatRect statusBtn({toolRightX + 175.f, toolY}, {160.f, 44.f});
            FloatRect reloadBtn({toolRightX + 350.f, toolY}, {44.f, 44.f});
            FloatRect editBtn({toolRightX + 404.f, toolY}, {150.f, 44.f});
            FloatRect deleteBtn({toolRightX + 566.f, toolY}, {125.f, 44.f});
            FloatRect addBtn({toolRightX + 704.f, toolY}, {165.f, 44.f});

            if (searchBox.contains(mouse)) {
                if (!searchQuery.empty() && mouse.x >= startX + searchW - 32.f) {
                    searchQuery.clear();
                    applyFilter();
                }
                searchActive = true;
                genreDropdownOpen = false;
                statusDropdownOpen = false;
                return;
            } else {
                searchActive = false;
            }

            if (genreBtn.contains(mouse)) {
                genreDropdownOpen = !genreDropdownOpen;
                statusDropdownOpen = false;
                return;
            }

            if (statusBtn.contains(mouse)) {
                statusDropdownOpen = !statusDropdownOpen;
                genreDropdownOpen = false;
                return;
            }

            if (reloadBtn.contains(mouse)) {
                searchQuery = L"";
                selectedGenre = "all";
                selectedStatus = "all";
                refreshData();
                showToast(L"Đã làm mới dữ liệu!");
                return;
            }

            if (addBtn.contains(mouse)) {
                openAddModal();
                return;
            }

            if (editBtn.contains(mouse)) {
                if (selectedRow >= 0 && selectedRow < static_cast<int>(filteredIndices.size())) {
                    openEditModal(filteredIndices[selectedRow]);
                } else {
                    showToast(L"Vui lòng chọn 1 phim để sửa!", Color(245, 158, 11));
                }
                return;
            }

            if (deleteBtn.contains(mouse)) {
                if (selectedRow >= 0 && selectedRow < static_cast<int>(filteredIndices.size())) {
                    openDeleteModal(filteredIndices[selectedRow]);
                } else {
                    showToast(L"Vui lòng chọn 1 phim để xóa!", Color(220, 38, 38));
                }
                return;
            }

            // Click trên các dòng của bảng phim
            const float tableY = 314.f;
            const float hdrH = 44.f;
            const float rowH = 68.f;
            const float tableW = contentW;
            const float col8X = startX + tableW - 120.f;

            int startIdx = (currentPage - 1) * moviesPerPage;
            int displayCount = max(0, min(moviesPerPage, static_cast<int>(filteredIndices.size()) - startIdx));
            for (int r = 0; r < displayCount; ++r) {
                int idx = startIdx + r;
                float rY = tableY + hdrH + r * rowH;
                FloatRect rBounds({startX, rY}, {tableW, rowH});
                FloatRect editIconBtn({col8X + 20.f, rY + (rowH - 34.f) / 2.f}, {34.f, 34.f});
                FloatRect delIconBtn({col8X + 64.f, rY + (rowH - 34.f) / 2.f}, {34.f, 34.f});

                if (rBounds.contains(mouse)) {
                    if (editIconBtn.contains(mouse)) {
                        openEditModal(filteredIndices[idx]);
                    } else if (delIconBtn.contains(mouse)) {
                        openDeleteModal(filteredIndices[idx]);
                    } else {
                        selectedRow = (selectedRow == idx) ? -1 : idx;
                    }
                    return;
                }
            }

            // Thanh phân trang bảng phim (Pagination)
            float pagY = tableY + hdrH + moviesPerPage * rowH + 12.f;
            float rightEdge = startX + tableW;
            int numPageBtns = min(5, totalPages);
            int startP = max(1, min(currentPage - 2, totalPages - numPageBtns + 1));
            int endP = min(totalPages, startP + numPageBtns - 1);
            float btnPrevW = 65.f;
            float btnNextW = 61.f;
            float btnPageW = 32.f;
            float btnPagH = 31.f;
            float gap = 5.f;
            float totalBtnsW = btnPrevW + gap + (endP - startP + 1) * (btnPageW + gap) - gap + gap + btnNextW;
            float curBtnX = rightEdge - totalBtnsW;

            // Nút Trước
            FloatRect btnPrev({curBtnX, pagY}, {btnPrevW, btnPagH});
            if (btnPrev.contains(mouse) && currentPage > 1) {
                currentPage--;
                selectedRow = -1;
                scrollOffset = (currentPage - 1) * moviesPerPage;
                return;
            }
            curBtnX += btnPrevW + gap;

            // Các nút số trang 1, 2, 3, ...
            for (int p = startP; p <= endP; ++p) {
                FloatRect btnP({curBtnX, pagY}, {btnPageW, btnPagH});
                if (btnP.contains(mouse)) {
                    currentPage = p;
                    selectedRow = -1;
                    scrollOffset = (currentPage - 1) * moviesPerPage;
                    return;
                }
                curBtnX += btnPageW + gap;
            }

            // Nút Tiếp
            FloatRect btnNext({curBtnX, pagY}, {btnNextW, btnPagH});
            if (btnNext.contains(mouse) && currentPage < totalPages) {
                currentPage++;
                selectedRow = -1;
                scrollOffset = (currentPage - 1) * moviesPerPage;
                return;
            }
        }
    }

    // 3. NHẬP TÌM KIẾM TỪ BÀN PHÍM (HỖ TRỢ TIẾNG VIỆT UNICODE)
    if (const auto* keyEvent = event.getIf<Event::KeyPressed>()) {
        if (searchActive) {
            if (keyEvent->code == Keyboard::Key::Backspace && !searchQuery.empty()) {
                searchQuery.pop_back();
                currentPage = 1;
                applyFilter();
                return;
            }
            if (keyEvent->code == Keyboard::Key::Escape) {
                searchQuery.clear();
                searchActive = false;
                currentPage = 1;
                applyFilter();
                return;
            }
        }
    }
    if (const auto* textEvent = event.getIf<Event::TextEntered>()) {
        if (searchActive) {
            char32_t uc = textEvent->unicode;
            if (uc >= 32 && uc != 127 && searchQuery.size() < 50) {
                searchQuery.push_back((wchar_t)uc);
                currentPage = 1;
                applyFilter();
            }
        }
    }

    // 4. CHUYỂN TRANG BẰNG CUỘN CHUỘT
    if (const auto* wheel = event.getIf<Event::MouseWheelScrolled>()) {
        if (wheel->delta > 0 && currentPage > 1) {
            currentPage--;
            selectedRow = -1;
            scrollOffset = (currentPage - 1) * moviesPerPage;
        } else if (wheel->delta < 0 && currentPage < totalPages) {
            currentPage++;
            selectedRow = -1;
            scrollOffset = (currentPage - 1) * moviesPerPage;
        }
    }
}

void MoviePanel::update(Vector2f mouse, bool /*mousePressed*/) {
    if (currentPopup != NONE) {
        const float popupW = 680.f;
        const float popupH = (currentPopup == DELETE_CONFIRM) ? 240.f : 590.f;
        const float popupX = (1728.f - popupW) / 2.f;
        const float popupY = (972.f - popupH) / 2.f;

        if (currentPopup == DELETE_CONFIRM) {
            FloatRect cancelBtn({popupX + 40.f, popupY + 160.f}, {280.f, 44.f});
            FloatRect okBtn({popupX + 360.f, popupY + 160.f}, {280.f, 44.f});
            btnDeleteCancelHover = cancelBtn.contains(mouse);
            btnDeleteOkHover = okBtn.contains(mouse);
        } else {
            FloatRect cancelBtn({popupX + 40.f, popupY + 516.f}, {280.f, 44.f});
            FloatRect saveBtn({popupX + 360.f, popupY + 516.f}, {280.f, 44.f});
            btnPopupCancelHover = cancelBtn.contains(mouse);
            btnPopupSaveHover = saveBtn.contains(mouse);
        }
        return;
    }

    const float startX = position.x + 30.f;
    const float contentW = max(1200.f, width - 60.f);
    const float searchW = max(320.f, contentW - 884.f);
    const float toolRightX = startX + searchW + 15.f;
    const float toolY = 220.f;

    FloatRect searchBox({startX, toolY}, {searchW, 44.f});
    FloatRect genreBtn({toolRightX, toolY}, {160.f, 44.f});
    FloatRect statusBtn({toolRightX + 175.f, toolY}, {160.f, 44.f});
    FloatRect reloadBtn({toolRightX + 350.f, toolY}, {44.f, 44.f});
    FloatRect editBtn({toolRightX + 404.f, toolY}, {150.f, 44.f});
    FloatRect deleteBtn({toolRightX + 566.f, toolY}, {125.f, 44.f});
    FloatRect addBtn({toolRightX + 704.f, toolY}, {165.f, 44.f});

    btnSearchHover = searchBox.contains(mouse);
    btnGenreHover = genreBtn.contains(mouse);
    btnStatusHover = statusBtn.contains(mouse);
    btnReloadHover = reloadBtn.contains(mouse);
    btnEditHover = editBtn.contains(mouse);
    btnDeleteHover = deleteBtn.contains(mouse);
    btnAddHover = addBtn.contains(mouse);

    if (genreDropdownOpen) {
        hoveredGenreIndex = -1;
        for (size_t g = 0; g < 9; ++g) {
            FloatRect gb({toolRightX, toolY + 46.f + g * 36.f}, {160.f, 36.f});
            if (gb.contains(mouse)) hoveredGenreIndex = static_cast<int>(g);
        }
    }

    if (statusDropdownOpen) {
        hoveredStatusIndex = -1;
        for (size_t s = 0; s < 3; ++s) {
            FloatRect sb({toolRightX + 175.f, toolY + 46.f + s * 36.f}, {160.f, 36.f});
            if (sb.contains(mouse)) hoveredStatusIndex = static_cast<int>(s);
        }
    }

    // Table rows hover
    const float tableY = 314.f;
    const float hdrH = 44.f;
    const float rowH = 68.f;
    const float tableW = contentW;
    const float col8X = startX + tableW - 120.f;

    hoveredRow = -1;
    hoveredEditRow = -1;
    hoveredDeleteRow = -1;

    int startIdx = (currentPage - 1) * moviesPerPage;
    int displayCount = max(0, min(moviesPerPage, static_cast<int>(filteredIndices.size()) - startIdx));
    for (int r = 0; r < displayCount; ++r) {
        int idx = startIdx + r;
        float rY = tableY + hdrH + r * rowH;
        FloatRect rBounds({startX, rY}, {tableW, rowH});
        FloatRect editIconBtn({col8X + 20.f, rY + (rowH - 34.f) / 2.f}, {34.f, 34.f});
        FloatRect delIconBtn({col8X + 64.f, rY + (rowH - 34.f) / 2.f}, {34.f, 34.f});

        if (editIconBtn.contains(mouse)) hoveredEditRow = idx;
        if (delIconBtn.contains(mouse)) hoveredDeleteRow = idx;
        if (rBounds.contains(mouse)) hoveredRow = idx;
    }
}

void MoviePanel::render(RenderWindow& window) {
    const float startX = position.x + 30.f;
    const float contentW = max(1200.f, width - 60.f);
    const float searchW = max(320.f, contentW - 884.f);
    const float toolRightX = startX + searchW + 15.f;

    // 1. Unified Section Header Card (matching RoomPanel card style)
    UITheme::drawSectionHeaderCard(window, startX, position.y + 16.f, contentW,
                                   L"Danh s\u00e1ch v\u00e0 \u0111i\u1ec1u ph\u1ed1i phim",
                                   to_wstring(allMovies.size()) + L" Phim",
                                   Color(224, 242, 254), Color(8, 125, 181),
                                   L"Qu\u1ea3n l\u00fd th\u00f4ng tin phim, \u0111\u1ecbnh d\u1ea1ng chi\u1ebfu, l\u1ecbch ph\u00e1t h\u00e0nh v\u00e0 tr\u1ea1ng th\u00e1i tr\u00ecnh chi\u1ebfu.",
                                   boldFont, font, true);

    // 2. 4 KPI STAT CARDS (-20% card height & typography)
    const float cardY = 114.f;
    const float cardH = 90.f;
    const float cardGap = 20.f;
    const float cardW = (contentW - 3.f * cardGap) / 4.f;

    int countActive = 0, countUpcoming = 0;
    for (const auto& r : allMovies) {
        string st = (r.size() >= 14) ? r[13] : "Đang chiếu";
        if (st == "Đang chiếu" || st.empty()) countActive++;
        else countUpcoming++;
    }

    auto drawMovieKpi = [&](Vector2f pos, const wstring& title, const wstring& value, const wstring& sub, Color subCol) {
        RoundedRectRenderer::draw(window, pos, {cardW, cardH}, 8.f, UITheme::Surface, 1.f, UITheme::BorderLight);
        FontManager::drawCenteredText(window, boldFont, title, 14, UITheme::TextSecondary, {{pos.x, pos.y + 4.f}, {cardW, 24.f}});
        FontManager::drawCenteredText(window, boldFont, value, 24, UITheme::TextPrimary, {{pos.x, pos.y + 28.f}, {cardW, 34.f}});
        FontManager::drawCenteredText(window, boldFont, sub, 14, subCol, {{pos.x, pos.y + cardH - 28.f}, {cardW, 22.f}});
    };

    drawMovieKpi({startX, cardY}, L"Phim đang chiếu", to_wstring(countActive), L"Đang tạo doanh thu", UITheme::SuccessDark);
    drawMovieKpi({startX + cardW + cardGap, cardY}, L"Phim sắp khởi chiếu", to_wstring(countUpcoming), L"Đã mở bán sớm", UITheme::InfoDark);
    drawMovieKpi({startX + (cardW + cardGap) * 2.f, cardY}, L"Tổng suất hôm nay", L"42", L"12 phòng chiếu full", Color(126, 34, 206));
    drawMovieKpi({startX + (cardW + cardGap) * 3.f, cardY}, L"Tổng phim quản lý", to_wstring(allMovies.size()), L"Kho dữ liệu CineCore", UITheme::PrimaryNavy);

    // 5. TOOLBAR (Search & Filters & Actions)
    const float toolY = 220.f;

    // Search Input
    Color searchBorder = searchActive ? Color(8, 125, 181) : Color(203, 213, 225);
    RoundedRectRenderer::draw(window, {startX, toolY}, {searchW, 44.f}, 10.f, Color::White, searchActive ? 1.8f : 1.2f, searchBorder);
    drawSearchIcon(window, {startX + 14.f, toolY + 14.f}, searchActive ? Color(8, 125, 181) : Color(100, 116, 139));

    if (searchQuery.empty()) {
        Text ph(font, L"T\u00ecm t\u00ean phim...", UITheme::BodySize);
        ph.setFillColor(Color(100, 116, 139));
        ph.setPosition({startX + 38.f, toolY + 11.f});
        window.draw(ph);
        if (searchActive && caretVisible) {
            RectangleShape caret({2.f, 20.f});
            caret.setFillColor(Color(8, 125, 181));
            caret.setPosition({startX + 38.f, toolY + 12.f});
            window.draw(caret);
        }
    } else {
        Text val(font, searchQuery, UITheme::BodySize);
        val.setFillColor(Color(23, 32, 51));
        val.setPosition({startX + 38.f, toolY + 11.f});
        window.draw(val);

        if (searchActive && caretVisible) {
            RectangleShape caret({2.f, 20.f});
            caret.setFillColor(Color(8, 125, 181));
            caret.setPosition({startX + 40.f + val.getLocalBounds().size.x, toolY + 12.f});
            window.draw(caret);
        }

        UITheme::drawCloseIcon(window, {startX + searchW - 20.f, toolY + 22.f}, 11.f, Color(148, 163, 184), 1.8f);
    }

    // Genre Dropdown Button
    RoundedRectRenderer::draw(window, {toolRightX, toolY}, {160.f, 44.f}, 10.f, Color::White, 1.2f, Color(203, 213, 225));
    wstring gLabel = (selectedGenre == "all") ? L"T\u1ea5t c\u1ea3 th\u1ec3 lo\u1ea1i" : s2ws(selectedGenre);
    Text gt(font, gLabel, UITheme::BodySize);
    gt.setFillColor(Color(30, 41, 59));
    gt.setPosition({toolRightX + 11.f, toolY + 11.f});
    window.draw(gt);
    drawDownArrow(window, {toolRightX + 139.f, toolY + 20.f}, Color(51, 65, 85));

    // Status Dropdown Button
    RoundedRectRenderer::draw(window, {toolRightX + 175.f, toolY}, {160.f, 44.f}, 10.f, Color::White, 1.2f, Color(203, 213, 225));
    wstring stLabel = (selectedStatus == "all") ? L"Tr\u1ea1ng th\u00e1i: T\u1ea5t c\u1ea3" : (selectedStatus == "dang_chieu" ? L"\u0110ang chi\u1ebfu" : L"S\u1eafp chi\u1ebfu");
    Text stt(font, stLabel, UITheme::BodySize);
    stt.setFillColor(Color(30, 41, 59));
    stt.setPosition({toolRightX + 185.f, toolY + 11.f});
    window.draw(stt);
    drawDownArrow(window, {toolRightX + 317.f, toolY + 20.f}, Color(51, 65, 85));

    // Reload Button
    Color relCol = btnReloadHover ? Color(241, 245, 249) : Color::White;
    RoundedRectRenderer::draw(window, {toolRightX + 350.f, toolY}, {44.f, 44.f}, 10.f, relCol, 1.2f, Color(203, 213, 225));
    UITheme::drawReloadIcon(window, {toolRightX + 372.f, toolY + 22.f}, 16.f, Color(30, 41, 59));

    // Edit Button (Amber/Orange)
    Color editCol = btnEditHover ? Color(217, 119, 6) : Color(245, 158, 11);
    RoundedRectRenderer::draw(window, {toolRightX + 404.f, toolY}, {150.f, 44.f}, 10.f, editCol);
    drawPencilIcon(window, {toolRightX + 416.f, toolY + 15.f}, Color::White);
    Text editTxt(boldFont, L"S\u1eeda th\u00f4ng tin", UITheme::ButtonTextSize);
    editTxt.setFillColor(Color::White);
    editTxt.setPosition({toolRightX + 436.f, toolY + 11.f});
    window.draw(editTxt);

    // Delete Button (Crimson)
    Color delCol = btnDeleteHover ? Color(220, 38, 38) : Color(220, 38, 38);
    RoundedRectRenderer::draw(window, {toolRightX + 566.f, toolY}, {125.f, 44.f}, 10.f, delCol);
    drawTrashIcon(window, {toolRightX + 578.f, toolY + 15.f}, Color::White);
    Text delTxt(boldFont, L"X\u00f3a phim", UITheme::ButtonTextSize);
    delTxt.setFillColor(Color::White);
    delTxt.setPosition({toolRightX + 601.f, toolY + 11.f});
    window.draw(delTxt);

    // Add Button (Ocean Blue)
    Color addCol = btnAddHover ? Color(18, 59, 93) : Color(8, 125, 181);
    RoundedRectRenderer::draw(window, {toolRightX + 704.f, toolY}, {165.f, 44.f}, 10.f, addCol);
    drawPlusIcon(window, {toolRightX + 716.f, toolY + 17.f}, Color::White);
    Text addTxt(boldFont, L"Th\u00eam phim m\u1edbi", UITheme::ButtonTextSize);
    addTxt.setFillColor(Color::White);
    addTxt.setPosition({toolRightX + 735.f, toolY + 11.f});
    window.draw(addTxt);

    // 6. DATA TABLE SUBHEADER
    const float subY = 280.f;
    Text subTitle(boldFont, L"DANH S\u00c1CH PHIM CHI\u1ebeU R\u1ea0P", UITheme::SectionTitleSize);
    subTitle.setFillColor(Color(23, 32, 51));
    subTitle.setPosition({std::round(startX + 24.f), std::round(subY + 1.f)});
    FloatRect subB = subTitle.getGlobalBounds();
    drawFunnelIcon(window, {std::round(startX + 2.f), std::round(subB.position.y + (subB.size.y - 14.f) / 2.f)}, Color(8, 125, 181));
    window.draw(subTitle);

    // Right count badge
    wstring countStr = L"Trang " + to_wstring(currentPage) + L"/" + to_wstring(totalPages) + L" (" + to_wstring(filteredIndices.size()) + L" phim)";
    RoundedRectRenderer::draw(window, {startX + contentW - 220.f, subY - 4.f}, {220.f, 30.f}, 15.f, Color(241, 245, 249), 1.2f, Color(203, 213, 225));
    Text cBadge(boldFont, countStr, UITheme::SmallBadgeSize);
    cBadge.setFillColor(Color(30, 41, 59));
    FloatRect cbb = cBadge.getLocalBounds();
    cBadge.setPosition({startX + contentW - 220.f + (220.f - cbb.size.x) / 2.f - cbb.position.x, subY + 2.f});
    window.draw(cBadge);

    // 7. TABLE HEADER (Ocean Blue #0077B6)
    const float tblY = 314.f;
    const float tblW = contentW;
    const float hdrH = 44.f;
    const float col2W = max(280.f, tblW - 915.f);

    RoundedRectRenderer::draw(window, {startX, tblY}, {tblW, hdrH}, 8.f, Color(8, 125, 181));

    // Column headers
    vector<pair<wstring, float>> cols = {
        {L"", 45.f},
        {L"ID PHIM", 95.f},
        {L"T\u00caN PHIM & TH\u1ec2 LO\u1ea0I", col2W},
        {L"\u0110\u1ea0O DI\u1ec4N", 155.f},
        {L"QU\u1ed0C GIA", 125.f},
        {L"TH\u1edcI L\u01af\u1ee2NG", 115.f},
        {L"NG\u00c0Y CHI\u1ebeU", 120.f},
        {L"TR\u1ea0NG TH\u00c1I", 140.f},
        {L"THAO T\u00c1C", 120.f}
    };

    vector<float> colX(cols.size(), startX);
    for (size_t c = 0; c < cols.size(); ++c) {
        if (c > 0) {
            colX[c] = colX[c - 1] + cols[c - 1].second;
            RectangleShape hSep({1.f, hdrH - 16.f});
            hSep.setFillColor(Color(255, 255, 255, 55));
            hSep.setPosition({colX[c], tblY + 8.f});
            window.draw(hSep);
        }
        if (!cols[c].first.empty()) {
            Text hText(boldFont, cols[c].first, UITheme::TableHeaderSize);
            hText.setFillColor(Color::White);
            FloatRect hb = hText.getLocalBounds();
            float hY = tblY + (hdrH - hb.size.y) / 2.f - hb.position.y - 1.f;
            if (c == 1 || c >= 3) {
                hText.setPosition({colX[c] + (cols[c].second - hb.size.x) / 2.f - hb.position.x, hY});
            } else {
                hText.setPosition({colX[c] + 10.f, hY});
            }
            window.draw(hText);
        }
    }

    // 8. TABLE ROWS
    const float rowH = 68.f;
    int startIdx = (currentPage - 1) * moviesPerPage;
    int displayCount = max(0, min(moviesPerPage, static_cast<int>(filteredIndices.size()) - startIdx));

    for (int r = 0; r < displayCount; ++r) {
        int idx = startIdx + r;
        int origIdx = filteredIndices[idx];
        const auto& row = allMovies[origIdx];
        float rY = tblY + hdrH + r * rowH;

        bool isSelected = (idx == selectedRow);
        bool isHovered = (idx == hoveredRow);

        Color rowBg = isSelected ? Color(224, 242, 254) : (isHovered ? Color(248, 250, 252) : Color::White);
        RoundedRectRenderer::draw(window, {startX, rY}, {tblW, rowH}, 0.f, rowBg);

        // Horizontal 1px row separator
        RectangleShape rLine({tblW, 1.f});
        rLine.setFillColor(Color(226, 232, 240));
        rLine.setPosition({startX, rY + rowH - 1.f});
        window.draw(rLine);

        // Vertical 1px column separators aligned with header
        float vColX = startX;
        for (size_t c = 0; c < cols.size(); ++c) {
            if (c > 0) {
                RectangleShape vLine({1.f, rowH});
                vLine.setFillColor(Color(226, 232, 240));
                vLine.setPosition({vColX, rY});
                window.draw(vLine);
            }
            vColX += cols[c].second;
        }

        // Checkbox column
        Color cbBorder = isSelected ? Color(8, 125, 181) : Color(203, 213, 225);
        float cbY = rY + (rowH - 20.f) / 2.f;
        RoundedRectRenderer::draw(window, {startX + 13.f, cbY}, {20.f, 20.f}, 4.f, Color::White, 1.2f, cbBorder);
        if (isSelected) {
            drawCheckmark(window, {startX + 17.f, cbY + 5.f}, Color(8, 125, 181));
        }

        // Col 1: ID Phim (12px bold, centered)
        string idStr = (row.size() > 0) ? row[0] : "";
        Text idText(boldFont, s2ws(idStr), 12);
        idText.setFillColor(Color(8, 125, 181));
        FloatRect idb = idText.getLocalBounds();
        idText.setPosition({
            std::round(colX[1] + (cols[1].second - idb.size.x) / 2.f - idb.position.x),
            std::round(rY + (rowH - idb.size.y) / 2.f - idb.position.y - 1.f)
        });
        window.draw(idText);

        // Col 2: Poster thumbnail + Title + Format & Genre
        float col2X = colX[2] + 8.f;
        float maxTitleW = cols[2].second - 68.f;
        string lang = (row.size() > 4) ? row[4] : "";
        string fmtStr = "2D / Atmos";
        if (lang.find("2D") != string::npos || lang.find("IMAX") != string::npos ||
            lang.find("4DX") != string::npos || lang.find("Lồng tiếng") != string::npos) {
            fmtStr = (lang == "Lồng tiếng") ? "2D Lồng tiếng" : lang;
        } else if (idx % 4 == 1) {
            fmtStr = "IMAX 3D";
        } else if (idx % 4 == 2) {
            fmtStr = "2D Digital";
        } else if (idx % 4 == 3) {
            fmtStr = "4DX / 3D";
        }

        // Poster box: Real cinema movie poster loaded from assets/posters
        string pPath = (row.size() > 12) ? row[12] : "";
        const Texture* pTex = getPosterTexture(pPath, idStr);
        if (pTex) {
            // Draw background card & border
            RoundedRectRenderer::draw(window, {col2X - 1.f, rY + 8.f}, {40.f, 52.f}, 6.f, Color::White, 1.2f, UITheme::BorderLight);
            Sprite spr(*pTex);
            Vector2u sz = pTex->getSize();
            spr.setScale({38.f / (float)sz.x, 50.f / (float)sz.y});
            spr.setPosition({col2X, rY + 9.f});
            window.draw(spr);
            // Subtle glossy outline
            RoundedRectRenderer::draw(window, {col2X, rY + 9.f}, {38.f, 50.f}, 5.f, Color::Transparent, 1.f, Color(255, 255, 255, 60));
        } else {
            // Fallback film frame
            RoundedRectRenderer::draw(window, {col2X, rY + 9.f}, {38.f, 50.f}, 6.f, UITheme::TextPrimary);
            RoundedRectRenderer::draw(window, {col2X + 2.f, rY + 11.f}, {34.f, 46.f}, 4.f, Color(30, 41, 59));
            drawFilmIcon(window, {col2X + 11.f, rY + 22.f}, UITheme::PrimaryBorder);
            Text postFmt(boldFont, s2ws(fmtStr.substr(0, 3)), 8);
            postFmt.setFillColor(UITheme::TextTertiary);
            postFmt.setPosition({col2X + 9.f, rY + 42.f});
            window.draw(postFmt);
        }

        // Title
        string titleStr = (row.size() > 1) ? row[1] : "";
        wstring wTitle = s2ws(titleStr);
        Text titleTxt(boldFont, wTitle, UITheme::TableBodySize);
        if (titleTxt.getLocalBounds().size.x > maxTitleW) {
            while (wTitle.length() > 3 && titleTxt.getLocalBounds().size.x > maxTitleW - 10.f) {
                wTitle.pop_back();
                titleTxt.setString(wTitle + L"...");
            }
        }
        titleTxt.setFillColor(UITheme::TextPrimary);
        titleTxt.setPosition({std::round(col2X + 46.f), std::round(rY + 4.f)});
        window.draw(titleTxt);

        // Format Pill (e.g. 2D / Atmos, IMAX 3D, 4DX/3D, 2D Lồng tiếng)
        Text fmtTxt(boldFont, s2ws(fmtStr), UITheme::SmallBadgeSize);
        FloatRect fb = fmtTxt.getLocalBounds();
        float fmtPillW = fb.size.x + 14.f;
        float fmtPillH = 20.f;
        RoundedRectRenderer::draw(window, {col2X + 46.f, rY + 25.f}, {fmtPillW, fmtPillH}, 4.f, UITheme::SurfaceSubtle, 1.f, UITheme::BorderMedium);
        fmtTxt.setFillColor(Color(30, 41, 59));
        fmtTxt.setPosition({std::round(col2X + 53.f - fb.position.x), std::round(rY + 25.f + (fmtPillH - fb.size.y) / 2.f - fb.position.y - 1.f)});
        window.draw(fmtTxt);

        // Genre text
        string genreStr = (row.size() > 5) ? row[5] : "";
        wstring wGenre = s2ws(genreStr);
        Text gTxt(font, wGenre, UITheme::HelperTextSize);
        if (gTxt.getLocalBounds().size.x > maxTitleW) {
            while (wGenre.length() > 3 && gTxt.getLocalBounds().size.x > maxTitleW - 10.f) {
                wGenre.pop_back();
                gTxt.setString(wGenre + L"...");
            }
        }
        gTxt.setFillColor(UITheme::TextSecondary);
        gTxt.setPosition({std::round(col2X + 46.f), std::round(rY + 47.f)});
        window.draw(gTxt);

        // Col 3: Director (Centered)
        float col3X = colX[3];
        float col3W = cols[3].second;
        string dirStr = (row.size() > 9) ? row[9] : "";
        wstring wDir = s2ws(dirStr);
        Text dirTxt(font, wDir, UITheme::TableBodySize);
        if (dirTxt.getLocalBounds().size.x > col3W - 18.f) {
            while (wDir.length() > 3 && dirTxt.getLocalBounds().size.x > col3W - 24.f) {
                wDir.pop_back();
                dirTxt.setString(wDir + L"...");
            }
        }
        dirTxt.setFillColor(Color(30, 41, 59));
        FloatRect db = dirTxt.getLocalBounds();
        dirTxt.setPosition({
            std::round(col3X + (col3W - db.size.x) / 2.f - db.position.x),
            std::round(rY + (rowH - db.size.y) / 2.f - db.position.y - 1.f)
        });
        window.draw(dirTxt);

        // Col 4: Country pill (Centered)
        float col4X = colX[4];
        float col4W = cols[4].second;
        string ctry = (row.size() > 3) ? row[3] : "Việt Nam";

        Color ctryBg = Color(254, 226, 226);
        Color ctryText = Color(220, 38, 38);
        string code = "VN ";
        if (ctry.find("Mỹ") != string::npos) {
            ctryBg = Color(224, 242, 254);
            ctryText = Color(8, 125, 181);
            code = "US ";
        } else if (ctry.find("Hàn") != string::npos) {
            ctryBg = Color(243, 232, 255);
            ctryText = Color(124, 58, 237);
            code = "KR ";
        } else if (ctry.find("Nga") != string::npos) {
            ctryBg = Color(220, 252, 231);
            ctryText = Color(22, 163, 74);
            code = "RU ";
        } else if (ctry.find("Nhật") != string::npos) {
            ctryBg = Color(254, 243, 199);
            ctryText = Color(217, 119, 6);
            code = "JP ";
        }

        float pillW = 112.f;
        float pillH = 28.f;
        float pillX = col4X + (col4W - pillW) / 2.f;
        float pillY = rY + (rowH - pillH) / 2.f;

        RoundedRectRenderer::draw(window, {pillX, pillY}, {pillW, pillH}, pillH / 2.f, ctryBg);
        Text ctryDisplay(boldFont, s2ws(code + ctry), UITheme::SmallBadgeSize);
        ctryDisplay.setFillColor(ctryText);
        FloatRect ctb = ctryDisplay.getLocalBounds();
        ctryDisplay.setPosition({
            std::round(pillX + (pillW - ctb.size.x) / 2.f - ctb.position.x),
            std::round(pillY + (pillH - ctb.size.y) / 2.f - ctb.position.y - 1.f)
        });
        window.draw(ctryDisplay);

        // Col 5: Duration (Single line, centered)
        float col5X = colX[5];
        float col5W = cols[5].second;
        string dur = (row.size() > 6) ? row[6] : "120";
        Text durTxt(font, s2ws(dur) + L" ph\u00fat", UITheme::TableBodySize);
        durTxt.setFillColor(UITheme::TextPrimary);
        FloatRect dvb = durTxt.getLocalBounds();
        durTxt.setPosition({
            std::round(col5X + (col5W - dvb.size.x) / 2.f - dvb.position.x),
            std::round(rY + (rowH - dvb.size.y) / 2.f - dvb.position.y - 1.f)
        });
        window.draw(durTxt);

        // Col 6: Release Date (Centered)
        float col6X = colX[6];
        float col6W = cols[6].second;
        string relDate = (row.size() > 7) ? row[7] : "";
        Text dateTxt(font, s2ws(relDate), UITheme::TableBodySize);
        dateTxt.setFillColor(Color(30, 41, 59));
        FloatRect dtb = dateTxt.getLocalBounds();
        dateTxt.setPosition({
            std::round(col6X + (col6W - dtb.size.x) / 2.f - dtb.position.x),
            std::round(rY + (rowH - dtb.size.y) / 2.f - dtb.position.y - 1.f)
        });
        window.draw(dateTxt);

        // Col 7: Status Pill (Centered inside column)
        float col7X = colX[7];
        float col7W = cols[7].second;
        string st = (row.size() >= 14) ? row[13] : "Đang chiếu";
        if (st.empty()) st = "Đang chiếu";
        bool isPlaying = (st == "Đang chiếu");
        wstring stText = isPlaying ? L"Đang chiếu" : L"Sắp chiếu";
        Text stMeasure(boldFont, stText, UITheme::SmallBadgeSize);
        float badgeW = stMeasure.getLocalBounds().size.x + 20.f + 12.f;
        UITheme::drawBadge(window, {col7X + (col7W - badgeW) / 2.f, rY + (rowH - 28.f) / 2.f},
                           stText,
                           isPlaying ? UITheme::BadgeVariant::Success : UITheme::BadgeVariant::Warning,
                           boldFont, true, 28.f);

        // Col 8: Action Icons (Edit & Delete, centered)
        float col8X = colX[8];
        float actBtnY = rY + (rowH - 34.f) / 2.f;
        Color editBg = (idx == hoveredEditRow) ? UITheme::WarningLight : UITheme::SurfaceSubtle;
        Color editIcon = (idx == hoveredEditRow) ? UITheme::WarningDark : UITheme::TextSecondary;
        RoundedRectRenderer::draw(window, {col8X + 20.f, actBtnY}, {34.f, 34.f}, 8.f, editBg, 1.f, UITheme::BorderLight);
        drawPencilIcon(window, {col8X + 31.5f, actBtnY + 10.75f}, editIcon);

        Color delBg = (idx == hoveredDeleteRow) ? UITheme::DangerLight : UITheme::SurfaceSubtle;
        Color delIcon = (idx == hoveredDeleteRow) ? UITheme::DangerDark : UITheme::TextSecondary;
        RoundedRectRenderer::draw(window, {col8X + 64.f, actBtnY}, {34.f, 34.f}, 8.f, delBg, 1.f, UITheme::BorderLight);
        drawTrashIcon(window, {col8X + 74.f, actBtnY + 10.f}, delIcon);
    }

    // 8b. THANH PHÂN TRANG (PAGINATION BAR)
    float pagY = tblY + hdrH + moviesPerPage * rowH + 12.f;
    int fromIdx = filteredIndices.empty() ? 0 : (startIdx + 1);
    int toIdx = min(static_cast<int>(filteredIndices.size()), startIdx + moviesPerPage);
    wstring pageInfoStr = filteredIndices.empty()
        ? L"Không có phim nào phù hợp"
        : (L"Hiển thị " + to_wstring(fromIdx) + L" – " + to_wstring(toIdx) + L" trên tổng số " + to_wstring(filteredIndices.size()) + L" phim");
    Text pageInfo(font, pageInfoStr, UITheme::HelperTextSize);
    pageInfo.setFillColor(Color(100, 116, 139));
    pageInfo.setPosition({startX, pagY + 7.f});
    window.draw(pageInfo);

    float rightEdge = startX + tblW;
    int numPageBtns = min(5, totalPages);
    int startP = max(1, min(currentPage - 2, totalPages - numPageBtns + 1));
    int endP = min(totalPages, startP + numPageBtns - 1);
    float btnPrevW = 65.f;
    float btnNextW = 61.f;
    float btnPageW = 32.f;
    float btnPagH = 31.f;
    float gap = 5.f;
    float totalBtnsW = btnPrevW + gap + (endP - startP + 1) * (btnPageW + gap) - gap + gap + btnNextW;
    float curBtnX = rightEdge - totalBtnsW;
    const unsigned int pagFontSize = 13;

    // Nút Trước
    bool canPrev = (currentPage > 1);
    RoundedRectRenderer::draw(window, {curBtnX, pagY}, {btnPrevW, btnPagH}, 6.f, Color::White, 1.2f, Color(226, 232, 240));
    Text prTxt(boldFont, L"Trước", pagFontSize);
    prTxt.setFillColor(canPrev ? Color(71, 85, 105) : Color(203, 213, 225));
    FloatRect prb = prTxt.getLocalBounds();
    prTxt.setPosition({curBtnX + (btnPrevW - prb.size.x) / 2.f - prb.position.x, pagY + (btnPagH - prb.size.y) / 2.f - prb.position.y});
    window.draw(prTxt);
    curBtnX += btnPrevW + gap;

    // Các nút số trang 1, 2, 3, ...
    for (int p = startP; p <= endP; ++p) {
        bool isAct = (p == currentPage);
        Color pBg = isAct ? Color(8, 125, 181) : Color::White;
        Color pBorder = isAct ? Color(8, 125, 181) : Color(226, 232, 240);
        Color pFg = isAct ? Color::White : Color(51, 65, 85);

        RoundedRectRenderer::draw(window, {curBtnX, pagY}, {btnPageW, btnPagH}, 6.f, pBg, 1.2f, pBorder);
        Text pTxt(boldFont, to_wstring(p), pagFontSize);
        pTxt.setFillColor(pFg);
        FloatRect pb = pTxt.getLocalBounds();
        pTxt.setPosition({curBtnX + (btnPageW - pb.size.x) / 2.f - pb.position.x, pagY + (btnPagH - pb.size.y) / 2.f - pb.position.y});
        window.draw(pTxt);
        curBtnX += btnPageW + gap;
    }

    // Nút Tiếp
    bool canNext = (currentPage < totalPages);
    RoundedRectRenderer::draw(window, {curBtnX, pagY}, {btnNextW, btnPagH}, 6.f, Color::White, 1.2f, Color(226, 232, 240));
    Text nxTxt(boldFont, L"Tiếp", pagFontSize);
    nxTxt.setFillColor(canNext ? Color(71, 85, 105) : Color(203, 213, 225));
    FloatRect nxb = nxTxt.getLocalBounds();
    nxTxt.setPosition({curBtnX + (btnNextW - nxb.size.x) / 2.f - nxb.position.x, pagY + (btnPagH - nxb.size.y) / 2.f - nxb.position.y});
    window.draw(nxTxt);

    // 9. Dropdown menus (if open)
    if (genreDropdownOpen) {
        vector<string> genres = {"Tất cả thể loại", "Hành động", "Hoạt hình", "Tội phạm", "Viễn tưởng", "Tâm lý", "Hài", "Kinh dị", "Gia đình"};
        RoundedRectRenderer::draw(window, {toolRightX, toolY + 46.f}, {160.f, genres.size() * 36.f}, 8.f, Color::White, 1.2f, Color(203, 213, 225));
        for (size_t g = 0; g < genres.size(); ++g) {
            Color itemBg = (static_cast<int>(g) == hoveredGenreIndex) ? Color(241, 245, 249) : Color::White;
            RoundedRectRenderer::draw(window, {toolRightX + 1.f, toolY + 47.f + g * 36.f}, {158.f, 34.f}, 4.f, itemBg);
            Text it(font, s2ws(genres[g]), UITheme::BodySize);
            it.setFillColor(Color(23, 32, 51));
            FloatRect itb = it.getLocalBounds();
            it.setPosition({
                std::round(toolRightX + 11.f - itb.position.x),
                std::round(toolY + 47.f + g * 36.f + (34.f - itb.size.y) / 2.f - itb.position.y)
            });
            window.draw(it);
        }
    }

    if (statusDropdownOpen) {
        vector<string> statuses = {"Trạng thái: Tất cả", "Đang chiếu", "Sắp chiếu"};
        RoundedRectRenderer::draw(window, {toolRightX + 175.f, toolY + 46.f}, {160.f, statuses.size() * 36.f}, 8.f, Color::White, 1.2f, Color(203, 213, 225));
        for (size_t s = 0; s < statuses.size(); ++s) {
            Color itemBg = (static_cast<int>(s) == hoveredStatusIndex) ? Color(241, 245, 249) : Color::White;
            RoundedRectRenderer::draw(window, {toolRightX + 176.f, toolY + 47.f + s * 36.f}, {158.f, 34.f}, 4.f, itemBg);
            Text it(font, s2ws(statuses[s]), UITheme::BodySize);
            it.setFillColor(Color(23, 32, 51));
            FloatRect itb = it.getLocalBounds();
            it.setPosition({
                std::round(toolRightX + 185.f - itb.position.x),
                std::round(toolY + 47.f + s * 36.f + (34.f - itb.size.y) / 2.f - itb.position.y)
            });
            window.draw(it);
        }
    }

    // 10. MODAL POPUP
    if (currentPopup != NONE) {
        RectangleShape overlay({1728.f, 972.f});
        overlay.setFillColor(Color(15, 23, 42, 140));
        window.draw(overlay);

        const float popupW = 680.f;
        const float popupH = (currentPopup == DELETE_CONFIRM) ? 240.f : 590.f;
        const float popupX = (1728.f - popupW) / 2.f;
        const float popupY = (972.f - popupH) / 2.f;

        RoundedRectRenderer::draw(window, {popupX, popupY}, {popupW, popupH}, 16.f, Color::White, 1.f, Color(226, 232, 240));

        if (currentPopup == DELETE_CONFIRM) {
            Text pTitle(boldFont, L"X\u00c1C NH\u1eacN X\u00d3A PHIM", 24);
            pTitle.setFillColor(Color(220, 38, 38));
            pTitle.setPosition({popupX + 40.f, popupY + 35.f});
            window.draw(pTitle);

            string delName = (selectedRow >= 0 && selectedRow < static_cast<int>(allMovies.size())) ? allMovies[selectedRow][1] : "";
            Text pMsg(font, L"B\u1ea1n c\u00f3 ch\u1eafc ch\u1eafn mu\u1ed1n x\u00f3a phim \"" + s2ws(delName) + L"\" kh\u00f4ng?\nThao t\u00e1c n\u00e0y kh\u00f4ng th\u1ec3 ho\u00e0n t\u00e1c.", 18);
            pMsg.setLineSpacing(1.25f);
            pMsg.setFillColor(Color(71, 85, 105));
            pMsg.setPosition({popupX + 40.f, popupY + 76.f});
            window.draw(pMsg);

            Color cBg = btnDeleteCancelHover ? Color(203, 213, 225) : Color(226, 232, 240);
            RoundedRectRenderer::draw(window, {popupX + 40.f, popupY + 160.f}, {280.f, 44.f}, 8.f, cBg);
            Text cTxt(boldFont, L"H\u1ee7y b\u1ecf", 17);
            cTxt.setFillColor(Color(51, 65, 85));
            FloatRect cb = cTxt.getLocalBounds();
            cTxt.setPosition({
                std::round(popupX + 40.f + (280.f - cb.size.x) / 2.f - cb.position.x),
                std::round(popupY + 160.f + (44.f - cb.size.y) / 2.f - cb.position.y)
            });
            window.draw(cTxt);

            Color okBg = btnDeleteOkHover ? Color(220, 38, 38) : Color(220, 38, 38);
            RoundedRectRenderer::draw(window, {popupX + 360.f, popupY + 160.f}, {280.f, 44.f}, 8.f, okBg);
            Text okTxt(boldFont, L"\u0110\u1ed3ng \u00fd x\u00f3a", 17);
            okTxt.setFillColor(Color::White);
            FloatRect okb = okTxt.getLocalBounds();
            okTxt.setPosition({
                std::round(popupX + 360.f + (280.f - okb.size.x) / 2.f - okb.position.x),
                std::round(popupY + 160.f + (44.f - okb.size.y) / 2.f - okb.position.y)
            });
            window.draw(okTxt);
        } else {
            wstring mTitle = (currentPopup == ADD) ? L"TH\u00caM PHIM M\u1edaI" : L"CH\u1ec8NH S\u1eecA TH\u00d4NG TIN PHIM";
            Text pTitle(boldFont, mTitle, 24);
            pTitle.setFillColor(Color(23, 32, 51));
            pTitle.setPosition({popupX + 40.f, popupY + 30.f});
            window.draw(pTitle);

            struct FormField {
                wstring label;
                wstring* val;
                int idx;
            };

            vector<FormField> leftFields = {
                {L"T\u00ean phim*", &inputTitle, 0},
                {L"Th\u1ec3 lo\u1ea1i*", &inputGenres, 1},
                {L"\u0110\u1ecbnh d\u1ea1ng", &inputFormat, 2},
                {L"\u0110\u1ea1o di\u1ec5n*", &inputDirector, 3}
            };
            vector<FormField> rightFields = {
                {L"Qu\u1ed1c gia*", &inputCountry, 4},
                {L"Th\u1eddi l\u01b0\u1ee3ng (ph\u00fat)*", &inputDuration, 5},
                {L"Ng\u00e0y chi\u1ebfu (DD/MM/YYYY)*", &inputReleaseDate, 6},
                {L"Tr\u1ea1ng th\u00e1i (\u0110ang chi\u1ebfu / S\u1eafp chi\u1ebfu)", &inputStatus, 7}
            };

            const float leftX = popupX + 40.f;
            const float rightX = popupX + 360.f;
            const float startY = popupY + 75.f;
            const float gapY = 68.f;

            for (int i = 0; i < 4; ++i) {
                // Left
                bool lAct = (activeInputBox == leftFields[i].idx);
                Text lbl1(boldFont, leftFields[i].label, 14);
                lbl1.setFillColor(Color(30, 41, 59));
                lbl1.setPosition({leftX, startY + i * gapY});
                window.draw(lbl1);

                Color b1 = lAct ? Color(8, 125, 181) : Color(203, 213, 225);
                RoundedRectRenderer::draw(window, {leftX, startY + i * gapY + 20.f}, {280.f, 40.f}, 8.f,
                                          lAct ? Color::White : Color(248, 250, 252), lAct ? 1.8f : 1.2f, b1);
                Text v1(font, *leftFields[i].val + (lAct ? L"|" : L""), 14);
                v1.setFillColor(Color(23, 32, 51));
                v1.setPosition({leftX + 10.f, startY + i * gapY + 29.f});
                window.draw(v1);

                // Right
                Text lbl2(boldFont, rightFields[i].label, 14);
                lbl2.setFillColor(Color(30, 41, 59));
                lbl2.setPosition({rightX, startY + i * gapY});
                window.draw(lbl2);

                if (i < 3) {
                    bool rAct = (activeInputBox == rightFields[i].idx);
                    Color b2 = rAct ? Color(8, 125, 181) : Color(203, 213, 225);
                    RoundedRectRenderer::draw(window, {rightX, startY + i * gapY + 20.f}, {280.f, 40.f}, 8.f,
                                              rAct ? Color::White : Color(248, 250, 252), rAct ? 1.8f : 1.2f, b2);
                    Text v2(font, *rightFields[i].val + (rAct ? L"|" : L""), 14);
                    v2.setFillColor(Color(23, 32, 51));
                    v2.setPosition({rightX + 10.f, startY + i * gapY + 29.f});
                    window.draw(v2);
                } else {
                    // Pick selector for Status (Đang chiếu / Sắp chiếu)
                    bool isPlayingPick = (inputStatus != L"Sắp chiếu");
                    float pickY = startY + i * gapY + 20.f;

                    Color p1Bg = isPlayingPick ? Color(220, 252, 231) : Color(248, 250, 252);
                    Color p1Border = isPlayingPick ? Color(21, 128, 61) : Color(203, 213, 225);
                    Color p1TxtCol = isPlayingPick ? Color(21, 128, 61) : Color(71, 85, 105);
                    RoundedRectRenderer::draw(window, {rightX, pickY}, {135.f, 40.f}, 8.f, p1Bg, isPlayingPick ? 1.6f : 1.1f, p1Border);
                    CircleShape d1(4.f); d1.setFillColor(p1TxtCol); d1.setPosition({rightX + 14.f, pickY + 16.f}); window.draw(d1);
                    Text p1Txt(isPlayingPick ? boldFont : font, L"\u0110ang chi\u1ebfu", 14);
                    p1Txt.setFillColor(p1TxtCol);
                    FloatRect p1b = p1Txt.getLocalBounds();
                    p1Txt.setPosition({std::round(rightX + 28.f - p1b.position.x), std::round(pickY + (40.f - p1b.size.y) / 2.f - p1b.position.y)});
                    window.draw(p1Txt);

                    Color p2Bg = !isPlayingPick ? Color(254, 243, 199) : Color(248, 250, 252);
                    Color p2Border = !isPlayingPick ? Color(217, 119, 6) : Color(203, 213, 225);
                    Color p2TxtCol = !isPlayingPick ? Color(180, 83, 9) : Color(71, 85, 105);
                    RoundedRectRenderer::draw(window, {rightX + 145.f, pickY}, {135.f, 40.f}, 8.f, p2Bg, !isPlayingPick ? 1.6f : 1.1f, p2Border);
                    CircleShape d2(4.f); d2.setFillColor(p2TxtCol); d2.setPosition({rightX + 159.f, pickY + 16.f}); window.draw(d2);
                    Text p2Txt(!isPlayingPick ? boldFont : font, L"S\u1eafp chi\u1ebfu", 14);
                    p2Txt.setFillColor(p2TxtCol);
                    FloatRect p2b = p2Txt.getLocalBounds();
                    p2Txt.setPosition({std::round(rightX + 173.f - p2b.position.x), std::round(pickY + (40.f - p2b.size.y) / 2.f - p2b.position.y)});
                    window.draw(p2Txt);
                }
            }

            // Poster row (full-width input + preset picker button)
            float posterRowY = startY + 4 * gapY;
            Text lblPoster(boldFont, L"Poster (t\u00ean file ho\u1eb7c \u0111\u01b0\u1eddng d\u1eabn)", 14);
            lblPoster.setFillColor(Color(30, 41, 59));
            lblPoster.setPosition({leftX, posterRowY});
            window.draw(lblPoster);

            bool pAct = (activeInputBox == 8);
            Color pbCol = pAct ? Color(8, 125, 181) : Color(203, 213, 225);
            RoundedRectRenderer::draw(window, {leftX, posterRowY + 20.f}, {490.f, 40.f}, 8.f,
                                      pAct ? Color::White : Color(248, 250, 252), pAct ? 1.8f : 1.2f, pbCol);
            Text pVal(font, inputPosterPath + (pAct ? L"|" : L""), 14);
            pVal.setFillColor(Color(23, 32, 51));
            pVal.setPosition({leftX + 10.f, posterRowY + 29.f});
            window.draw(pVal);

            RoundedRectRenderer::draw(window, {leftX + 500.f, posterRowY + 20.f}, {100.f, 40.f}, 8.f, Color(224, 242, 254), 1.f, Color(147, 197, 253));
            Text pBtnTxt(boldFont, L"Ch\u1ecdn m\u1eabu", 14);
            pBtnTxt.setFillColor(Color(8, 125, 181));
            FloatRect pbb = pBtnTxt.getLocalBounds();
            pBtnTxt.setPosition({
                std::round(leftX + 500.f + (100.f - pbb.size.x) / 2.f - pbb.position.x),
                std::round(posterRowY + 20.f + (40.f - pbb.size.y) / 2.f - pbb.position.y)
            });
            window.draw(pBtnTxt);

            // Buttons
            Color cBg = btnPopupCancelHover ? Color(203, 213, 225) : Color(226, 232, 240);
            RoundedRectRenderer::draw(window, {leftX, popupY + 516.f}, {280.f, 44.f}, 8.f, cBg);
            Text cTxt(boldFont, L"\u0110\u00f3ng", 17);
            cTxt.setFillColor(Color(51, 65, 85));
            FloatRect cb = cTxt.getLocalBounds();
            cTxt.setPosition({
                std::round(leftX + (280.f - cb.size.x) / 2.f - cb.position.x),
                std::round(popupY + 516.f + (44.f - cb.size.y) / 2.f - cb.position.y)
            });
            window.draw(cTxt);

            Color sBg = btnPopupSaveHover ? Color(18, 59, 93) : Color(8, 125, 181);
            RoundedRectRenderer::draw(window, {rightX, popupY + 516.f}, {280.f, 44.f}, 8.f, sBg);
            Text sTxt(boldFont, L"L\u01b0u th\u00f4ng tin", 17);
            sTxt.setFillColor(Color::White);
            FloatRect sb = sTxt.getLocalBounds();
            sTxt.setPosition({
                std::round(rightX + (280.f - sb.size.x) / 2.f - sb.position.x),
                std::round(popupY + 516.f + (44.f - sb.size.y) / 2.f - sb.position.y)
            });
            window.draw(sTxt);
        }
    }

    // 11. TOAST NOTIFICATION
    if (notificationVisible) {
        if (notificationClock.getElapsedTime().asSeconds() > 3.0f) {
            notificationVisible = false;
        } else {
            Text nMsg(boldFont, notificationMessage, 14);
            nMsg.setFillColor(Color::White);
            FloatRect nmb = nMsg.getLocalBounds();
            float notW = max(380.f, min(width - 80.f, nmb.size.x + 48.f));
            float notH = 42.f;
            float notX = position.x + (width - notW) / 2.f;
            float notY = position.y + height - 60.f;
            RoundedRectRenderer::draw(window, {notX, notY}, {notW, notH}, 8.f, Color(22, 163, 74, 245), 1.5f, Color(21, 128, 61));

            nMsg.setPosition({
                std::round(notX + (notW - nmb.size.x) / 2.f - nmb.position.x),
                std::round(notY + (notH - nmb.size.y) / 2.f - nmb.position.y)
            });
            window.draw(nMsg);
        }
    }
}
