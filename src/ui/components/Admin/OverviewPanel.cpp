#include "UI/components/Admin/OverviewPanel.h"

#include "repositories/booking/ComboRepository.h"
#include "repositories/booking/ShowtimeRepository.h"
#include "repositories/UserRepository.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <map>
#include <sstream>

using sf::Color;
using sf::FloatRect;
using sf::RenderTarget;
using sf::Vector2f;

OverviewPanel::OverviewPanel(sf::Font& fontRef, float w, float h)
    : font(fontRef),
      width(w),
      height(h),
      position(0.f, 0.f),
      ticketsFilePath(resolveDataPath("data/tickets.txt")),
      usersFilePath(resolveDataPath("data/users.txt")),
    showtimesFilePath(resolveDataPath("data/showtimes.txt")),
    combosFilePath(resolveDataPath("data/combo.txt")),
      background(Vector2f(w, h)),
      revenueTodayCard(font, {250.f, 110.f}, cardOutlineCyan),
      newCustomersCard(font, {250.f, 110.f}, cardOutlineGreen),
      ticketsCard(font, {250.f, 110.f}, cardOutlineYellow),
      totalRevenueCard(font, {250.f, 110.f}, cardOutlineRed),
            movieRevenueCard(font, {250.f, 110.f}, cardOutlineCyan),
            comboRevenueCard(font, {250.f, 110.f}, cardOutlineYellow),
      ticketChartCard(Vector2f(0.f, 0.f)),
      revenueChartCard(Vector2f(0.f, 0.f)),
      movieTableCard(Vector2f(0.f, 0.f)),
      revenueTableCard(Vector2f(0.f, 0.f)) {
    initializeUI();
    loadData();
}

std::string OverviewPanel::resolveDataPath(const std::string& relative) const {
    namespace fs = std::filesystem;
    const std::array<std::string, 4> prefixes = {"../", "./", "../../", ""};
    for (const auto& prefix : prefixes) {
        fs::path candidate = fs::path(prefix) / relative;
        if (fs::exists(candidate)) {
            return candidate.string();
        }
    }
    return relative;
}

void OverviewPanel::initializeUI() {
    background.setFillColor(Color(244, 247, 252));
    background.setPosition(position);

    float margin = 16.f;
    float cardGap = 12.f;
    float statWidth = (width - margin * 2.f - cardGap * 2.f) / 3.f;
    float statHeight = 70.f;
    Vector2f statSize(statWidth, statHeight);

    revenueTodayCard.setSize(statSize);
    newCustomersCard.setSize(statSize);
    ticketsCard.setSize(statSize);
    totalRevenueCard.setSize(statSize);
    movieRevenueCard.setSize(statSize);
    comboRevenueCard.setSize(statSize);

    const float row1Y = position.y + margin;
    const float row2Y = row1Y + statHeight + cardGap;

    revenueTodayCard.setPosition({position.x + margin, row1Y});
    newCustomersCard.setPosition({revenueTodayCard.getPosition().x + statWidth + cardGap, row1Y});
    ticketsCard.setPosition({newCustomersCard.getPosition().x + statWidth + cardGap, row1Y});

    totalRevenueCard.setPosition({position.x + margin, row2Y});
    movieRevenueCard.setPosition({totalRevenueCard.getPosition().x + statWidth + cardGap, row2Y});
    comboRevenueCard.setPosition({movieRevenueCard.getPosition().x + statWidth + cardGap, row2Y});

    // Layout: Tiêu đề ngoài khung, biểu đồ + bảng
    // Reduce unused spacing so the 4 cards can show more rows.
    const float titleSpace = 28.f;
    const float chartGap = 20.f;
    const float rowInsetTop = 16.f;

    float chartTop = position.y + margin + (statHeight * 2.f + cardGap) + titleSpace;  // Space for headings
    float totalChartHeight = height - chartTop - margin;
    float singleRowHeight = (totalChartHeight - chartGap) / 2.f;
    
    float chartWidth = (width - margin * 2.f) * 0.5f;  // 65% cho biểu đồ
    float tableWidth = (width - margin * 2.f) * 0.5f - 12.f;  // 35% cho bảng

    // Biểu đồ cột (trên) - không có tiêu đề bên trong
    ticketChartCard.setSize({chartWidth, std::max(180.f, singleRowHeight - rowInsetTop)});
    ticketChartCard.setPosition({position.x + margin, chartTop + rowInsetTop});
    ticketChartCard.setFillColor(Color::White);
    ticketChartCard.setOutlineThickness(1.f);
    ticketChartCard.setOutlineColor(Color(226, 232, 240));

    // Bảng phim (bên phải biểu đồ cột)
    movieTableCard.setSize({tableWidth, std::max(180.f, singleRowHeight - rowInsetTop)});
    movieTableCard.setPosition({ticketChartCard.getPosition().x + chartWidth + 12.f, chartTop + rowInsetTop});
    movieTableCard.setFillColor(Color::White);
    movieTableCard.setOutlineThickness(1.f);
    movieTableCard.setOutlineColor(Color(226, 232, 240));

    // Biểu đồ đường (dưới)
    float row2Top = chartTop + singleRowHeight + chartGap;
    revenueChartCard.setSize({chartWidth, std::max(180.f, singleRowHeight - rowInsetTop)});
    revenueChartCard.setPosition({position.x + margin, row2Top + rowInsetTop});
    revenueChartCard.setFillColor(Color::White);
    revenueChartCard.setOutlineThickness(1.f);
    revenueChartCard.setOutlineColor(Color(226, 232, 240));

    // Bảng doanh thu (bên phải biểu đồ đường)
    revenueTableCard.setSize({tableWidth, std::max(180.f, singleRowHeight - rowInsetTop)});
    revenueTableCard.setPosition({revenueChartCard.getPosition().x + chartWidth + 12.f, row2Top + rowInsetTop});
    revenueTableCard.setFillColor(Color::White);
    revenueTableCard.setOutlineThickness(1.f);
    revenueTableCard.setOutlineColor(Color(226, 232, 240));
}

