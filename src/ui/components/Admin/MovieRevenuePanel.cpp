#include "UI/components/Admin/MovieRevenuePanel.h"
#include "UI/components/Admin/FontManager.h"
#include "UI/components/Admin/RoundedRectRenderer.h"
#include "UI/components/Admin/UITheme.h"
#include "repositories/booking/ComboRepository.h"
#include "repositories/booking/ShowtimeRepository.h"
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

MovieRevenuePanel::MovieRevenuePanel(Font& fontRef, float w, float h)
    : font(fontRef),
      boldFont(FontManager::instance().bold()),
      width(w),
      height(h),
      position(280.f, 0.f),
      ticketsFilePath(FileUtils::resolveDataPath("data/tickets.txt")),
      showtimesFilePath(FileUtils::resolveDataPath("data/showtimes.txt")),
      combosFilePath(FileUtils::resolveDataPath("data/combo.txt"))
{
    initDefaultStats();
    loadData();
}

void MovieRevenuePanel::setPosition(const Vector2f& pos) {
    position = pos;
    width = 1728.f - pos.x;
}

string MovieRevenuePanel::resolveDataPath(const string& relative) const {
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

int MovieRevenuePanel::countSeats(const string& seatList) const {
    if (seatList.empty() || seatList == "Không có" || seatList == "None") return 0;
    int count = 1;
    for (char c : seatList) {
        if (c == ',' || c == ';') count++;
    }
    return count;
}

string MovieRevenuePanel::trim(const string& text) const {
    size_t first = text.find_first_not_of(" \t\r\n");
    if (first == string::npos) return "";
    size_t last = text.find_last_not_of(" \t\r\n");
    return text.substr(first, (last - first + 1));
}

long long MovieRevenuePanel::computeComboRevenue(const string& comboList) const {
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

string MovieRevenuePanel::toCurrency(long long amount) const {
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

String MovieRevenuePanel::toSfString(const string& text) const {
    return String::fromUtf8(text.begin(), text.end());
}

void MovieRevenuePanel::initDefaultStats() {
    dateFilterOptions.clear();
    dateFilterOptions.push_back({L"Tất cả thời gian", ""});
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
        swprintf(labelBuf, 64, L"Kỳ: Tháng %02d/%04d", m, y);
        char keyBuf[16];
        snprintf(keyBuf, sizeof(keyBuf), "%04d-%02d", y, m);
        dateFilterOptions.push_back({labelBuf, keyBuf});
    }
    moviesList.clear();
    monthsList.clear();
    combosList.clear();
}

void MovieRevenuePanel::loadData() {
    loadShowtimePrices();
    loadComboPrices();
    loadTickets();
    calculateStats();
}

void MovieRevenuePanel::loadShowtimePrices() {
    showtimePrices.clear();
    ShowtimeRepository repo;
    const auto consume = [&](const string& path) {
        DLL<Showtime> showtimes = repo.loadFromFile(toSfString(path));
        Node<Showtime>* node = showtimes.getHead();
        while (node) {
            showtimePrices[node->data.showtime_id.toAnsiString()] = node->data.price;
            node = node->next;
        }
    };
    consume(showtimesFilePath);
    consume(FileUtils::resolveDataPath("data/showtimes_history.txt"));
}

void MovieRevenuePanel::loadComboPrices() {
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

void MovieRevenuePanel::loadTickets() {
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

void MovieRevenuePanel::calculateStats() {
    unordered_map<string, string> movieIdByTitle;
    unordered_map<string, string> movieGenreByTitle;
    {
        ifstream mf(FileUtils::resolveDataPath("data/movies.txt"), ios::binary);
        string mline; bool hdr = true;
        while (mf.is_open() && getline(mf, mline)) {
            if (!mline.empty() && mline.back() == '\r') mline.pop_back();
            if (mline.empty()) continue;
            if (hdr) { hdr = false; continue; }
            vector<string> cols;
            stringstream mss(mline); string tok;
            while (getline(mss, tok, '|')) cols.push_back(tok);
            if (cols.size() >= 6 && !cols[1].empty()) {
                movieIdByTitle[cols[1]] = cols[0];
                movieGenreByTitle[cols[1]] = cols[5];
            }
        }
    }

    unordered_map<string, int> showtimesPerMovieId;
    {
        auto countShowtimes = [&](const string& spath) {
            ifstream sf(spath, ios::binary);
            string sline; bool hdr = true;
            while (sf.is_open() && getline(sf, sline)) {
                if (!sline.empty() && sline.back() == '\r') sline.pop_back();
                if (sline.empty()) continue;
                if (hdr) { hdr = false; continue; }
                stringstream sss(sline);
                string sid, mid;
                getline(sss, sid, '|');
                getline(sss, mid, '|');
                if (!mid.empty()) showtimesPerMovieId[mid]++;
            }
        };
        countShowtimes(showtimesFilePath);
        countShowtimes(FileUtils::resolveDataPath("data/showtimes_history.txt"));
    }

    unordered_map<string, string> comboNameById;
    unordered_map<string, string> comboIdByName;
    {
        ComboRepository cRepo;
        DLL<Combo> combos = cRepo.loadFromFile(toSfString(combosFilePath));
        for (Node<Combo>* n = combos.getHead(); n; n = n->next) {
            auto u8n = n->data.name.toUtf8();
            auto u8i = n->data.id.toUtf8();
            string nameStr(u8n.begin(), u8n.end());
            string idStr(u8i.begin(), u8i.end());
            comboNameById[idStr] = nameStr;
            comboIdByName[nameStr] = idStr;
        }
    }

    string activeYm = (selectedDateFilter >= 0 && selectedDateFilter < (int)dateFilterOptions.size())
                      ? dateFilterOptions[selectedDateFilter].second : "";

    auto extractYm = [](const string& d) -> string {
        if (d.size() >= 7 && d[4] == '-') return d.substr(0, 7);
        if (d.size() >= 10 && d[2] == '/' && d[5] == '/') return d.substr(6, 4) + "-" + d.substr(3, 2);
        return "";
    };

    totalRevenue = 0;
    movieRevenue = 0;
    comboRevenue = 0;
    totalTickets = 0;
    totalCombos = 0;

    struct MovieAgg { int tickets = 0; long long revenue = 0; };
    struct MonthAgg { long long ticketRev = 0; long long comboRev = 0; long long totalRev = 0; };
    struct ComboAgg { string name; int unitPrice = 0; int qty = 0; long long rev = 0; };

    unordered_map<string, MovieAgg> movieAgg;
    unordered_map<string, MonthAgg> monthAgg;
    unordered_map<string, ComboAgg> comboAgg;

    for (Node<Ticket>* node = ticketsCache.getHead(); node; node = node->next) {
        const Ticket& t = node->data;
        string ym = extractYm(t.bookedDate);
        if (ym.empty()) ym = extractYm(t.date);

        long long ticketTotal = static_cast<long long>(t.price);
        long long cPart = computeComboRevenue(t.comboName);
        if (cPart > ticketTotal) cPart = ticketTotal;
        long long mPart = ticketTotal - cPart;

        if (!ym.empty()) {
            auto& ma = monthAgg[ym];
            ma.ticketRev += mPart;
            ma.comboRev += cPart;
            ma.totalRev += ticketTotal;
        }

        if (!activeYm.empty() && ym != activeYm) {
            continue;
        }

        int seats = max(1, countSeats(t.booked));
        totalRevenue += ticketTotal;
        movieRevenue += mPart;
        comboRevenue += cPart;
        totalTickets += seats;

        auto& mv = movieAgg[t.title];
        mv.tickets += seats;
        mv.revenue += ticketTotal;

        if (!t.comboName.empty() && t.comboName != "Không có" && t.comboName != "None") {
            stringstream css(t.comboName);
            string item;
            while (getline(css, item, ',')) {
                item = trim(item);
                if (item.empty()) continue;
                string code = item, name = item;
                int qty = 1;
                size_t col = item.find(":x");
                if (col != string::npos) {
                    code = item.substr(0, col);
                    try { qty = max(1, stoi(item.substr(col + 2))); } catch (...) { qty = 1; }
                    name = comboNameById.count(code) ? comboNameById[code] : code;
                } else {
                    size_t xpos = item.rfind(" x");
                    if (xpos != string::npos) {
                        name = item.substr(0, xpos);
                        try { qty = max(1, stoi(item.substr(xpos + 2))); } catch (...) { qty = 1; }
                    }
                    code = comboIdByName.count(name) ? comboIdByName[name] : name;
                }
                int unitPr = comboPrices.count(code) ? comboPrices[code] : (comboPrices.count(name) ? comboPrices[name] : 0);
                auto& ca = comboAgg[code];
                ca.name = name;
                ca.unitPrice = unitPr;
                ca.qty += qty;
                ca.rev += static_cast<long long>(unitPr) * qty;
                totalCombos += qty;
            }
        }
    }

    moviesList.clear();
    int fallbackIdx = 1;
    for (const auto& kv : movieAgg) {
        MovieRevItem item;
        item.id = movieIdByTitle.count(kv.first) ? movieIdByTitle[kv.first] : ("M" + to_string(fallbackIdx++));
        item.title = s2ws(kv.first);
        item.shortTitle = item.title.size() > 10 ? (item.title.substr(0, 9) + L"..") : item.title;
        item.genre = movieGenreByTitle.count(kv.first) ? s2ws(movieGenreByTitle[kv.first]) : L"Phim chiếu rạp";
        item.showtimes = showtimesPerMovieId.count(item.id) ? showtimesPerMovieId[item.id] : 0;
        item.tickets = kv.second.tickets;
        item.revenue = kv.second.revenue;
        item.share = totalRevenue > 0 ? static_cast<float>(kv.second.revenue) * 100.f / static_cast<float>(totalRevenue) : 0.f;
        moviesList.push_back(item);
    }
    sort(moviesList.begin(), moviesList.end(), [](const MovieRevItem& a, const MovieRevItem& b) {
        if (a.revenue != b.revenue) return a.revenue > b.revenue;
        return a.tickets > b.tickets;
    });
    if (!moviesList.empty()) moviesList[0].isHot = true;

    monthsList.clear();
    vector<string> sortedYms;
    for (const auto& kv : monthAgg) sortedYms.push_back(kv.first);
    sort(sortedYms.begin(), sortedYms.end());
    long long maxMonthRev = 0;
    int maxMonthIdx = -1;
    for (size_t i = 0; i < sortedYms.size(); ++i) {
        const string& ym = sortedYms[i];
        const auto& ma = monthAgg[ym];
        MonthRevItem mi;
        mi.month = (ym.size() == 7) ? (ym.substr(5, 2) + "/" + ym.substr(0, 4)) : ym;
        mi.displayMonth = L"Tháng " + s2ws(mi.month);
        mi.ticketRev = ma.ticketRev;
        mi.comboRev = ma.comboRev;
        mi.totalRev = ma.totalRev;
        mi.comboShare = ma.totalRev > 0 ? static_cast<float>(ma.comboRev) * 100.f / static_cast<float>(ma.totalRev) : 0.f;
        if (i == 0) {
            mi.growth = L"---";
        } else {
            long long prev = monthAgg[sortedYms[i - 1]].totalRev;
            if (prev > 0) {
                double pct = (static_cast<double>(ma.totalRev - prev) * 100.0) / static_cast<double>(prev);
                wstringstream wss;
                wss << fixed << setprecision(1) << (pct >= 0 ? L"+" : L"") << pct << L"%";
                mi.growth = wss.str();
            } else {
                mi.growth = ma.totalRev > 0 ? L"+100.0%" : L"0.0%";
            }
        }
        if (ma.totalRev > maxMonthRev) {
            maxMonthRev = ma.totalRev;
            maxMonthIdx = static_cast<int>(i);
        }
        monthsList.push_back(mi);
    }
    if (maxMonthIdx >= 0 && maxMonthRev > 0) monthsList[maxMonthIdx].isRecord = true;

    combosList.clear();
    long long totalComboAggRev = 0;
    for (const auto& kv : comboAgg) totalComboAggRev += kv.second.rev;
    for (const auto& kv : comboAgg) {
        ComboRevItem ci;
        ci.code = kv.first;
        ci.name = s2ws(kv.second.name);
        ci.unitPrice = kv.second.unitPrice;
        ci.quantity = kv.second.qty;
        ci.revenue = kv.second.rev;
        ci.share = totalComboAggRev > 0 ? static_cast<float>(kv.second.rev) * 100.f / static_cast<float>(totalComboAggRev) : 0.f;
        combosList.push_back(ci);
    }
    sort(combosList.begin(), combosList.end(), [](const ComboRevItem& a, const ComboRevItem& b) {
        if (a.revenue != b.revenue) return a.revenue > b.revenue;
        return a.quantity > b.quantity;
    });
    if (!combosList.empty()) combosList[0].isTop1 = true;
}

void MovieRevenuePanel::openExportedFile(const string& path) const {
#ifdef _WIN32
    ShellExecuteA(NULL, "open", path.c_str(), NULL, NULL, SW_SHOWNORMAL);
#else
    system(("start \"\" \"" + path + "\"").c_str());
#endif
}

bool MovieRevenuePanel::exportRevenueReportToCsv() {
    string outPath = FileUtils::resolveDataPath("data/doanh_thu_report.csv");
    ofstream out(outPath, ios::binary);
    if (!out.is_open()) {
        outPath = "doanh_thu_report.csv";
        out.open(outPath, ios::binary);
        if (!out.is_open()) return false;
    }

    // UTF-8 BOM để Excel mở trực tiếp tiếng Việt không lỗi font
    const unsigned char bom[] = {0xEF, 0xBB, 0xBF};
    out.write(reinterpret_cast<const char*>(bom), sizeof(bom));

    time_t now = time(nullptr);
    tm ltm{};
#if defined(_WIN32)
    localtime_s(&ltm, &now);
#else
    localtime_r(&now, &ltm);
#endif
    char dateBuf[32];
    strftime(dateBuf, sizeof(dateBuf), "%d/%m/%Y %H:%M", &ltm);
    double moviePct = totalRevenue > 0 ? (static_cast<double>(movieRevenue) * 100.0 / static_cast<double>(totalRevenue)) : 0.0;
    double comboPct = totalRevenue > 0 ? (static_cast<double>(comboRevenue) * 100.0 / static_cast<double>(totalRevenue)) : 0.0;

    out << "BÁO CÁO DOANH THU & QUYẾT TOÁN TÀI CHÍNH RẠP CINECORE\n";
    out << "Thời gian kết xuất," << dateBuf << "\n\n";

    out << "CHỈ SỐ TỔNG QUAN,GIÁ TRỊ\n";
    out << "Tổng doanh thu toàn rạp," << totalRevenue << " VNĐ\n";
    out << fixed << setprecision(1);
    out << "Doanh thu vé phim," << movieRevenue << " VNĐ (Chiếm " << moviePct << "%)\n";
    out << "Doanh thu combo F&B," << comboRevenue << " VNĐ (Chiếm " << comboPct << "%)\n";
    out << "Tổng số vé đã xuất," << totalTickets << " vé\n";
    out << "Tổng sản phẩm combo," << totalCombos << " combo\n\n";

    out << "DOANH THU CHI TIẾT THEO PHIM\n";
    out << "Hạng,Mã phim,Tên phim,Thể loại,Suất chiếu,Số vé bán,Doanh thu (VNĐ),Tỷ trọng\n";
    for (size_t i = 0; i < moviesList.size(); ++i) {
        out << (i + 1) << "," << moviesList[i].id << ",\"" << UITheme::wstrToUtf8(moviesList[i].title) << "\",\""
            << UITheme::wstrToUtf8(moviesList[i].genre) << "\"," << moviesList[i].showtimes << ","
            << moviesList[i].tickets << "," << moviesList[i].revenue << "," << moviesList[i].share << "%\n";
    }

    out << "\nTĂNG TRƯỞNG THEO THÁNG\n";
    out << "Tháng,Doanh thu vé (VNĐ),Doanh thu combo (VNĐ),Tổng doanh thu (VNĐ),Biến động\n";
    for (const auto& m : monthsList) {
        out << m.month << "," << m.ticketRev << "," << m.comboRev << "," << m.totalRev << ",\""
            << UITheme::wstrToUtf8(m.growth) << "\"\n";
    }

    out << "\nTOP COMBO F&B BÁN CHẠY\n";
    out << "Mã combo,Tên combo,Đơn giá (VNĐ),Số lượng,Doanh thu (VNĐ),Tỷ trọng\n";
    for (const auto& c : combosList) {
        out << c.code << ",\"" << UITheme::wstrToUtf8(c.name) << "\"," << c.unitPrice << ","
            << c.quantity << "," << c.revenue << "," << c.share << "%\n";
    }
    out.flush();
    const bool writeOk = out.good();
    out.close();
    if (!writeOk) return false;

    openExportedFile(outPath);
    reportModalOpen = true;
    return true;
}

void MovieRevenuePanel::refresh() {
    reportModalOpen = false;
    loadData();
    toastActive = true;
    toastTimer = 2.5f;
    toastMessage = L"Đã đồng bộ và cập nhật dữ liệu doanh thu mới nhất!";
}

void MovieRevenuePanel::handleEvent(const Event& event, const RenderWindow& /*window*/) {
    // 0. Xử lý click Modal Báo cáo Doanh thu nếu đang mở
    if (reportModalOpen) {
        float mw = 760.f, mh = 530.f;
        float mx = position.x + (width - mw) / 2.f;
        float my = position.y + (height - mh) / 2.f;

        // 0b. Xử lý click Modal Chi tiết tháng nếu đang mở
    if (detailMonthIndex >= 0 && detailMonthIndex < (int)monthsList.size()) {
        float mw = 540.f, mh = 370.f;
        float mx = position.x + (width - mw) / 2.f;
        float my = position.y + (height - mh) / 2.f;

        if (auto mousePress = event.getIf<Event::MouseButtonPressed>()) {
            if (mousePress->button == Mouse::Button::Left) {
                Vector2f mPos(static_cast<float>(mousePress->position.x), static_cast<float>(mousePress->position.y));

                FloatRect closeXRect({mx + mw - 38.f, my + 14.f}, {26.f, 26.f});
                if (closeXRect.contains(mPos)) {
                    detailMonthIndex = -1;
                    return;
                }

                // Nút "Lọc theo tháng này"
                FloatRect btnFilter({mx + mw - 305.f, my + mh - 52.f}, {180.f, 36.f});
                if (btnFilter.contains(mPos)) {
                    if (detailMonthIndex >= 0 && detailMonthIndex < (int)monthsList.size()) {
                        const string& mStr = monthsList[detailMonthIndex].month; // "MM/YYYY"
                        string targetYm = (mStr.size() == 7 && mStr[2] == '/') ? (mStr.substr(3, 4) + "-" + mStr.substr(0, 2)) : "";
                        int matchedIdx = 0;
                        for (size_t k = 0; k < dateFilterOptions.size(); ++k) {
                            if (dateFilterOptions[k].second == targetYm) { matchedIdx = (int)k; break; }
                        }
                        if (matchedIdx == 0 && !targetYm.empty()) {
                            dateFilterOptions.push_back({L"Kỳ: " + monthsList[detailMonthIndex].displayMonth, targetYm});
                            matchedIdx = (int)dateFilterOptions.size() - 1;
                        }
                        selectedDateFilter = matchedIdx;
                        calculateStats();
                        toastMessage = L"Đã lọc số liệu tổng quan theo: " + monthsList[detailMonthIndex].displayMonth;
                        toastTimer = 2.5f;
                        toastActive = true;
                    }
                    detailMonthIndex = -1;
                    return;
                }

                // Nút "Đóng"
                FloatRect btnClose({mx + mw - 115.f, my + mh - 52.f}, {92.f, 36.f});
                if (btnClose.contains(mPos)) {
                    detailMonthIndex = -1;
                    return;
                }

                // Click ngoài modal -> đóng
                if (!FloatRect({mx, my}, {mw, mh}).contains(mPos)) {
                    detailMonthIndex = -1;
                    return;
                }
            }
        }
        return;
    }

    if (auto mousePress = event.getIf<Event::MouseButtonPressed>()) {
            if (mousePress->button == Mouse::Button::Left) {
                Vector2f mPos(static_cast<float>(mousePress->position.x), static_cast<float>(mousePress->position.y));

                // Nút đóng (X)
                FloatRect closeXRect({mx + mw - 38.f, my + 14.f}, {24.f, 24.f});
                if (closeXRect.contains(mPos)) {
                    reportModalOpen = false;
                    return;
                }

                // Nút "Mở bằng Excel / CSV"
                FloatRect btnExcel({mx + mw - 340.f, my + mh - 50.f}, {210.f, 36.f});
                if (btnExcel.contains(mPos)) {
                    openExportedFile(FileUtils::resolveDataPath("data/doanh_thu_report.csv"));
                    return;
                }

                // Nút "Đóng"
                FloatRect btnClose({mx + mw - 115.f, my + mh - 50.f}, {100.f, 36.f});
                if (btnClose.contains(mPos)) {
                    reportModalOpen = false;
                    return;
                }

                // Click ra ngoài modal -> đóng
                FloatRect modalRect({mx, my}, {mw, mh});
                if (!modalRect.contains(mPos)) {
                    reportModalOpen = false;
                    return;
                }
            }
        }
        return;
    }

    // 0b. Xử lý click Modal Chi tiết tháng nếu đang mở
    if (detailMonthIndex >= 0 && detailMonthIndex < (int)monthsList.size()) {
        float mw = 540.f, mh = 370.f;
        float mx = position.x + (width - mw) / 2.f;
        float my = position.y + (height - mh) / 2.f;

        if (auto mousePress = event.getIf<Event::MouseButtonPressed>()) {
            if (mousePress->button == Mouse::Button::Left) {
                Vector2f mPos(static_cast<float>(mousePress->position.x), static_cast<float>(mousePress->position.y));

                FloatRect closeXRect({mx + mw - 38.f, my + 14.f}, {26.f, 26.f});
                if (closeXRect.contains(mPos)) {
                    detailMonthIndex = -1;
                    return;
                }

                // Nút "Lọc theo tháng này"
                FloatRect btnFilter({mx + mw - 305.f, my + mh - 52.f}, {180.f, 36.f});
                if (btnFilter.contains(mPos)) {
                    if (detailMonthIndex >= 0 && detailMonthIndex < (int)monthsList.size()) {
                        const string& mStr = monthsList[detailMonthIndex].month; // "MM/YYYY"
                        string targetYm = (mStr.size() == 7 && mStr[2] == '/') ? (mStr.substr(3, 4) + "-" + mStr.substr(0, 2)) : "";
                        int matchedIdx = 0;
                        for (size_t k = 0; k < dateFilterOptions.size(); ++k) {
                            if (dateFilterOptions[k].second == targetYm) { matchedIdx = (int)k; break; }
                        }
                        if (matchedIdx == 0 && !targetYm.empty()) {
                            dateFilterOptions.push_back({L"Kỳ: " + monthsList[detailMonthIndex].displayMonth, targetYm});
                            matchedIdx = (int)dateFilterOptions.size() - 1;
                        }
                        selectedDateFilter = matchedIdx;
                        calculateStats();
                        toastMessage = L"Đã lọc số liệu tổng quan theo: " + monthsList[detailMonthIndex].displayMonth;
                        toastTimer = 2.5f;
                        toastActive = true;
                    }
                    detailMonthIndex = -1;
                    return;
                }

                // Nút "Đóng"
                FloatRect btnClose({mx + mw - 115.f, my + mh - 52.f}, {92.f, 36.f});
                if (btnClose.contains(mPos)) {
                    detailMonthIndex = -1;
                    return;
                }

                // Click ngoài modal -> đóng
                if (!FloatRect({mx, my}, {mw, mh}).contains(mPos)) {
                    detailMonthIndex = -1;
                    return;
                }
            }
        }
        return;
    }

    if (auto mousePress = event.getIf<Event::MouseButtonPressed>()) {
        if (mousePress->button == Mouse::Button::Left) {
            Vector2f mPos(static_cast<float>(mousePress->position.x), static_cast<float>(mousePress->position.y));

            // 1. Tab Bar Clicking: y ≈ position.y + 206.f, height 36.f
            float tabY = position.y + 206.f;
            float tabX = position.x + 20.f;

            // Tab 0: Số vé bán ra theo phim (~200px)
            FloatRect t0Rect({tabX, tabY}, {200.f, 36.f});
            if (t0Rect.contains(mPos)) {
                activeTab = RevenueTab::TICKETS_BY_MOVIE;
                return;
            }

            // Tab 1: Tổng doanh thu (~140px)
            FloatRect t1Rect({tabX + 210.f, tabY}, {140.f, 36.f});
            if (t1Rect.contains(mPos)) {
                activeTab = RevenueTab::TOTAL_REVENUE;
                return;
            }

            // Tab 2: Doanh thu theo phim (~180px)
            FloatRect t2Rect({tabX + 360.f, tabY}, {180.f, 36.f});
            if (t2Rect.contains(mPos)) {
                activeTab = RevenueTab::REVENUE_BY_MOVIE;
                return;
            }

            // Tab 3: Doanh thu theo combo (~190px)
            FloatRect t3Rect({tabX + 550.f, tabY}, {190.f, 36.f});
            if (t3Rect.contains(mPos)) {
                activeTab = RevenueTab::REVENUE_BY_COMBO;
                return;
            }

            // 2. Buttons in Header: Tải dữ liệu & Xuất Excel
            FloatRect reloadRect({position.x + width - 260.f, position.y + 37.f}, {110.f, 38.f});
            if (reloadRect.contains(mPos)) {
                refresh();
                return;
            }

            FloatRect exportRect({position.x + width - 140.f, position.y + 37.f}, {105.f, 38.f});
            bool clickBottomExport = (activeTab == RevenueTab::TOTAL_REVENUE) &&
                FloatRect({position.x + 20.f + (width - 40.f) * 0.89f - 12.f, position.y + height - 74.f}, {116.f, 36.f}).contains(mPos);
            if (exportRect.contains(mPos) || clickBottomExport) {
                bool ok = exportRevenueReportToCsv();
                toastActive = true;
                toastTimer = 3.f;
                toastMessage = ok ? L"Đã xuất báo cáo và mở trong Excel / CSV thành công!"
                                  : L"Lỗi: Không thể tạo file báo cáo CSV!";
                return;
            }

            if (activeTab == RevenueTab::TOTAL_REVENUE &&
                FloatRect({position.x + width - 146.f, position.y + 576.f}, {106.f, 30.f}).contains(mPos)) {
                activeTab = RevenueTab::REVENUE_BY_MOVIE;
                return;
            }

            // 3. Date Range Filter Box & Dropdown
            FloatRect dateBox({position.x + width - 530.f, position.y + 37.f}, {260.f, 38.f});
            if (dateBox.contains(mPos)) {
                dateDropdownOpen = !dateDropdownOpen;
                return;
            }

            if (dateDropdownOpen) {
                float ddX = position.x + width - 530.f;
                float ddY = position.y + 79.f;
                float itemH = 36.f;
                for (int i = 0; i < (int)dateFilterOptions.size(); ++i) {
                    FloatRect itemRect({ddX, ddY + i * itemH}, {260.f, itemH});
                    if (itemRect.contains(mPos)) {
                        selectedDateFilter = i;
                        dateDropdownOpen = false;
                        calculateStats();
                        toastMessage = L"Đã lọc thống kê theo: " + dateFilterOptions[i].first;
                        toastTimer = 2.5f;
                        toastActive = true;
                        return;
                    }
                }
                dateDropdownOpen = false;
            }

            // 4. Click "Chi tiết" trên bảng Tổng kết doanh thu theo tháng (Tab 2)
            if (activeTab == RevenueTab::TOTAL_REVENUE && !monthsList.empty()) {
                const float contentY = position.y + 250.f;
                const float availableW = width - 40.f;
                const float topH = 296.f;
                const float bTableY = contentY + topH + 14.f;
                const float bTableH = height - 28.f - bTableY - 4.f;
                const float col_act = position.x + 20.f + availableW * 0.89f;
                const float rTop = bTableY + 46.f + 34.f;
                const float footerH = 48.f;
                const float rH = (bTableH - (rTop - bTableY) - footerH) / static_cast<float>(monthsList.size());

                for (size_t i = 0; i < monthsList.size(); ++i) {
                    float ry = rTop + i * rH;
                    FloatRect detailRect({col_act - 8.f, ry + (rH - 30.f) / 2.f}, {86.f, 30.f});
                    if (detailRect.contains(mPos)) {
                        detailMonthIndex = static_cast<int>(i);
                        return;
                    }
                }
            }
        }
    }
}

void MovieRevenuePanel::update(const Vector2f& mousePos, bool /*mouseDown*/) {
    // Check tab hover
    hoveredTab = -1;
    float tabY = position.y + 206.f;
    float tabX = position.x + 20.f;
    if (FloatRect({tabX, tabY}, {200.f, 36.f}).contains(mousePos)) hoveredTab = 0;
    else if (FloatRect({tabX + 210.f, tabY}, {140.f, 36.f}).contains(mousePos)) hoveredTab = 1;
    else if (FloatRect({tabX + 360.f, tabY}, {180.f, 36.f}).contains(mousePos)) hoveredTab = 2;
    else if (FloatRect({tabX + 550.f, tabY}, {190.f, 36.f}).contains(mousePos)) hoveredTab = 3;

    // Button hovers
    reloadHovered = FloatRect({position.x + width - 260.f, position.y + 37.f}, {110.f, 38.f}).contains(mousePos);
    exportHovered = FloatRect({position.x + width - 140.f, position.y + 37.f}, {105.f, 38.f}).contains(mousePos);

    hoveredDateItem = -1;
    if (dateDropdownOpen) {
        float ddX = position.x + width - 530.f;
        float ddY = position.y + 79.f;
        float itemH = 36.f;
        for (int i = 0; i < 5; ++i) {
            if (FloatRect({ddX, ddY + i * itemH}, {260.f, itemH}).contains(mousePos)) {
                hoveredDateItem = i;
                break;
            }
        }
    }

    if (reportModalOpen) {
        float mw = 760.f, mh = 530.f;
        float mx = position.x + (width - mw) / 2.f;
        float my = position.y + (height - mh) / 2.f;
        modalOpenExcelHovered = FloatRect({mx + mw - 340.f, my + mh - 50.f}, {210.f, 36.f}).contains(mousePos);
        modalCloseHovered = FloatRect({mx + mw - 115.f, my + mh - 50.f}, {100.f, 36.f}).contains(mousePos);
    }

    if (detailMonthIndex >= 0 && detailMonthIndex < (int)monthsList.size()) {
        float mw = 540.f, mh = 370.f;
        float mx = position.x + (width - mw) / 2.f;
        float my = position.y + (height - mh) / 2.f;
        detailFilterBtnHovered = FloatRect({mx + mw - 305.f, my + mh - 52.f}, {180.f, 36.f}).contains(mousePos);
        detailCloseBtnHovered = FloatRect({mx + mw - 115.f, my + mh - 52.f}, {92.f, 36.f}).contains(mousePos);
    }

    hoveredDetailRow = -1;
    if (detailMonthIndex < 0 && !reportModalOpen && activeTab == RevenueTab::TOTAL_REVENUE && !monthsList.empty()) {
        const float contentY = position.y + 250.f;
        const float availableW = width - 40.f;
        const float topH = 296.f;
        const float bTableY = contentY + topH + 14.f;
        const float bTableH = height - 28.f - bTableY - 4.f;
        const float col_act = position.x + 20.f + availableW * 0.89f;
        float rTop = bTableY + 58.f + 34.f;
        const float footerH = 50.f;
        const float rH = (bTableH - (rTop - bTableY) - footerH) / static_cast<float>(monthsList.size());

        for (size_t i = 0; i < monthsList.size(); ++i) {
            float ry = rTop + i * rH;
            FloatRect detailRect({col_act - 6.f, ry + (rH - 30.f) / 2.f}, {88.f, 30.f});
            if (detailRect.contains(mousePos)) {
                hoveredDetailRow = static_cast<int>(i);
                break;
            }
        }
    }

    if (toastActive) {
        toastTimer -= 0.016f;
        if (toastTimer <= 0.f) toastActive = false;
    }
}

void MovieRevenuePanel::render(RenderTarget& target) const {
    // 1. Off-white Canvas Background
    RectangleShape bg(Vector2f(width, height));
    bg.setPosition(position);
    bg.setFillColor(Color(241, 245, 249)); // Slate-100
    target.draw(bg);

    // 2. Header
    drawHeader(target);

    // 3. 3 KPI Summary Cards
    drawKpiCards(target);

    // 4. Sub-Tabs Bar
    drawTabBar(target);

    // 5. Active Tab View
    switch (activeTab) {
        case RevenueTab::TICKETS_BY_MOVIE:
            drawTab1TicketsByMovie(target);
            break;
        case RevenueTab::TOTAL_REVENUE:
            drawTab2TotalRevenue(target);
            break;
        case RevenueTab::REVENUE_BY_MOVIE:
            drawTab3RevenueByMovie(target);
            break;
        case RevenueTab::REVENUE_BY_COMBO:
            drawTab4RevenueByCombo(target);
            break;
    }

    // 6. Bottom Engine Status Bar
    drawBottomBar(target);

    // 6b. Dropdown chọn kỳ doanh thu
    if (dateDropdownOpen) {
        float ddX = position.x + width - 530.f;
        float ddY = position.y + 79.f;
        float ddW = 260.f;
        float itemH = 36.f;
        const int optCount = static_cast<int>(dateFilterOptions.size());
        RoundedRectRenderer::draw(target, {ddX, ddY}, {ddW, optCount * itemH}, 8.f, Color::White, 1.2f, Color(203, 213, 225));
        for (int i = 0; i < optCount; ++i) {
            Color itemBg = (i == hoveredDateItem) ? Color(241, 245, 249) : (i == selectedDateFilter ? Color(224, 242, 254) : Color::White);
            Color itemFg = (i == selectedDateFilter) ? Color(8, 125, 181) : Color(51, 65, 85);
            RoundedRectRenderer::draw(target, {ddX + 2.f, ddY + i * itemH + 2.f}, {ddW - 4.f, itemH - 4.f}, 4.f, itemBg);
            Text itTxt(i == selectedDateFilter ? boldFont : font, dateFilterOptions[i].first, 16);
            itTxt.setFillColor(itemFg);
            FloatRect itb = itTxt.getLocalBounds();
            itTxt.setPosition({ddX + 16.f, std::round(ddY + i * itemH + (itemH - itb.size.y) / 2.f - itb.position.y)});
            target.draw(itTxt);
            if (i == selectedDateFilter) {
                UITheme::drawCheckmark(target, {ddX + ddW - 20.f, ddY + i * itemH + 18.f}, 0.7f, Color(8, 125, 181), 1.8f);
            }
        }
    }

    // 7. Modal Báo cáo doanh thu (Hiển thị ngay trên màn hình khi nhấn xuất báo cáo)
    if (reportModalOpen) {
        drawReportModal(target);
    }

    // 7b. Modal Chi tiết doanh thu từng tháng
    if (detailMonthIndex >= 0 && detailMonthIndex < (int)monthsList.size()) {
        drawMonthDetailModal(target);
    }

    // 8. Toast Notification
    if (toastActive) {
        drawToast(target);
    }
}

void MovieRevenuePanel::drawHeader(RenderTarget& target) const {
    wstring badgeStr = L"Quyết toán tài chính theo kỳ";
    Color bBg = Color(224, 242, 254);
    Color bTxt = Color(8, 125, 181);

    if (activeTab == RevenueTab::TOTAL_REVENUE) {
        badgeStr = L"Quyết toán thực tế";
        bBg = Color(220, 252, 231);
        bTxt = Color(21, 128, 61);
    } else if (activeTab == RevenueTab::REVENUE_BY_MOVIE) {
        badgeStr = L"Cập nhật lúc 22:45 hôm nay";
        bBg = Color(241, 245, 249);
        bTxt = Color(100, 116, 139);
    } else if (activeTab == RevenueTab::REVENUE_BY_COMBO) {
        badgeStr = L"Kỳ Q3-Q4/2025";
        bBg = Color(254, 243, 199);
        bTxt = Color(217, 119, 6);
    }

    UITheme::drawSectionHeaderCard(target, position.x + 20.f, position.y + 14.f, width - 40.f,
                                   L"Quản lý doanh thu",
                                   badgeStr, bBg, bTxt,
                                   L"Thống kê chi tiết bán vé, cơ cấu sản phẩm F&B và doanh thu phim chiếu rạp.",
                                   boldFont, font, false);

    // Right Controls inside header card
    float btnY = position.y + 37.f;
    Vector2f dateRangePos(position.x + width - 530.f, btnY);
    RoundedRectRenderer::draw(target, dateRangePos, {260.f, 38.f}, 8.f, Color::White, dateDropdownOpen ? 1.6f : 1.f, dateDropdownOpen ? Color(8, 125, 181) : Color(203, 213, 225));

    wstring curDateStr = (!dateFilterOptions.empty() && selectedDateFilter >= 0 && selectedDateFilter < (int)dateFilterOptions.size())
                         ? dateFilterOptions[selectedDateFilter].first
                         : L"Tất cả thời gian";
    Text dateRangeTxt(font, curDateStr, 16);
    dateRangeTxt.setFillColor(Color(30, 41, 59));
    FloatRect drb = dateRangeTxt.getLocalBounds();
    dateRangeTxt.setPosition({std::round(dateRangePos.x + 14.f), std::round(dateRangePos.y + (38.f - drb.size.y) / 2.f - drb.position.y)});
    target.draw(dateRangeTxt);
    UITheme::drawChevronDown(target, {dateRangePos.x + 242.f, dateRangePos.y + 19.f}, 4.2f, Color(100, 116, 139));

    // Button "Tải dữ liệu"
    Vector2f reloadPos(position.x + width - 260.f, btnY);
    Color reloadBg = reloadHovered ? Color(241, 245, 249) : Color::White;
    Color reloadBorder = reloadHovered ? Color(8, 125, 181) : Color(203, 213, 225);
    RoundedRectRenderer::draw(target, reloadPos, {110.f, 38.f}, 8.f, reloadBg, 1.f, reloadBorder);
    UITheme::drawReloadIcon(target, {reloadPos.x + 20.f, reloadPos.y + 19.f}, 15.f, Color(71, 85, 105));
    Text reloadTxt(boldFont, L"Tải dữ liệu", 14);
    reloadTxt.setFillColor(Color(51, 65, 85));
    FloatRect relB = reloadTxt.getLocalBounds();
    reloadTxt.setPosition({std::round(reloadPos.x + 34.f), std::round(reloadPos.y + (38.f - relB.size.y) / 2.f - relB.position.y)});
    target.draw(reloadTxt);

    // Button "Xuất Excel"
    Vector2f exportPos(position.x + width - 140.f, btnY);
    Color exportBg = exportHovered ? Color(2, 132, 199) : Color(8, 125, 181);
    RoundedRectRenderer::draw(target, exportPos, {105.f, 38.f}, 8.f, exportBg);
    Text exportTxt(boldFont, L"Xuất Excel", 14);
    exportTxt.setFillColor(Color::White);
    FloatRect expB = exportTxt.getLocalBounds();
    exportTxt.setPosition({std::round(exportPos.x + (105.f - expB.size.x) / 2.f - expB.position.x), std::round(exportPos.y + (38.f - expB.size.y) / 2.f - expB.position.y)});
    target.draw(exportTxt);
}

void MovieRevenuePanel::drawKpiCards(RenderTarget& target) const {
    const float cardY = position.y + 106.f;
    const float cardH = 94.f;
    const float margin = 20.f;
    const float gap = 14.f;
    const float availableW = width - margin * 2.f;
    const float cardW = (availableW - gap * 2.f) / 3.f; // ~460px

    // 1. Tổng doanh thu (01/09/2025 - 24/12/2025)
    {
        Vector2f cPos(position.x + margin, cardY);
        RoundedRectRenderer::draw(target, cPos, {cardW, cardH}, 8.f, Color::White, 1.f, Color(226, 232, 240));

        Text lbl(font);
        lbl.setString(L"Tổng doanh thu toàn rạp (01/09/2025 - 24/12/2025)");
        lbl.setCharacterSize(16);
        lbl.setFillColor(Color(100, 116, 139));
        lbl.setPosition({cPos.x + 16.f, std::round(cPos.y + 10.f)});
        target.draw(lbl);

        // Big value
        Text val(boldFont);
        val.setString(s2ws(toCurrency(totalRevenue)));
        val.setCharacterSize(24);
        val.setFillColor(Color(23, 32, 51));
        val.setPosition({cPos.x + 16.f, std::round(cPos.y + 34.f)});
        target.draw(val);

        // Footer
        Text fLeft(boldFont);
        fLeft.setString(L"+18.4% so với quý trước");
        fLeft.setCharacterSize(15);
        fLeft.setFillColor(Color(21, 128, 61));
        fLeft.setPosition({cPos.x + 16.f, std::round(cPos.y + 68.f)});
        target.draw(fLeft);

        Text fRight(font);
        fRight.setString(L"Đã quyết toán VAT");
        fRight.setCharacterSize(15);
        fRight.setFillColor(Color(148, 163, 184));
        fRight.setPosition({std::round(cPos.x + cardW - 16.f - fRight.getGlobalBounds().size.x), std::round(cPos.y + 68.f)});
        target.draw(fRight);
    }

    // 2. Doanh thu phim (01/09/2025 - 24/12/2025)
    {
        Vector2f cPos(position.x + margin + (cardW + gap), cardY);
        bool isMovieTab = (activeTab == RevenueTab::REVENUE_BY_MOVIE);
        Color borderCol = isMovieTab ? Color(8, 125, 181) : Color(226, 232, 240);
        float borderW = isMovieTab ? 2.f : 1.f;
        RoundedRectRenderer::draw(target, cPos, {cardW, cardH}, 8.f, Color::White, borderW, borderCol);

        Text lbl(font);
        lbl.setString(L"Doanh thu phim (01/09/2025 - 24/12/2025)");
        lbl.setCharacterSize(16);
        lbl.setFillColor(Color(100, 116, 139));
        lbl.setPosition({cPos.x + 16.f, std::round(cPos.y + 10.f)});
        target.draw(lbl);

        Text val(boldFont);
        val.setString(s2ws(toCurrency(movieRevenue)));
        val.setCharacterSize(24);
        val.setFillColor(Color(23, 32, 51));
        val.setPosition({cPos.x + 16.f, std::round(cPos.y + 34.f)});
        target.draw(val);

        Text fLeft(boldFont);
        fLeft.setString(L"Chiếm 59.1% tổng nguồn thu");
        fLeft.setCharacterSize(15);
        fLeft.setFillColor(Color(8, 125, 181));
        fLeft.setPosition({cPos.x + 16.f, std::round(cPos.y + 68.f)});
        target.draw(fLeft);

        Text fRight(font);
        fRight.setString(to_wstring(totalTickets) + L" vé đã xuất");
        fRight.setCharacterSize(15);
        fRight.setFillColor(Color(148, 163, 184));
        fRight.setPosition({std::round(cPos.x + cardW - 16.f - fRight.getGlobalBounds().size.x), std::round(cPos.y + 68.f)});
        target.draw(fRight);
    }

    // 3. Doanh thu combo (01/09/2025 - 24/12/2025)
    {
        Vector2f cPos(position.x + margin + 2 * (cardW + gap), cardY);
        bool isComboTab = (activeTab == RevenueTab::REVENUE_BY_COMBO);
        Color borderCol = isComboTab ? Color(245, 158, 11) : Color(226, 232, 240);
        float borderW = isComboTab ? 2.f : 1.f;
        Color bgCol = isComboTab ? Color(255, 251, 235) : Color::White;
        RoundedRectRenderer::draw(target, cPos, {cardW, cardH}, 8.f, bgCol, borderW, borderCol);

        Text lbl(font);
        lbl.setString(L"Doanh thu combo F&B (01/09/2025 - 24/12/2025)");
        lbl.setCharacterSize(16);
        lbl.setFillColor(Color(100, 116, 139));
        lbl.setPosition({cPos.x + 16.f, std::round(cPos.y + 10.f)});
        target.draw(lbl);

        Text val(boldFont);
        val.setString(s2ws(toCurrency(comboRevenue)));
        val.setCharacterSize(24);
        val.setFillColor(isComboTab ? Color(180, 83, 9) : Color(23, 32, 51));
        val.setPosition({cPos.x + 16.f, std::round(cPos.y + 34.f)});
        target.draw(val);

        Text fLeft(boldFont);
        fLeft.setString(L"Chiếm 40.9% tổng nguồn thu");
        fLeft.setCharacterSize(15);
        fLeft.setFillColor(Color(217, 119, 6));
        fLeft.setPosition({cPos.x + 16.f, std::round(cPos.y + 68.f)});
        target.draw(fLeft);

        Text fRight(font);
        fRight.setString(to_wstring(totalCombos) + L" Combo & Nước");
        fRight.setCharacterSize(15);
        fRight.setFillColor(Color(148, 163, 184));
        fRight.setPosition({std::round(cPos.x + cardW - 16.f - fRight.getGlobalBounds().size.x), std::round(cPos.y + 68.f)});
        target.draw(fRight);
    }
}

void MovieRevenuePanel::drawTabBar(RenderTarget& target) const {
    const float tabY = position.y + 206.f;
    const float tabH = 34.f;
    float tabX = position.x + 20.f;

    struct TabDef {
        RevenueTab tab;
        wstring title;
        float width;
    };

    const array<TabDef, 4> tabs = {
        TabDef{RevenueTab::TICKETS_BY_MOVIE, L"Số vé bán ra theo phim", 200.f},
        TabDef{RevenueTab::TOTAL_REVENUE, L"Tổng doanh thu", 140.f},
        TabDef{RevenueTab::REVENUE_BY_MOVIE, L"Doanh thu theo phim", 180.f},
        TabDef{RevenueTab::REVENUE_BY_COMBO, L"Doanh thu theo combo", 190.f}
    };

    for (size_t i = 0; i < tabs.size(); ++i) {
        bool isActive = (activeTab == tabs[i].tab);
        bool isHov = (hoveredTab == (int)i);

        Color bg = isActive ? Color(8, 125, 181) : (isHov ? Color(241, 245, 249) : Color::White);
        Color border = isActive ? Color(8, 125, 181) : Color(203, 213, 225);
        Color txtCol = isActive ? Color::White : Color(51, 65, 85);

        RoundedRectRenderer::draw(target, {tabX, tabY}, {tabs[i].width, tabH}, 6.f, bg, 1.f, border);

        Text tTxt(boldFont);
        tTxt.setString(tabs[i].title);
        tTxt.setCharacterSize(14);
        tTxt.setFillColor(txtCol);
        FloatRect tb = tTxt.getLocalBounds();
        tTxt.setPosition({
            tabX + (tabs[i].width - tb.size.x) / 2.f - tb.position.x,
            tabY + (tabH - tb.size.y) / 2.f - tb.position.y
        });
        target.draw(tTxt);

        tabX += tabs[i].width + 10.f;
    }
}

void MovieRevenuePanel::drawTab1TicketsByMovie(RenderTarget& target) const {
    const float fullW = width - 40.f;
    const float topCardY = position.y + 250.f;
    const float topCardH = 262.f;
    const float botCardY = topCardY + topCardH + 12.f;
    const float botCardH = height - 28.f - botCardY - 4.f;

    // =========================================================================
    // PHẦN 1 (TRÊN): BIỂU ĐỒ ĐỐI CHIẾU SỐ LƯỢNG VÉ
    // =========================================================================
    {
        Vector2f cPos(position.x + 20.f, topCardY);
        RoundedRectRenderer::draw(target, cPos, {fullW, topCardH}, 10.f, Color::White, 1.f, Color(226, 232, 240));

        Text title(boldFont, L"BIỂU ĐỒ ĐỐI CHIẾU SỐ LƯỢNG VÉ", 14);
        title.setFillColor(Color(23, 32, 51));
        title.setPosition({cPos.x + 24.f, cPos.y + 18.f});
        target.draw(title);

        // Badge "Đơn vị: Vé" ở góc phải trên
        Vector2f unitBoxSize(92.f, 28.f);
        Vector2f unitBoxPos(cPos.x + fullW - 24.f - unitBoxSize.x, cPos.y + 14.f);
        RoundedRectRenderer::draw(target, unitBoxPos, unitBoxSize, 6.f, Color(248, 250, 252), 1.f, Color(226, 232, 240));
        Text unit(boldFont, L"Đơn vị: Vé", 14);
        unit.setFillColor(Color(71, 85, 105));
        FloatRect ub = unit.getLocalBounds();
        unit.setPosition({
            unitBoxPos.x + (unitBoxSize.x - ub.size.x) / 2.f - ub.position.x,
            unitBoxPos.y + (unitBoxSize.y - ub.size.y) / 2.f - ub.position.y
        });
        target.draw(unit);

        // Khu vực vẽ biểu đồ cột trải rộng toàn bộ chiều ngang
        const float chartLeft = cPos.x + 56.f;
        const float chartBottom = cPos.y + 190.f;
        const float chartW = fullW - 84.f;
        const float maxVal = 40.f;
        const float chartH = 122.f;

        const array<int, 5> yTicks = {0, 10, 20, 30, 40};
        for (int tick : yTicks) {
            float y = chartBottom - (tick / maxVal) * chartH;
            Text lbl(font, to_string(tick), 14);
            lbl.setFillColor(Color(148, 163, 184));
            FloatRect lb = lbl.getLocalBounds();
            lbl.setPosition({cPos.x + 24.f, y - lb.size.y / 2.f - lb.position.y});
            target.draw(lbl);

            RectangleShape line(Vector2f(chartW, 1.f));
            line.setPosition({chartLeft, y});
            line.setFillColor(Color(241, 245, 249));
            target.draw(line);
        }

        int n = min(8, (int)moviesList.size());
        float colW = 48.f;
        float colGap = (chartW - (n * colW)) / (n + 1);

        for (int i = 0; i < n; ++i) {
            float bx = chartLeft + colGap + i * (colW + colGap);
            float bHeight = (moviesList[i].tickets / maxVal) * chartH;
            float by = chartBottom - bHeight;

            Color bCol = (i < 5) ? Color(8, 125, 181) : Color(148, 163, 184);
            RoundedRectRenderer::draw(target, {bx, by}, {colW, bHeight}, 5.f, bCol);

            Text valTxt(boldFont, to_string(moviesList[i].tickets), 14);
            valTxt.setFillColor(i == 0 ? Color(8, 125, 181) : Color(51, 65, 85));
            FloatRect vb = valTxt.getLocalBounds();
            valTxt.setPosition({bx + (colW - vb.size.x) / 2.f - vb.position.x, by - 20.f});
            target.draw(valTxt);

            Text nameTxt(i == 0 ? boldFont : font, moviesList[i].shortTitle, 14);
            nameTxt.setFillColor(i == 0 ? Color(23, 32, 51) : Color(100, 116, 139));
            FloatRect nb = nameTxt.getLocalBounds();
            nameTxt.setPosition({bx + (colW - nb.size.x) / 2.f - nb.position.x, chartBottom + 6.f});
            target.draw(nameTxt);
        }

        // Chân thẻ biểu đồ: Hộp phim dẫn đầu bên trái & Tổng số vé bên phải
        wstring topMovieTitle = moviesList.empty() ? L"Tử Chiến Trên Không" : moviesList[0].title;
        int topMovieTickets = moviesList.empty() ? 31 : moviesList[0].tickets;
        long long topMovieRev = moviesList.empty() ? 3890000 : moviesList[0].revenue;

        wstring leadStr = L"Phim dẫn đầu: " + topMovieTitle + L" (" + to_wstring(topMovieTickets) + L" vé — Doanh thu: " + s2ws(toCurrency(topMovieRev)) + L")";
        Text leadTxt(boldFont, leadStr, 14);
        leadTxt.setFillColor(Color(146, 64, 14));
        FloatRect ltb = leadTxt.getLocalBounds();

        Vector2f boxPos(cPos.x + 24.f, cPos.y + topCardH - 42.f);
        Vector2f boxSize(ltb.size.x + 44.f, 32.f);
        RoundedRectRenderer::draw(target, boxPos, boxSize, 8.f, Color(255, 251, 235), 1.f, Color(253, 230, 138));

        // Biểu tượng cúp vàng nhỏ bên trái
        CircleShape cupBowl(5.f);
        cupBowl.setFillColor(Color(245, 158, 11));
        cupBowl.setPosition({boxPos.x + 12.f, boxPos.y + 8.f});
        target.draw(cupBowl);
        RectangleShape cupStem({3.f, 5.f});
        cupStem.setFillColor(Color(217, 119, 6));
        cupStem.setPosition({boxPos.x + 15.5f, boxPos.y + 17.f});
        target.draw(cupStem);
        RectangleShape cupBase({9.f, 2.5f});
        cupBase.setFillColor(Color(180, 83, 9));
        cupBase.setPosition({boxPos.x + 12.5f, boxPos.y + 21.5f});
        target.draw(cupBase);

        leadTxt.setPosition({
            boxPos.x + 30.f - ltb.position.x,
            boxPos.y + (boxSize.y - ltb.size.y) / 2.f - ltb.position.y
        });
        target.draw(leadTxt);

        // Tổng số vé xuất rạp bên phải
        wstring rightTotalStr = L"Tổng số vé xuất rạp giai đoạn: " + to_wstring(totalTickets) + L" vé";
        Text rightTotalTxt(boldFont, rightTotalStr, 14);
        rightTotalTxt.setFillColor(Color(51, 65, 85));
        FloatRect rtb = rightTotalTxt.getLocalBounds();
        rightTotalTxt.setPosition({
            cPos.x + fullW - 24.f - rtb.size.x - rtb.position.x,
            boxPos.y + (boxSize.y - rtb.size.y) / 2.f - rtb.position.y
        });
        target.draw(rightTotalTxt);
    }

    // =========================================================================
    // PHẦN 2 (DƯỚI): BẢNG SỐ VÉ BÁN RA THEO PHIM (CỠ CHỮ LỚN, RÕ RÀNG)
    // =========================================================================
    {
        Vector2f cPos(position.x + 20.f, botCardY);
        RoundedRectRenderer::draw(target, cPos, {fullW, botCardH}, 10.f, Color::White, 1.f, Color(226, 232, 240));

        // Tiêu đề bảng theo phong cách Tổng quan (chấm xanh + tiêu đề đậm)
        CircleShape sDot(4.f);
        sDot.setPosition({cPos.x + 20.f, cPos.y + 21.f});
        sDot.setFillColor(Color(21, 128, 61));
        target.draw(sDot);

        Text sTitle(boldFont, L"Số vé bán ra theo phim (01/09/2025 – 24/12/2025)", 17);
        sTitle.setFillColor(Color(23, 32, 51));
        sTitle.setPosition({cPos.x + 34.f, std::round(cPos.y + 13.f)});
        target.draw(sTitle);

        // Badge tổng hợp góc phải trên của bảng
        wstring legStr = L"● Số vé bán ra (Vé)   |   Tổng cộng: " + to_wstring(totalTickets) + L" vé";
        Text sLegend(boldFont, legStr, 14);
        sLegend.setFillColor(Color(8, 125, 181));
        FloatRect slb = sLegend.getLocalBounds();
        Vector2f legBoxSize(slb.size.x + 28.f, 30.f);
        Vector2f legBoxPos(cPos.x + fullW - 20.f - legBoxSize.x, cPos.y + 10.f);
        RoundedRectRenderer::draw(target, legBoxPos, legBoxSize, 8.f, Color(240, 249, 255), 1.f, Color(186, 230, 253));
        sLegend.setPosition({
            legBoxPos.x + (legBoxSize.x - slb.size.x) / 2.f - slb.position.x,
            legBoxPos.y + (legBoxSize.y - slb.size.y) / 2.f - slb.position.y
        });
        target.draw(sLegend);

        // Khung bảng & các cột có đường ngăn cách giống bảng Tổng quan
        float tblX = cPos.x + 20.f;
        float tblW = fullW - 40.f;
        float tblRight = tblX + tblW;
        float thBoxY = cPos.y + 48.f;
        float thBoxH = 34.f;

        float div0 = tblX + 54.f;
        float div1 = tblX + std::floor(tblW * 0.34f);
        float div2 = tblX + std::floor(tblW * 0.50f);
        float div3 = tblX + std::floor(tblW * 0.61f);
        float div4 = tblX + std::floor(tblW * 0.72f);
        float div5 = tblX + std::floor(tblW * 0.85f);

        // Thanh tiêu đề Ocean Blue bo góc + vạch ngăn dọc
        RoundedRectRenderer::draw(target, {tblX, thBoxY}, {tblW, thBoxH}, 6.f, Color(8, 125, 181));
        for (float dx : {div0, div1, div2, div3, div4, div5}) {
            RectangleShape hDiv(Vector2f(1.f, thBoxH));
            hDiv.setPosition({dx, thBoxY});
            hDiv.setFillColor(Color(255, 255, 255, 55));
            target.draw(hDiv);
        }

        auto drawRevHdr = [&](const wstring& lbl, float xStart, float xEnd) {
            Text h(boldFont, lbl, 13);
            h.setFillColor(Color::White);
            FloatRect hb = h.getLocalBounds();
            h.setPosition({
                std::round(xStart + (xEnd - xStart - hb.size.x) / 2.f - hb.position.x),
                std::round(thBoxY + (thBoxH - hb.size.y) / 2.f - hb.position.y)
            });
            target.draw(h);
        };
        drawRevHdr(L"#", tblX, div0);
        drawRevHdr(L"TÊN PHIM", div0, div1);
        drawRevHdr(L"THỂ LOẠI", div1, div2);
        drawRevHdr(L"SUẤT CHIẾU", div2, div3);
        drawRevHdr(L"SỐ VÉ BÁN", div3, div4);
        drawRevHdr(L"DOANH THU PHIM", div4, div5);
        drawRevHdr(L"TỶ TRỌNG", div5, tblRight);

        // Các dòng dữ liệu phim
        float rowTop = thBoxY + thBoxH;
        float footerH = 38.f;
        int rowCount = max(1, (int)moviesList.size());
        float rowH = (botCardH - (rowTop - cPos.y) - footerH - 12.f) / (float)rowCount;

        int sumShowtimes = 0;
        for (size_t i = 0; i < moviesList.size(); ++i) {
            sumShowtimes += moviesList[i].showtimes;
            float ry = rowTop + i * rowH;
            float centerY = ry + rowH / 2.f;

            RoundedRectRenderer::draw(target, {tblX, ry}, {tblW, rowH}, 0.f, Color::White, 0.5f, Color(226, 232, 240));
            for (float dx : {div0, div1, div2, div3, div4, div5}) {
                RectangleShape rDiv(Vector2f(1.f, rowH));
                rDiv.setPosition({dx, ry});
                rDiv.setFillColor(Color(241, 245, 249));
                target.draw(rDiv);
            }

            // Hạng (Rank Circle đồng bộ kiểu Tổng quan cho cả top 3 và 4+)
            Color rBg = (i == 0) ? Color(245, 158, 11)
                      : (i == 1) ? Color(148, 163, 184)
                      : (i == 2) ? Color(217, 119, 6)
                                 : Color(241, 245, 249);
            Color rFg = (i < 3) ? Color::White : Color(100, 116, 139);
            float rRadius = 10.f;
            CircleShape c(rRadius);
            c.setPosition({std::round(tblX + (div0 - tblX) / 2.f - rRadius), std::round(centerY - rRadius)});
            c.setFillColor(rBg);
            target.draw(c);

            Text rTxt(boldFont, to_string(i + 1), 13);
            rTxt.setFillColor(rFg);
            FloatRect rb = rTxt.getLocalBounds();
            rTxt.setPosition({
                std::round(tblX + (div0 - tblX) / 2.f - rb.size.x / 2.f - rb.position.x),
                std::round(centerY - rb.size.y / 2.f - rb.position.y)
            });
            target.draw(rTxt);

            // Tên phim + nhãn HOT
            Text tTxt(boldFont, moviesList[i].title, 13);
            tTxt.setFillColor(Color(23, 32, 51));
            FloatRect ttb = tTxt.getLocalBounds();
            tTxt.setPosition({div0 + 14.f, std::round(centerY - ttb.size.y / 2.f - ttb.position.y)});
            target.draw(tTxt);

            if (moviesList[i].isHot) {
                Vector2f hotSize(42.f, 20.f);
                Vector2f hotPos(div0 + 14.f + ttb.size.x + 10.f, std::round(centerY - hotSize.y / 2.f));
                RoundedRectRenderer::draw(target, hotPos, hotSize, 4.f, Color(254, 226, 226));
                Text hotTxt(boldFont, L"HOT", 12);
                hotTxt.setFillColor(Color(225, 29, 72));
                FloatRect hb = hotTxt.getLocalBounds();
                hotTxt.setPosition({
                    std::round(hotPos.x + (hotSize.x - hb.size.x) / 2.f - hb.position.x),
                    std::round(hotPos.y + (hotSize.y - hb.size.y) / 2.f - hb.position.y)
                });
                target.draw(hotTxt);
            }

            // Thể loại (căn giữa)
            Text gTxt(font, moviesList[i].genre, 13);
            gTxt.setFillColor(Color(100, 116, 139));
            FloatRect gtb = gTxt.getLocalBounds();
            gTxt.setPosition({
                std::round(div1 + (div2 - div1 - gtb.size.x) / 2.f - gtb.position.x),
                std::round(centerY - gtb.size.y / 2.f - gtb.position.y)
            });
            target.draw(gTxt);

            // Suất chiếu (căn giữa)
            Text sTxt(boldFont, to_string(moviesList[i].showtimes), 13);
            sTxt.setFillColor(Color(51, 65, 85));
            FloatRect stb = sTxt.getLocalBounds();
            sTxt.setPosition({
                std::round(div2 + (div3 - div2 - stb.size.x) / 2.f - stb.position.x),
                std::round(centerY - stb.size.y / 2.f - stb.position.y)
            });
            target.draw(sTxt);

            // Số vé bán (căn giữa, xanh dương giống Tổng quan)
            Text tickTxt(boldFont, to_wstring(moviesList[i].tickets) + L" vé", 13);
            tickTxt.setFillColor(Color(8, 125, 181));
            FloatRect tkb = tickTxt.getLocalBounds();
            tickTxt.setPosition({
                std::round(div3 + (div4 - div3 - tkb.size.x) / 2.f - tkb.position.x),
                std::round(centerY - tkb.size.y / 2.f - tkb.position.y)
            });
            target.draw(tickTxt);

            // Doanh thu phim (căn giữa, đỏ crimson giống Tổng quan)
            Text revTxt(boldFont, s2ws(toCurrency(moviesList[i].revenue)), 14);
            revTxt.setFillColor(Color(225, 29, 72));
            FloatRect rvb = revTxt.getLocalBounds();
            revTxt.setPosition({
                std::round(div4 + (div5 - div4 - rvb.size.x) / 2.f - rvb.position.x),
                std::round(centerY - rvb.size.y / 2.f - rvb.position.y)
            });
            target.draw(revTxt);

            // Tỷ trọng (Thanh tiến trình nhỏ + % căn giữa trong cột)
            float barTrackW = 64.f;
            float barH = 6.f;
            stringstream ss;
            ss << fixed << setprecision(1) << moviesList[i].share << "%";
            Text shTxt(boldFont, ss.str(), 13);
            shTxt.setFillColor(Color(71, 85, 105));
            FloatRect shb = shTxt.getLocalBounds();

            float totalShareW = barTrackW + 10.f + shb.size.x;
            float shareStartX = div5 + (tblRight - div5 - totalShareW) / 2.f;
            Vector2f barPos(std::round(shareStartX), std::round(centerY - barH / 2.f));
            RoundedRectRenderer::draw(target, barPos, {barTrackW, barH}, 3.f, Color(241, 245, 249));
            float fillW = max(5.f, min(barTrackW, (moviesList[i].share / 40.f) * barTrackW));
            RoundedRectRenderer::draw(target, barPos, {fillW, barH}, 3.f, Color(8, 125, 181));

            shTxt.setPosition({
                std::round(shareStartX + barTrackW + 10.f - shb.position.x),
                std::round(centerY - shb.size.y / 2.f - shb.position.y)
            });
            target.draw(shTxt);
        }

        // Dòng tổng cộng dưới đáy bảng (TỔNG CỘNG TOÀN BỘ)
        float footY = rowTop + rowCount * rowH;
        RoundedRectRenderer::draw(target, {tblX, footY}, {tblW, footerH}, 0.f, Color(248, 250, 252), 0.5f, Color(226, 232, 240));
        for (float dx : {div2, div3, div4, div5}) {
            RectangleShape fDiv(Vector2f(1.f, footerH));
            fDiv.setPosition({dx, footY});
            fDiv.setFillColor(Color(226, 232, 240));
            target.draw(fDiv);
        }

        float fCenterY = footY + footerH / 2.f;

        Text fTitle(boldFont, L"TỔNG CỘNG TOÀN BỘ", 13);
        fTitle.setFillColor(Color(71, 85, 105));
        FloatRect ftb = fTitle.getLocalBounds();
        fTitle.setPosition({tblX + 16.f, std::round(fCenterY - ftb.size.y / 2.f - ftb.position.y)});
        target.draw(fTitle);

        Text fShow(boldFont, to_wstring(sumShowtimes) + L" suất", 13);
        fShow.setFillColor(Color(23, 32, 51));
        FloatRect fsb = fShow.getLocalBounds();
        fShow.setPosition({
            std::round(div2 + (div3 - div2 - fsb.size.x) / 2.f - fsb.position.x),
            std::round(fCenterY - fsb.size.y / 2.f - fsb.position.y)
        });
        target.draw(fShow);

        Text fTick(boldFont, to_wstring(totalTickets) + L" vé", 14);
        fTick.setFillColor(Color(8, 125, 181));
        FloatRect ftkb = fTick.getLocalBounds();
        fTick.setPosition({
            std::round(div3 + (div4 - div3 - ftkb.size.x) / 2.f - ftkb.position.x),
            std::round(fCenterY - ftkb.size.y / 2.f - ftkb.position.y)
        });
        target.draw(fTick);

        Text fRev(boldFont, s2ws(toCurrency(movieRevenue)), 14);
        fRev.setFillColor(Color(225, 29, 72));
        FloatRect frvb = fRev.getLocalBounds();
        fRev.setPosition({
            std::round(div4 + (div5 - div4 - frvb.size.x) / 2.f - frvb.position.x),
            std::round(fCenterY - frvb.size.y / 2.f - frvb.position.y)
        });
        target.draw(fRev);

        Text fShare(boldFont, L"100.0%", 13);
        fShare.setFillColor(Color(23, 32, 51));
        FloatRect fshb = fShare.getLocalBounds();
        fShare.setPosition({
            std::round(div5 + (tblRight - div5 - fshb.size.x) / 2.f - fshb.position.x),
            std::round(fCenterY - fshb.size.y / 2.f - fshb.position.y)
        });
        target.draw(fShare);
    }
}

void MovieRevenuePanel::drawTab2TotalRevenue(RenderTarget& target) const {
    const float contentY = position.y + 250.f;
    const float availableW = width - 40.f;
    const float topH = 296.f;
    const float leftW = availableW * 0.56f;
    const float rightW = availableW - leftW - 14.f;

    // =========================================================================
    // PHẦN 1A (TRÊN - TRÁI): BIỂU ĐỒ ĐỐI SOÁT DÒNG TIỀN THEO THÁNG
    // =========================================================================
    {
        Vector2f cPos(position.x + 20.f, contentY);
        RoundedRectRenderer::draw(target, cPos, {leftW, topH}, 10.f, Color::White, 1.f, Color(226, 232, 240));

        Text title(boldFont, L"Biểu đồ đối soát dòng tiền theo tháng", 14);
        title.setFillColor(Color(23, 32, 51));
        title.setPosition({cPos.x + 24.f, cPos.y + 14.f});
        target.draw(title);

        Text sub(font, L"Doanh thu vé chiếu và sản phẩm F&B qua các chu kỳ (0 - 5.000.000 đ)", 14);
        sub.setFillColor(Color(148, 163, 184));
        sub.setPosition({cPos.x + 24.f, cPos.y + 38.f});
        target.draw(sub);

        // Chú thích góc phải trên: ● Vé phim (xanh)  ● Combo F&B (cam)
        Text leg2(boldFont, L"● Combo F&B", 14);
        leg2.setFillColor(Color(245, 158, 11));
        FloatRect l2b = leg2.getLocalBounds();
        leg2.setPosition({cPos.x + leftW - 24.f - l2b.size.x, cPos.y + 18.f});
        target.draw(leg2);

        Text leg1(boldFont, L"● Vé phim", 14);
        leg1.setFillColor(Color(8, 125, 181));
        FloatRect l1b = leg1.getLocalBounds();
        leg1.setPosition({leg2.getPosition().x - 20.f - l1b.size.x, cPos.y + 18.f});
        target.draw(leg1);

        // Khu vực biểu đồ đường (Line Chart)
        const float chartL = cPos.x + 36.f;
        const float chartB = cPos.y + 232.f;
        const float chartW = leftW - 72.f;
        const float maxVal = 5000000.f;
        const float chartH = 145.f;

        // Các đường lưới ngang mờ
        for (int g = 0; g <= 3; ++g) {
            float gy = chartB - (g / 3.f) * chartH;
            RectangleShape gLine(Vector2f(chartW, 1.f));
            gLine.setPosition({chartL, gy});
            gLine.setFillColor(g == 0 ? Color(226, 232, 240) : Color(248, 250, 252));
            target.draw(gLine);
        }

        const array<float, 4> xNorm = {0.10f, 0.37f, 0.64f, 0.91f};
        vector<Vector2f> tPts, cPts;

        for (size_t i = 0; i < monthsList.size(); ++i) {
            float px = chartL + xNorm[i] * chartW;
            float ty = chartB - ((float)monthsList[i].ticketRev / maxVal) * chartH;
            float cy = chartB - ((float)monthsList[i].comboRev / maxVal) * chartH;
            tPts.push_back({px, ty});
            cPts.push_back({px, cy});
        }

        // Đường biểu diễn Doanh thu vé phim (Xanh)
        for (size_t i = 0; i + 1 < tPts.size(); ++i) {
            Vector2f p1 = tPts[i], p2 = tPts[i + 1];
            Vector2f dir = p2 - p1;
            float len = sqrt(dir.x * dir.x + dir.y * dir.y);
            if (len > 0.f) {
                RectangleShape line(Vector2f(len, 3.f));
                line.setOrigin({0.f, 1.5f});
                line.setPosition(p1);
                line.setRotation(degrees(atan2(dir.y, dir.x) * 180.f / 3.14159265f));
                line.setFillColor(Color(8, 125, 181));
                target.draw(line);
            }
        }

        // Đường biểu diễn Doanh thu Combo F&B (Cam)
        for (size_t i = 0; i + 1 < cPts.size(); ++i) {
            Vector2f p1 = cPts[i], p2 = cPts[i + 1];
            Vector2f dir = p2 - p1;
            float len = sqrt(dir.x * dir.x + dir.y * dir.y);
            if (len > 0.f) {
                RectangleShape line(Vector2f(len, 3.f));
                line.setOrigin({0.f, 1.5f});
                line.setPosition(p1);
                line.setRotation(degrees(atan2(dir.y, dir.x) * 180.f / 3.14159265f));
                line.setFillColor(Color(245, 158, 11));
                target.draw(line);
            }
        }

        // Các điểm nút tròn và nhãn trục X cỡ chữ lớn
        const array<wstring, 4> mNames = {L"Tháng 09", L"Tháng 10", L"Tháng 11", L"Tháng 12"};
        const array<wstring, 4> mRevs = {L"1.675M", L"2.915M", L"4.305M", L"2.947M"};

        for (size_t i = 0; i < tPts.size(); ++i) {
            CircleShape ring1(6.5f); ring1.setPosition({tPts[i].x - 6.5f, tPts[i].y - 6.5f}); ring1.setFillColor(Color::White); target.draw(ring1);
            CircleShape c1(4.8f); c1.setPosition({tPts[i].x - 4.8f, tPts[i].y - 4.8f}); c1.setFillColor(Color(8, 125, 181)); target.draw(c1);

            CircleShape ring2(6.5f); ring2.setPosition({cPts[i].x - 6.5f, cPts[i].y - 6.5f}); ring2.setFillColor(Color::White); target.draw(ring2);
            CircleShape c2(4.8f); c2.setPosition({cPts[i].x - 4.8f, cPts[i].y - 4.8f}); c2.setFillColor(Color(245, 158, 11)); target.draw(c2);

            Text ml(boldFont, mNames[i], 14);
            ml.setFillColor(Color(30, 41, 59));
            FloatRect mb = ml.getLocalBounds();
            ml.setPosition({tPts[i].x - mb.size.x / 2.f - mb.position.x, chartB + 12.f});
            target.draw(ml);

            Text mr(boldFont, mRevs[i], 14);
            mr.setFillColor(Color(148, 163, 184));
            FloatRect mrb = mr.getLocalBounds();
            mr.setPosition({tPts[i].x - mrb.size.x / 2.f - mrb.position.x, chartB + 32.f});
            target.draw(mr);
        }

        // Nhãn nổi "Đỉnh T11: 4.305.000 đ" phía trên điểm Tháng 11
        if (tPts.size() > 2) {
            Text peakTxt(boldFont, L"Đỉnh T11: 4.305.000 Đ", 14);
            peakTxt.setFillColor(Color::White);
            FloatRect ptb = peakTxt.getLocalBounds();
            Vector2f peakSize(ptb.size.x + 24.f, 28.f);
            Vector2f peakPos(tPts[2].x - peakSize.x / 2.f, tPts[2].y - 44.f);
            RoundedRectRenderer::draw(target, peakPos, peakSize, 7.f, Color(23, 32, 51));

            ConvexShape ptr(3);
            ptr.setPoint(0, {tPts[2].x - 5.f, peakPos.y + peakSize.y});
            ptr.setPoint(1, {tPts[2].x + 5.f, peakPos.y + peakSize.y});
            ptr.setPoint(2, {tPts[2].x, peakPos.y + peakSize.y + 5.f});
            ptr.setFillColor(Color(23, 32, 51));
            target.draw(ptr);

            peakTxt.setPosition({
                peakPos.x + (peakSize.x - ptb.size.x) / 2.f - ptb.position.x,
                peakPos.y + (peakSize.y - ptb.size.y) / 2.f - ptb.position.y
            });
            target.draw(peakTxt);
        }
    }

    // =========================================================================
    // PHẦN 1B (TRÊN - PHẢI): CƠ CẤU TỶ TRỌNG DOANH THU
    // =========================================================================
    {
        Vector2f cPos(position.x + 20.f + leftW + 14.f, contentY);
        RoundedRectRenderer::draw(target, cPos, {rightW, topH}, 10.f, Color::White, 1.f, Color(226, 232, 240));

        Text title(boldFont, L"Cơ cấu tỷ trọng doanh thu", 14);
        title.setFillColor(Color(23, 32, 51));
        title.setPosition({cPos.x + 24.f, cPos.y + 14.f});
        target.draw(title);

        Text sub(font, L"Tỷ lệ tương quan giữa bán vé phòng chiếu và dịch vụ bắp nước", 14);
        sub.setFillColor(Color(148, 163, 184));
        sub.setPosition({cPos.x + 24.f, cPos.y + 38.f});
        target.draw(sub);

        // 1. Biểu đồ tròn Donut 2 múi màu lớn và sắc nét
        Vector2f donutCenter(cPos.x + rightW * 0.28f, cPos.y + 136.f);
        float R = 62.f;  // Bán kính ngoài lớn hơn
        float r = 41.f;  // Bán kính trong
        const float PI = 3.14159265f;

        float movieShare = 0.591f;
        float sepGap = 0.025f;

        auto drawDonutSegment = [&](float startA, float endA, Color col) {
            int steps = max(16, static_cast<int>((endA - startA) * 30.f));
            VertexArray strip(PrimitiveType::TriangleStrip, (steps + 1) * 2);
            for (int i = 0; i <= steps; ++i) {
                float a = startA + (endA - startA) * (static_cast<float>(i) / steps);
                float ca = cos(a);
                float sa = sin(a);
                strip[i * 2].position = Vector2f(donutCenter.x + R * ca, donutCenter.y + R * sa);
                strip[i * 2].color = col;
                strip[i * 2 + 1].position = Vector2f(donutCenter.x + r * ca, donutCenter.y + r * sa);
                strip[i * 2 + 1].color = col;
            }
            target.draw(strip);
        };

        // Múi 1: Doanh thu vé phim (Xanh #0284C7)
        float a0 = -PI / 2.f + sepGap;
        float a1 = -PI / 2.f + 2.f * PI * movieShare - sepGap;
        drawDonutSegment(a0, a1, Color(8, 125, 181));

        // Múi 2: Doanh thu combo bắp nước (Cam #F59E0B)
        float a2 = -PI / 2.f + 2.f * PI * movieShare + sepGap;
        float a3 = 3.f * PI / 2.f - sepGap;
        drawDonutSegment(a2, a3, Color(245, 158, 11));

        CircleShape innerHole(r - 0.5f, 64);
        innerHole.setPosition({donutCenter.x - r + 0.5f, donutCenter.y - r + 0.5f});
        innerHole.setFillColor(Color::White);
        target.draw(innerHole);

        // Chữ chính giữa biểu đồ tròn
        Text dTitle(boldFont, L"TỔNG CỘNG", 14);
        dTitle.setFillColor(Color(148, 163, 184));
        FloatRect dtb = dTitle.getLocalBounds();
        dTitle.setPosition({
            donutCenter.x - dtb.size.x / 2.f - dtb.position.x,
            donutCenter.y - 14.f - dtb.position.y
        });
        target.draw(dTitle);

        Text dVal(boldFont, L"11.84M", 14);
        dVal.setFillColor(Color(23, 32, 51));
        FloatRect dvb = dVal.getLocalBounds();
        dVal.setPosition({
            donutCenter.x - dvb.size.x / 2.f - dvb.position.x,
            donutCenter.y + 4.f - dvb.position.y
        });
        target.draw(dVal);

        // 2. Chú thích màu sắc (Legend) bên phải Donut với cỡ chữ lớn
        Vector2f legPos(cPos.x + rightW * 0.52f, cPos.y + 84.f);

        // Mục 1: Vé phim
        RoundedRectRenderer::draw(target, {legPos.x, legPos.y + 4.f}, {12.f, 12.f}, 3.f, Color(8, 125, 181));
        Text l1Title(boldFont, L"Vé phim (59.1%)", 14);
        l1Title.setFillColor(Color(23, 32, 51));
        l1Title.setPosition({legPos.x + 20.f, legPos.y});
        target.draw(l1Title);

        Text l1Val(boldFont, s2ws(toCurrency(movieRevenue)), 14);
        l1Val.setFillColor(Color(8, 125, 181));
        l1Val.setPosition({legPos.x + 20.f, legPos.y + 22.f});
        target.draw(l1Val);

        // Mục 2: Combo bắp nước
        float leg2Y = legPos.y + 58.f;
        RoundedRectRenderer::draw(target, {legPos.x, leg2Y + 4.f}, {12.f, 12.f}, 3.f, Color(245, 158, 11));
        Text l2Title(boldFont, L"Combo bắp nước (40.9%)", 14);
        l2Title.setFillColor(Color(23, 32, 51));
        l2Title.setPosition({legPos.x + 20.f, leg2Y});
        target.draw(l2Title);

        Text l2Val(boldFont, s2ws(toCurrency(comboRevenue)), 14);
        l2Val.setFillColor(Color(245, 158, 11));
        l2Val.setPosition({legPos.x + 20.f, leg2Y + 22.f});
        target.draw(l2Val);

        // 3. Ba ô chỉ số trung bình ở đáy thẻ (TB / VÉ, TB / COMBO, TỶ LỆ KÈM) căn giữa, chữ to
        float pillY = cPos.y + topH - 68.f;
        float pW = (rightW - 48.f - 24.f) / 3.f;
        float pH = 54.f;

        auto drawStatPill = [&](int idx, const wstring& l, const wstring& v) {
            Vector2f pPos(cPos.x + 24.f + idx * (pW + 12.f), pillY);
            RoundedRectRenderer::draw(target, pPos, {pW, pH}, 8.f, Color(248, 250, 252));

            Text pl(boldFont, l, 14);
            pl.setFillColor(Color(148, 163, 184));
            FloatRect plb = pl.getLocalBounds();
            pl.setPosition({pPos.x + (pW - plb.size.x) / 2.f - plb.position.x, pPos.y + 9.f});
            target.draw(pl);

            Text pv(boldFont, v, 14);
            pv.setFillColor(Color(23, 32, 51));
            FloatRect pvb = pv.getLocalBounds();
            pv.setPosition({pPos.x + (pW - pvb.size.x) / 2.f - pvb.position.x, pPos.y + 28.f});
            target.draw(pv);
        };

        drawStatPill(0, L"TB / VÉ", L"69.300 đ");
        drawStatPill(1, L"TB / COMBO", L"54.400 đ");
        drawStatPill(2, L"TỶ LỆ KÈM", L"88.1%");
    }

    // =========================================================================
    // PHẦN 2 (DƯỚI): BẢNG TỔNG KẾT DOANH THU THEO THÁNG (CỠ CHỮ TO, DỄ NHÌN)
    // =========================================================================
    {
        float bTableY = contentY + topH + 14.f;
        float bTableH = height - 28.f - bTableY - 4.f;
        Vector2f cPos(position.x + 20.f, bTableY);
        RoundedRectRenderer::draw(target, cPos, {availableW, bTableH}, 10.f, Color::White, 1.f, Color(226, 232, 240));

        // Header trái: Tiêu đề + pill Niên độ + phụ đề
        Text title(boldFont, L"Tổng kết doanh thu theo tháng", 14);
        title.setFillColor(Color(23, 32, 51));
        title.setPosition({cPos.x + 24.f, cPos.y + 12.f});
        target.draw(title);

        float titleW = title.getGlobalBounds().size.x;
        Vector2f eraSize(142.f, 22.f);
        Vector2f eraPos(cPos.x + 24.f + titleW + 12.f, cPos.y + 13.f);
        RoundedRectRenderer::draw(target, eraPos, eraSize, 11.f, Color(241, 245, 249));
        Text eraTxt(boldFont, L"Niên độ Q3 & Q4/2025", 11.f);
        eraTxt.setFillColor(Color(71, 85, 105));
        FloatRect erab = eraTxt.getLocalBounds();
        eraTxt.setPosition({
            eraPos.x + (eraSize.x - erab.size.x) / 2.f - erab.position.x,
            eraPos.y + (eraSize.y - erab.size.y) / 2.f - erab.position.y
        });
        target.draw(eraTxt);

        Text subTbl(font, L"Báo cáo kiểm toán luồng tiền bán vé và chuỗi dịch vụ ẩm thực (F&B)", 12.f);
        subTbl.setFillColor(Color(100, 116, 139));
        subTbl.setPosition({cPos.x + 24.f, cPos.y + 36.f});
        target.draw(subTbl);

        // Header phải: Pill Đơn vị tính + Phân tích sâu
        Vector2f deepSize(106.f, 30.f);
        Vector2f deepPos(cPos.x + availableW - 20.f - deepSize.x, cPos.y + 16.f);
        RoundedRectRenderer::draw(target, deepPos, deepSize, 7.f, Color(8, 125, 181));
        Text deepTxt(boldFont, L"Phân tích sâu", 12.f);
        deepTxt.setFillColor(Color::White);
        FloatRect dpb = deepTxt.getLocalBounds();
        deepTxt.setPosition({
            deepPos.x + (deepSize.x - dpb.size.x) / 2.f - dpb.position.x,
            deepPos.y + (deepSize.y - dpb.size.y) / 2.f - dpb.position.y
        });
        target.draw(deepTxt);

        Vector2f unitPillSize(236.f, 30.f);
        Vector2f unitPillPos(deepPos.x - 10.f - unitPillSize.x, cPos.y + 16.f);
        RoundedRectRenderer::draw(target, unitPillPos, unitPillSize, 7.f, Color(248, 250, 252), 1.f, Color(226, 232, 240));
        Text unitTxt(boldFont, L"Đơn vị tính: Việt Nam Đồng (VND)", 14);
        unitTxt.setFillColor(Color(71, 85, 105));
        FloatRect utb = unitTxt.getLocalBounds();
        unitTxt.setPosition({
            unitPillPos.x + (unitPillSize.x - utb.size.x) / 2.f - utb.position.x,
            unitPillPos.y + (unitPillSize.y - utb.size.y) / 2.f - utb.position.y
        });
        target.draw(unitTxt);

        // Tọa độ các cột theo tỷ lệ phần trăm chiều ngang
        float col_m   = cPos.x + 24.f;
        float col_tv  = cPos.x + availableW * 0.21f;
        float col_cv  = cPos.x + availableW * 0.37f;
        float col_tot = cPos.x + availableW * 0.52f;
        float col_gr  = cPos.x + availableW * 0.66f;
        float col_sh  = cPos.x + availableW * 0.77f;
        float col_act = cPos.x + availableW * 0.89f;

        // Thanh tiêu đề cột
        float thBoxY = cPos.y + 58.f;
        float thBoxH = 34.f;
        RectangleShape thBg(Vector2f(availableW - 2.f, thBoxH));
        thBg.setPosition({cPos.x + 1.f, thBoxY});
        thBg.setFillColor(Color(248, 250, 252));
        target.draw(thBg);

        float thY = thBoxY + 8.f;
        Text th1(boldFont, L"THÁNG / KỲ", 12.f); th1.setFillColor(Color(100, 116, 139)); th1.setPosition({col_m, thY}); target.draw(th1);
        Text th2(boldFont, L"DOANH THU VÉ (Đ)", 12.f); th2.setFillColor(Color(100, 116, 139)); th2.setPosition({col_tv, thY}); target.draw(th2);
        Text th3(boldFont, L"DOANH THU COMBO (Đ)", 12.f); th3.setFillColor(Color(100, 116, 139)); th3.setPosition({col_cv, thY}); target.draw(th3);
        Text th4(boldFont, L"TỔNG DOANH THU (Đ)", 12.f); th4.setFillColor(Color(100, 116, 139)); th4.setPosition({col_tot, thY}); target.draw(th4);
        Text th5(boldFont, L"TĂNG TRƯỞNG", 12.f); th5.setFillColor(Color(100, 116, 139)); th5.setPosition({col_gr, thY}); target.draw(th5);
        Text th6(boldFont, L"ĐÓNG GÓP F&B", 12.f); th6.setFillColor(Color(100, 116, 139)); th6.setPosition({col_sh, thY}); target.draw(th6);
        Text th7(boldFont, L"THAO TÁC", 12.f); th7.setFillColor(Color(100, 116, 139)); th7.setPosition({col_act + 10.f, thY}); target.draw(th7);

        RectangleShape div(Vector2f(availableW - 2.f, 1.f));
        div.setPosition({cPos.x + 1.f, thBoxY + thBoxH});
        div.setFillColor(Color(226, 232, 240));
        target.draw(div);

        // Helper vẽ mũi tên tăng/giảm nhỏ bằng vector (không phụ thuộc font)
        auto drawTrendArrow = [&](Vector2f center, bool up, Color col) {
            RectangleShape stem({1.8f, 8.f});
            stem.setFillColor(col);
            stem.setPosition({center.x - 0.9f, center.y - 4.f});
            target.draw(stem);

            ConvexShape head(3);
            if (up) {
                head.setPoint(0, {center.x, center.y - 5.5f});
                head.setPoint(1, {center.x - 3.5f, center.y - 1.5f});
                head.setPoint(2, {center.x + 3.5f, center.y - 1.5f});
            } else {
                head.setPoint(0, {center.x, center.y + 5.5f});
                head.setPoint(1, {center.x - 3.5f, center.y + 1.5f});
                head.setPoint(2, {center.x + 3.5f, center.y + 1.5f});
            }
            head.setFillColor(col);
            target.draw(head);
        };

        // 4 Dòng dữ liệu tháng + 1 Dòng tổng kết dưới đáy
        float rTop = thBoxY + thBoxH;
        float footerH = 50.f;
        int rowCount = max(1, (int)monthsList.size());
        float rH = (bTableH - (rTop - cPos.y) - footerH) / (float)rowCount;

        for (size_t i = 0; i < monthsList.size(); ++i) {
            float ry = rTop + i * rH;
            float centerY = ry + rH / 2.f;

            if (i > 0) {
                RectangleShape rLine(Vector2f(availableW - 2.f, 1.f));
                rLine.setPosition({cPos.x + 1.f, ry});
                rLine.setFillColor(Color(241, 245, 249));
                target.draw(rLine);
            }

            // Chấm tròn màu chỉ thị đầu dòng + Tháng / Kỳ + nhãn Kỷ lục
            Color dotCol = (i == 0) ? Color(59, 130, 246)
                         : (monthsList[i].isRecord ? Color(244, 63, 94) : Color(203, 213, 225));
            CircleShape rowDot(3.8f);
            rowDot.setPosition({col_m, centerY - 3.8f});
            rowDot.setFillColor(dotCol);
            target.draw(rowDot);

            Text tm(boldFont, monthsList[i].displayMonth, 14);
            tm.setFillColor(Color(23, 32, 51));
            FloatRect tmb = tm.getLocalBounds();
            tm.setPosition({col_m + 14.f, centerY - tmb.size.y / 2.f - tmb.position.y});
            target.draw(tm);

            if (monthsList[i].isRecord) {
                Vector2f recSize(64.f, 22.f);
                Vector2f recPos(col_m + 14.f + tmb.size.x + 10.f, centerY - recSize.y / 2.f);
                RoundedRectRenderer::draw(target, recPos, recSize, 11.f, Color(254, 226, 226));
                Text recTxt(boldFont, L"★ Kỷ lục", 11.f);
                recTxt.setFillColor(Color(225, 29, 72));
                FloatRect rcb = recTxt.getLocalBounds();
                recTxt.setPosition({
                    recPos.x + (recSize.x - rcb.size.x) / 2.f - rcb.position.x,
                    recPos.y + (recSize.y - rcb.size.y) / 2.f - rcb.position.y
                });
                target.draw(recTxt);
            }

            // Doanh thu vé
            Text tv(font, s2ws(toCurrency(monthsList[i].ticketRev)), 14);
            tv.setFillColor(Color(51, 65, 85));
            FloatRect tvb = tv.getLocalBounds();
            tv.setPosition({col_tv + 18.f, centerY - tvb.size.y / 2.f - tvb.position.y});
            target.draw(tv);

            // Doanh thu combo
            Text tc(font, s2ws(toCurrency(monthsList[i].comboRev)), 14);
            tc.setFillColor(Color(51, 65, 85));
            FloatRect tcb = tc.getLocalBounds();
            tc.setPosition({col_cv + 28.f, centerY - tcb.size.y / 2.f - tcb.position.y});
            target.draw(tc);

            // Tổng doanh thu tháng
            Text tt(boldFont, s2ws(toCurrency(monthsList[i].totalRev)), 15.f);
            tt.setFillColor(Color(23, 32, 51));
            FloatRect ttb = tt.getLocalBounds();
            tt.setPosition({col_tot + 20.f, centerY - ttb.size.y / 2.f - ttb.position.y});
            target.draw(tt);

            // Tăng trưởng (dạng pill bo tròn gọn đẹp)
            bool hasGrowth = (monthsList[i].growth != L"---" && !monthsList[i].growth.empty());
            Vector2f grPillSize(84.f, 26.f);
            Vector2f grPillPos(col_gr + 2.f, centerY - grPillSize.y / 2.f);
            if (!hasGrowth) {
                RoundedRectRenderer::draw(target, grPillPos, grPillSize, 13.f, Color(241, 245, 249));
                Text tg(boldFont, L"---", 13.f);
                tg.setFillColor(Color(148, 163, 184));
                FloatRect tgb = tg.getLocalBounds();
                tg.setPosition({
                    grPillPos.x + (grPillSize.x - tgb.size.x) / 2.f - tgb.position.x,
                    grPillPos.y + (grPillSize.y - tgb.size.y) / 2.f - tgb.position.y
                });
                target.draw(tg);
            } else {
                bool isUp = (monthsList[i].growth[0] == '+');
                Color pillBg = isUp ? Color(220, 252, 231) : Color(254, 226, 226);
                Color pillFg = isUp ? Color(21, 128, 61) : Color(225, 29, 72);
                RoundedRectRenderer::draw(target, grPillPos, grPillSize, 13.f, pillBg);

                drawTrendArrow({grPillPos.x + 15.f, centerY}, isUp, pillFg);

                Text tg(boldFont, monthsList[i].growth, 14);
                tg.setFillColor(pillFg);
                FloatRect tgb = tg.getLocalBounds();
                tg.setPosition({
                    grPillPos.x + 24.f - tgb.position.x,
                    grPillPos.y + (grPillSize.y - tgb.size.y) / 2.f - tgb.position.y
                });
                target.draw(tg);
            }

            // Đóng góp F&B: % + thanh tiến trình nhỏ bên cạnh
            stringstream ss; ss << fixed << setprecision(1) << monthsList[i].comboShare << "%";
            Text ts(boldFont, ss.str(), 14);
            ts.setFillColor(monthsList[i].isRecord ? Color(217, 119, 6) : Color(51, 65, 85));
            FloatRect tsb = ts.getLocalBounds();
            ts.setPosition({col_sh, centerY - tsb.size.y / 2.f - tsb.position.y});
            target.draw(ts);

            float fbBarW = 64.f;
            float fbBarH = 6.f;
            Vector2f fbBarPos(col_sh + 52.f, centerY - fbBarH / 2.f);
            RoundedRectRenderer::draw(target, fbBarPos, {fbBarW, fbBarH}, 3.f, Color(241, 245, 249));
            float fbFillW = max(6.f, min(fbBarW, (monthsList[i].comboShare / 60.f) * fbBarW));
            Color fbBarCol = monthsList[i].isRecord ? Color(245, 158, 11) : Color(37, 99, 235);
            RoundedRectRenderer::draw(target, fbBarPos, {fbFillW, fbBarH}, 3.f, fbBarCol);

            // Thao tác: nút "Chi tiết >" có viền, hiệu ứng hover và nổi bật tháng kỷ lục
            bool isDetHov = (hoveredDetailRow == static_cast<int>(i));
            Vector2f detBtnSize(88.f, 30.f);
            Vector2f detBtnPos(col_act - 6.f, centerY - detBtnSize.y / 2.f);
            Color detBg = isDetHov
                ? (monthsList[i].isRecord ? Color(254, 243, 199) : Color(224, 242, 254))
                : Color::White;
            Color detBorder = monthsList[i].isRecord ? Color(253, 230, 138) : (isDetHov ? Color(8, 125, 181) : Color(226, 232, 240));
            Color detFg = monthsList[i].isRecord ? Color(217, 119, 6) : Color(8, 125, 181);

            RoundedRectRenderer::draw(target, detBtnPos, detBtnSize, 7.f, detBg, 1.f, detBorder);
            Text ta(boldFont, L"Chi tiết  >", 14);
            ta.setFillColor(detFg);
            FloatRect tab = ta.getLocalBounds();
            ta.setPosition({
                detBtnPos.x + (detBtnSize.x - tab.size.x) / 2.f - tab.position.x,
                detBtnPos.y + (detBtnSize.y - tab.size.y) / 2.f - tab.position.y
            });
            target.draw(ta);
        }

        // Dòng tổng kết dưới đáy bảng: TỔNG DOANH THU KỲ
        float footY = cPos.y + bTableH - footerH;
        RoundedRectRenderer::draw(target, {cPos.x + 1.f, footY}, {availableW - 2.f, footerH - 1.f}, 8.f, Color(248, 250, 252));
        RectangleShape footLine(Vector2f(availableW - 2.f, 1.f));
        footLine.setPosition({cPos.x + 1.f, footY});
        footLine.setFillColor(Color(226, 232, 240));
        target.draw(footLine);

        float fCenterY = footY + footerH / 2.f;

        // Icon hộp vuông xanh nhỏ + TỔNG DOANH THU KỲ
        Vector2f sumIconPos(col_m, fCenterY - 10.f);
        RoundedRectRenderer::draw(target, sumIconPos, {20.f, 20.f}, 5.f, Color(37, 99, 235));
        RectangleShape icLine1({10.f, 2.f}); icLine1.setFillColor(Color::White); icLine1.setPosition({sumIconPos.x + 5.f, sumIconPos.y + 6.f}); target.draw(icLine1);
        RectangleShape icLine2({10.f, 2.f}); icLine2.setFillColor(Color::White); icLine2.setPosition({sumIconPos.x + 5.f, sumIconPos.y + 10.f}); target.draw(icLine2);
        RectangleShape icLine3({6.f, 2.f});  icLine3.setFillColor(Color::White); icLine3.setPosition({sumIconPos.x + 5.f, sumIconPos.y + 14.f}); target.draw(icLine3);

        Text fTitle(boldFont, L"TỔNG DOANH THU KỲ", 14);
        fTitle.setFillColor(Color(23, 32, 51));
        FloatRect ftb = fTitle.getLocalBounds();
        fTitle.setPosition({col_m + 28.f, fCenterY - ftb.size.y / 2.f - ftb.position.y});
        target.draw(fTitle);

        Text fTv(boldFont, s2ws(toCurrency(movieRevenue)), 15.f);
        fTv.setFillColor(Color(8, 125, 181));
        FloatRect ftvb = fTv.getLocalBounds();
        fTv.setPosition({col_tv + 18.f, fCenterY - ftvb.size.y / 2.f - ftvb.position.y});
        target.draw(fTv);

        Text fCv(boldFont, s2ws(toCurrency(comboRevenue)), 15.f);
        fCv.setFillColor(Color(245, 158, 11));
        FloatRect fcvb = fCv.getLocalBounds();
        fCv.setPosition({col_cv + 28.f, fCenterY - fcvb.size.y / 2.f - fcvb.position.y});
        target.draw(fCv);

        Text fTot(boldFont, s2ws(toCurrency(totalRevenue)), 14);
        fTot.setFillColor(Color(23, 32, 51));
        FloatRect ftotb = fTot.getLocalBounds();
        fTot.setPosition({col_tot + 16.f, fCenterY - ftotb.size.y / 2.f - ftotb.position.y});
        target.draw(fTot);

        // Pill tăng trưởng tổng kỳ
        Vector2f fGrSize(84.f, 26.f);
        Vector2f fGrPos(col_gr + 2.f, fCenterY - fGrSize.y / 2.f);
        RoundedRectRenderer::draw(target, fGrPos, fGrSize, 13.f, Color(220, 252, 231));
        drawTrendArrow({fGrPos.x + 15.f, fCenterY}, true, Color(21, 128, 61));
        Text fGr(boldFont, L"+18.4%", 14);
        fGr.setFillColor(Color(21, 128, 61));
        FloatRect fgrb = fGr.getLocalBounds();
        fGr.setPosition({fGrPos.x + 24.f - fgrb.position.x, fGrPos.y + (fGrSize.y - fgrb.size.y) / 2.f - fgrb.position.y});
        target.draw(fGr);

        // Đóng góp F&B trung bình + thanh bar
        Text fSh(boldFont, L"TB 40.9%", 14);
        fSh.setFillColor(Color(23, 32, 51));
        FloatRect fshb = fSh.getLocalBounds();
        fSh.setPosition({col_sh - 10.f, fCenterY - fshb.size.y / 2.f - fshb.position.y});
        target.draw(fSh);

        Vector2f fFbBarPos(col_sh + 58.f, fCenterY - 3.f);
        RoundedRectRenderer::draw(target, fFbBarPos, {58.f, 6.f}, 3.f, Color(226, 232, 240));
        RoundedRectRenderer::draw(target, fFbBarPos, {38.f, 6.f}, 3.f, Color(245, 158, 11));

        // Nút đen "Xuất báo cáo" ở góc phải dòng tổng kết
        Vector2f expBtnSize(112.f, 32.f);
        Vector2f expBtnPos(col_act - 12.f, fCenterY - expBtnSize.y / 2.f);
        RoundedRectRenderer::draw(target, expBtnPos, expBtnSize, 8.f, Color(23, 32, 51));
        Text expBtnTxt(boldFont, L"Xuất báo cáo", 14);
        expBtnTxt.setFillColor(Color::White);
        FloatRect ebb = expBtnTxt.getLocalBounds();
        expBtnTxt.setPosition({
            expBtnPos.x + (expBtnSize.x - ebb.size.x) / 2.f - ebb.position.x,
            expBtnPos.y + (expBtnSize.y - ebb.size.y) / 2.f - ebb.position.y
        });
        target.draw(expBtnTxt);
    }
}

void MovieRevenuePanel::drawTab3RevenueByMovie(RenderTarget& target) const {
    const float contentY = position.y + 250.f;
    const float availableW = width - 40.f;
    const float totalH = height - 28.f - contentY - 4.f;
    const float leftW = availableW * 0.56f;
    const float rightW = availableW - leftW - 14.f;

    // Left Card: XẾP HẠNG DOANH THU THEO PHIM (VND)
    {
        Vector2f cPos(position.x + 20.f, contentY);
        RoundedRectRenderer::draw(target, cPos, {leftW, totalH}, 10.f, Color::White, 1.f, Color(226, 232, 240));

        Text title(boldFont, L"XẾP HẠNG DOANH THU THEO PHIM (VND)", 14);
        title.setFillColor(Color(23, 32, 51));
        title.setPosition({cPos.x + 24.f, cPos.y + 16.f});
        target.draw(title);

        Text sub(font, L"Thang đo định mức tối đa: 4.000.000 Đ", 14);
        sub.setFillColor(Color(148, 163, 184));
        sub.setPosition({cPos.x + 24.f, cPos.y + 40.f});
        target.draw(sub);

        Vector2f bSize(134.f, 28.f);
        Vector2f bPos(cPos.x + leftW - 24.f - bSize.x, cPos.y + 16.f);
        RoundedRectRenderer::draw(target, bPos, bSize, 6.f, Color(240, 249, 255), 1.f, Color(186, 230, 253));
        Text bTxt(boldFont, L"TOP 8 BOX OFFICE", 14);
        bTxt.setFillColor(Color(8, 125, 181));
        FloatRect btb = bTxt.getLocalBounds();
        bTxt.setPosition({
            bPos.x + (bSize.x - btb.size.x) / 2.f - btb.position.x,
            bPos.y + (bSize.y - btb.size.y) / 2.f - btb.position.y
        });
        target.draw(bTxt);

        // 8 Horizontal Progress Bars trải đều chiều cao thẻ
        float rTop = cPos.y + 80.f;
        int nRows = max(1, (int)moviesList.size());
        float rH = (totalH - 96.f) / static_cast<float>(nRows);
        float maxRev = 4000000.f;
        for (const auto& m : moviesList) {
            if ((float)m.revenue > maxRev) maxRev = (float)m.revenue * 1.05f;
        }

        float nameX = cPos.x + 54.f;
        float barStartX = cPos.x + 258.f;
        float barEndX = cPos.x + leftW - 142.f;
        float maxBarW = max(60.f, barEndX - barStartX);

        for (size_t i = 0; i < moviesList.size(); ++i) {
            float ry = rTop + i * rH;
            float centerY = ry + rH / 2.f;

            // Rank
            Text rTxt(boldFont, to_string(i + 1), 14);
            Color rankCol = (i == 0) ? Color(245, 158, 11) : (i == 1 ? Color(100, 116, 139) : (i == 2 ? Color(180, 83, 9) : Color(148, 163, 184)));
            rTxt.setFillColor(rankCol);
            FloatRect rtb = rTxt.getLocalBounds();
            rTxt.setPosition({cPos.x + 24.f, centerY - rtb.size.y / 2.f - rtb.position.y});
            target.draw(rTxt);

            // Movie Name (cắt gọn nếu quá dài để không đè lên thanh bar)
            wstring titleStr = moviesList[i].title;
            Text mTxt(i == 0 ? boldFont : font, titleStr, 14);
            mTxt.setFillColor(i == 0 ? Color(23, 32, 51) : Color(51, 65, 85));
            if (mTxt.getGlobalBounds().size.x > 190.f) {
                while (!titleStr.empty() && mTxt.getGlobalBounds().size.x > 176.f) {
                    titleStr.pop_back();
                    mTxt.setString(titleStr + L"...");
                }
            }
            FloatRect mtb = mTxt.getLocalBounds();
            mTxt.setPosition({nameX, centerY - mtb.size.y / 2.f - mtb.position.y});
            target.draw(mTxt);

            // Background Track
            float barH = 12.f;
            RoundedRectRenderer::draw(target, {barStartX, centerY - barH / 2.f}, {maxBarW, barH}, 6.f, Color(241, 245, 249));

            // Progress Bar
            float barW = min(maxBarW, max(8.f, ((float)moviesList[i].revenue / maxRev) * maxBarW));
            Color barCol = (i == 0) ? Color(8, 125, 181) : (i < 3 ? Color(56, 189, 248) : Color(125, 211, 252));
            RoundedRectRenderer::draw(target, {barStartX, centerY - barH / 2.f}, {barW, barH}, 6.f, barCol);

            // Revenue text (căn phải cách lề 24px)
            Text revTxt(boldFont, s2ws(toCurrency(moviesList[i].revenue)), 14);
            revTxt.setFillColor(Color(23, 32, 51));
            FloatRect revB = revTxt.getLocalBounds();
            revTxt.setPosition({cPos.x + leftW - 24.f - revB.size.x - revB.position.x, centerY - revB.size.y / 2.f - revB.position.y});
            target.draw(revTxt);
        }
    }

    // Right Column: Top 1 Highlight + Thể loại
    {
        float rightX = position.x + 20.f + leftW + 14.f;
        float top1H = 216.f;

        // 1. Dark Top 1 Card
        Vector2f top1Pos(rightX, contentY);
        RoundedRectRenderer::draw(target, top1Pos, {rightW, top1H}, 10.f, Color(23, 32, 51));

        Vector2f t1BadgeSize(210.f, 26.f);
        Vector2f t1BadgePos(top1Pos.x + 24.f, top1Pos.y + 20.f);
        RoundedRectRenderer::draw(target, t1BadgePos, t1BadgeSize, 6.f, Color(51, 41, 26));
        Text t1Badge(boldFont, L"TOP 1 PHIM BÁN CHẠY NHẤT", 14);
        t1Badge.setFillColor(Color(245, 158, 11));
        FloatRect t1bb = t1Badge.getLocalBounds();
        t1Badge.setPosition({
            t1BadgePos.x + (t1BadgeSize.x - t1bb.size.x) / 2.f - t1bb.position.x,
            t1BadgePos.y + (t1BadgeSize.y - t1bb.size.y) / 2.f - t1bb.position.y
        });
        target.draw(t1Badge);

        wstring topTitle = moviesList.empty() ? L"Tử Chiến Trên Không" : moviesList[0].title;
        wstring topGenre = moviesList.empty() ? L"Hành động / Kịch tính" : moviesList[0].genre;
        long long topRev = moviesList.empty() ? 3890000 : moviesList[0].revenue;
        int topTicks = moviesList.empty() ? 31 : moviesList[0].tickets;

        Text t1Title(boldFont, topTitle, 24);
        t1Title.setFillColor(Color::White);
        t1Title.setPosition({top1Pos.x + 24.f, top1Pos.y + 58.f});
        target.draw(t1Title);

        Text t1Genre(font, L"Thể loại: " + topGenre, 14);
        t1Genre.setFillColor(Color(148, 163, 184));
        t1Genre.setPosition({top1Pos.x + 24.f, top1Pos.y + 90.f});
        target.draw(t1Genre);

        RectangleShape t1Div(Vector2f(rightW - 48.f, 1.f));
        t1Div.setPosition({top1Pos.x + 24.f, top1Pos.y + 126.f});
        t1Div.setFillColor(Color(30, 41, 59));
        target.draw(t1Div);

        Text t1RevLbl(font, L"Doanh thu ghi nhận", 14);
        t1RevLbl.setFillColor(Color(148, 163, 184));
        t1RevLbl.setPosition({top1Pos.x + 24.f, top1Pos.y + 142.f});
        target.draw(t1RevLbl);

        Text t1Rev(boldFont, s2ws(toCurrency(topRev)), 24);
        t1Rev.setFillColor(Color(52, 211, 153));
        t1Rev.setPosition({top1Pos.x + 24.f, top1Pos.y + 164.f});
        target.draw(t1Rev);

        Text t1TickLbl(font, L"Số lượng phát hành", 14);
        t1TickLbl.setFillColor(Color(148, 163, 184));
        FloatRect t1tlb = t1TickLbl.getLocalBounds();
        t1TickLbl.setPosition({top1Pos.x + rightW - 24.f - t1tlb.size.x, top1Pos.y + 142.f});
        target.draw(t1TickLbl);

        Text t1Tick(boldFont, L"Tổng vé: " + to_wstring(topTicks) + L" vé", 24);
        t1Tick.setFillColor(Color(147, 197, 253));
        FloatRect t1tkb = t1Tick.getLocalBounds();
        t1Tick.setPosition({top1Pos.x + rightW - 24.f - t1tkb.size.x, top1Pos.y + 166.f});
        target.draw(t1Tick);

        // 2. Thể loại Breakdown Card
        float gY = contentY + top1H + 14.f;
        float gH = totalH - top1H - 14.f;
        Vector2f gPos(rightX, gY);
        RoundedRectRenderer::draw(target, gPos, {rightW, gH}, 10.f, Color::White, 1.f, Color(226, 232, 240));

        Text gTitle(boldFont, L"CƠ CẤU DOANH THU THEO THỂ LOẠI", 14);
        gTitle.setFillColor(Color(23, 32, 51));
        gTitle.setPosition({gPos.x + 24.f, gPos.y + 18.f});
        target.draw(gTitle);

        Text gUnit(font, L"Tỷ trọng %", 14);
        gUnit.setFillColor(Color(148, 163, 184));
        FloatRect gub = gUnit.getLocalBounds();
        gUnit.setPosition({gPos.x + rightW - 24.f - gub.size.x, gPos.y + 20.f});
        target.draw(gUnit);

        // 4 Categories
        struct CatItem { wstring name; float pct; Color col; };
        const array<CatItem, 4> cats = {
            CatItem{L"Hành động", 58.0f, Color(8, 125, 181)},
            CatItem{L"Hoạt hình", 18.4f, Color(245, 158, 11)},
            CatItem{L"Tâm lý", 13.6f, Color(225, 29, 72)},
            CatItem{L"Lịch sử", 10.0f, Color(21, 128, 61)}
        };

        float catTop = gPos.y + 62.f;
        float catStep = (gH - 80.f) / 4.f;
        float fullBarW = rightW - 48.f;

        for (size_t i = 0; i < cats.size(); ++i) {
            float cy = catTop + i * catStep;

            CircleShape dot(5.f);
            dot.setPosition({gPos.x + 24.f, cy + 5.f});
            dot.setFillColor(cats[i].col);
            target.draw(dot);

            Text cName(boldFont, cats[i].name, 14);
            cName.setFillColor(Color(23, 32, 51));
            cName.setPosition({gPos.x + 42.f, cy});
            target.draw(cName);

            stringstream ss; ss << fixed << setprecision(1) << cats[i].pct << "%";
            Text cPct(boldFont, ss.str(), 14);
            cPct.setFillColor(Color(51, 65, 85));
            FloatRect cpb = cPct.getLocalBounds();
            cPct.setPosition({gPos.x + rightW - 24.f - cpb.size.x, cy});
            target.draw(cPct);

            Vector2f barP(gPos.x + 24.f, cy + 28.f);
            RoundedRectRenderer::draw(target, barP, {fullBarW, 10.f}, 5.f, Color(241, 245, 249));
            RoundedRectRenderer::draw(target, barP, {fullBarW * (cats[i].pct / 100.f), 10.f}, 5.f, cats[i].col);
        }
    }
}

void MovieRevenuePanel::drawTab4RevenueByCombo(RenderTarget& target) const {
    const float contentY = position.y + 250.f;
    const float availableW = width - 40.f;

    // =========================================================================
    // PHẦN 1 (TRÊN): BIỂU ĐỒ DOANH THU COMBO (10 CỘT CAM)
    // =========================================================================
    const float chartCardH = 284.f;
    Vector2f cPos(position.x + 20.f, contentY);
    RoundedRectRenderer::draw(target, cPos, {availableW, chartCardH}, 10.f, Color::White, 1.f, Color(226, 232, 240));

    Text title(boldFont, L"Doanh thu combo", 14);
    title.setFillColor(Color(23, 32, 51));
    title.setPosition({cPos.x + 24.f, cPos.y + 14.f});
    target.draw(title);

    float titleW = title.getGlobalBounds().size.x;
    Vector2f bPos(cPos.x + 24.f + titleW + 12.f, cPos.y + 15.f);
    Vector2f bSize(116.f, 22.f);
    RoundedRectRenderer::draw(target, bPos, bSize, 5.f, Color(254, 243, 199));
    Text bTxt(boldFont, L"10 MÃ SẢN PHẨM", 14);
    bTxt.setFillColor(Color(180, 83, 9));
    FloatRect bb = bTxt.getLocalBounds();
    bTxt.setPosition({
        bPos.x + (bSize.x - bb.size.x) / 2.f - bb.position.x,
        bPos.y + (bSize.y - bb.size.y) / 2.f - bb.position.y
    });
    target.draw(bTxt);

    Text sub(font, L"Biểu đồ thể hiện mức thu nhập thực tế từ từng combo bắp rang và nước uống (Đơn vị: VNĐ)", 12.f);
    sub.setFillColor(Color(100, 116, 139));
    sub.setPosition({cPos.x + 24.f, cPos.y + 40.f});
    target.draw(sub);

    // Header phải: Legend + Pill Max mục tiêu
    Vector2f maxPillSize(166.f, 28.f);
    Vector2f maxPillPos(cPos.x + availableW - 22.f - maxPillSize.x, cPos.y + 14.f);
    RoundedRectRenderer::draw(target, maxPillPos, maxPillSize, 6.f, Color(248, 250, 252), 1.f, Color(226, 232, 240));
    Text maxTxt(boldFont, L"Max mục tiêu: 1.000.000đ", 11.f);
    maxTxt.setFillColor(Color(71, 85, 105));
    FloatRect mxb = maxTxt.getLocalBounds();
    maxTxt.setPosition({
        maxPillPos.x + (maxPillSize.x - mxb.size.x) / 2.f - mxb.position.x,
        maxPillPos.y + (maxPillSize.y - mxb.size.y) / 2.f - mxb.position.y
    });
    target.draw(maxTxt);

    Text legTxt(boldFont, L"Doanh thu đạt được", 12.f);
    legTxt.setFillColor(Color(71, 85, 105));
    FloatRect ltb = legTxt.getLocalBounds();
    float legX = maxPillPos.x - 18.f - ltb.size.x;
    legTxt.setPosition({legX, cPos.y + 20.f});
    target.draw(legTxt);
    RoundedRectRenderer::draw(target, {legX - 18.f, cPos.y + 22.f}, {11.f, 11.f}, 2.5f, Color(245, 158, 11));

    // Grid ngang + nhãn trục Y bên phải (250k, 500k, 750k, 1.000k)
    const float chartLeft = cPos.x + 36.f;
    const float chartRight = cPos.x + availableW - 68.f;
    const float chartBottom = cPos.y + 248.f;
    const float chartH = 166.f;
    const float maxVal = 1000000.f;

    const array<pair<float, wstring>, 4> yTicks = {{
        {0.25f, L"250k"},
        {0.50f, L"500k"},
        {0.75f, L"750k"},
        {1.00f, L"1.000k"}
    }};
    for (const auto& tk : yTicks) {
        float gy = chartBottom - tk.first * chartH;
        RectangleShape gridLine(Vector2f(chartRight - chartLeft, 1.f));
        gridLine.setPosition({chartLeft, gy});
        gridLine.setFillColor(Color(241, 245, 249));
        target.draw(gridLine);

        Text yLbl(font, tk.second, 14);
        yLbl.setFillColor(Color(148, 163, 184));
        yLbl.setPosition({chartRight + 10.f, gy - 7.f});
        target.draw(yLbl);
    }

    RectangleShape baseLine(Vector2f(chartRight - chartLeft, 1.f));
    baseLine.setPosition({chartLeft, chartBottom});
    baseLine.setFillColor(Color(226, 232, 240));
    target.draw(baseLine);

    // 10 Cột doanh thu combo
    const float chartW = chartRight - chartLeft;
    int n = min(10, (int)combosList.size());
    float colW = 50.f;
    float colGap = (chartW - (n * colW)) / (n + 1);

    for (int i = 0; i < n; ++i) {
        float bx = chartLeft + colGap + i * (colW + colGap);
        float bHeight = ((float)combosList[i].revenue / maxVal) * chartH;
        float by = chartBottom - bHeight;

        RoundedRectRenderer::draw(target, {bx, by}, {colW, bHeight}, 5.f, Color(245, 158, 11));

        Text valTxt(boldFont, to_string(combosList[i].revenue / 1000) + "k", 12.f);
        valTxt.setFillColor(Color(23, 32, 51));
        FloatRect vb = valTxt.getLocalBounds();
        valTxt.setPosition({bx + (colW - vb.size.x) / 2.f - vb.position.x, by - 19.f});
        target.draw(valTxt);

        Text codeTxt(boldFont, combosList[i].code, 12.f);
        codeTxt.setFillColor(Color(71, 85, 105));
        FloatRect cb = codeTxt.getLocalBounds();
        codeTxt.setPosition({bx + (colW - cb.size.x) / 2.f - cb.position.x, chartBottom + 8.f});
        target.draw(codeTxt);
    }

    // =========================================================================
    // PHẦN 2 (GIỮA): HAI THẺ NHẬN ĐỊNH NỔI BẬT
    // =========================================================================
    const float callY = contentY + chartCardH + 12.f;
    const float callH = 66.f;
    const float callW = (availableW - 14.f) / 2.f;

    // Callout 1: COMBO DẪN ĐẦU DOANH THU
    Vector2f c1Pos(position.x + 20.f, callY);
    RoundedRectRenderer::draw(target, c1Pos, {callW, callH}, 8.f, Color(255, 253, 247), 1.f, Color(252, 211, 77));
    CircleShape c1Dot(3.5f);
    c1Dot.setFillColor(Color(245, 158, 11));
    c1Dot.setPosition({c1Pos.x + 16.f, c1Pos.y + 15.f});
    target.draw(c1Dot);
    Text c1T(boldFont, L"COMBO DẪN ĐẦU DOANH THU", 14);
    c1T.setFillColor(Color(180, 83, 9));
    c1T.setPosition({c1Pos.x + 28.f, c1Pos.y + 11.f});
    target.draw(c1T);

    Text c1B(font, L"CB05: 1 CocaCola + 1 Bắp Phô Mai mang về 600.000 đ (6 lượt chọn phổ biến).", 14);
    c1B.setFillColor(Color(51, 65, 85));
    c1B.setPosition({c1Pos.x + 16.f, c1Pos.y + 34.f});
    target.draw(c1B);

    // Callout 2: XU HƯỚNG VỊ GIÁC RẠP CHIẾU
    Vector2f c2Pos(position.x + 20.f + callW + 14.f, callY);
    RoundedRectRenderer::draw(target, c2Pos, {callW, callH}, 8.f, Color(240, 249, 255), 1.f, Color(125, 211, 252));
    CircleShape c2Dot(3.5f);
    c2Dot.setFillColor(Color(8, 125, 181));
    c2Dot.setPosition({c2Pos.x + 16.f, c2Pos.y + 15.f});
    target.draw(c2Dot);
    Text c2T(boldFont, L"XU HƯỚNG VỊ GIÁC RẠP CHIẾU", 14);
    c2T.setFillColor(Color(8, 125, 181));
    c2T.setPosition({c2Pos.x + 28.f, c2Pos.y + 11.f});
    target.draw(c2T);

    Text c2B(font, L"Hương vị Bắp Phô Mai chiếm áp đảo 62% tổng số bắp bán ra (CB03, CB05, CB07, CB09, CB10).", 12.5f);
    c2B.setFillColor(Color(18, 59, 93));
    c2B.setPosition({c2Pos.x + 16.f, c2Pos.y + 34.f});
    target.draw(c2B);

    // =========================================================================
    // PHẦN 3 (DƯỚI): BẢNG CHI TIẾT SẢN LƯỢNG & DOANH SỐ 10 MÃ COMBO
    // =========================================================================
    const float tblY = callY + callH + 12.f;
    const float tblH = height - 32.f - tblY;
    if (tblH > 140.f) {
        Vector2f tPos(position.x + 20.f, tblY);
        RoundedRectRenderer::draw(target, tPos, {availableW, tblH}, 10.f, Color::White, 1.f, Color(226, 232, 240));

        Text tblTitle(boldFont, L"Chi tiết sản lượng & Doanh số 10 mã combo", 14);
        tblTitle.setFillColor(Color(23, 32, 51));
        tblTitle.setPosition({tPos.x + 24.f, tPos.y + 12.f});
        target.draw(tblTitle);

        Text tblSub(font, L"Báo cáo kết xuất từ hệ thống bán lẻ F&B CineCore", 14);
        tblSub.setFillColor(Color(148, 163, 184));
        tblSub.setPosition({tPos.x + 24.f, tPos.y + 34.f});
        target.draw(tblSub);

        wstring totalComboStr = L"Tổng cộng: " + s2ws(toCurrency(comboRevenue));
        Vector2f totPillSize(172.f, 28.f);
        Vector2f totPillPos(tPos.x + availableW - 22.f - totPillSize.x, tPos.y + 15.f);
        RoundedRectRenderer::draw(target, totPillPos, totPillSize, 6.f, Color(248, 250, 252), 1.f, Color(226, 232, 240));
        Text totTxt(boldFont, totalComboStr, 14);
        totTxt.setFillColor(Color(71, 85, 105));
        FloatRect ttb = totTxt.getLocalBounds();
        totTxt.setPosition({
            totPillPos.x + (totPillSize.x - ttb.size.x) / 2.f - ttb.position.x,
            totPillPos.y + (totPillSize.y - ttb.size.y) / 2.f - ttb.position.y
        });
        target.draw(totTxt);

        // Header cột
        const float colHPosY = tPos.y + 56.f;
        const float colHBarH = 30.f;
        RectangleShape hStrip(Vector2f(availableW - 2.f, colHBarH));
        hStrip.setPosition({tPos.x + 1.f, colHPosY});
        hStrip.setFillColor(Color(248, 250, 252));
        target.draw(hStrip);

        RectangleShape hLine(Vector2f(availableW, 1.f));
        hLine.setPosition({tPos.x, colHPosY + colHBarH});
        hLine.setFillColor(Color(226, 232, 240));
        target.draw(hLine);

        const float cx_code  = tPos.x + 24.f;
        const float cx_desc  = tPos.x + availableW * 0.12f;
        const float cx_qty   = tPos.x + availableW * 0.43f;
        const float cx_price = tPos.x + availableW * 0.58f;
        const float cx_rev   = tPos.x + availableW * 0.74f;
        const float cx_share = tPos.x + availableW * 0.89f;

        auto drawColHdr = [&](float x, const wstring& str) {
            Text th(boldFont, str, 11.f);
            th.setFillColor(Color(100, 116, 139));
            th.setPosition({x, colHPosY + 8.f});
            target.draw(th);
        };
        drawColHdr(cx_code,  L"MÃ COMBO");
        drawColHdr(cx_desc,  L"CHI TIẾT GÓI SẢN PHẨM");
        drawColHdr(cx_qty,   L"SỐ LƯỢNG BÁN");
        drawColHdr(cx_price, L"ĐƠN GIÁ THAM CHIẾU");
        drawColHdr(cx_rev,   L"DOANH THU ĐẠT ĐƯỢC");
        drawColHdr(cx_share, L"TỶ TRỌNG F&B");

        const float rStartY = colHPosY + colHBarH;
        const int maxRows = min(5, (int)combosList.size());
        const float rH = (tblH - (rStartY - tPos.y) - 4.f) / static_cast<float>(maxRows);

        for (int i = 0; i < maxRows; ++i) {
            const auto& cbItem = combosList[i];
            float ry = rStartY + i * rH;
            float textY = ry + (rH - 16.f) / 2.f;

            if (i > 0) {
                RectangleShape rDiv(Vector2f(availableW - 48.f, 1.f));
                rDiv.setPosition({tPos.x + 24.f, ry});
                rDiv.setFillColor(Color(241, 245, 249));
                target.draw(rDiv);
            }

            Text tdCode(boldFont, cbItem.code, 13.f);
            tdCode.setFillColor(Color(217, 119, 6));
            tdCode.setPosition({cx_code, textY});
            target.draw(tdCode);

            Text tdDesc(font, cbItem.name, 13.f);
            tdDesc.setFillColor(Color(51, 65, 85));
            tdDesc.setPosition({cx_desc, textY});
            target.draw(tdDesc);

            Text tdQty(boldFont, to_wstring(cbItem.quantity) + L" combo", 13.f);
            tdQty.setFillColor(Color(23, 32, 51));
            tdQty.setPosition({cx_qty, textY});
            target.draw(tdQty);

            Text tdPrice(font, s2ws(toCurrency(cbItem.unitPrice)), 13.f);
            tdPrice.setFillColor(Color(100, 116, 139));
            tdPrice.setPosition({cx_price, textY});
            target.draw(tdPrice);

            Text tdRev(boldFont, s2ws(toCurrency(cbItem.revenue)), 14);
            tdRev.setFillColor(Color(23, 32, 51));
            tdRev.setPosition({cx_rev, textY});
            target.draw(tdRev);

            wstringstream shSs;
            shSs << fixed << setprecision(1) << cbItem.share << L"%";
            Text tdShare(boldFont, shSs.str(), 13.f);
            tdShare.setFillColor(Color(217, 119, 6));
            tdShare.setPosition({cx_share + 14.f, textY});
            target.draw(tdShare);
        }
    }
}

void MovieRevenuePanel::drawBottomBar(RenderTarget& target) const {
    float barY = height - 28.f;

    RectangleShape div(Vector2f(width - 40.f, 1.f));
    div.setPosition({position.x + 20.f, barY - 4.f});
    div.setFillColor(Color(226, 232, 240));
    target.draw(div);

    Text authText(font);
    authText.setString(L"Phụ trách: Admin CineCore (Toàn quyền)  •  1728x972 HD");
    authText.setCharacterSize(12.f);
    authText.setFillColor(Color(100, 116, 139));
    FloatRect ab = authText.getLocalBounds();
    authText.setPosition({position.x + width - ab.size.x - 20.f, barY});
    target.draw(authText);
}

void MovieRevenuePanel::drawToast(RenderTarget& target) const {
    Text toast(boldFont);
    toast.setString(toastMessage);
    toast.setCharacterSize(14);
    toast.setFillColor(Color::White);
    FloatRect tb = toast.getLocalBounds();

    float tw = max(380.f, min(width - 80.f, tb.size.x + 48.f));
    float th = 42.f;
    Vector2f tPos(position.x + (width - tw) / 2.f, position.y + height - 60.f);

    RoundedRectRenderer::draw(target, tPos, {tw, th}, 8.f, Color(22, 163, 74, 245), 1.5f, Color(21, 128, 61));
    toast.setPosition({std::round(tPos.x + (tw - tb.size.x) / 2.f - tb.position.x), std::round(tPos.y + (th - tb.size.y) / 2.f - tb.position.y)});
    target.draw(toast);
}

void MovieRevenuePanel::drawReportModal(RenderTarget& target) const {
    // 1. Phông nền tối mờ toàn màn hình
    RectangleShape dimBg(Vector2f(width + 300.f, height + 100.f));
    dimBg.setPosition({position.x - 300.f, position.y - 50.f});
    dimBg.setFillColor(Color(15, 23, 42, 175));
    target.draw(dimBg);

    // 2. Khung Modal (Rộng 760px, Cao 530px)
    float mw = 760.f;
    float mh = 530.f;
    Vector2f mPos(position.x + (width - mw) / 2.f, position.y + (height - mh) / 2.f);

    RoundedRectRenderer::draw(target, mPos, {mw, mh}, 12.f, Color::White, 1.2f, Color(203, 213, 225));

    // Header Modal
    Text mTitle(boldFont);
    mTitle.setString(L"BÁO CÁO TÀI CHÍNH & DOANH THU CHI TIẾT");
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
    Text mSub(font, L"Niên độ Q3 & Q4/2025  •  Rạp CineCore Đà Nẵng  •  Đã kiểm toán định dạng UTF-8 Excel", 14);
    mSub.setFillColor(Color(100, 116, 139));
    mSub.setPosition({mPos.x + 24.f, mPos.y + 44.f});
    target.draw(mSub);

    // Đường kẻ phân cách
    RectangleShape sep1(Vector2f(mw - 48.f, 1.f));
    sep1.setPosition({mPos.x + 24.f, mPos.y + 68.f});
    sep1.setFillColor(Color(226, 232, 240));
    target.draw(sep1);

    // 3. 3 Thẻ thống kê tổng quan bên trong Modal
    float kpiY = mPos.y + 80.f;
    float kpiW = (mw - 48.f - 20.f) / 3.f; // ~230px
    float kpiH = 68.f;

    auto drawMiniKpi = [&](int idx, const wstring& lbl, const wstring& val, const wstring& note, Color valCol) {
        Vector2f kp(mPos.x + 24.f + idx * (kpiW + 10.f), kpiY);
        RoundedRectRenderer::draw(target, kp, {kpiW, kpiH}, 6.f, Color(248, 250, 252), 1.f, Color(226, 232, 240));
        Text l(font, lbl, 14);
        l.setFillColor(Color(100, 116, 139));
        l.setPosition({kp.x + 12.f, kp.y + 8.f});
        target.draw(l);

        Text v(boldFont, val, 14);
        v.setFillColor(valCol);
        v.setPosition({kp.x + 12.f, kp.y + 28.f});
        target.draw(v);

        Text n(font, note, 14);
        n.setFillColor(Color(148, 163, 184));
        n.setPosition({kp.x + 12.f, kp.y + 48.f});
        target.draw(n);
    };

    drawMiniKpi(0, L"TỔNG DOANH THU TOÀN RẠP", s2ws(toCurrency(totalRevenue)), L"+18.4% so với quý trước", Color(23, 32, 51));
    drawMiniKpi(1, L"DOANH THU VÉ PHIM", s2ws(toCurrency(movieRevenue)), to_wstring(totalTickets) + L" vé đã xuất (59.1%)", Color(37, 99, 235));
    drawMiniKpi(2, L"DOANH THU COMBO F&B", s2ws(toCurrency(comboRevenue)), to_wstring(totalCombos) + L" Combo & Nước (40.9%)", Color(217, 119, 6));

    // 4. Bảng dữ liệu Top 5 Doanh thu Phim
    float tblY = mPos.y + 162.f;
    Text tblHead(boldFont, L"BẢNG ĐỐI CHIẾU DOANH THU & SỐ LƯỢNG VÉ THEO PHIM (TOP 5)", 14);
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
    int showN = min(5, (int)moviesList.size());
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
        Text mTitleTxt(boldFont, moviesList[i].title, 14);
        mTitleTxt.setFillColor(Color(23, 32, 51));
        mTitleTxt.setPosition({c2X, ry + 9.f});
        target.draw(mTitleTxt);

        // Số vé
        Text tksTxt(boldFont, to_wstring(moviesList[i].tickets) + L" vé", 14);
        tksTxt.setFillColor(Color(37, 99, 235));
        tksTxt.setPosition({c3X, ry + 9.f});
        target.draw(tksTxt);

        // Doanh thu
        Text revTxt(boldFont, s2ws(toCurrency(moviesList[i].revenue)), 14);
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
    Text pathTxt(font, L"Vị trí lưu file: data/doanh_thu_report.csv (Mở tự động bằng Excel)", 14);
    pathTxt.setFillColor(Color(100, 116, 139));
    pathTxt.setPosition({mPos.x + 24.f, mPos.y + mh - 42.f});
    target.draw(pathTxt);

    // Nút "Mở bằng Excel / CSV"
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

void MovieRevenuePanel::drawMonthDetailModal(RenderTarget& target) const {
    if (detailMonthIndex < 0 || detailMonthIndex >= (int)monthsList.size()) return;
    const auto& m = monthsList[detailMonthIndex];

    RectangleShape dimBg(Vector2f(width + 300.f, height + 100.f));
    dimBg.setPosition({position.x - 300.f, position.y - 50.f});
    dimBg.setFillColor(Color(15, 23, 42, 175));
    target.draw(dimBg);

    float mw = 540.f;
    float mh = 370.f;
    Vector2f mPos(position.x + (width - mw) / 2.f, position.y + (height - mh) / 2.f);
    RoundedRectRenderer::draw(target, mPos, {mw, mh}, 12.f, Color::White, 1.2f, Color(203, 213, 225));

    Text mTitle(boldFont, L"CHI TIẾT DOANH THU — " + m.displayMonth, 14);
    mTitle.setFillColor(Color(23, 32, 51));
    mTitle.setPosition({mPos.x + 24.f, mPos.y + 20.f});
    target.draw(mTitle);

    if (m.isRecord) {
        Vector2f bPos(mPos.x + 24.f + mTitle.getGlobalBounds().size.x + 12.f, mPos.y + 20.f);
        RoundedRectRenderer::draw(target, bPos, {68.f, 24.f}, 12.f, Color(254, 226, 226));
        Text bTxt(boldFont, L"Kỷ lục", 14);
        bTxt.setFillColor(Color(225, 29, 72));
        FloatRect bb = bTxt.getLocalBounds();
        bTxt.setPosition({bPos.x + (68.f - bb.size.x) / 2.f - bb.position.x, bPos.y + (24.f - bb.size.y) / 2.f - bb.position.y});
        target.draw(bTxt);
    }

    UITheme::drawCloseIcon(target, {mPos.x + mw - 25.f, mPos.y + 27.f}, 11.f, Color(100, 116, 139), 2.f);

    Text mSub(font, L"Tổng hợp đối soát vé phim & combo F&B trong kỳ báo cáo", 14);
    mSub.setFillColor(Color(100, 116, 139));
    mSub.setPosition({mPos.x + 24.f, mPos.y + 46.f});
    target.draw(mSub);

    RectangleShape sep1(Vector2f(mw - 48.f, 1.f));
    sep1.setPosition({mPos.x + 24.f, mPos.y + 72.f});
    sep1.setFillColor(Color(226, 232, 240));
    target.draw(sep1);

    // Dữ liệu vé & combo ước tính theo từng tháng
    const array<int, 4> monthTickets = {14, 25, 37, 25};
    const array<int, 4> monthCombos  = {13, 21, 33, 22};
    int tCount = (detailMonthIndex < 4) ? monthTickets[detailMonthIndex] : 0;
    int cCount = (detailMonthIndex < 4) ? monthCombos[detailMonthIndex] : 0;
    float ticketPct = 100.f - m.comboShare;

    stringstream ssTicket, ssCombo;
    ssTicket << fixed << setprecision(1) << ticketPct << "% tổng doanh thu";
    ssCombo  << fixed << setprecision(1) << m.comboShare << "% tổng doanh thu";

    float cardW = (mw - 48.f - 14.f) / 2.f;
    float cardH = 92.f;

    auto drawDetailCard = [&](int col, int row, const wstring& lbl, const wstring& val, const wstring& note, Color valCol) {
        Vector2f cp(mPos.x + 24.f + col * (cardW + 14.f), mPos.y + 88.f + row * (cardH + 14.f));
        RoundedRectRenderer::draw(target, cp, {cardW, cardH}, 8.f, Color(248, 250, 252), 1.f, Color(226, 232, 240));

        Text l(font, lbl, 14);
        l.setFillColor(Color(100, 116, 139));
        l.setPosition({cp.x + 14.f, cp.y + 12.f});
        target.draw(l);

        Text v(boldFont, val, 24);
        v.setFillColor(valCol);
        v.setPosition({cp.x + 14.f, cp.y + 34.f});
        target.draw(v);

        Text n(boldFont, note, 14);
        n.setFillColor(Color(100, 116, 139));
        n.setPosition({cp.x + 14.f, cp.y + 64.f});
        target.draw(n);
    };

    drawDetailCard(0, 0, L"TỔNG DOANH THU THÁNG", s2ws(toCurrency(m.totalRev)), L"Tăng trưởng: " + m.growth, Color(23, 32, 51));
    drawDetailCard(1, 0, L"SẢN LƯỢNG XUẤT BÁN", to_wstring(tCount) + L" vé  •  " + to_wstring(cCount) + L" combo", L"Tỷ lệ kèm F&B ổn định", Color(8, 125, 181));
    drawDetailCard(0, 1, L"DOANH THU VÉ PHIM", s2ws(toCurrency(m.ticketRev)), s2ws(ssTicket.str()), Color(8, 125, 181));
    drawDetailCard(1, 1, L"DOANH THU COMBO F&B", s2ws(toCurrency(m.comboRev)), s2ws(ssCombo.str()), Color(245, 158, 11));

    RectangleShape sep2(Vector2f(mw - 48.f, 1.f));
    sep2.setPosition({mPos.x + 24.f, mPos.y + mh - 68.f});
    sep2.setFillColor(Color(226, 232, 240));
    target.draw(sep2);

    // Nút "Lọc theo tháng này"
    Vector2f btnFPos(mPos.x + mw - 305.f, mPos.y + mh - 52.f);
    Color fBg = detailFilterBtnHovered ? Color(18, 59, 93) : Color(8, 125, 181);
    RoundedRectRenderer::draw(target, btnFPos, {180.f, 36.f}, 6.f, fBg);
    Text btnFTxt(boldFont, L"Lọc theo tháng này", 14);
    btnFTxt.setFillColor(Color::White);
    FloatRect fb = btnFTxt.getLocalBounds();
    btnFTxt.setPosition({btnFPos.x + (180.f - fb.size.x) / 2.f - fb.position.x, btnFPos.y + (36.f - fb.size.y) / 2.f - fb.position.y});
    target.draw(btnFTxt);

    // Nút "Đóng"
    Vector2f btnCPos(mPos.x + mw - 115.f, mPos.y + mh - 52.f);
    Color cBg = detailCloseBtnHovered ? Color(226, 232, 240) : Color(241, 245, 249);
    RoundedRectRenderer::draw(target, btnCPos, {92.f, 36.f}, 6.f, cBg, 1.f, Color(203, 213, 225));
    Text btnCTxt(boldFont, L"Đóng", 14);
    btnCTxt.setFillColor(Color(51, 65, 85));
    FloatRect cb = btnCTxt.getLocalBounds();
    btnCTxt.setPosition({btnCPos.x + (92.f - cb.size.x) / 2.f - cb.position.x, btnCPos.y + (36.f - cb.size.y) / 2.f - cb.position.y});
    target.draw(btnCTxt);
}
