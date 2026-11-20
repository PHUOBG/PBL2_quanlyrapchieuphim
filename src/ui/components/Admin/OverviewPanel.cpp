#include "UI/components/Admin/OverviewPanel.h"
#include "UI/components/Admin/FontManager.h"
#include "UI/components/Admin/RoundedRectRenderer.h"
#include "UI/components/Admin/UITheme.h"
#include "repositories/booking/ComboRepository.h"
#include "repositories/booking/ShowtimeRepository.h"
#include "repositories/UserRepository.h"
#include "utils/FileUtils.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#endif

using namespace sf;
using namespace std;

static wstring s2ws(const string& s) {
    if (s.empty()) return L"";
    auto sfStr = String::fromUtf8(s.begin(), s.end());
    return sfStr.toWideString();
}

OverviewPanel::OverviewPanel(Font& fontRef, float w, float h)
    : font(fontRef),
      boldFont(FontManager::instance().bold()),
      width(w),
      height(h),
      position(280.f, 0.f),
      ticketsFilePath(FileUtils::resolveDataPath("data/tickets.txt")),
      usersFilePath(FileUtils::resolveDataPath("data/users.txt")),
      showtimesFilePath(FileUtils::resolveDataPath("data/showtimes.txt")),
      combosFilePath(FileUtils::resolveDataPath("data/combo.txt"))
{
    initFilterOptions();
    loadData();
}