void OverviewPanel::loadData() {
    if (dataLoaded) return;  // Không load lại nếu đã có dữ liệu
    
    loadShowtimePrices();
    loadComboPrices();
    loadTickets();
    loadRegistrations();
    calculateDailyRevenue();
    calculateMonthlyStats();
    calculateTopMovies();
    calculateMonthlyTrend();
    
    dataLoaded = true;
}

void OverviewPanel::loadTickets() {
    topMovies.clear();
    monthlyTrend.clear();

    AdminTicketRepository repo(ticketsFilePath);
    ticketsCache = repo.loadAll();
}

void OverviewPanel::loadRegistrations() {
    registrationTimes.clear();

    UserRepository repo(usersFilePath);
    const std::vector<User> users = repo.getAllUsers();
    for (const auto& user : users) {
        registrationTimes[user.getEmail()] = user.getRegisteredAt();
    }
}

void OverviewPanel::loadShowtimePrices() {
    showtimeSeatPrices.clear();

    ShowtimeRepository repo;
    const auto consume = [&](const std::string& path) {
        DLL<Showtime> showtimes = repo.loadFromFile(toSfString(path));
        Node<Showtime>* node = showtimes.getHead();
        while (node) {
            showtimeSeatPrices[node->data.showtime_id.toAnsiString()] = node->data.price;
            node = node->next;
        }
    };

    consume(showtimesFilePath);
    consume(resolveDataPath("data/showtimes_history.txt"));
}

void OverviewPanel::loadComboPrices() {
    comboPrices.clear();

    auto toUtf8 = [](const sf::String& value) {
        const sf::U8String u8 = value.toUtf8();
        std::string out;
        out.reserve(u8.size());
        for (auto ch : u8) {
            out.push_back(static_cast<char>(ch));
        }
        return out;
    };

    ComboRepository repo;
    DLL<Combo> combos = repo.loadFromFile(toSfString(combosFilePath));
    Node<Combo>* node = combos.getHead();
    while (node) {
        // Support both legacy name and Direction B combo_id tokens
        comboPrices[toUtf8(node->data.name)] = node->data.price;
        comboPrices[toUtf8(node->data.id)] = node->data.price;
        node = node->next;
    }
}

void OverviewPanel::calculateDailyRevenue() {
    std::time_t now = std::time(nullptr);
    std::tm* local = std::localtime(&now);

    char buffer[16];
    std::strftime(buffer, sizeof(buffer), "%d/%m/%Y", local);
    std::string today(buffer);

    long long total = 0;
    Node<Ticket>* node = ticketsCache.getHead();
    while (node) {
        if (node->data.bookedDate == today) {
            total += static_cast<long long>(node->data.price);
        }
        node = node->next;
    }

    dailyRevenue = total;
    revenueTodayCard.setTitleWithDate("Doanh thu trong ngày", today);
    revenueTodayCard.setOutlineThickness(3.f);
    revenueTodayCard.setOutlineColor(cardOutlineCyan);
    revenueTodayCard.setValue(toCurrency(dailyRevenue));
}

void OverviewPanel::calculateMonthlyStats() {
    std::time_t now = std::time(nullptr);
    std::tm* local = std::localtime(&now);

    char buffer[16];
    std::strftime(buffer, sizeof(buffer), "%m/%Y", local);
    std::string currentMonth(buffer);

    long long totalRevenue = 0;
    long long movieRevenue = 0;
    long long comboRevenue = 0;
    int ticketsCount = 0;
    std::time_t monthStart;
    {
        std::tm temp = *local;
        temp.tm_mday = 1;
        temp.tm_hour = 0;
        temp.tm_min = 0;
        temp.tm_sec = 0;
        monthStart = std::mktime(&temp);
    }

    Node<Ticket>* node = ticketsCache.getHead();
    while (node) {
        const Ticket& ticket = node->data;
        if (ticket.bookedDate.size() >= 7) {
            std::string month = ticket.bookedDate.substr(3);
            if (month == currentMonth) {
                const int seatCount = std::max(1, countSeats(ticket.booked));
                const long long ticketTotal = static_cast<long long>(ticket.price);
                const long long rawComboPart = computeComboRevenue(ticket.comboName);
                const long long comboPart = std::max(0LL, std::min(rawComboPart, ticketTotal));
                const long long moviePart = computeTicketRevenue(ticket, seatCount, comboPart);

                totalRevenue += ticketTotal;
                movieRevenue += moviePart;
                comboRevenue += comboPart;
                ticketsCount += seatCount;
            }
        }
        node = node->next;
    }

    int newCustomers = 0;
    for (const auto& entry : registrationTimes) {
        if (entry.second >= monthStart) {
            newCustomers++;
        }
    }

    totalRevenueThisMonth = totalRevenue;
    totalTicketsThisMonth = ticketsCount;
    newCustomersThisMonth = newCustomers;

    std::string monthDisplay = "T" + currentMonth;

    newCustomersCard.setTitleWithDate("Khách hàng mới", monthDisplay);
    newCustomersCard.setOutlineThickness(3.f);
    newCustomersCard.setOutlineColor(cardOutlineGreen);
    newCustomersCard.setValue(std::to_string(newCustomersThisMonth));

    ticketsCard.setTitleWithDate("Tổng vé bán ra", monthDisplay);
    ticketsCard.setOutlineThickness(3.f);
    ticketsCard.setOutlineColor(cardOutlineYellow);
    ticketsCard.setValue(std::to_string(totalTicketsThisMonth));

    totalRevenueCard.setTitleWithDate("Tổng doanh thu", monthDisplay);
    totalRevenueCard.setOutlineThickness(3.f);
    totalRevenueCard.setOutlineColor(cardOutlineRed);
    totalRevenueCard.setValue(toCurrency(totalRevenueThisMonth));

    movieRevenueCard.setTitleWithDate("Doanh thu theo phim", monthDisplay);
    movieRevenueCard.setOutlineThickness(3.f);
    movieRevenueCard.setOutlineColor(cardOutlineCyan);
    movieRevenueCard.setValue(toCurrency(movieRevenue));

    comboRevenueCard.setTitleWithDate("Doanh thu combo", monthDisplay);
    comboRevenueCard.setOutlineThickness(3.f);
    comboRevenueCard.setOutlineColor(cardOutlineYellow);
    comboRevenueCard.setValue(toCurrency(comboRevenue));
}

void OverviewPanel::calculateTopMovies() {
    topMovies.clear();

    std::unordered_map<std::string, int> ticketCounts;
    std::unordered_map<std::string, long long> revenueCounts;
    
    Node<Ticket>* node = ticketsCache.getHead();
    while (node) {
        const Ticket& ticket = node->data;
        const int seats = std::max(1, countSeats(ticket.booked));
        ticketCounts[ticket.title] += seats;
        revenueCounts[ticket.title] += static_cast<long long>(ticket.price);
        node = node->next;
    }

    for (const auto& entry : ticketCounts) {
        MovieTicketCount item;
        item.title = entry.first;
        item.tickets = entry.second;
        item.revenue = revenueCounts[entry.first];
        topMovies.push_back(item);
    }

    sortTopMovies();
}

void OverviewPanel::calculateMonthlyTrend() {
    monthlyTrend.clear();

    std::map<std::pair<int, int>, long long> revenueByMonth;
    Node<Ticket>* node = ticketsCache.getHead();
    while (node) {
        const Ticket& ticket = node->data;
        if (ticket.bookedDate.size() >= 10) {
            try {
                int month = std::stoi(ticket.bookedDate.substr(3, 2));
                int year = std::stoi(ticket.bookedDate.substr(6, 4));

                revenueByMonth[{year, month}] += static_cast<long long>(ticket.price);
            } catch (...) {}
        }
        node = node->next;
    }

    for (const auto& entry : revenueByMonth) {
        MonthlyRevenuePoint point{};
        point.year = entry.first.first;
        point.month = entry.first.second;
        point.revenue = entry.second;
        monthlyTrend.push_back(point);
    }

    sortMonthlyTrend();
}

int OverviewPanel::countSeats(const std::string& seatList) const {
    if (seatList.empty()) return 0;

    int count = 0;
    std::size_t start = 0;
    while (start < seatList.size()) {
        std::size_t end = seatList.find(',', start);
        std::string token = (end == std::string::npos)
            ? seatList.substr(start)
            : seatList.substr(start, end - start);

        // Trim whitespace
        std::size_t first = token.find_first_not_of(" \t\n\r");
        if (first != std::string::npos) {
            std::size_t last = token.find_last_not_of(" \t\n\r");
            token = token.substr(first, last - first + 1);
            if (!token.empty()) {
                count++;
            }
        }

        if (end == std::string::npos) break;
        start = end + 1;
    }
    return count;
}

std::string OverviewPanel::trim(const std::string& text) const {
    const std::size_t start = text.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    const std::size_t end = text.find_last_not_of(" \t\n\r");
    return text.substr(start, end - start + 1);
}