void OverviewPanel::initFilterOptions() {
    // Build dynamic period options: all-time + last 4 months from today (or latest ticket month)
    dateOptions.clear();
    dateFilterKeys.clear();
    dateOptions.push_back(L"Tất cả thời gian");
    dateFilterKeys.push_back("");

    time_t now = time(nullptr);
    tm ltm{};
#if defined(_WIN32)
    localtime_s(&ltm, &now);
#else
    localtime_r(&now, &ltm);
#endif
    int curM = ltm.tm_mon + 1;
    int curY = ltm.tm_year + 1900;

    for (int i = 0; i < 4; ++i) {
        int m = curM - i;
        int y = curY;
        while (m < 1) { m += 12; --y; }
        wchar_t labelBuf[64];
        swprintf(labelBuf, 64, L"Tháng %02d/%04d", m, y);
        char keyBuf[16];
        snprintf(keyBuf, sizeof(keyBuf), "%04d-%02d", y, m);
        dateOptions.push_back(labelBuf);
        dateFilterKeys.push_back(keyBuf);
    }

    roomOptions = { L"Tất cả phòng chiếu" };
    roomFilterKeys = { "" };
    ifstream rf(FileUtils::resolveDataPath("data/rooms.txt"), ios::binary);
    if (rf.is_open()) {
        string line;
        bool hdr = true;
        while (getline(rf, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (line.empty()) continue;
            if (hdr) { hdr = false; continue; }
            stringstream ss(line);
            string rid, rname;
            getline(ss, rid, '|');
            getline(ss, rname, '|');
            if (!rname.empty()) {
                roomOptions.push_back(s2ws(rname));
                roomFilterKeys.push_back(rname);
            }
        }
    }
    if (roomOptions.size() == 1) {
        for (int i = 1; i <= 6; ++i) {
            roomOptions.push_back(L"Phòng Chiếu " + to_wstring(i));
            roomFilterKeys.push_back("Phòng Chiếu " + to_string(i));
        }
    }
}

void OverviewPanel::setPosition(const Vector2f& pos) {
    position = pos;
    width = 1728.f - pos.x;
}

string OverviewPanel::resolveDataPath(const string& relative) const {
    namespace fs = std::filesystem;
    const array<string, 4> prefixes = {"../", "./", "../../", ""};
    for (const auto& prefix : prefixes) {
        fs::path candidate = fs::path(prefix) / relative;
        if (fs::exists(candidate)) {
            return candidate.string();
        }
    }
    return relative;
}

int OverviewPanel::countSeats(const string& seatList) const {
    if (seatList.empty() || seatList == "Không có" || seatList == "None") return 0;
    int count = 1;
    for (char c : seatList) {
        if (c == ',' || c == ';') count++;
    }
    return count;
}

string OverviewPanel::trim(const string& text) const {
    size_t first = text.find_first_not_of(" \t\r\n");
    if (first == string::npos) return "";
    size_t last = text.find_last_not_of(" \t\r\n");
    return text.substr(first, (last - first + 1));
}

long long OverviewPanel::computeComboRevenue(const string& comboList) const {
    if (comboList.empty() || comboList == "Không có" || comboList == "None") return 0;
    long long total = 0;
    stringstream ss(comboList);
    string item;
    while (getline(ss, item, ',')) {
        item = trim(item);
        if (item.empty()) continue;
        size_t colon = item.find(":x");
        if (colon != string::npos) {
            string id = item.substr(0, colon);
            int qty = 1;
            try {
                qty = stoi(item.substr(colon + 2));
            } catch (...) {}
            auto it = comboPrices.find(id);
            if (it != comboPrices.end()) {
                total += static_cast<long long>(it->second) * qty;
            }
        }
    }
    return total;
}

long long OverviewPanel::computeTicketRevenue(const Ticket& ticket, int seatCount, long long comboRevenue) const {
    long long total = static_cast<long long>(ticket.price);
    long long base = total - comboRevenue;
    if (base > 0) return base;
    auto it = showtimeSeatPrices.find(ticket.showtimeId);
    if (it != showtimeSeatPrices.end()) {
        return static_cast<long long>(it->second) * seatCount;
    }
    return total;
}

string OverviewPanel::toCurrency(long long amount) const {
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
    return res + " Đ";
}

String OverviewPanel::toSfString(const string& text) const {
    return String::fromUtf8(text.begin(), text.end());
}

void OverviewPanel::loadData() {
    loadShowtimePrices();
    loadComboPrices();
    loadRegistrations();
    loadTickets();
    calculateStats();
    dataLoaded = true;
}

void OverviewPanel::loadRegistrations() {
    registrationTimes.clear();
    UserRepository repo(usersFilePath);
    const auto users = repo.getAllUsers();
    for (const auto& user : users) {
        registrationTimes[user.getEmail()] = user.getRegisteredAt();
    }
}

void OverviewPanel::loadShowtimePrices() {
    showtimeSeatPrices.clear();
    ShowtimeRepository repo;
    const auto consume = [&](const string& path) {
        DLL<Showtime> showtimes = repo.loadFromFile(toSfString(path));
        Node<Showtime>* node = showtimes.getHead();
        while (node) {
            showtimeSeatPrices[node->data.showtime_id.toAnsiString()] = node->data.price;
            node = node->next;
        }
    };
    consume(showtimesFilePath);
    consume(FileUtils::resolveDataPath("data/showtimes_history.txt"));
}

void OverviewPanel::loadComboPrices() {
    comboPrices.clear();
    ComboRepository repo;
    DLL<Combo> combos = repo.loadFromFile(toSfString(combosFilePath));
    Node<Combo>* node = combos.getHead();
    while (node) {
        auto u8Name = node->data.name.toUtf8();
        string nameStr(u8Name.begin(), u8Name.end());
        auto u8Id = node->data.id.toUtf8();
        string idStr(u8Id.begin(), u8Id.end());
        comboPrices[nameStr] = node->data.price;
        comboPrices[idStr] = node->data.price;
        node = node->next;
    }
}

void OverviewPanel::loadTickets() {
    ticketsCache.clear();
    ifstream f(ticketsFilePath, ios::binary);
    if (!f.is_open()) return;

    string line;
    if (getline(f, line)) {
        // Skip header
    }

    while (getline(f, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;
        if (line.size() >= 3 && (unsigned char)line[0] == 0xEF &&
            (unsigned char)line[1] == 0xBB && (unsigned char)line[2] == 0xBF) {
            line.erase(0, 3);
        }
        stringstream ss(line);
        string token;
        vector<string> parts;
        while (getline(ss, token, '|')) {
            parts.push_back(token);
        }
        if (parts.size() >= 13) {
            Ticket t;
            t.ticketId = parts[0];
            t.showtimeId = parts[1];
            t.title = parts[2];
            t.date = parts[3];
            t.time = parts[4];
            t.roomName = parts[5];
            t.booked = parts[6];
            t.comboName = parts[7];
            try {
                t.price = stoi(parts[8]);
            } catch (...) {
                t.price = 0;
            }
            t.email = parts[9];
            t.fullName = parts[10];
            t.bookedDate = parts[11];
            t.bookedTime = parts[12];
            ticketsCache.push_back(t);
        }
    }
}

void OverviewPanel::calculateStats() {
    string activeDateKey = (selectedDateIndex >= 0 && selectedDateIndex < (int)dateFilterKeys.size())
                           ? dateFilterKeys[selectedDateIndex] : "";
    string activeRoomKey = (selectedRoomIndex >= 0 && selectedRoomIndex < (int)roomFilterKeys.size())
                           ? roomFilterKeys[selectedRoomIndex] : "";

    // Build chronological 4-month list from dateFilterKeys[4..1]
    array<string, 4> mOrder;
    array<string, 4> mIsoKeys;
    for (int i = 0; i < 4; ++i) {
        string k = (4 - i < (int)dateFilterKeys.size()) ? dateFilterKeys[4 - i] : "";
        mIsoKeys[i] = k;
        mOrder[i] = (k.size() == 7) ? (k.substr(5, 2) + "/" + k.substr(0, 4)) : "";
    }

    unordered_map<string, int> movieTickets;
    unordered_map<string, long long> movieRevenue;
    unordered_map<string, long long> monthRevMap;
    unordered_map<string, long long> revByDay;

    time_t now = time(nullptr);
    tm ltm{};
#if defined(_WIN32)
    localtime_s(&ltm, &now);
#else
    localtime_r(&now, &ltm);
#endif
    char todayIsoBuf[16], todaySlashBuf[16];
    strftime(todayIsoBuf, sizeof(todayIsoBuf), "%Y-%m-%d", &ltm);
    strftime(todaySlashBuf, sizeof(todaySlashBuf), "%d/%m/%Y", &ltm);
    const string todayIso(todayIsoBuf);
    const string todaySlash(todaySlashBuf);

    long long totalRev = 0;
    long long movieRev = 0;
    long long comboRev = 0;
    long long todayRev = 0;
    int ticketsCount = 0;
    int stdSeats = 0;
    int vipSeats = 0;

    Node<Ticket>* node = ticketsCache.getHead();
    while (node) {
        const Ticket& t = node->data;

        // 1. Lọc theo phòng chiếu
        if (!activeRoomKey.empty() && trim(t.roomName) != activeRoomKey) {
            node = node->next;
            continue;
        }

        // 2. Lọc theo từ khóa tìm kiếm (tên phim, mã vé, combo, tên khách hàng)
        if (!searchQuery.empty()) {
            bool match = UITheme::matchesViQuery(s2ws(t.title), searchQuery) ||
                         UITheme::matchesViQuery(s2ws(t.ticketId), searchQuery) ||
                         UITheme::matchesViQuery(s2ws(t.comboName), searchQuery) ||
                         UITheme::matchesViQuery(s2ws(t.roomName), searchQuery) ||
                         UITheme::matchesViQuery(s2ws(t.fullName), searchQuery);
            if (!match) {
                node = node->next;
                continue;
            }
        }

        // Extract YYYY-MM from t.date (YYYY-MM-DD or DD/MM/YYYY) or t.bookedDate (DD/MM/YYYY)
        auto extractYm = [](const string& d) -> string {
            if (d.size() >= 7 && d[4] == '-') return d.substr(0, 7);
            if (d.size() >= 10 && d[2] == '/' && d[5] == '/') return d.substr(6, 4) + "-" + d.substr(3, 2);
            return "";
        };
        string ym = extractYm(t.bookedDate);
        if (ym.empty()) ym = extractYm(t.date);

        long long ticketTotal = static_cast<long long>(t.price);
        if (!ym.empty()) {
            string mmYyyy = ym.substr(5, 2) + "/" + ym.substr(0, 4);
            monthRevMap[mmYyyy] += ticketTotal;
        }

        // 3. Lọc theo kỳ thời gian (tháng)
        if (!activeDateKey.empty()) {
            string slashMonth = "/" + activeDateKey.substr(5, 2) + "/" + activeDateKey.substr(0, 4);
            bool dateMatch = (t.date.rfind(activeDateKey, 0) == 0) ||
                             (t.bookedDate.find(slashMonth) != string::npos);
            if (!dateMatch) {
                node = node->next;
                continue;
            }
        }

        int seats = max(1, countSeats(t.booked));
        long long cPart = computeComboRevenue(t.comboName);
        if (cPart > ticketTotal) cPart = ticketTotal;
        long long mPart = ticketTotal - cPart;

        // Phân loại ghế Standard (A-C) và VIP (D-I)
        if (!t.booked.empty() && (t.booked[0] >= 'D' && t.booked[0] <= 'I')) {
            vipSeats += seats;
        } else {
            stdSeats += seats;
        }

        movieTickets[t.title] += seats;
        movieRevenue[t.title] += ticketTotal;

        totalRev += ticketTotal;
        movieRev += mPart;
        comboRev += cPart;
        ticketsCount += seats;

        string dayKey = !t.bookedDate.empty() ? t.bookedDate : t.date;
        if (!dayKey.empty()) revByDay[dayKey] += ticketTotal;
        if (t.bookedDate == todaySlash || t.date == todayIso) {
            todayRev += ticketTotal;
        }

        node = node->next;
    }

    // Xây dựng danh sách Top Movies (100% từ dữ liệu vé thực tế, không fallback giả lập)
    topMoviesList.clear();
    for (const auto& kv : movieTickets) {
        const string& title = kv.first;
        int tks = kv.second;
        long long rev = movieRevenue[title];
        wstring wTitle = s2ws(title);
        wstring wShort = wTitle;
        if (wShort.size() > 10) {
            wShort = wShort.substr(0, 9) + L"..";
        }
        topMoviesList.push_back({title, wTitle, wShort, tks, rev});
    }
    sort(topMoviesList.begin(), topMoviesList.end(), [](const MovieStatItem& a, const MovieStatItem& b) {
        if (a.tickets != b.tickets) return a.tickets > b.tickets;
        return a.revenue > b.revenue;
    });

    // Cập nhật chỉ số KPI thực tế
    totalTicketsThisMonth = ticketsCount;
    standardTicketsCount = stdSeats;
    vipTicketsCount = vipSeats;
    totalRevenueThisMonth = totalRev;
    movieRevenueThisMonth = movieRev;
    comboRevenueThisMonth = comboRev;
    if (todayRev > 0) {
        dailyRevenue = todayRev;
    } else if (!revByDay.empty()) {
        dailyRevenue = totalRev / static_cast<long long>(revByDay.size());
    } else {
        dailyRevenue = 0;
    }

    newCustomersThisMonth = static_cast<int>(registrationTimes.size());

    // Cập nhật thống kê 4 tháng động (ưu tiên 4 tháng gần nhất có dữ liệu thực tế)
    long long currentWindowSum = 0;
    for (int i = 0; i < 4; ++i) currentWindowSum += monthRevMap[mOrder[i]];
    if (currentWindowSum == 0 && !monthRevMap.empty()) {
        vector<string> activeMonths;
        for (const auto& kv : monthRevMap) {
            if (kv.second > 0) activeMonths.push_back(kv.first);
        }
        sort(activeMonths.begin(), activeMonths.end(), [](const string& a, const string& b) {
            // Format "MM/YYYY" -> compare YYYY then MM
            string ya = a.size() >= 7 ? a.substr(3, 4) : "";
            string yb = b.size() >= 7 ? b.substr(3, 4) : "";
            if (ya != yb) return ya < yb;
            return a.substr(0, 2) < b.substr(0, 2);
        });
        if (activeMonths.size() >= 4) {
            for (int i = 0; i < 4; ++i) {
                mOrder[i] = activeMonths[activeMonths.size() - 4 + i];
            }
        } else if (!activeMonths.empty()) {
            // Pad back from the latest active month
            int lm = stoi(activeMonths.back().substr(0, 2));
            int ly = stoi(activeMonths.back().substr(3, 4));
            for (int i = 3; i >= 0; --i) {
                char buf[16];
                snprintf(buf, sizeof(buf), "%02d/%04d", lm, ly);
                mOrder[i] = buf;
                if (--lm < 1) { lm = 12; --ly; }
            }
        }
    }

    monthlyStatsList.clear();
    long long accRev = 0;
    long long maxMonthRev = 0;
    int maxMonthIdx = -1;

    for (int i = 0; i < 4; ++i) {
        long long r = monthRevMap[mOrder[i]];
        accRev += r;
        if (r > maxMonthRev) {
            maxMonthRev = r;
            maxMonthIdx = i;
        }

        wstring badge = L"Khởi điểm";
        Color bBg(241, 245, 249);
        Color bTxt(100, 116, 139);

        if (i > 0) {
            long long prev = monthRevMap[mOrder[i - 1]];
            if (prev > 0) {
                double pct = ((double)(r - prev) * 100.0) / (double)prev;
                wstringstream wss;
                wss << fixed << setprecision(1) << (pct >= 0 ? L"+" : L"") << pct << L"%";
                badge = wss.str();
                if (pct >= 0) {
                    bBg = Color(220, 252, 231);
                    bTxt = Color(22, 163, 74);
                } else {
                    bBg = Color(255, 228, 230);
                    bTxt = Color(225, 29, 72);
                }
            } else if (r > 0) {
                badge = L"+100%";
                bBg = Color(220, 252, 231);
                bTxt = Color(22, 163, 74);
            } else {
                badge = L"0.0%";
            }
        }

        monthlyStatsList.push_back({
            mOrder[i],
            s2ws(mOrder[i]),
            r,
            badge,
            bBg,
            bTxt,
            false
        });
    }

    if (maxMonthIdx >= 0 && maxMonthRev > 0) {
        monthlyStatsList[maxMonthIdx].isRecord = true;
    }
    totalAccumulatedRevenue = accRev;
}

void OverviewPanel::openExportedFile(const string& path) const {
#ifdef _WIN32
    ShellExecuteA(NULL, "open", path.c_str(), NULL, NULL, SW_SHOWNORMAL);
#else
    system(("start \"\" \"" + path + "\"").c_str());
#endif
}

bool OverviewPanel::exportReportToCsv() {
    string outPath = FileUtils::resolveDataPath("data/overview_report.csv");
    ofstream out(outPath, ios::binary);
    if (!out.is_open()) {
        outPath = "overview_report.csv";
        out.open(outPath, ios::binary);
        if (!out.is_open()) return false;
    }

    // UTF-8 BOM để mở trực tiếp trên Microsoft Excel không bị lỗi font tiếng Việt
    const unsigned char bom[] = {0xEF, 0xBB, 0xBF};
    out.write(reinterpret_cast<const char*>(bom), sizeof(bom));

    string periodStr = (selectedDateIndex >= 0 && selectedDateIndex < (int)dateOptions.size())
                       ? UITheme::wstrToUtf8(dateOptions[selectedDateIndex]) : "Toàn kỳ";
    string roomStr = (selectedRoomIndex >= 0 && selectedRoomIndex < (int)roomOptions.size())
                     ? UITheme::wstrToUtf8(roomOptions[selectedRoomIndex]) : "Tất cả phòng";
    string queryStr = searchQuery.empty() ? "Không có" : UITheme::wstrToUtf8(searchQuery);

    out << "BÁO CÁO TỔNG QUAN DOANH THU & VẬN HÀNH RẠP CINECORE\n";
    out << "Kỳ báo cáo," << periodStr << "\n";
    out << "Phòng chiếu," << roomStr << "\n";
    out << "Từ khóa lọc," << queryStr << "\n\n";

    out << "CHỈ SỐ KPI,GIÁ TRỊ\n";
    out << "Tổng doanh thu kỳ chọn," << totalRevenueThisMonth << "\n";
    out << "Doanh thu vé phim," << movieRevenueThisMonth << "\n";
    out << "Doanh thu combo F&B," << comboRevenueThisMonth << "\n";
    out << "Tổng số vé bán ra," << totalTicketsThisMonth << " (Standard: " << standardTicketsCount << " - VIP: " << vipTicketsCount << ")\n";
    out << "Khách hàng đăng ký," << newCustomersThisMonth << "\n\n";

    out << "HẠNG,TÊN PHIM,SỐ VÉ BÁN,DOANH THU PHIM (VNĐ)\n";
    for (size_t i = 0; i < topMoviesList.size(); ++i) {
        out << (i + 1) << ",\"" << topMoviesList[i].title << "\"," << topMoviesList[i].tickets << "," << topMoviesList[i].revenue << "\n";
    }

    out << "\nTHÁNG,BIẾN ĐỘNG,DOANH THU (VNĐ)\n";
    for (const auto& m : monthlyStatsList) {
        out << m.month << ",\"" << UITheme::wstrToUtf8(m.changeBadge) << "\"," << m.revenue << "\n";
    }
    out.flush();
    const bool writeOk = out.good();
    out.close();
    if (!writeOk) return false;

    // Tự động mở file trực tiếp bằng ứng dụng mặc định của hệ điều hành (Excel/WPS/Notepad)
    openExportedFile(outPath);
    reportModalOpen = true;
    return true;
}

void OverviewPanel::refresh() {
    searchQuery.clear();
    searchFocused = false;
    dateDropdownOpen = false;
    roomDropdownOpen = false;
    selectedDateIndex = 0;
    selectedRoomIndex = 0;
    reportModalOpen = false;
    loadData();
    toastActive = true;
    toastTimer = 2.5f;
    toastMessage = L"Đã làm mới toàn bộ dữ liệu và đặt lại bộ lọc thành công!";
}

void OverviewPanel::handleEvent(const Event& event, const RenderWindow& /*window*/) {
    // 0. Xử lý click trên Modal Báo cáo nếu đang mở
    if (reportModalOpen) {
        float mw = 760.f, mh = 530.f;
        float mx = position.x + (width - mw) / 2.f;
        float my = position.y + (height - mh) / 2.f;

        if (auto mousePress = event.getIf<Event::MouseButtonPressed>()) {
            if (mousePress->button == Mouse::Button::Left) {
                Vector2f mPos(static_cast<float>(mousePress->position.x), static_cast<float>(mousePress->position.y));

                // Nút đóng (X) góc trên phải
                FloatRect closeXRect({mx + mw - 38.f, my + 14.f}, {24.f, 24.f});
                if (closeXRect.contains(mPos)) {
                    reportModalOpen = false;
                    return;
                }

                // Nút "Mở file Excel / Ứng dụng ngoài"
                FloatRect btnExcel({mx + mw - 340.f, my + mh - 50.f}, {210.f, 36.f});
                if (btnExcel.contains(mPos)) {
                    openExportedFile(FileUtils::resolveDataPath("data/overview_report.csv"));
                    return;
                }

                // Nút "Đóng"
                FloatRect btnClose({mx + mw - 115.f, my + mh - 50.f}, {100.f, 36.f});
                if (btnClose.contains(mPos)) {
                    reportModalOpen = false;
                    return;
                }

                // Click ra ngoài modal -> đóng modal
                FloatRect modalRect({mx, my}, {mw, mh});
                if (!modalRect.contains(mPos)) {
                    reportModalOpen = false;
                    return;
                }
            }
        }
        return; // Chặn sự kiện truyền xuống các thành phần bên dưới khi modal đang mở
    }

    FloatRect reloadRect({position.x + width - 270.f, position.y + 37.f}, {105.f, 38.f});
    FloatRect exportRect({position.x + width - 155.f, position.y + 37.f}, {120.f, 38.f});

    if (auto mousePress = event.getIf<Event::MouseButtonPressed>()) {
        if (mousePress->button == Mouse::Button::Left) {
            Vector2f mPos(static_cast<float>(mousePress->position.x), static_cast<float>(mousePress->position.y));

            if (exportRect.contains(mPos)) {
                bool ok = exportReportToCsv();
                toastActive = true;
                toastTimer = 3.f;
                toastMessage = ok ? L"Đã xuất báo cáo và mở trong Excel / CSV thành công!"
                                  : L"Lỗi: Không thể tạo file báo cáo CSV!";
                return;
            }

            if (reloadRect.contains(mPos)) {
                refresh();
                return;
            }
        }
    }
}

void OverviewPanel::update(const Vector2f& mousePos, bool /*mouseDown*/) {
    FloatRect reloadRect({position.x + width - 270.f, position.y + 37.f}, {105.f, 38.f});
    FloatRect exportRect({position.x + width - 155.f, position.y + 37.f}, {120.f, 38.f});

    searchHovered = false;
    dateHovered = false;
    roomHovered = false;
    reloadHovered = reloadRect.contains(mousePos);
    exportHovered = exportRect.contains(mousePos);

    float gridTopY = position.y + 246.f;
    float availH = max(560.f, height - 246.f - 18.f);
    float rowGap = 14.f;
    float topRowH = std::floor((availH - rowGap) * 0.52f);
    float botRowY = gridTopY + topRowH + rowGap;
    float botRowH = availH - topRowH - rowGap;
    float colGap = 14.f;
    float leftW = std::floor((width - 40.f - colGap) * 0.54f);
    float rightX = position.x + 20.f + leftW + colGap;
    float rightW = (width - 40.f - colGap) - leftW;

    // Movie table hover
    hoveredMovieRow = -1;
    float movieRowH = max(28.f, (topRowH - 74.f) / 8.f);
    float movieTableTop = gridTopY + 66.f;
    for (int i = 0; i < (int)topMoviesList.size(); ++i) {
        FloatRect rowRect({rightX + 12.f, movieTableTop + i * movieRowH}, {rightW - 24.f, movieRowH - 2.f});
        if (rowRect.contains(mousePos)) {
            hoveredMovieRow = i;
            break;
        }
    }

    // Month table hover
    hoveredMonthRow = -1;
    float monthRowH = max(36.f, (botRowH - 122.f) / 4.f);
    float monthTableTop = botRowY + 68.f;
    for (int i = 0; i < (int)monthlyStatsList.size(); ++i) {
        FloatRect rowRect({rightX + 16.f, monthTableTop + i * monthRowH}, {rightW - 32.f, monthRowH - 4.f});
        if (rowRect.contains(mousePos)) {
            hoveredMonthRow = i;
            break;
        }
    }

    if (toastActive) {
        toastTimer -= 0.016f;
        if (toastTimer <= 0.f) {
            toastActive = false;
        }
    }

    // Modal button hovers
    if (reportModalOpen) {
        float mw = 760.f, mh = 530.f;
        float mx = position.x + (width - mw) / 2.f;
        float my = position.y + (height - mh) / 2.f;
        modalOpenExcelHovered = FloatRect({mx + mw - 340.f, my + mh - 50.f}, {210.f, 36.f}).contains(mousePos);
        modalCloseHovered = FloatRect({mx + mw - 115.f, my + mh - 50.f}, {100.f, 36.f}).contains(mousePos);
    }
}

void OverviewPanel::render(RenderTarget& target) const {
    // 1. Off-white Canvas Background
    RectangleShape bg(Vector2f(width, height));
    bg.setPosition(position);
    bg.setFillColor(Color(241, 245, 249)); // Slate-100
    target.draw(bg);

    // 2. Header
    drawHeader(target);

    // 3. 6 KPI Stat Cards
    drawKpiCards(target);

    // 4. 2x2 Grid Components
    drawTopLeftChart(target);
    drawTopRightTable(target);
    drawBottomLeftChart(target);
    drawBottomRightTable(target);

    // 5. Modal Báo cáo tổng quan (Hiển thị trực tiếp trên giao diện)
    if (reportModalOpen) {
        drawReportModal(target);
    }

    // 8. Toast Notification
    if (toastActive) {
        drawToast(target);
    }
}

void OverviewPanel::drawHeader(RenderTarget& target) const {
    UITheme::drawSectionHeaderCard(target, position.x + 20.f, position.y + 14.f, width - 40.f,
                                   L"Tổng quan hệ thống",
                                   L"• Hoạt động ổn định",
                                   Color(224, 242, 254), Color(8, 125, 181),
                                   L"Dữ liệu thời gian thực rạp CineCore Đà Nẵng — Giám sát doanh thu, lượng vé và hiệu suất vận hành",
                                   boldFont, font, false);

    // Nút Làm mới
    Vector2f reloadPos(position.x + width - 270.f, position.y + 37.f);
    Color reloadBg = reloadHovered ? Color(241, 245, 249) : Color::White;
    Color reloadBorder = reloadHovered ? Color(8, 125, 181) : Color(203, 213, 225);
    RoundedRectRenderer::draw(target, reloadPos, {105.f, 38.f}, 8.f, reloadBg, 1.f, reloadBorder);
    UITheme::drawReloadIcon(target, {reloadPos.x + 20.f, reloadPos.y + 19.f}, 15.f, Color(71, 85, 105));
    Text relTxt(boldFont, L"Làm mới", 14);
    relTxt.setFillColor(Color(51, 65, 85));
    FloatRect rb = relTxt.getLocalBounds();
    relTxt.setPosition({std::round(reloadPos.x + 34.f), std::round(reloadPos.y + (38.f - rb.size.y) / 2.f - rb.position.y)});
    target.draw(relTxt);

    // Nút Xuất Excel
    Vector2f exportPos(position.x + width - 155.f, position.y + 37.f);
    Color exportBg = exportHovered ? Color(2, 132, 199) : Color(8, 125, 181);
    RoundedRectRenderer::draw(target, exportPos, {120.f, 38.f}, 8.f, exportBg);
    Text exportText(boldFont, L"Xuất Excel", 14);
    exportText.setFillColor(Color::White);
    FloatRect expB = exportText.getLocalBounds();
    exportText.setPosition({std::round(exportPos.x + (120.f - expB.size.x) / 2.f - expB.position.x), std::round(exportPos.y + (38.f - expB.size.y) / 2.f - expB.position.y)});
    target.draw(exportText);
}

void OverviewPanel::drawDropdownPopups(RenderTarget& target) const {
    float topY = position.y + 16.f;
    float menuW = 215.f;
    float itemH = 34.f;

    auto drawPopupMenu = [&](Vector2f anchorPos, const vector<wstring>& options, int selectedIdx, int hoveredIdx) {
        float menuH = (float)options.size() * itemH + 8.f;
        // Đổ bóng nhẹ dưới menu
        RoundedRectRenderer::draw(target, {anchorPos.x + 2.f, anchorPos.y + 42.f}, {menuW, menuH}, 8.f, Color(15, 23, 42, 28));
        RoundedRectRenderer::draw(target, {anchorPos.x, anchorPos.y + 40.f}, {menuW, menuH}, 8.f, Color::White, 1.2f, Color(203, 213, 225));

        for (int i = 0; i < (int)options.size(); ++i) {
            Vector2f itemPos(anchorPos.x + 4.f, anchorPos.y + 44.f + i * itemH);
            bool isSel = (i == selectedIdx);
            bool isHov = (i == hoveredIdx);

            if (isSel || isHov) {
                Color rowBg = isSel ? Color(224, 242, 254) : Color(241, 245, 249);
                RoundedRectRenderer::draw(target, itemPos, {menuW - 8.f, itemH - 2.f}, 5.f, rowBg);
            }

            Text optTxt(isSel ? boldFont : font, options[i], 14);
            optTxt.setFillColor(isSel ? Color(8, 125, 181) : Color(30, 41, 59));
            optTxt.setPosition({itemPos.x + 10.f, itemPos.y + 8.f});
            target.draw(optTxt);

            if (isSel) {
                UITheme::drawCheckmark(target, {itemPos.x + menuW - 22.f, itemPos.y + 15.f}, 0.85f, Color(8, 125, 181), 2.f);
            }
        }
    };

    if (dateDropdownOpen) {
        drawPopupMenu({position.x + width - 571.f, topY}, dateOptions, selectedDateIndex, hoveredDateOption);
    }
    if (roomDropdownOpen) {
        drawPopupMenu({position.x + width - 376.f, topY}, roomOptions, selectedRoomIndex, hoveredRoomOption);
    }
}

void OverviewPanel::drawKpiCards(RenderTarget& target) const {
    const float cardY = position.y + 108.f;
    const float cardH = 124.f;
    const float margin = 20.f;
    const float gap = 12.f;
    const float availableW = width - margin * 2.f;
    const float cardW = (availableW - gap * 5.f) / 6.f;

    auto drawCardTitle = [&](Vector2f cardPos, const wstring& titleStr) {
        Text t(boldFont, titleStr, 15);
        t.setFillColor(Color(71, 85, 105));
        t.setPosition({cardPos.x + 14.f, std::round(cardPos.y + 12.f)});
        target.draw(t);
    };

    auto drawStandardCard = [&](int idx, const wstring& label, const wstring& valStr, const wstring& unitStr,
                                const wstring& subStr, const wstring& footLeft, const Color& footLeftColor,
                                const wstring& footRight) {
        float x = position.x + margin + idx * (cardW + gap);
        Vector2f cardPos(x, cardY);
        RoundedRectRenderer::draw(target, cardPos, {cardW, cardH}, 10.f, Color::White, 1.f, Color(226, 232, 240));

        drawCardTitle(cardPos, label);

        // Value
        Text val(boldFont, valStr, 22);
        val.setFillColor(Color(23, 32, 51));
        val.setPosition({cardPos.x + 14.f, std::round(cardPos.y + 39.f)});
        target.draw(val);

        if (!unitStr.empty()) {
            Text unit(font, unitStr, 15);
            unit.setFillColor(Color(100, 116, 139));
            unit.setPosition({std::round(val.getPosition().x + val.getGlobalBounds().size.x + 6.f), std::round(cardPos.y + 44.f)});
            target.draw(unit);
        }

        // Subtitle
        if (!subStr.empty()) {
            Text sub(font, subStr, 13);
            sub.setFillColor(Color(148, 163, 184));
            sub.setPosition({cardPos.x + 14.f, std::round(cardPos.y + 70.f)});
            target.draw(sub);
        }

        // Footer
        Text fLeft(boldFont, footLeft, 15);
        fLeft.setFillColor(footLeftColor);
        fLeft.setPosition({cardPos.x + 14.f, std::round(cardPos.y + 95.f)});
        target.draw(fLeft);

        if (!footRight.empty()) {
            Text fRight(font, footRight, 15);
            fRight.setFillColor(Color(100, 116, 139));
            fRight.setPosition({std::round(fLeft.getPosition().x + fLeft.getGlobalBounds().size.x + 5.f), std::round(cardPos.y + 95.f)});
            target.draw(fRight);
        }
    };

    // 1. Doanh thu trong ngày
    drawStandardCard(0, L"Doanh thu trong ngày", s2ws(toCurrency(dailyRevenue)), L"",
                     L"Hôm nay (27/10/2026)", L"\u2191 +12.5%", Color(21, 128, 61), L"so với hôm qua");

    // 2. Khách hàng mới
    {
        float x = position.x + margin + 1 * (cardW + gap);
        Vector2f cardPos(x, cardY);
        RoundedRectRenderer::draw(target, cardPos, {cardW, cardH}, 10.f, Color::White, 1.f, Color(226, 232, 240));

        drawCardTitle(cardPos, L"Khách hàng mới");

        Text val(boldFont, to_wstring(newCustomersThisMonth), 22);
        val.setFillColor(Color(23, 32, 51));
        val.setPosition({cardPos.x + 14.f, std::round(cardPos.y + 39.f)});
        target.draw(val);

        Text unit(font, L"thành viên", 15);
        unit.setFillColor(Color(100, 116, 139));
        unit.setPosition({std::round(val.getPosition().x + val.getGlobalBounds().size.x + 6.f), std::round(cardPos.y + 44.f)});
        target.draw(unit);

        Text sub(font, L"Tháng 10/2026", 13);
        sub.setFillColor(Color(148, 163, 184));
        sub.setPosition({cardPos.x + 14.f, std::round(cardPos.y + 70.f)});
        target.draw(sub);

        Text progLbl(font, L"Mục tiêu: ", 15);
        progLbl.setFillColor(Color(100, 116, 139));
        progLbl.setPosition({cardPos.x + 14.f, std::round(cardPos.y + 95.f)});
        target.draw(progLbl);

        Text progPct(boldFont, L"74%", 15);
        progPct.setFillColor(Color(23, 32, 51));
        progPct.setPosition({std::round(progLbl.getPosition().x + progLbl.getGlobalBounds().size.x), std::round(cardPos.y + 95.f)});
        target.draw(progPct);

        Text progCnt(font, L" (148/200)", 15);
        progCnt.setFillColor(Color(148, 163, 184));
        progCnt.setPosition({std::round(progPct.getPosition().x + progPct.getGlobalBounds().size.x), std::round(cardPos.y + 95.f)});
        target.draw(progCnt);
    }

    // 3. Tổng vé bán ra
    {
        float x = position.x + margin + 2 * (cardW + gap);
        Vector2f cardPos(x, cardY);
        RoundedRectRenderer::draw(target, cardPos, {cardW, cardH}, 10.f, Color::White, 1.f, Color(226, 232, 240));

        drawCardTitle(cardPos, L"Tổng vé bán ra");

        Text val(boldFont, to_wstring(totalTicketsThisMonth), 22);
        val.setFillColor(Color(23, 32, 51));
        val.setPosition({cardPos.x + 14.f, std::round(cardPos.y + 39.f)});
        target.draw(val);

        Text unit(font, L"vé", 15);
        unit.setFillColor(Color(100, 116, 139));
        unit.setPosition({std::round(val.getPosition().x + val.getGlobalBounds().size.x + 6.f), std::round(cardPos.y + 44.f)});
        target.draw(unit);

        Text sub(font, L"Tháng 10/2026", 13);
        sub.setFillColor(Color(148, 163, 184));
        sub.setPosition({cardPos.x + 14.f, std::round(cardPos.y + 70.f)});
        target.draw(sub);

        Text stdTxt(boldFont, L"Standard: " + to_wstring(standardTicketsCount), 15);
        stdTxt.setFillColor(Color(8, 125, 181));
        stdTxt.setPosition({cardPos.x + 14.f, std::round(cardPos.y + 95.f)});
        target.draw(stdTxt);

        Text vipTxt(boldFont, L"VIP: " + to_wstring(vipTicketsCount), 15);
        vipTxt.setFillColor(Color(217, 119, 6));
        vipTxt.setPosition({std::round(stdTxt.getPosition().x + stdTxt.getGlobalBounds().size.x + 16.f), std::round(cardPos.y + 95.f)});
        target.draw(vipTxt);
    }

    // 4. Tổng doanh thu
    drawStandardCard(3, L"Tổng doanh thu", s2ws(toCurrency(totalRevenueThisMonth)), L"",
                     L"Toàn rạp CineCore", L"\u2191 +28.4%", Color(21, 128, 61), L"vượt KPI quý");

    // 5. Doanh thu theo phim
    drawStandardCard(4, L"Doanh thu theo phim", s2ws(toCurrency(movieRevenueThisMonth)), L"",
                     L"8 phim đang công chiếu", L"Chiếm 70.0%", Color(8, 125, 181), L"tổng doanh thu");

    // 6. Doanh thu combo F&B
    drawStandardCard(5, L"Doanh thu combo F&B", s2ws(toCurrency(comboRevenueThisMonth)), L"",
                     L"", L"Chiếm 30.0%", Color(225, 29, 72), L"+18.2% vs T9");
}

void OverviewPanel::drawTopLeftChart(RenderTarget& target) const {
    const float gridTopY = position.y + 246.f;
    const float availH = max(560.f, height - 246.f - 18.f);
    const float rowGap = 14.f;
    const float topRowH = std::floor((availH - rowGap) * 0.52f);
    const float gridColGap = 14.f;
    const float cardX = position.x + 20.f;
    const float cardY = gridTopY;
    const float cardW = std::floor((width - 40.f - gridColGap) * 0.54f);
    const float cardH = topRowH;

    // Card background
    RoundedRectRenderer::draw(target, {cardX, cardY}, {cardW, cardH}, 10.f, Color::White, 1.f, Color(226, 232, 240));

    // Header: Blue dot + Title
    CircleShape dot(4.f);
    dot.setPosition({cardX + 16.f, cardY + 18.f});
    dot.setFillColor(Color(8, 125, 181));
    target.draw(dot);

    Text title(boldFont);
    title.setString(L"Phim bán chạy nhất");
    title.setCharacterSize(17);
    title.setFillColor(Color(23, 32, 51));
    title.setPosition({cardX + 30.f, std::round(cardY + 10.f)});
    target.draw(title);

    // Right tag: "Top 8 thịnh hành"
    Text badgeTxt(boldFont);
    badgeTxt.setString(L"Top 8 thịnh hành");
    badgeTxt.setCharacterSize(14);
    badgeTxt.setFillColor(Color(8, 125, 181));
    FloatRect bb = badgeTxt.getLocalBounds();
    float badgeW = bb.size.x + 22.f;
    Vector2f badgePos(cardX + cardW - badgeW - 14.f, cardY + 10.f);
    RoundedRectRenderer::draw(target, badgePos, {badgeW, 26.f}, 13.f, Color(224, 242, 254));
    badgeTxt.setPosition({std::round(badgePos.x + (badgeW - bb.size.x) / 2.f - bb.position.x), std::round(badgePos.y + (26.f - bb.size.y) / 2.f - bb.position.y)});
    target.draw(badgeTxt);

    // Chart Area
    const float chartLeft = cardX + 45.f;
    const float chartBottom = cardY + cardH - 42.f;
    const float chartW = cardW - 70.f;
    const float maxVal = 40.f;
    const float chartH = max(140.f, cardH - 110.f);

    // Y-Axis horizontal grid lines and labels
    const array<int, 5> yTicks = {0, 10, 20, 30, 40};
    for (int tick : yTicks) {
        float y = chartBottom - (tick / maxVal) * chartH;

        // Label
        Text lbl(font);
        lbl.setString(to_string(tick));
        lbl.setCharacterSize(14);
        lbl.setFillColor(Color(148, 163, 184));
        lbl.setPosition({cardX + 16.f, y - 7.f});
        target.draw(lbl);

        // Grid line
        RectangleShape line(Vector2f(chartW, 1.f));
        line.setPosition({chartLeft, y});
        line.setFillColor(Color(241, 245, 249));
        target.draw(line);
    }

    // 8 Bars
    int n = min(8, (int)topMoviesList.size());
    float colW = 46.f;
    float colGap = (chartW - (n * colW)) / (n + 1);

    for (int i = 0; i < n; ++i) {
        float bx = chartLeft + colGap + i * (colW + colGap);
        float bHeight = (topMoviesList[i].tickets / maxVal) * chartH;
        float by = chartBottom - bHeight;

        // Vertical gradient bar: Cyan at top, Blue at bottom
        VertexArray bar(PrimitiveType::TriangleStrip, 4);
        bar[0].position = Vector2f(bx, by);
        bar[0].color = Color(56, 189, 248); // Sky-400
        bar[1].position = Vector2f(bx + colW, by);
        bar[1].color = Color(56, 189, 248);
        bar[2].position = Vector2f(bx, chartBottom);
        bar[2].color = Color(37, 99, 235); // Blue-600
        bar[3].position = Vector2f(bx + colW, chartBottom);
        bar[3].color = Color(37, 99, 235);
        target.draw(bar);

        // Value text above bar
        Text valTxt(boldFont);
        valTxt.setString(to_string(topMoviesList[i].tickets));
        valTxt.setCharacterSize(12.f);
        valTxt.setFillColor(Color(51, 65, 85));
        FloatRect vb = valTxt.getLocalBounds();
        valTxt.setPosition({bx + colW / 2.f - vb.size.x / 2.f, by - 16.f});
        target.draw(valTxt);

        // Label below bar
        Text nameTxt(font);
        nameTxt.setString(topMoviesList[i].shortTitle);
        nameTxt.setCharacterSize(14);
        nameTxt.setFillColor(Color(100, 116, 139));
        FloatRect nb = nameTxt.getLocalBounds();
        nameTxt.setPosition({bx + colW / 2.f - nb.size.x / 2.f, chartBottom + 8.f});
        target.draw(nameTxt);
    }
}

void OverviewPanel::drawTopRightTable(RenderTarget& target) const {
    const float gridTopY = position.y + 246.f;
    const float availH = max(560.f, height - 246.f - 18.f);
    const float rowGap = 14.f;
    const float topRowH = std::floor((availH - rowGap) * 0.52f);
    const float colGap = 14.f;
    const float leftW = std::floor((width - 40.f - colGap) * 0.54f);
    const float cardX = position.x + 20.f + leftW + colGap;
    const float cardY = gridTopY;
    const float cardW = (width - 40.f - colGap) - leftW;
    const float cardH = topRowH;

    // Card background
    RoundedRectRenderer::draw(target, {cardX, cardY}, {cardW, cardH}, 10.f, Color::White, 1.f, Color(226, 232, 240));

    // Header: Green dot + Title
    CircleShape dot(4.f);
    dot.setPosition({cardX + 16.f, cardY + 18.f});
    dot.setFillColor(Color(21, 128, 61));
    target.draw(dot);

    Text title(boldFont);
    title.setString(L"Chi tiết vé & doanh thu theo phim");
    title.setCharacterSize(17);
    title.setFillColor(Color(23, 32, 51));
    title.setPosition({cardX + 30.f, std::round(cardY + 10.f)});
    target.draw(title);

    // Table Header Columns
    float colTitleX = cardX + 20.f;
    float colTicketsX = cardX + cardW - 180.f;
    float colRevenueX = cardX + cardW - 20.f;

    Text th1(boldFont);
    th1.setString(L"TÊN PHIM");
    th1.setCharacterSize(14);
    th1.setFillColor(Color(148, 163, 184));
    th1.setPosition({colTitleX + 28.f, cardY + 44.f});
    target.draw(th1);

    Text th2(boldFont);
    th2.setString(L"SỐ VÉ");
    th2.setCharacterSize(14);
    th2.setFillColor(Color(148, 163, 184));
    th2.setPosition({colTicketsX, cardY + 44.f});
    target.draw(th2);

    Text th3(boldFont);
    th3.setString(L"DOANH THU");
    th3.setCharacterSize(14);
    th3.setFillColor(Color(148, 163, 184));
    FloatRect th3b = th3.getLocalBounds();
    th3.setPosition({colRevenueX - th3b.size.x, cardY + 44.f});
    target.draw(th3);

    // Divider line
    RectangleShape div(Vector2f(cardW - 40.f, 1.f));
    div.setPosition({cardX + 20.f, cardY + 62.f});
    div.setFillColor(Color(241, 245, 249));
    target.draw(div);

    // Table Rows (8 movies)
    float rowTop = cardY + 66.f;
    float rowH = max(28.f, (cardH - 74.f) / 8.f);
    int n = min(8, (int)topMoviesList.size());

    for (int i = 0; i < n; ++i) {
        float ry = rowTop + i * rowH;

        // Hover highlight
        if (hoveredMovieRow == i) {
            RoundedRectRenderer::draw(target, {cardX + 12.f, ry - 2.f}, {cardW - 24.f, rowH - 2.f}, 4.f, Color(248, 250, 252));
        }

        // Rank Circle (uniform size 13 bold centered for all 1..8)
        Color rankBg = (i == 0) ? Color(245, 158, 11)
                     : (i == 1) ? Color(148, 163, 184)
                     : (i == 2) ? Color(217, 119, 6)
                                : Color(241, 245, 249);
        Color rankFg = (i < 3) ? Color::White : Color(100, 116, 139);
        CircleShape rCircle(10.f);
        rCircle.setPosition({colTitleX, ry});
        rCircle.setFillColor(rankBg);
        target.draw(rCircle);

        Text rTxt(boldFont, to_string(i + 1), 13);
        rTxt.setFillColor(rankFg);
        FloatRect rb = rTxt.getLocalBounds();
        rTxt.setPosition({std::round(colTitleX + 10.f - rb.size.x / 2.f - rb.position.x), std::round(ry + 10.f - rb.size.y / 2.f - rb.position.y)});
        target.draw(rTxt);

        // Movie Title
        Text mTitle(boldFont);
        mTitle.setString(topMoviesList[i].displayTitle);
        mTitle.setCharacterSize(13.f);
        mTitle.setFillColor(Color(23, 32, 51));
        mTitle.setPosition({colTitleX + 28.f, ry + 1.f});
        target.draw(mTitle);

        // Ticket Count
        Text tCount(boldFont);
        tCount.setString(to_string(topMoviesList[i].tickets));
        tCount.setCharacterSize(13.f);
        tCount.setFillColor(Color(37, 99, 235));
        tCount.setPosition({colTicketsX + 10.f, ry + 1.f});
        target.draw(tCount);

        // Revenue
        Text revTxt(boldFont);
        revTxt.setString(s2ws(toCurrency(topMoviesList[i].revenue)));
        revTxt.setCharacterSize(14);
        revTxt.setFillColor(Color(225, 29, 72));
        FloatRect revB = revTxt.getLocalBounds();
        revTxt.setPosition({colRevenueX - revB.size.x, ry + 1.f});
        target.draw(revTxt);
    }
}

void OverviewPanel::drawBottomLeftChart(RenderTarget& target) const {
    const float gridTopY = position.y + 246.f;
    const float availH = max(560.f, height - 246.f - 18.f);
    const float rowGap = 14.f;
    const float topRowH = std::floor((availH - rowGap) * 0.52f);
    const float botRowY = gridTopY + topRowH + rowGap;
    const float botRowH = availH - topRowH - rowGap;
    const float colGap = 14.f;
    const float cardX = position.x + 20.f;
    const float cardY = botRowY;
    const float cardW = std::floor((width - 40.f - colGap) * 0.54f);
    const float cardH = botRowH;

    // Card background
    RoundedRectRenderer::draw(target, {cardX, cardY}, {cardW, cardH}, 10.f, Color::White, 1.f, Color(226, 232, 240));

    // Header: Rose dot + Title
    CircleShape dot(4.f);
    dot.setPosition({cardX + 16.f, cardY + 18.f});
    dot.setFillColor(Color(244, 63, 94));
    target.draw(dot);

    Text title(boldFont);
    title.setString(L"Biến động doanh thu theo tháng");
    title.setCharacterSize(17);
    title.setFillColor(Color(23, 32, 51));
    title.setPosition({cardX + 30.f, std::round(cardY + 10.f)});
    target.draw(title);

    // Dynamic peak badge
    wstring peakLabel = L"● Đạt đỉnh: --";
    long long maxPeakRev = 0;
    for (const auto& m : monthlyStatsList) {
        if (m.revenue > maxPeakRev) {
            maxPeakRev = m.revenue;
            peakLabel = L"● Đạt đỉnh: " + m.displayMonth + L" (" + s2ws(toCurrency(m.revenue)) + L")";
        }
    }
    Text badgeTxt(boldFont, peakLabel, 14);
    badgeTxt.setFillColor(Color(219, 39, 119));
    FloatRect bb = badgeTxt.getLocalBounds();
    float badgeW = bb.size.x + 22.f;
    Vector2f badgePos(cardX + cardW - badgeW - 14.f, cardY + 10.f);
    RoundedRectRenderer::draw(target, badgePos, {badgeW, 26.f}, 13.f, Color(253, 242, 248), 1.f, Color(251, 207, 232));
    badgeTxt.setPosition({std::round(badgePos.x + (badgeW - bb.size.x) / 2.f - bb.position.x), std::round(badgePos.y + (26.f - bb.size.y) / 2.f - bb.position.y)});
    target.draw(badgeTxt);

    // Area & Line Chart
    const float chartLeft = cardX + 94.f;
    const float chartBottom = cardY + cardH - 42.f;
    const float chartW = cardW - 118.f;
    long long ceilStep = 1000000LL;
    long long maxRevInt = max(5000000LL, ((maxPeakRev + ceilStep - 1) / ceilStep) * ceilStep);
    const float maxRev = static_cast<float>(maxRevInt);
    const float chartH = max(120.f, cardH - 110.f);

    // Y-Axis horizontal grid lines and labels (6 ticks: 0..maxRevInt)
    for (int t = 0; t <= 5; ++t) {
        long long val = (maxRevInt * t) / 5;
        float y = chartBottom - ((float)val / maxRev) * chartH;

        Text lbl(font);
        lbl.setString(s2ws(toCurrency(val)));
        lbl.setCharacterSize(13);
        lbl.setFillColor(Color(148, 163, 184));
        FloatRect lb = lbl.getLocalBounds();
        lbl.setPosition({std::round(chartLeft - lb.size.x - 10.f), std::round(y - 7.f)});
        target.draw(lbl);

        RectangleShape line(Vector2f(chartW, 1.f));
        line.setPosition({chartLeft, y});
        line.setFillColor(Color(241, 245, 249));
        target.draw(line);
    }

    // 4 Trend Data Points
    struct PointDef {
        float x;
        float y;
        wstring label;
        bool isPeak;
    };

    vector<PointDef> pts;
    int mCount = (int)monthlyStatsList.size();
    float pGap = chartW / (float)(mCount + 1);

    for (int i = 0; i < mCount; ++i) {
        float px = chartLeft + (i + 1) * pGap;
        float py = chartBottom - ((float)monthlyStatsList[i].revenue / maxRev) * chartH;
        wstring l = monthlyStatsList[i].displayMonth;
        if (monthlyStatsList[i].isRecord) l += L" (Peak)";
        pts.push_back({px, py, l, monthlyStatsList[i].isRecord});
    }

    // 1. Fill Area with Gradient
    for (size_t i = 0; i + 1 < pts.size(); ++i) {
        VertexArray strip(PrimitiveType::TriangleStrip, 4);
        strip[0].position = Vector2f(pts[i].x, pts[i].y);
        strip[0].color = Color(244, 63, 94, 90);
        strip[1].position = Vector2f(pts[i + 1].x, pts[i + 1].y);
        strip[1].color = Color(244, 63, 94, 90);
        strip[2].position = Vector2f(pts[i].x, chartBottom);
        strip[2].color = Color(244, 63, 94, 5);
        strip[3].position = Vector2f(pts[i + 1].x, chartBottom);
        strip[3].color = Color(244, 63, 94, 5);
        target.draw(strip);
    }

    // 2. Thick Line connecting points
    for (size_t i = 0; i + 1 < pts.size(); ++i) {
        Vector2f p1(pts[i].x, pts[i].y);
        Vector2f p2(pts[i + 1].x, pts[i + 1].y);
        Vector2f dir = p2 - p1;
        float len = sqrt(dir.x * dir.x + dir.y * dir.y);
        if (len > 0.f) {
            RectangleShape lineSeg(Vector2f(len, 3.f));
            lineSeg.setPosition(p1);
            lineSeg.setRotation(degrees(atan2(dir.y, dir.x) * 180.f / 3.14159265f));
            lineSeg.setFillColor(Color(236, 72, 153));
            target.draw(lineSeg);
        }
    }

    // 3. Point Markers & X-Axis Labels
    for (const auto& pt : pts) {
        if (pt.isPeak) {
            // Peak glowing circle
            CircleShape glow(9.f);
            glow.setPosition({pt.x - 9.f, pt.y - 9.f});
            glow.setFillColor(Color(219, 39, 119));
            target.draw(glow);

            CircleShape inner(5.f);
            inner.setPosition({pt.x - 5.f, pt.y - 5.f});
            inner.setFillColor(Color::White);
            target.draw(inner);
        } else {
            CircleShape outer(5.f);
            outer.setPosition({pt.x - 5.f, pt.y - 5.f});
            outer.setFillColor(Color(236, 72, 153));
            target.draw(outer);

            CircleShape inner(2.5f);
            inner.setPosition({pt.x - 2.5f, pt.y - 2.5f});
            inner.setFillColor(Color::White);
            target.draw(inner);
        }

        // X-Axis label
        Text xLbl(boldFont);
        xLbl.setString(pt.label);
        xLbl.setCharacterSize(12.f);
        xLbl.setFillColor(Color(100, 116, 139));
        FloatRect xb = xLbl.getLocalBounds();
        xLbl.setPosition({pt.x - xb.size.x / 2.f, chartBottom + 8.f});
        target.draw(xLbl);
    }
}

void OverviewPanel::drawBottomRightTable(RenderTarget& target) const {
    const float gridTopY = position.y + 246.f;
    const float availH = max(560.f, height - 246.f - 18.f);
    const float rowGap = 14.f;
    const float topRowH = std::floor((availH - rowGap) * 0.52f);
    const float botRowY = gridTopY + topRowH + rowGap;
    const float botRowH = availH - topRowH - rowGap;
    const float colGap = 14.f;
    const float leftW = std::floor((width - 40.f - colGap) * 0.54f);
    const float cardX = position.x + 20.f + leftW + colGap;
    const float cardY = botRowY;
    const float cardW = (width - 40.f - colGap) - leftW;
    const float cardH = botRowH;

    // Card background
    RoundedRectRenderer::draw(target, {cardX, cardY}, {cardW, cardH}, 10.f, Color::White, 1.f, Color(226, 232, 240));

    // Header: Rose dot + Title
    CircleShape dot(4.f);
    dot.setPosition({cardX + 16.f, cardY + 18.f});
    dot.setFillColor(Color(244, 63, 94));
    target.draw(dot);

    Text title(boldFont);
    title.setString(L"Chi tiết & Tăng trưởng theo tháng");
    title.setCharacterSize(17);
    title.setFillColor(Color(23, 32, 51));
    title.setPosition({cardX + 30.f, std::round(cardY + 10.f)});
    target.draw(title);

    // Table Header Columns
    float colChangeX = cardX + std::floor(cardW * 0.44f);
    float colRevX = cardX + cardW - 24.f;

    float monthColLeft = cardX + 24.f;
    float monthColRight = colChangeX - 16.f;

    Text th1(boldFont);
    th1.setString(L"THÁNG");
    th1.setCharacterSize(14);
    th1.setFillColor(Color(148, 163, 184));
    FloatRect th1b = th1.getLocalBounds();
    th1.setPosition({std::round(monthColLeft + ((monthColRight - monthColLeft) - th1b.size.x) / 2.f - th1b.position.x), cardY + 44.f});
    target.draw(th1);

    Text th2(boldFont);
    th2.setString(L"BIẾN ĐỘNG");
    th2.setCharacterSize(14);
    th2.setFillColor(Color(148, 163, 184));
    th2.setPosition({colChangeX, cardY + 44.f});
    target.draw(th2);

    Text th3(boldFont);
    th3.setString(L"DOANH THU");
    th3.setCharacterSize(14);
    th3.setFillColor(Color(148, 163, 184));
    FloatRect th3b = th3.getLocalBounds();
    th3.setPosition({colRevX - th3b.size.x, cardY + 44.f});
    target.draw(th3);

    // Divider line
    RectangleShape div(Vector2f(cardW - 48.f, 1.f));
    div.setPosition({cardX + 24.f, cardY + 62.f});
    div.setFillColor(Color(241, 245, 249));
    target.draw(div);

    // 4 Month Rows
    float rowTop = cardY + 68.f;
    float rowH = max(36.f, (cardH - 122.f) / 4.f);
    const array<Color, 4> dotColors = {Color(8, 125, 181), Color(21, 128, 61), Color(244, 63, 94), Color(245, 158, 11)};

    for (size_t i = 0; i < monthlyStatsList.size(); ++i) {
        float ry = rowTop + i * rowH;

        if (hoveredMonthRow == (int)i) {
            RoundedRectRenderer::draw(target, {cardX + 16.f, ry - 4.f}, {cardW - 32.f, rowH - 4.f}, 4.f, Color(248, 250, 252));
        }

        // Month text + Colored Bullet Dot aligned uniformly in [monthColLeft, monthColRight]
        Text mTxt(boldFont);
        mTxt.setString(monthlyStatsList[i].displayMonth);
        mTxt.setCharacterSize(14);
        mTxt.setFillColor(Color(23, 32, 51));
        FloatRect mb = mTxt.getLocalBounds();
        float itemW = 7.f + 9.f + mb.size.x;
        float startMX = monthColLeft + ((monthColRight - monthColLeft) - itemW) / 2.f;
        float textY = monthlyStatsList[i].isRecord ? (ry - 2.f) : (ry + 2.f);

        CircleShape rDot(3.5f);
        rDot.setPosition({std::round(startMX), std::round(textY + 6.f)});
        rDot.setFillColor(dotColors[i % 4]);
        target.draw(rDot);

        mTxt.setPosition({std::round(startMX + 16.f - mb.position.x), std::round(textY)});
        target.draw(mTxt);

        if (monthlyStatsList[i].isRecord) {
            float recW = 54.f;
            float recX = monthColLeft + ((monthColRight - monthColLeft) - recW) / 2.f;
            Vector2f recPos(std::round(recX), std::round(ry + 18.f));
            RoundedRectRenderer::draw(target, recPos, {recW, 18.f}, 4.f, Color(255, 228, 230));
            Text recTxt(boldFont);
            recTxt.setString(L"Kỷ lục");
            recTxt.setCharacterSize(12);
            recTxt.setFillColor(Color(225, 29, 72));
            FloatRect rcb = recTxt.getLocalBounds();
            recTxt.setPosition({std::round(recPos.x + (recW - rcb.size.x) / 2.f - rcb.position.x), std::round(recPos.y + (18.f - rcb.size.y) / 2.f - rcb.position.y)});
            target.draw(recTxt);
        }

        // Change Badge pill
        Vector2f chgPos(colChangeX, ry);
        RoundedRectRenderer::draw(target, chgPos, {82.f, 24.f}, 4.f, monthlyStatsList[i].badgeBg);
        Text chgTxt(boldFont);
        chgTxt.setString(monthlyStatsList[i].changeBadge);
        chgTxt.setCharacterSize(12.f);
        chgTxt.setFillColor(monthlyStatsList[i].badgeText);
        FloatRect chgB = chgTxt.getLocalBounds();
        chgTxt.setPosition({chgPos.x + 41.f - chgB.size.x / 2.f, chgPos.y + 3.f});
        target.draw(chgTxt);

        // Revenue
        Text revTxt(boldFont);
        revTxt.setString(s2ws(toCurrency(monthlyStatsList[i].revenue)));
        revTxt.setCharacterSize(14);
        revTxt.setFillColor(Color(23, 32, 51));
        FloatRect revB = revTxt.getLocalBounds();
        revTxt.setPosition({colRevX - revB.size.x, ry + 2.f});
        target.draw(revTxt);
    }

    // Divider before summary
    float sumDivY = rowTop + 4 * rowH + 6.f;
    RectangleShape sDiv(Vector2f(cardW - 48.f, 1.f));
    sDiv.setPosition({cardX + 24.f, sumDivY});
    sDiv.setFillColor(Color(226, 232, 240));
    target.draw(sDiv);

    // Summary row
    Text sumLbl(font);
    sumLbl.setString(L"Tổng doanh thu tích lũy 4 tháng:");
    sumLbl.setCharacterSize(16);
    sumLbl.setFillColor(Color(100, 116, 139));
    sumLbl.setPosition({cardX + 24.f, std::round(sumDivY + 10.f)});
    target.draw(sumLbl);

    Text sumVal(boldFont);
    sumVal.setString(s2ws(toCurrency(totalAccumulatedRevenue)));
    sumVal.setCharacterSize(18.f);
    sumVal.setFillColor(Color(23, 32, 51));
    FloatRect sb = sumVal.getLocalBounds();
    sumVal.setPosition({colRevX - sb.size.x, sumDivY + 8.f});
    target.draw(sumVal);
}

void OverviewPanel::drawBottomBar(RenderTarget& target) const {
    float barY = height - 28.f;

    // Top subtle divider
    RectangleShape div(Vector2f(width - 40.f, 1.f));
    div.setPosition({position.x + 20.f, barY - 4.f});
    div.setFillColor(Color(226, 232, 240));
    target.draw(div);

    // Engine Host status
    Text engText(font);
    engText.setString(L"CineCore Host DB: Connected (0.12ms)  |  Bộ nhớ Cache: 98.4% khả dụng  |  UTF-8 • PBL2 DUYTAN CINEMA ENGINE");
    engText.setCharacterSize(12.f);
    engText.setFillColor(Color(100, 116, 139));
    engText.setPosition({position.x + 20.f, barY});
    target.draw(engText);

    // Right login status
    Text authText(font);
    authText.setString(L"Đăng nhập bởi: admin@cinecore.vn  •  1728x1080 HD Ready");
    authText.setCharacterSize(12.f);
    authText.setFillColor(Color(100, 116, 139));
    FloatRect ab = authText.getLocalBounds();
    authText.setPosition({position.x + width - ab.size.x - 20.f, barY});
}

void OverviewPanel::drawToast(RenderTarget& target) const {
    Text toast(boldFont);
    toast.setString(toastMessage);
    toast.setCharacterSize(14);
    toast.setFillColor(Color::White);
    FloatRect tb = toast.getLocalBounds();

    float tw = max(360.f, tb.size.x + 48.f);
    float th = 42.f;
    Vector2f tPos(position.x + (width - tw) / 2.f, position.y + height - 60.f);

    RoundedRectRenderer::draw(target, tPos, {tw, th}, 8.f, Color(22, 163, 74, 245), 1.5f, Color(21, 128, 61));
    toast.setPosition({std::round(tPos.x + (tw - tb.size.x) / 2.f - tb.position.x), std::round(tPos.y + (th - tb.size.y) / 2.f - tb.position.y)});
    target.draw(toast);
}

void OverviewPanel::drawReportModal(RenderTarget& target) const {
    // 1. Phông nền tối mờ toàn màn hình
    RectangleShape dimBg(Vector2f(width + 300.f, height + 100.f));
    dimBg.setPosition({position.x - 300.f, position.y - 50.f});
    dimBg.setFillColor(Color(15, 23, 42, 175));
    target.draw(dimBg);

    // 2. Thân Modal (Rộng 760px, Cao 530px)
    float mw = 760.f;
    float mh = 530.f;
    Vector2f mPos(position.x + (width - mw) / 2.f, position.y + (height - mh) / 2.f);

    RoundedRectRenderer::draw(target, mPos, {mw, mh}, 12.f, Color::White, 1.2f, Color(203, 213, 225));

    // Header Modal: Tiêu đề + Huy hiệu thành công
    Text mTitle(boldFont);
    mTitle.setString(L"BÁO CÁO TỔNG QUAN DOANH THU & VẬN HÀNH");
    mTitle.setCharacterSize(14);
    mTitle.setFillColor(Color(23, 32, 51));
    mTitle.setPosition({mPos.x + 24.f, mPos.y + 20.f});
    target.draw(mTitle);

    float titleW = mTitle.getGlobalBounds().size.x;
    Vector2f badgePos(mPos.x + 24.f + titleW + 12.f, mPos.y + 20.f);
    RoundedRectRenderer::draw(target, badgePos, {130.f, 22.f}, 11.f, Color(220, 252, 231), 1.f, Color(167, 243, 208));
    Text badgeTxt(boldFont);
    badgeTxt.setString(L"• ĐÃ XUẤT CSV");
    badgeTxt.setCharacterSize(14);
    badgeTxt.setFillColor(Color(21, 128, 61));
    badgeTxt.setPosition({badgePos.x + 12.f, badgePos.y + 3.f});
    target.draw(badgeTxt);

    // Nút đóng (X)
    UITheme::drawCloseIcon(target, {mPos.x + mw - 26.f, mPos.y + 26.f}, 12.f, Color(100, 116, 139), 2.f);

    // Subtitle
    string periodStr = (selectedDateIndex >= 0 && selectedDateIndex < (int)dateOptions.size())
                       ? UITheme::wstrToUtf8(dateOptions[selectedDateIndex]) : "Toàn kỳ";
    string roomStr = (selectedRoomIndex >= 0 && selectedRoomIndex < (int)roomOptions.size())
                     ? UITheme::wstrToUtf8(roomOptions[selectedRoomIndex]) : "Tất cả phòng";
    wstring subInfo = L"Kỳ: " + s2ws(periodStr) + L"  •  " + s2ws(roomStr) + L"  •  Cập nhật thời gian thực";
    Text mSub(font, subInfo, 14);
    mSub.setFillColor(Color(100, 116, 139));
    mSub.setPosition({mPos.x + 24.f, mPos.y + 44.f});
    target.draw(mSub);

    // Đường kẻ phân cách
    RectangleShape sep1(Vector2f(mw - 48.f, 1.f));
    sep1.setPosition({mPos.x + 24.f, mPos.y + 68.f});
    sep1.setFillColor(Color(226, 232, 240));
    target.draw(sep1);

    // 3. 4 Mini KPI Cards trong Modal
    float kpiY = mPos.y + 80.f;
    float kpiW = (mw - 48.f - 30.f) / 4.f; // ~170px
    float kpiH = 68.f;

    auto drawMiniKpi = [&](int idx, const wstring& lbl, const wstring& val, Color valCol) {
        Vector2f kp(mPos.x + 24.f + idx * (kpiW + 10.f), kpiY);
        RoundedRectRenderer::draw(target, kp, {kpiW, kpiH}, 6.f, Color(248, 250, 252), 1.f, Color(226, 232, 240));
        Text l(font, lbl, 14);
        l.setFillColor(Color(100, 116, 139));
        l.setPosition({kp.x + 12.f, kp.y + 8.f});
        target.draw(l);

        Text v(boldFont, val, 14);
        v.setFillColor(valCol);
        v.setPosition({kp.x + 12.f, kp.y + 32.f});
        target.draw(v);
    };

    drawMiniKpi(0, L"TỔNG DOANH THU", s2ws(toCurrency(totalRevenueThisMonth)), Color(23, 32, 51));
    drawMiniKpi(1, L"DOANH THU PHIM", s2ws(toCurrency(movieRevenueThisMonth)), Color(37, 99, 235));
    drawMiniKpi(2, L"DOANH THU F&B", s2ws(toCurrency(comboRevenueThisMonth)), Color(217, 119, 6));
    drawMiniKpi(3, L"TỔNG VÉ XUẤT", to_wstring(totalTicketsThisMonth) + L" vé", Color(21, 128, 61));

    // 4. Bảng Chi tiết Top 5 Phim trong Modal
    float tblY = mPos.y + 162.f;
    Text tblHead(boldFont, L"BẢNG ĐỐI CHIẾU DOANH THU CÁC PHIM HÀNG ĐẦU (TOP 5)", 14);
    tblHead.setFillColor(Color(23, 32, 51));
    tblHead.setPosition({mPos.x + 24.f, tblY});
    target.draw(tblHead);

    float thY = tblY + 24.f;
    RoundedRectRenderer::draw(target, {mPos.x + 24.f, thY}, {mw - 48.f, 32.f}, 4.f, Color(241, 245, 249));

    float c1X = mPos.x + 36.f;
    float c2X = mPos.x + 85.f;
    float c3X = mPos.x + 460.f;
    float c4X = mPos.x + mw - 36.f;

    Text th1(boldFont, L"#", 14); th1.setFillColor(Color(100, 116, 139)); th1.setPosition({c1X, thY + 7.f}); target.draw(th1);
    Text th2(boldFont, L"TÊN PHIM CHIẾU", 14); th2.setFillColor(Color(100, 116, 139)); th2.setPosition({c2X, thY + 7.f}); target.draw(th2);
    Text th3(boldFont, L"SỐ VÉ BÁN", 14); th3.setFillColor(Color(100, 116, 139)); th3.setPosition({c3X, thY + 7.f}); target.draw(th3);
    Text th4(boldFont, L"DOANH THU PHIM", 14); th4.setFillColor(Color(100, 116, 139));
    FloatRect th4b = th4.getLocalBounds();
    th4.setPosition({c4X - th4b.size.x, thY + 7.f}); target.draw(th4);

    // 5 dòng phim
    float rowTop = thY + 34.f;
    float rowH = 36.f;
    int showN = min(5, (int)topMoviesList.size());
    for (int i = 0; i < showN; ++i) {
        float ry = rowTop + i * rowH;
        if (i % 2 == 1) {
            RoundedRectRenderer::draw(target, {mPos.x + 24.f, ry}, {mw - 48.f, rowH}, 0.f, Color(248, 250, 252));
        }

        // Rank
        Color rCol = (i == 0) ? Color(245, 158, 11) : (i == 1 ? Color(148, 163, 184) : Color(217, 119, 6));
        CircleShape rCir(8.f);
        rCir.setPosition({c1X - 2.f, ry + 9.f});
        rCir.setFillColor(i < 3 ? rCol : Color(241, 245, 249));
        target.draw(rCir);

        Text rTxt(boldFont, to_wstring(i + 1), 14);
        rTxt.setFillColor(i < 3 ? Color::White : Color(100, 116, 139));
        FloatRect rtb = rTxt.getLocalBounds();
        rTxt.setPosition({c1X + 6.f - rtb.size.x / 2.f - rtb.position.x, ry + 10.f});
        target.draw(rTxt);

        // Tên phim
        Text mTitleTxt(boldFont, topMoviesList[i].displayTitle, 14);
        mTitleTxt.setFillColor(Color(23, 32, 51));
        mTitleTxt.setPosition({c2X, ry + 9.f});
        target.draw(mTitleTxt);

        // Số vé
        Text tksTxt(boldFont, to_wstring(topMoviesList[i].tickets) + L" vé", 14);
        tksTxt.setFillColor(Color(37, 99, 235));
        tksTxt.setPosition({c3X, ry + 9.f});
        target.draw(tksTxt);

        // Doanh thu
        Text revTxt(boldFont, s2ws(toCurrency(topMoviesList[i].revenue)), 14);
        revTxt.setFillColor(Color(225, 29, 72));
        FloatRect revB = revTxt.getLocalBounds();
        revTxt.setPosition({c4X - revB.size.x, ry + 9.f});
        target.draw(revTxt);
    }

    // Đường kẻ phân cách trước nút điều khiển
    RectangleShape sep2(Vector2f(mw - 48.f, 1.f));
    sep2.setPosition({mPos.x + 24.f, mPos.y + mh - 66.f});
    sep2.setFillColor(Color(226, 232, 240));
    target.draw(sep2);

    // File path info
    Text pathTxt(font, L"Vị trí lưu file: data/overview_report.csv (Mở tự động bằng Excel)", 14);
    pathTxt.setFillColor(Color(100, 116, 139));
    pathTxt.setPosition({mPos.x + 24.f, mPos.y + mh - 42.f});
    target.draw(pathTxt);

    // Nút "Mở file Excel / Ứng dụng ngoài"
    Vector2f btnExPos(mPos.x + mw - 340.f, mPos.y + mh - 50.f);
    Color exBg = modalOpenExcelHovered ? Color(18, 59, 93) : Color(8, 125, 181);
    RoundedRectRenderer::draw(target, btnExPos, {210.f, 36.f}, 6.f, exBg);
    Text btnExTxt(boldFont, L"Mở bằng Excel / CSV", 14);
    btnExTxt.setFillColor(Color::White);
    FloatRect exb = btnExTxt.getLocalBounds();
    btnExTxt.setPosition({btnExPos.x + (210.f - exb.size.x) / 2.f - exb.position.x, btnExPos.y + 9.f});
    target.draw(btnExTxt);

    // Nút "Đóng"
    Vector2f btnClPos(mPos.x + mw - 115.f, mPos.y + mh - 50.f);
    Color clBg = modalCloseHovered ? Color(226, 232, 240) : Color(241, 245, 249);
    RoundedRectRenderer::draw(target, btnClPos, {100.f, 36.f}, 6.f, clBg, 1.f, Color(203, 213, 225));
    Text btnClTxt(boldFont, L"Đóng", 14);
    btnClTxt.setFillColor(Color(51, 65, 85));
    FloatRect clb = btnClTxt.getLocalBounds();
    btnClTxt.setPosition({btnClPos.x + (100.f - clb.size.x) / 2.f - clb.position.x, btnClPos.y + 9.f});
    target.draw(btnClTxt);
}