long long OverviewPanel::computeComboRevenue(const std::string& comboList) const {
    if (comboList.empty()) return 0;
    if (comboList == "Không có") return 0;

    long long total = 0;
    std::size_t start = 0;
    while (start < comboList.size()) {
        std::size_t end = comboList.find(',', start);
        std::string item = (end == std::string::npos)
            ? comboList.substr(start)
            : comboList.substr(start, end - start);
        item = trim(item);

        if (!item.empty()) {
            int quantity = 1;

            // Direction B: "CBxx:xN" (preferred)
            std::size_t dirPos = item.find(":x");
            if (dirPos != std::string::npos) {
                try {
                    quantity = std::max(1, std::stoi(trim(item.substr(dirPos + 2))));
                } catch (...) {
                    quantity = 1;
                }
                item = trim(item.substr(0, dirPos));
            } else {
                // Legacy: "Name xN"
                std::size_t xPos = item.rfind('x');
                if (xPos != std::string::npos) {
                    try {
                        quantity = std::max(1, std::stoi(trim(item.substr(xPos + 1))));
                    } catch (...) {
                        quantity = 1;
                    }
                    item = trim(item.substr(0, xPos));
                }
            }

            auto it = comboPrices.find(item);
            if (it != comboPrices.end()) {
                total += static_cast<long long>(it->second) * quantity;
            }
        }

        if (end == std::string::npos) break;
        start = end + 1;
    }
    return total;
}

long long OverviewPanel::computeTicketRevenue(const Ticket& ticket, int seatCount, long long comboRevenue) const {
    // Split must be consistent with total revenue source-of-truth: Ticket.price.
    // Movie revenue is always (ticketTotal - comboPart) so that movie + combo == total.
    if (seatCount <= 0) return 0;

    const long long ticketTotal = static_cast<long long>(ticket.price);
    const long long safeCombo = std::max(0LL, std::min(comboRevenue, ticketTotal));
    const long long moviePart = ticketTotal - safeCombo;
    return std::max(0LL, moviePart);
}

std::string OverviewPanel::toCurrency(long long amount) const {
    std::string digits = std::to_string(amount);
    int insertPosition = static_cast<int>(digits.length()) - 3;
    while (insertPosition > 0) {
        digits.insert(static_cast<std::size_t>(insertPosition), ".");
        insertPosition -= 3;
    }
    return digits + "đ";
}

// Làm tròn lên thành số đẹp
long long OverviewPanel::roundUpNice(long long value) const {
    if (value <= 0) return 100;
    if (value <= 100) return 100;
    if (value <= 500) return 500;
    if (value <= 1000) return 1000;
    
    // Tìm bậc của số (10, 100, 1000, ...)
    long long magnitude = 1;
    long long temp = value;
    while (temp >= 10) {
        temp /= 10;
        magnitude *= 10;
    }
    
    // Làm tròn lên theo bậc
    long long rounded = ((value + magnitude - 1) / magnitude) * magnitude;
    
    // Điều chỉnh cho đẹp (1, 2, 5, 10)
    long long digit = rounded / magnitude;
    if (digit <= 1) rounded = magnitude;
    else if (digit <= 2) rounded = 2 * magnitude;
    else if (digit <= 5) rounded = 5 * magnitude;
    else rounded = 10 * magnitude;
    
    return rounded;
}

sf::String OverviewPanel::toSfString(const std::string& text) const {
    return sf::String::fromUtf8(text.begin(), text.end());
}

std::string OverviewPanel::ellipsize(const std::string& text, std::size_t maxChars) const {
    // Đếm số ký tự UTF-8 thực sự (không phải bytes)
    std::size_t charCount = 0;
    std::size_t bytePos = 0;
    
    while (bytePos < text.size()) {
        unsigned char c = static_cast<unsigned char>(text[bytePos]);
        if ((c & 0x80) == 0) {
            // ASCII (1 byte)
            bytePos += 1;
        } else if ((c & 0xE0) == 0xC0) {
            // 2 bytes UTF-8
            bytePos += 2;
        } else if ((c & 0xF0) == 0xE0) {
            // 3 bytes UTF-8 (tiếng Việt thường dùng)
            bytePos += 3;
        } else if ((c & 0xF8) == 0xF0) {
            // 4 bytes UTF-8
            bytePos += 4;
        } else {
            bytePos += 1;
        }
        charCount++;
    }
    
    // Nếu số ký tự <= maxChars, trả về nguyên bản
    if (charCount <= maxChars) return text;
    if (maxChars <= 3) {
        // Cắt maxChars ký tự UTF-8
        std::size_t cutBytes = 0;
        std::size_t cutChars = 0;
        while (cutBytes < text.size() && cutChars < maxChars) {
            unsigned char c = static_cast<unsigned char>(text[cutBytes]);
            if ((c & 0x80) == 0) cutBytes += 1;
            else if ((c & 0xE0) == 0xC0) cutBytes += 2;
            else if ((c & 0xF0) == 0xE0) cutBytes += 3;
            else if ((c & 0xF8) == 0xF0) cutBytes += 4;
            else cutBytes += 1;
            cutChars++;
        }
        return text.substr(0, cutBytes);
    }
    
    // Cắt (maxChars - 3) ký tự UTF-8 và thêm "..."
    std::size_t targetChars = maxChars - 3;
    std::size_t cutBytes = 0;
    std::size_t cutChars = 0;
    while (cutBytes < text.size() && cutChars < targetChars) {
        unsigned char c = static_cast<unsigned char>(text[cutBytes]);
        if ((c & 0x80) == 0) cutBytes += 1;
        else if ((c & 0xE0) == 0xC0) cutBytes += 2;
        else if ((c & 0xF0) == 0xE0) cutBytes += 3;
        else if ((c & 0xF8) == 0xF0) cutBytes += 4;
        else cutBytes += 1;
        cutChars++;
    }
    return text.substr(0, cutBytes) + "...";
}

void OverviewPanel::sortTopMovies() {
    std::vector<MovieTicketCount> items;
    Node<MovieTicketCount>* node = topMovies.getHead();
    while (node) {
        items.push_back(node->data);
        node = node->next;
    }

    std::sort(items.begin(), items.end(), [](const MovieTicketCount& a, const MovieTicketCount& b) {
        return a.tickets > b.tickets;
    });

    topMovies.clear();
    for (const auto& item : items) {
        topMovies.push_back(item);
    }
}

void OverviewPanel::sortMonthlyTrend() {
    std::vector<MonthlyRevenuePoint> items;
    Node<MonthlyRevenuePoint>* node = monthlyTrend.getHead();
    while (node) {
        items.push_back(node->data);
        node = node->next;
    }

    std::sort(items.begin(), items.end(), [](const MonthlyRevenuePoint& a, const MonthlyRevenuePoint& b) {
        if (a.year == b.year) return a.month < b.month;
        return a.year < b.year;
    });

    monthlyTrend.clear();
    for (const auto& item : items) {
        monthlyTrend.push_back(item);
    }
}

void OverviewPanel::drawTicketChart(sf::RenderTarget& target) const {
    // Tiêu đề ngoài khung
    sf::Text heading(font, toSfString("Phim bán chạy nhất"), 17);
    heading.setFillColor(Color(30, 41, 59));
    heading.setStyle(sf::Text::Bold);
    heading.setPosition(sf::Vector2f(ticketChartCard.getPosition().x, ticketChartCard.getPosition().y - 22.f));
    target.draw(heading);

    target.draw(ticketChartCard);

    if (topMovies.isEmpty()) {
        sf::Text empty(font, toSfString("Không có dữ liệu"), 13);
        empty.setFillColor(Color(120, 130, 140));
        empty.setPosition(sf::Vector2f(ticketChartCard.getPosition().x + 20.f, ticketChartCard.getPosition().y + 40.f));
        target.draw(empty);
        return;
    }

    std::vector<MovieTicketCount> items;
    const Node<MovieTicketCount>* node = topMovies.getHead();
    while (node && items.size() < 8) {
        items.push_back(node->data);
        node = node->next;
    }

    int maxTickets = 0;
    for (const auto& item : items) {
        maxTickets = std::max(maxTickets, item.tickets);
    }
    int roundedMax = static_cast<int>(roundUpNice(maxTickets));
    if (roundedMax == 0) roundedMax = 10;

    // Layout: nhãn phim nằm ngang bên dưới, cột lấp đầy chiều ngang
    float paddingLeft = 35.f;
    float paddingRight = 10.f;
    float paddingTop = 10.f;
    float labelAreaHeight = 22.f;  // Chiều cao vùng nhãn phim
    float paddingBottom = labelAreaHeight + 8.f;
    
    float chartWidth = ticketChartCard.getSize().x - paddingLeft - paddingRight;
    float chartHeight = ticketChartCard.getSize().y - paddingTop - paddingBottom;
    float originX = ticketChartCard.getPosition().x + paddingLeft;
    float originY = ticketChartCard.getPosition().y + ticketChartCard.getSize().y - paddingBottom;

    // Vẽ grid lines
    int gridLines = 5;
    for (int i = 0; i <= gridLines; ++i) {
        float y = originY - (chartHeight / gridLines) * static_cast<float>(i);
        sf::RectangleShape line({chartWidth, 1.f});
        line.setFillColor(Color(229, 233, 240));
        line.setPosition(sf::Vector2f(originX, y));
        target.draw(line);

        int labelValue = roundedMax * i / gridLines;
        sf::Text label(font, toSfString(std::to_string(labelValue)), 11);
        label.setFillColor(Color(120, 130, 140));
        FloatRect bounds = label.getLocalBounds();
        label.setPosition(sf::Vector2f(originX - bounds.size.x - 6.f - bounds.position.x, y - bounds.size.y / 2.f - bounds.position.y));
        target.draw(label);
    }

    // Tính toán cột lấp đầy chiều ngang
    std::size_t numBars = items.size();
    float gap = 6.f;  // Khoảng cách giữa các cột
    float totalGaps = gap * (numBars + 1);
    float barWidth = (chartWidth - totalGaps) / static_cast<float>(numBars);
    
    // Giới hạn chiều rộng cột tối đa
    if (barWidth > 80.f) barWidth = 80.f;
    
    // Tính lại vị trí bắt đầu để căn giữa nếu cột bị giới hạn
    float totalBarsWidth = numBars * barWidth + totalGaps;
    float startX = originX + (chartWidth - totalBarsWidth) / 2.f + gap;

    for (std::size_t i = 0; i < numBars; ++i) {
        float barX = startX + i * (barWidth + gap);
        
        float ratio = items[i].tickets / static_cast<float>(roundedMax);
        float barHeight = ratio * chartHeight;
        if (barHeight < 4.f) barHeight = 4.f;
        float barY = originY - barHeight;

        // Vẽ cột
        sf::RectangleShape bar({barWidth, barHeight});
        bar.setFillColor(Color(59, 130, 246));
        bar.setPosition(sf::Vector2f(barX, barY));
        target.draw(bar);

        // Giá trị trên cột
        sf::Text value(font, toSfString(std::to_string(items[i].tickets)), 13);
        FloatRect valueBounds = value.getLocalBounds();
        float valueY = barY - valueBounds.size.y - 2.f - valueBounds.position.y;
        float minY = ticketChartCard.getPosition().y + paddingTop;
        if (valueY < minY) {
            valueY = barY + 2.f - valueBounds.position.y;
            value.setFillColor(Color::White);
        } else {
            value.setFillColor(Color(30, 41, 59));
        }
        value.setPosition(sf::Vector2f(
            barX + (barWidth - valueBounds.size.x) / 2.f - valueBounds.position.x,
            valueY
        ));
        target.draw(value);

        // Nhãn phim - nằm ngang, căn trái từ lề trái cột
        // Tính số ký tự tối đa vừa với chiều rộng cột
        float maxLabelWidth = barWidth - 2.f;  // Trừ margin nhỏ
        
        // Ước tính số ký tự (font 10, ~6px/char)
        int maxChars = static_cast<int>(maxLabelWidth / 6.f);
        if (maxChars < 3) maxChars = 3;
        
        std::string displayName = ellipsize(items[i].title, static_cast<std::size_t>(maxChars));
        
        // Dùng sf::String::fromUtf8 để fix lỗi font tiếng Việt
        sf::String sfDisplayName = sf::String::fromUtf8(displayName.begin(), displayName.end());
        sf::Text label(font, sfDisplayName, 10);
        label.setFillColor(Color(71, 85, 105));
        
        // Căn trái, bắt đầu từ lề trái của cột
        float labelY = originY + 6.f;
        label.setPosition(sf::Vector2f(barX, labelY));
        target.draw(label);
    }
}

void OverviewPanel::drawRevenueChart(sf::RenderTarget& target) const {
    // Tiêu đề ngoài khung
    sf::Text heading(font, toSfString("Doanh thu theo tháng"), 17);
    heading.setFillColor(Color(30, 41, 59));
    heading.setStyle(sf::Text::Bold);
    heading.setPosition(sf::Vector2f(revenueChartCard.getPosition().x, revenueChartCard.getPosition().y - 22.f));
    target.draw(heading);

    target.draw(revenueChartCard);

    if (monthlyTrend.isEmpty()) {
        sf::Text empty(font, toSfString("Không có dữ liệu"), 13);
        empty.setFillColor(Color(120, 130, 140));
        empty.setPosition(sf::Vector2f(revenueChartCard.getPosition().x + 20.f, revenueChartCard.getPosition().y + 40.f));
        target.draw(empty);
        return;
    }

    std::vector<MonthlyRevenuePoint> points;
    const Node<MonthlyRevenuePoint>* node = monthlyTrend.getHead();
    while (node) {
        points.push_back(node->data);
        node = node->next;
    }

    long long maxRevenue = 0;
    for (const auto& point : points) {
        maxRevenue = std::max(maxRevenue, point.revenue);
    }
    long long roundedMax = roundUpNice(maxRevenue);
    if (roundedMax == 0) roundedMax = 1000000;

    float paddingLeft = 90.f;
    float paddingRight = 20.f;
    float paddingTop = 15.f;
    float paddingBottom = 35.f;
    float chartWidth = revenueChartCard.getSize().x - paddingLeft - paddingRight;
    float chartHeight = revenueChartCard.getSize().y - paddingTop - paddingBottom;
    float originX = revenueChartCard.getPosition().x + paddingLeft;
    float originY = revenueChartCard.getPosition().y + revenueChartCard.getSize().y - paddingBottom;

    int gridLines = 5;
    for (int i = 0; i <= gridLines; ++i) {
        float y = originY - (chartHeight / gridLines) * static_cast<float>(i);
        sf::RectangleShape line({chartWidth, 1.f});
        line.setFillColor(Color(229, 233, 240));
        line.setPosition(sf::Vector2f(originX, y));
        target.draw(line);

        long long labelValue = roundedMax * i / gridLines;
        sf::Text label(font, toSfString(toCurrency(labelValue)), 11);
        label.setFillColor(Color(120, 130, 140));
        FloatRect bounds = label.getLocalBounds();
        label.setPosition(sf::Vector2f(originX - bounds.size.x - 6.f - bounds.position.x, y - bounds.size.y / 2.f - bounds.position.y));
        target.draw(label);
    }

    // Xử lý trường hợp chỉ có 1 điểm
    if (points.size() == 1) {
        float x = originX + chartWidth / 2.f;
        float ratio = points[0].revenue / static_cast<float>(roundedMax);
        float y = originY - ratio * chartHeight;

        sf::CircleShape dot(5.f);
        dot.setFillColor(Color(236, 72, 153));
        dot.setOrigin(sf::Vector2f(5.f, 5.f));
        dot.setPosition(sf::Vector2f(x, y));
        target.draw(dot);

        char labelBuffer[16];
        std::snprintf(labelBuffer, sizeof(labelBuffer), "%02d/%04d", points[0].month, points[0].year);
        sf::Text monthLabel(font, toSfString(labelBuffer), 10);
        monthLabel.setFillColor(Color(94, 106, 123));
        FloatRect monthBounds = monthLabel.getLocalBounds();
        monthLabel.setPosition(sf::Vector2f(x - monthBounds.size.x / 2.f - monthBounds.position.x, originY + 6.f - monthBounds.position.y));
        target.draw(monthLabel);
        return;
    }

    sf::VertexArray lineStrip(sf::PrimitiveType::LineStrip, points.size());
    std::vector<sf::CircleShape> dots;
    std::vector<sf::Text> monthLabels;
    dots.reserve(points.size());
    monthLabels.reserve(points.size());

    float step = chartWidth / static_cast<float>(points.size() - 1);
    for (std::size_t i = 0; i < points.size(); ++i) {
        float x = originX + step * static_cast<float>(i);
        float ratio = points[i].revenue / static_cast<float>(roundedMax);
        float y = originY - ratio * chartHeight;

        lineStrip[i].position = {x, y};
        lineStrip[i].color = Color(236, 72, 153);

        sf::CircleShape dot(4.f);
        dot.setFillColor(Color(236, 72, 153));
        dot.setOrigin(sf::Vector2f(4.f, 4.f));
        dot.setPosition(sf::Vector2f(x, y));
        dots.push_back(dot);

        // Không hiển thị giá trị trên điểm - sẽ hiển thị ở bảng bên phải

        char labelBuffer[16];
        std::snprintf(labelBuffer, sizeof(labelBuffer), "%02d/%04d", points[i].month, points[i].year);
        sf::Text monthLabel(font, toSfString(labelBuffer), 11);
        monthLabel.setFillColor(Color(94, 106, 123));
        FloatRect monthBounds = monthLabel.getLocalBounds();
        
        // Căn vị trí nhãn tháng:
        // - Tháng đầu tiên: căn trái (lề trái trùng với gridLine)
        // - Tháng cuối cùng: căn phải (lề phải trùng với gridLine)
        // - Các tháng giữa: căn giữa
        float labelX;
        if (i == 0) {
            // Căn trái - lề trái trùng với originX
            labelX = originX - monthBounds.position.x;
        } else if (i == points.size() - 1) {
            // Căn phải - lề phải trùng với originX + chartWidth
            labelX = originX + chartWidth - monthBounds.size.x - monthBounds.position.x;
        } else {
            // Căn giữa
            labelX = x - monthBounds.size.x / 2.f - monthBounds.position.x;
        }
        
        monthLabel.setPosition(sf::Vector2f(labelX, originY + 6.f - monthBounds.position.y));
        monthLabels.push_back(monthLabel);
    }

    target.draw(lineStrip);
    for (const auto& dot : dots) {
        target.draw(dot);
    }
    for (const auto& label : monthLabels) {
        target.draw(label);
    }
}

void OverviewPanel::drawMovieTable(sf::RenderTarget& target) const {
    // Tiêu đề bảng
    sf::Text heading(font, toSfString("Chi tiết vé & doanh thu"), 17);
    heading.setFillColor(Color(30, 41, 59));
    heading.setStyle(sf::Text::Bold);
    heading.setPosition(sf::Vector2f(movieTableCard.getPosition().x, movieTableCard.getPosition().y - 26.f));
    target.draw(heading);

    target.draw(movieTableCard);

    if (topMovies.isEmpty()) return;

    float padding = 12.f;
    float startX = movieTableCard.getPosition().x + padding;
    float startY = movieTableCard.getPosition().y + padding;
    float rowHeight = 28.f;
    float tableWidth = movieTableCard.getSize().x - padding * 2.f;

    // Header
    sf::Text headerMovie(font, toSfString("Phim"), 15);
    headerMovie.setFillColor(Color(71, 85, 105));
    headerMovie.setStyle(sf::Text::Bold);
    headerMovie.setPosition(sf::Vector2f(startX, startY));
    target.draw(headerMovie);

    sf::Text headerTickets(font, toSfString("Vé"), 15);
    headerTickets.setFillColor(Color(71, 85, 105));
    headerTickets.setStyle(sf::Text::Bold);
    headerTickets.setPosition(sf::Vector2f(startX + tableWidth * 0.58f, startY));
    target.draw(headerTickets);

    sf::Text headerRevenue(font, toSfString("Doanh thu"), 15);
    headerRevenue.setFillColor(Color(71, 85, 105));
    headerRevenue.setStyle(sf::Text::Bold);
    headerRevenue.setPosition(sf::Vector2f(startX + tableWidth * 0.72f, startY));
    target.draw(headerRevenue);

    // Đường kẻ header
    sf::RectangleShape headerLine({tableWidth, 1.f});
    headerLine.setFillColor(Color(226, 232, 240));
    headerLine.setPosition(sf::Vector2f(startX, startY + rowHeight - 6.f));
    target.draw(headerLine);

    // Dữ liệu
    float currentY = startY + rowHeight;
    const Node<MovieTicketCount>* node = topMovies.getHead();
    int count = 0;
    int maxRows = static_cast<int>((movieTableCard.getSize().y - padding * 2.f - rowHeight) / rowHeight);

    while (node && count < maxRows) {
        // Zebra striping
        if (count % 2 == 1) {
            sf::RectangleShape rowBg({tableWidth, rowHeight});
            rowBg.setFillColor(Color(248, 250, 252));
            rowBg.setPosition(sf::Vector2f(startX, currentY));
            target.draw(rowBg);
        }

        sf::Text movieText(font, toSfString(node->data.title), 14);
        movieText.setFillColor(Color(51, 65, 85));
        movieText.setPosition(sf::Vector2f(startX, currentY + 4.f));
        target.draw(movieText);

        sf::Text ticketText(font, toSfString(std::to_string(node->data.tickets)), 14);
        ticketText.setFillColor(Color(59, 130, 246));
        ticketText.setPosition(sf::Vector2f(startX + tableWidth * 0.58f, currentY + 4.f));
        target.draw(ticketText);

        sf::Text revenueText(font, toSfString(toCurrency(node->data.revenue)), 13);
        revenueText.setFillColor(Color(236, 72, 153));
        revenueText.setPosition(sf::Vector2f(startX + tableWidth * 0.72f, currentY + 4.f));
        target.draw(revenueText);

        currentY += rowHeight;
        node = node->next;
        count++;
    }
}

void OverviewPanel::drawRevenueTable(sf::RenderTarget& target) const {
    // Tiêu đề bảng
    sf::Text heading(font, toSfString("Chi tiết doanh thu theo tháng"), 17);
    heading.setFillColor(Color(30, 41, 59));
    heading.setStyle(sf::Text::Bold);
    heading.setPosition(sf::Vector2f(revenueTableCard.getPosition().x, revenueTableCard.getPosition().y - 26.f));
    target.draw(heading);

    target.draw(revenueTableCard);

    if (monthlyTrend.isEmpty()) return;

    float padding = 12.f;
    float startX = revenueTableCard.getPosition().x + padding;
    float startY = revenueTableCard.getPosition().y + padding;
    float rowHeight = 28.f;
    float tableWidth = revenueTableCard.getSize().x - padding * 2.f;

    // Header
    sf::Text headerMonth(font, toSfString("Tháng"), 15);
    headerMonth.setFillColor(Color(71, 85, 105));
    headerMonth.setStyle(sf::Text::Bold);
    headerMonth.setPosition(sf::Vector2f(startX, startY));
    target.draw(headerMonth);

    sf::Text headerRevenue(font, toSfString("Doanh thu"), 15);
    headerRevenue.setFillColor(Color(71, 85, 105));
    headerRevenue.setStyle(sf::Text::Bold);
    headerRevenue.setPosition(sf::Vector2f(startX + tableWidth * 0.4f, startY));
    target.draw(headerRevenue);

    // Đường kẻ header
    sf::RectangleShape headerLine({tableWidth, 1.f});
    headerLine.setFillColor(Color(226, 232, 240));
    headerLine.setPosition(sf::Vector2f(startX, startY + rowHeight - 6.f));
    target.draw(headerLine);

    // Dữ liệu
    float currentY = startY + rowHeight;
    const Node<MonthlyRevenuePoint>* node = monthlyTrend.getHead();
    int count = 0;
    int maxRows = static_cast<int>((revenueTableCard.getSize().y - padding * 2.f - rowHeight) / rowHeight);

    while (node && count < maxRows) {
        // Zebra striping
        if (count % 2 == 1) {
            sf::RectangleShape rowBg({tableWidth, rowHeight});
            rowBg.setFillColor(Color(248, 250, 252));
            rowBg.setPosition(sf::Vector2f(startX, currentY));
            target.draw(rowBg);
        }

        char monthBuffer[16];
        std::snprintf(monthBuffer, sizeof(monthBuffer), "%02d/%04d", node->data.month, node->data.year);
        sf::Text monthText(font, toSfString(monthBuffer), 14);
        monthText.setFillColor(Color(51, 65, 85));
        monthText.setPosition(sf::Vector2f(startX, currentY + 5.f));
        target.draw(monthText);

        sf::Text revenueText(font, toSfString(toCurrency(node->data.revenue)), 14);
        revenueText.setFillColor(Color(236, 72, 153));
        revenueText.setPosition(sf::Vector2f(startX + tableWidth * 0.4f, currentY + 5.f));
        target.draw(revenueText);

        currentY += rowHeight;
        node = node->next;
        count++;
    }
}

void OverviewPanel::setPosition(const Vector2f& pos) {
    position = pos;
    initializeUI();
}

void OverviewPanel::render(RenderTarget& target) const {
    target.draw(background);
    revenueTodayCard.render(target);
    newCustomersCard.render(target);
    ticketsCard.render(target);
    totalRevenueCard.render(target);
    movieRevenueCard.render(target);
    comboRevenueCard.render(target);

    drawTicketChart(target);
    drawMovieTable(target);
    drawRevenueChart(target);
    drawRevenueTable(target);
}

void OverviewPanel::refresh() {
    dataLoaded = false;  // Reset để load lại khi cần
    loadData();
}
