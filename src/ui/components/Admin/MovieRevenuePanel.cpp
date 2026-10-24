#include "UI/components/Admin/MovieRevenuePanel.h"

#include "repositories/MovieRepository.h"
#include "repositories/booking/ComboRepository.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <map>
#include <sstream>
#include <unordered_set>

using sf::Color;
using sf::Event;
using sf::FloatRect;
using sf::RenderTarget;
using sf::RenderWindow;
using sf::Vector2f;

namespace {
    std::string formatCurrency(long long amount) {
        std::string digits = std::to_string(amount);
        int insertPos = static_cast<int>(digits.length()) - 3;
        while (insertPos > 0) {
            digits.insert(static_cast<std::size_t>(insertPos), ".");
            insertPos -= 3;
        }
        return digits + "đ";
    }

    sf::String toSfString(const std::string& text) {
        return sf::String::fromUtf8(text.begin(), text.end());
    }

    // Làm tròn lên thành số đẹp
    long long roundUpNice(long long value) {
        if (value <= 0) return 100;
        if (value <= 100) return 100;
        if (value <= 500) return 500;
        if (value <= 1000) return 1000;
        
        long long magnitude = 1;
        long long temp = value;
        while (temp >= 10) {
            temp /= 10;
            magnitude *= 10;
        }
        
        long long rounded = ((value + magnitude - 1) / magnitude) * magnitude;
        long long digit = rounded / magnitude;
        if (digit <= 1) rounded = magnitude;
        else if (digit <= 2) rounded = 2 * magnitude;
        else if (digit <= 5) rounded = 5 * magnitude;
        else rounded = 10 * magnitude;
        
        return rounded;
    }
}

MovieRevenuePanel::MovieRevenuePanel(sf::Font& fontRef, float w, float h)
        : font(fontRef),
            width(w),
            height(h),
            position(0.f, 0.f),
            ticketsFilePath(resolveDataPath("data/tickets.txt")),
            moviesFilePath(resolveDataPath("data/movies.txt")),
            showtimesFilePath(resolveDataPath("data/showtimes.txt")),
            combosFilePath(resolveDataPath("data/combo.txt")),
            background(Vector2f(w, h)),
            titleText(fontRef, sf::String(), 20),
            fromLabel(fontRef, sf::String(), 14),
            toLabel(fontRef, sf::String(), 14),
            totalRevenueCard(fontRef, {250.f, 70.f}, Color(239, 68, 68)),
            movieRevenueCard(fontRef, {250.f, 70.f}, Color(13, 148, 136)),
            comboRevenueCard(fontRef, {250.f, 70.f}, Color(221, 180, 67)),
            loadButton(fontRef, "Tải dữ liệu", sf::Vector2f(140.f, 38.f), 8.f),
            ticketsToggle(fontRef),
            revenueToggle(fontRef),
            movieOnlyToggle(fontRef),
            comboToggle(fontRef) {
        initializeUI();
        layoutComponents();
        loadData();
}

std::string MovieRevenuePanel::resolveDataPath(const std::string& relative) const {
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

void MovieRevenuePanel::initializeUI() {
    background.setFillColor(Color(244, 247, 252));
    background.setPosition(position);

    const float dropdownHeight = 38.f;

    titleText.setCharacterSize(26);
    titleText.setString(toSfString("Quản lý doanh thu"));
    titleText.setFillColor(Color(27, 38, 59));
    titleText.setStyle(sf::Text::Bold);

    totalRevenueCard.setOutlineThickness(3.f);
    movieRevenueCard.setOutlineThickness(3.f);
    comboRevenueCard.setOutlineThickness(3.f);

    fromLabel.setString(toSfString("Từ"));
    fromLabel.setFillColor(Color(71, 85, 105));

    toLabel.setString(toSfString("Đến"));
    toLabel.setFillColor(Color(71, 85, 105));

    // Date dropdowns (From yyyy-mm-dd -> To yyyy-mm-dd)
    fromYearDropdown = std::make_unique<DropdownBox>(font, "", 0.f, 0.f, 100.f, dropdownHeight);
    fromMonthDropdown = std::make_unique<DropdownBox>(font, "", 0.f, 0.f, 85.f, dropdownHeight);
    fromDayDropdown = std::make_unique<DropdownBox>(font, "", 0.f, 0.f, 85.f, dropdownHeight);

    toYearDropdown = std::make_unique<DropdownBox>(font, "", 0.f, 0.f, 100.f, dropdownHeight);
    toMonthDropdown = std::make_unique<DropdownBox>(font, "", 0.f, 0.f, 85.f, dropdownHeight);
    toDayDropdown = std::make_unique<DropdownBox>(font, "", 0.f, 0.f, 85.f, dropdownHeight);

    for (auto* dropdown : {fromYearDropdown.get(), fromMonthDropdown.get(), fromDayDropdown.get(),
                           toYearDropdown.get(), toMonthDropdown.get(), toDayDropdown.get()}) {
        if (dropdown) dropdown->setMaxVisibleOptions(12);
    }

    if (fromMonthDropdown) fromMonthDropdown->setEnabled(true);
    if (fromDayDropdown) fromDayDropdown->setEnabled(true);
    if (toMonthDropdown) toMonthDropdown->setEnabled(true);
    if (toDayDropdown) toDayDropdown->setEnabled(true);

    loadButton.setSize(sf::Vector2f(140.f, dropdownHeight));
    loadButton.setRadius(8.f);
    loadButton.setFillColor(Color(59, 130, 246));
    loadButton.setOutline(0.f, Color::Transparent);
    loadButton.setTextColor(Color::White);

    auto configureToggle = [&](ModeToggle& toggle, const std::string& label) {
        toggle.shape.setSize(sf::Vector2f(200.f, dropdownHeight));
        toggle.shape.setFillColor(Color(229, 231, 235));
        toggle.shape.setOutlineThickness(1.f);
        toggle.shape.setOutlineColor(Color(209, 213, 219));
        toggle.label.setFont(font);
        toggle.label.setCharacterSize(14);
        toggle.label.setString(toSfString(label));
        toggle.label.setFillColor(Color(71, 85, 105));
        toggle.label.setStyle(sf::Text::Bold);
    };

    configureToggle(ticketsToggle, "Số vé bán ra theo phim");
    configureToggle(revenueToggle, "Tổng doanh thu");
    configureToggle(movieOnlyToggle, "Doanh thu theo phim");
    configureToggle(comboToggle, "Doanh thu theo combo");

    chartCard.setFillColor(Color::White);
    chartCard.setOutlineThickness(1.f);
    chartCard.setOutlineColor(Color(226, 232, 240));

    setChartMode(currentChartMode);
}

void MovieRevenuePanel::layoutComponents() {
    background.setPosition(position);

    // Keep title aligned with other admin management panels
    titleText.setPosition({position.x + 40.f, position.y + 20.f});

    // Align content with other Admin states (e.g., MoviePanel/VoucherPanel title/table margin)
    const float margin = 40.f;
    const float statHeight = 70.f;
    const float statGap = 12.f;
    const float statsY = position.y + 60.f;

    const float statWidth = (width - margin * 2.f - statGap * 2.f) / 3.f;
    totalRevenueCard.setSize({statWidth, statHeight});
    movieRevenueCard.setSize({statWidth, statHeight});
    comboRevenueCard.setSize({statWidth, statHeight});

    totalRevenueCard.setPosition({position.x + margin, statsY});
    movieRevenueCard.setPosition({totalRevenueCard.getPosition().x + statWidth + statGap, statsY});
    comboRevenueCard.setPosition({movieRevenueCard.getPosition().x + statWidth + statGap, statsY});

    const float controlY = statsY + statHeight + 18.f;
    const float dropdownHeight = 38.f;
    const float yearWidth = 100.f;
    const float monthWidth = 85.f;
    const float dayWidth = 85.f;
    const float fieldGap = 8.f;
    const float labelGap = 12.f;

    // Place labels inline with dropdowns to reduce crowding
    const FloatRect fromLabelBounds = fromLabel.getLocalBounds();
    const float labelInlineY = controlY + (dropdownHeight - fromLabelBounds.size.y) * 0.5f - fromLabelBounds.position.y;
    fromLabel.setPosition({position.x + margin, labelInlineY});
    const float fromFieldsX = fromLabel.getPosition().x + fromLabelBounds.position.x + fromLabelBounds.size.x + labelGap;

    const FloatRect toLabelBounds = toLabel.getLocalBounds();
    toLabel.setPosition({fromFieldsX + yearWidth + monthWidth + dayWidth + fieldGap * 2.f + 24.f, labelInlineY});
    const float toFieldsX = toLabel.getPosition().x + toLabelBounds.position.x + toLabelBounds.size.x + labelGap;

    if (fromYearDropdown) fromYearDropdown->setPosition(sf::Vector2f(fromFieldsX, controlY));
    if (fromMonthDropdown) fromMonthDropdown->setPosition(sf::Vector2f(fromFieldsX + yearWidth + fieldGap, controlY));
    if (fromDayDropdown) fromDayDropdown->setPosition(sf::Vector2f(fromFieldsX + yearWidth + monthWidth + fieldGap * 2.f, controlY));

    if (toYearDropdown) toYearDropdown->setPosition(sf::Vector2f(toFieldsX, controlY));
    if (toMonthDropdown) toMonthDropdown->setPosition(sf::Vector2f(toFieldsX + yearWidth + fieldGap, controlY));
    if (toDayDropdown) toDayDropdown->setPosition(sf::Vector2f(toFieldsX + yearWidth + monthWidth + fieldGap * 2.f, controlY));

    const float rowY = controlY + dropdownHeight + 12.f;
    const float toggleStartX = position.x + margin;
    const float toggleGap = 10.f;
    const float loadGap = 18.f;

    // Load button sits outside the 4 toggles, aligned to the right
    const FloatRect buttonBounds = loadButton.getGlobalBounds();
    const float buttonX = position.x + width - margin - buttonBounds.size.x;
    loadButton.setPosition(sf::Vector2f(buttonX, rowY));

    // 4 toggles share one row with equal width; expand to fill remaining space
    const float toggleRowRight = buttonX - loadGap;
    const float toggleRowWidth = std::max(0.f, toggleRowRight - toggleStartX);
    const float toggleWidth = std::max(150.f, (toggleRowWidth - toggleGap * 3.f) / 4.f);

    for (auto* toggle : {&ticketsToggle, &revenueToggle, &movieOnlyToggle, &comboToggle}) {
        toggle->shape.setSize(sf::Vector2f(toggleWidth, dropdownHeight));
    }

    ticketsToggle.shape.setPosition(sf::Vector2f(toggleStartX, rowY));
    revenueToggle.shape.setPosition(sf::Vector2f(toggleStartX + toggleWidth + toggleGap, rowY));
    movieOnlyToggle.shape.setPosition(sf::Vector2f(toggleStartX + (toggleWidth + toggleGap) * 2.f, rowY));
    comboToggle.shape.setPosition(sf::Vector2f(toggleStartX + (toggleWidth + toggleGap) * 3.f, rowY));

    const float chartTop = rowY + dropdownHeight + 16.f;
    chartCard.setSize(sf::Vector2f(width - margin * 2.f, std::max(260.f, height - (chartTop + 14.f))));
    chartCard.setPosition(sf::Vector2f(position.x + margin, chartTop));

    // Re-center toggle labels after moving
    setChartMode(currentChartMode);
}

bool MovieRevenuePanel::isAnyDropdownOpen() const {
    for (const auto* dropdown : {fromYearDropdown.get(), fromMonthDropdown.get(), fromDayDropdown.get(),
                                 toYearDropdown.get(), toMonthDropdown.get(), toDayDropdown.get()}) {
        if (dropdown && dropdown->isDropdownOpen()) {
            return true;
        }
    }
    return false;
}

void MovieRevenuePanel::loadData() {
    loadMovies();
    loadShowtimePrices();
    loadComboPrices();
    loadTickets();
    populateDropdowns();
    applySelection();
}

std::string MovieRevenuePanel::currentRangeLabel() const {
    if (selectedStartKey <= 0 || selectedEndKey <= 0) {
        return "";
    }
    return keyToDisplay(selectedStartKey) + " - " + keyToDisplay(selectedEndKey);
}

void MovieRevenuePanel::loadMovies() {
    knownMovieTitles.clear();

    auto toUtf8 = [](const sf::String& value) {
        const sf::U8String u8 = value.toUtf8();
        std::string out;
        out.reserve(u8.size());
        for (auto ch : u8) {
            out.push_back(static_cast<char>(ch));
        }
        return out;
    };

    MovieRepository repo(toSfString(moviesFilePath));
    const auto& movies = repo.getAllMovies();
    for (int i = 0; i < movies.getSize(); ++i) {
        const std::string title = trim(toUtf8(movies[i].title));
        if (!title.empty()) {
            knownMovieTitles.insert(title);
        }
    }
}

void MovieRevenuePanel::loadTickets() {
    ticketTree.clear();
    availableDateKeys.clear();

    AdminTicketRepository repo(ticketsFilePath);
    DLL<Ticket> tickets = repo.loadAll();

    Node<Ticket>* node = tickets.getHead();
    while (node) {
        long long key = dateToKey(node->data.bookedDate);
        if (key > 0) {
            ticketTree.insert(key, node->data);
            availableDateKeys.push_back(key);
        }
        node = node->next;
    }

    std::sort(availableDateKeys.begin(), availableDateKeys.end());
    availableDateKeys.erase(std::unique(availableDateKeys.begin(), availableDateKeys.end()), availableDateKeys.end());
}

void MovieRevenuePanel::loadShowtimePrices() {
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
    // Also load history so older tickets can resolve seat price
    consume(resolveDataPath("data/showtimes_history.txt"));
}

void MovieRevenuePanel::loadComboPrices() {
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
        // Support both name-based (legacy) and id-based (Direction B) lookups
        comboPrices[toUtf8(node->data.name)] = node->data.price;
        comboPrices[toUtf8(node->data.id)] = node->data.price;
        node = node->next;
    }
}

void MovieRevenuePanel::populateDropdowns() {
    if (!fromYearDropdown || !toYearDropdown || !fromMonthDropdown || !toMonthDropdown || !fromDayDropdown || !toDayDropdown) {
        return;
    }

    populateYears(*fromYearDropdown);
    populateYears(*toYearDropdown);

    populateMonths(*fromMonthDropdown, *fromYearDropdown);
    populateMonths(*toMonthDropdown, *toYearDropdown);
    populateDays(*fromDayDropdown, *fromYearDropdown, *fromMonthDropdown);
    populateDays(*toDayDropdown, *toYearDropdown, *toMonthDropdown);

    if (!availableDateKeys.empty()) {
        const auto applyKey = [&](long long key,
                                  DropdownBox& yearDropdown,
                                  DropdownBox& monthDropdown,
                                  DropdownBox& dayDropdown) {
            if (key <= 0) return;
            const int year = static_cast<int>(key / 10000LL);
            const int month = static_cast<int>((key / 100LL) % 100LL);
            const int day = static_cast<int>(key % 100LL);

            const std::string yearStr = std::to_string(year);
            char monthBuffer[4];
            std::snprintf(monthBuffer, sizeof(monthBuffer), "%02d", month);
            char dayBuffer[4];
            std::snprintf(dayBuffer, sizeof(dayBuffer), "%02d", day);

            yearDropdown.setSelectedValue(yearStr);
            populateMonths(monthDropdown, yearDropdown);
            monthDropdown.setSelectedValue(monthBuffer);
            populateDays(dayDropdown, yearDropdown, monthDropdown);
            dayDropdown.setSelectedValue(dayBuffer);
        };

        applyKey(availableDateKeys.front(), *fromYearDropdown, *fromMonthDropdown, *fromDayDropdown);
        applyKey(availableDateKeys.back(), *toYearDropdown, *toMonthDropdown, *toDayDropdown);
    }

    updateDropdownEnabling();
}

void MovieRevenuePanel::populateMonths(DropdownBox& dropdown, const DropdownBox& yearDropdown) const {
    (void)yearDropdown;
    std::vector<std::string> options;
    options.reserve(12);
    for (int month = 1; month <= 12; ++month) {
        char buffer[4];
        std::snprintf(buffer, sizeof(buffer), "%02d", month);
        options.emplace_back(buffer);
    }
    dropdown.setOptions(options);
    if (!options.empty()) {
        dropdown.setSelectedIndex(0);
    }
}

void MovieRevenuePanel::populateDays(DropdownBox& dropdown, const DropdownBox& yearDropdown, const DropdownBox& monthDropdown) const {
    const std::string selectedYear = yearDropdown.getSelectedValue();
    const std::string selectedMonth = monthDropdown.getSelectedValue();
    std::vector<std::string> options;

    int year = 0;
    int month = 0;
    try {
        year = std::stoi(selectedYear);
        month = std::stoi(selectedMonth);
    } catch (...) {
        year = 0;
        month = 0;
    }

    const int totalDays = (year > 0 && month > 0) ? daysInMonth(month, year) : 31;
    options.reserve(static_cast<std::size_t>(std::max(28, totalDays)));
    for (int day = 1; day <= totalDays; ++day) {
        char buffer[4];
        std::snprintf(buffer, sizeof(buffer), "%02d", day);
        options.emplace_back(buffer);
    }

    dropdown.setOptions(options);
    if (!options.empty()) {
        dropdown.setSelectedIndex(0);
    }
}

void MovieRevenuePanel::updateDropdownEnabling() {
    // Placeholders removed: dropdowns always contain numeric options.
    // Keep them enabled to allow direct selection.
    if (fromYearDropdown) fromYearDropdown->setEnabled(true);
    if (fromMonthDropdown) fromMonthDropdown->setEnabled(true);
    if (fromDayDropdown) fromDayDropdown->setEnabled(true);
    if (toYearDropdown) toYearDropdown->setEnabled(true);
    if (toMonthDropdown) toMonthDropdown->setEnabled(true);
    if (toDayDropdown) toDayDropdown->setEnabled(true);
}

void MovieRevenuePanel::populateYears(DropdownBox& dropdown) const {
    std::vector<std::string> options;
    std::unordered_set<int> uniqueYears;
    for (long long key : availableDateKeys) {
        uniqueYears.insert(static_cast<int>(key / 10000LL));
    }

    std::vector<int> years(uniqueYears.begin(), uniqueYears.end());
    std::sort(years.begin(), years.end());
    for (int year : years) {
        options.push_back(std::to_string(year));
    }

    if (options.empty()) {
        std::time_t now = std::time(nullptr);
        if (const std::tm* local = std::localtime(&now)) {
            options.push_back(std::to_string(local->tm_year + 1900));
        }
    }

    dropdown.setOptions(options);
    if (!options.empty()) {
        dropdown.setSelectedIndex(0);
    }
}

bool MovieRevenuePanel::isLeapYear(int year) const {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

int MovieRevenuePanel::daysInMonth(int month, int year) const {
    switch (month) {
        case 1: case 3: case 5: case 7: case 8: case 10: case 12: return 31;
        case 4: case 6: case 9: case 11: return 30;
        case 2: return isLeapYear(year) ? 29 : 28;
        default: return 30;
    }
}

long long MovieRevenuePanel::dateToKey(const std::string& displayDate) const {
    if (displayDate.size() != 10) return 0;
    try {
        int day = std::stoi(displayDate.substr(0, 2));
        int month = std::stoi(displayDate.substr(3, 2));
        int year = std::stoi(displayDate.substr(6, 4));
        return static_cast<long long>(year) * 10000LL + static_cast<long long>(month) * 100LL + day;
    } catch (...) {
        return 0;
    }
}

std::string MovieRevenuePanel::keyToDisplay(long long key) const {
    int year = static_cast<int>(key / 10000LL);
    int month = static_cast<int>((key / 100LL) % 100LL);
    int day = static_cast<int>(key % 100LL);
    char buffer[16];
    std::snprintf(buffer, sizeof(buffer), "%02d/%02d/%04d", day, month, year);
    return std::string(buffer);
}

void MovieRevenuePanel::applySelection() {
    if (!fromYearDropdown || !toYearDropdown) {
        return;
    }

    if (!fromMonthDropdown || !toMonthDropdown || !fromDayDropdown || !toDayDropdown) {
        return;
    }

    auto selectedInt = [](const DropdownBox& dropdown) -> int {
        try {
            return std::stoi(dropdown.getSelectedValue());
        } catch (...) {
            return 0;
        }
    };

    const int fromYear = selectedInt(*fromYearDropdown);
    const int fromMonth = selectedInt(*fromMonthDropdown);
    const int fromDay = selectedInt(*fromDayDropdown);

    const int toYear = selectedInt(*toYearDropdown);
    const int toMonth = selectedInt(*toMonthDropdown);
    const int toDay = selectedInt(*toDayDropdown);

    if (fromYear == 0 || toYear == 0 || fromMonth == 0 || toMonth == 0 || fromDay == 0 || toDay == 0) {
        filteredTickets.clear();
        movieStats.clear();
        return;
    }

    long long startKey = static_cast<long long>(fromYear) * 10000LL + static_cast<long long>(fromMonth) * 100LL + fromDay;
    long long endKey = static_cast<long long>(toYear) * 10000LL + static_cast<long long>(toMonth) * 100LL + toDay;
    if (startKey == 0 || endKey == 0) {
        return;
    }
    if (startKey > endKey) std::swap(startKey, endKey);

    selectedStartKey = startKey;
    selectedEndKey = endKey;

    filteredTickets = ticketTree.rangeQuery(startKey, endKey);
    updateMovieStats();
    updateComboStats();
    updateSummaryCards();

    // Keep sorting deterministic; combo stats always sorted by revenue desc.
    sortComboStats();
    if (currentChartMode == ChartMode::ComboRevenue) {
        // already sorted
    } else {
        sortStatsForCurrentMode();
    }
}

void MovieRevenuePanel::updateSummaryCards() {
    totalRevenueValue = 0;
    movieRevenueValue = 0;
    comboRevenueValue = 0;

    Node<Ticket>* node = filteredTickets.getHead();
    while (node) {
        const Ticket& ticket = node->data;
        const int seats = countSeats(ticket.booked);
        const long long ticketTotal = static_cast<long long>(ticket.price);
        const long long rawComboRev = computeComboRevenue(ticket.comboName);
        const long long comboRev = std::max(0LL, std::min(rawComboRev, ticketTotal));
        const long long movieRev = computeTicketRevenue(ticket, seats, comboRev);

        totalRevenueValue += ticketTotal;
        movieRevenueValue += movieRev;
        comboRevenueValue += comboRev;
        node = node->next;
    }

    const std::string range = currentRangeLabel();
    const std::string dateLabel = range.empty() ? "" : range;

    totalRevenueCard.setTitleWithDate("Tổng doanh thu", dateLabel);
    totalRevenueCard.setValue(formatCurrency(totalRevenueValue));

    movieRevenueCard.setTitleWithDate("Doanh thu phim", dateLabel);
    movieRevenueCard.setValue(formatCurrency(movieRevenueValue));

    comboRevenueCard.setTitleWithDate("Doanh thu combo", dateLabel);
    comboRevenueCard.setValue(formatCurrency(comboRevenueValue));
}

void MovieRevenuePanel::updateComboStats() {
    comboStats.clear();

    std::unordered_map<std::string, std::size_t> index;

    auto addCombo = [&](const std::string& label, int quantity) {
        if (label.empty() || quantity <= 0) return;
        auto priceIt = comboPrices.find(label);
        if (priceIt == comboPrices.end()) return;
        const long long revenue = static_cast<long long>(priceIt->second) * static_cast<long long>(quantity);

        auto it = index.find(label);
        if (it == index.end()) {
            ComboRevenueEntry entry;
            entry.label = label;
            entry.quantity = quantity;
            entry.revenue = revenue;
            comboStats.push_back(entry);
            index[label] = comboStats.size() - 1;
        } else {
            ComboRevenueEntry& entry = comboStats[it->second];
            entry.quantity += quantity;
            entry.revenue += revenue;
        }
    };

    Node<Ticket>* node = filteredTickets.getHead();
    while (node) {
        std::string comboList = trim(node->data.comboName);
        if (comboList.empty() || comboList == "Không có") {
            node = node->next;
            continue;
        }

        std::size_t start = 0;
        while (start < comboList.size()) {
            std::size_t end = comboList.find(',', start);
            std::string item = (end == std::string::npos)
                ? comboList.substr(start)
                : comboList.substr(start, end - start);
            item = trim(item);

            if (!item.empty()) {
                int quantity = 1;

                // Direction B: "CBxx:xN"
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

                addCombo(item, quantity);
            }

            if (end == std::string::npos) break;
            start = end + 1;
        }

        node = node->next;
    }

    // Deterministic ordering across reloads
    sortComboStats();
}

void MovieRevenuePanel::updateMovieStats() {
    movieStats.clear();
    std::unordered_map<std::string, std::size_t> indexByTitle;

    Node<Ticket>* node = filteredTickets.getHead();
    while (node) {
        const Ticket& ticket = node->data;
        const std::string normalizedTitle = trim(ticket.title);
        if (knownMovieTitles.find(normalizedTitle) == knownMovieTitles.end()) {
            node = node->next;
            continue;
        }
        int seats = countSeats(ticket.booked);
        const long long ticketTotal = static_cast<long long>(ticket.price);
        const long long rawComboRevenue = computeComboRevenue(ticket.comboName);
        const long long comboRevenue = std::max(0LL, std::min(rawComboRevenue, ticketTotal));
        const long long ticketRevenue = computeTicketRevenue(ticket, seats, comboRevenue);

        auto it = indexByTitle.find(normalizedTitle);
        if (it == indexByTitle.end()) {
            MovieRevenueEntry entry;
            entry.title = normalizedTitle;
            entry.ticketCount = seats;
            entry.comboRevenue = comboRevenue;
            entry.ticketRevenue = ticketRevenue;
            movieStats.push_back(entry);
            indexByTitle[normalizedTitle] = movieStats.size() - 1;
        } else {
            MovieRevenueEntry& entry = movieStats[it->second];
            entry.ticketCount += seats;
            entry.comboRevenue += comboRevenue;
            entry.ticketRevenue += ticketRevenue;
        }
        node = node->next;
    }
}

int MovieRevenuePanel::countSeats(const std::string& seatList) const {
    if (seatList.empty()) return 0;
    int count = 0;
    std::size_t start = 0;
    while (start < seatList.size()) {
        std::size_t end = seatList.find(',', start);
        std::string token = (end == std::string::npos)
            ? seatList.substr(start)
            : seatList.substr(start, end - start);
        if (!trim(token).empty()) {
            count++;
        }
        if (end == std::string::npos) break;
        start = end + 1;
    }
    return count;
}

std::string MovieRevenuePanel::trim(const std::string& text) const {
    std::size_t first = text.find_first_not_of(" \t\n\r");
    if (first == std::string::npos) return "";
    std::size_t last = text.find_last_not_of(" \t\n\r");
    return text.substr(first, last - first + 1);
}

std::string MovieRevenuePanel::ellipsize(const std::string& text, std::size_t maxChars) const {
    if (text.size() <= maxChars) {
        return text;
    }
    if (maxChars <= 3) {
        return text.substr(0, maxChars);
    }
    return text.substr(0, maxChars - 3) + "...";
}

long long MovieRevenuePanel::computeComboRevenue(const std::string& comboList) const {
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

long long MovieRevenuePanel::computeTicketRevenue(const Ticket& ticket, int seatCount, long long comboRevenue) const {
    // Split must be consistent with total revenue source-of-truth: Ticket.price.
    // Movie revenue is always (ticketTotal - comboPart) so that movie + combo == total.
    if (seatCount <= 0) return 0;

    const long long ticketTotal = static_cast<long long>(ticket.price);
    const long long safeCombo = std::max(0LL, std::min(comboRevenue, ticketTotal));
    const long long moviePart = ticketTotal - safeCombo;
    return std::max(0LL, moviePart);
}

void MovieRevenuePanel::setChartMode(ChartMode mode) {
    currentChartMode = mode;

    auto activate = [](ModeToggle& toggle, bool active) {
        toggle.active = active;
        if (active) {
            toggle.shape.setFillColor(Color(59, 130, 246));
            toggle.label.setFillColor(Color::White);
        } else {
            toggle.shape.setFillColor(Color(229, 231, 235));
            toggle.label.setFillColor(Color(71, 85, 105));
        }
    };

    activate(ticketsToggle, mode == ChartMode::Tickets);
    activate(revenueToggle, mode == ChartMode::Revenue);
    activate(movieOnlyToggle, mode == ChartMode::MovieRevenue);
    activate(comboToggle, mode == ChartMode::ComboRevenue);

    auto centerLabel = [](ModeToggle& toggle) {
        FloatRect bounds = toggle.label.getLocalBounds();
        toggle.label.setPosition(sf::Vector2f(
            toggle.shape.getPosition().x + (toggle.shape.getSize().x - bounds.size.x) / 2.f - bounds.position.x,
            toggle.shape.getPosition().y + (toggle.shape.getSize().y - bounds.size.y) / 2.f - bounds.position.y
        ));
    };

    centerLabel(ticketsToggle);
    centerLabel(revenueToggle);
    centerLabel(movieOnlyToggle);
    centerLabel(comboToggle);

    if (currentChartMode == ChartMode::ComboRevenue) {
        sortComboStats();
    } else {
        sortStatsForCurrentMode();
    }
}

void MovieRevenuePanel::sortComboStats() {
    if (comboStats.empty()) return;
    std::stable_sort(comboStats.begin(), comboStats.end(), [](const ComboRevenueEntry& a, const ComboRevenueEntry& b) {
        if (a.revenue == b.revenue) {
            if (a.quantity == b.quantity) {
                return a.label < b.label;
            }
            return a.quantity > b.quantity;
        }
        return a.revenue > b.revenue;
    });
}

void MovieRevenuePanel::sortStatsForCurrentMode() {
    if (movieStats.empty()) {
        return;
    }

    if (currentChartMode == ChartMode::MovieRevenue) {
        std::stable_sort(movieStats.begin(), movieStats.end(), [](const MovieRevenueEntry& a, const MovieRevenueEntry& b) {
            if (a.ticketRevenue == b.ticketRevenue) {
                if (a.ticketCount == b.ticketCount) {
                    return a.title < b.title;
                }
                return a.ticketCount > b.ticketCount;
            }
            return a.ticketRevenue > b.ticketRevenue;
        });
        return;
    }

    if (currentChartMode == ChartMode::Revenue) {
        std::stable_sort(movieStats.begin(), movieStats.end(), [](const MovieRevenueEntry& a, const MovieRevenueEntry& b) {
            const long long aValue = a.totalRevenue();
            const long long bValue = b.totalRevenue();
            if (aValue == bValue) {
                if (a.ticketCount == b.ticketCount) {
                    return a.title < b.title;
                }
                return a.ticketCount > b.ticketCount;
            }
            return aValue > bValue;
        });
    } else {
        std::stable_sort(movieStats.begin(), movieStats.end(), [](const MovieRevenueEntry& a, const MovieRevenueEntry& b) {
            if (a.ticketCount == b.ticketCount) {
                const long long aValue = a.totalRevenue();
                const long long bValue = b.totalRevenue();
                if (aValue == bValue) {
                    return a.title < b.title;
                }
                return aValue > bValue;
            }
            return a.ticketCount > b.ticketCount;
        });
    }
}

void MovieRevenuePanel::drawChart(RenderTarget& target) const {
    target.draw(chartCard);

    const std::string headingText = (currentChartMode == ChartMode::ComboRevenue)
        ? "Doanh thu combo"
        : (currentChartMode == ChartMode::MovieRevenue)
            ? "Doanh thu phim"
            : (currentChartMode == ChartMode::Revenue)
                ? "Tổng doanh thu theo phim"
                : "Số vé bán ra theo phim";
    sf::Text heading(font, toSfString(headingText), 18);
    heading.setFillColor(Color(27, 38, 59));
    heading.setStyle(sf::Text::Bold);
    heading.setPosition(sf::Vector2f(chartCard.getPosition().x + 24.f, chartCard.getPosition().y + 14.f));
    target.draw(heading);

    if ((currentChartMode == ChartMode::ComboRevenue && comboStats.empty()) ||
        (currentChartMode != ChartMode::ComboRevenue && movieStats.empty())) {
        sf::Text empty(font, toSfString("Không có dữ liệu"), 14);
        empty.setFillColor(Color(120, 130, 140));
        empty.setPosition(sf::Vector2f(chartCard.getPosition().x + 24.f, chartCard.getPosition().y + 70.f));
        target.draw(empty);
        return;
    }

    const std::size_t barCount = (currentChartMode == ChartMode::ComboRevenue)
        ? std::min<std::size_t>(comboStats.size(), 12)
        : std::min<std::size_t>(movieStats.size(), 12);
    
    // Tính maxValue động từ dữ liệu thực
    double actualMaxValue = 0.0;
    for (std::size_t i = 0; i < barCount; ++i) {
        double value = 0.0;
        if (currentChartMode == ChartMode::ComboRevenue) {
            value = static_cast<double>(comboStats[i].revenue);
        } else if (currentChartMode == ChartMode::Revenue) {
            value = static_cast<double>(movieStats[i].totalRevenue());
        } else if (currentChartMode == ChartMode::MovieRevenue) {
            value = static_cast<double>(movieStats[i].ticketRevenue);
        } else {
            value = static_cast<double>(movieStats[i].ticketCount);
        }
        if (value > actualMaxValue) {
            actualMaxValue = value;
        }
    }
    
    // Làm tròn maxValue thành số đẹp
    double maxValue = static_cast<double>(roundUpNice(static_cast<long long>(actualMaxValue)));
    if (maxValue <= 0.0) {
        const bool isRevenue = (currentChartMode == ChartMode::Revenue || currentChartMode == ChartMode::MovieRevenue || currentChartMode == ChartMode::ComboRevenue);
        maxValue = isRevenue ? 1000000.0 : 100.0;
    }

    const bool isRevenueMode = (currentChartMode == ChartMode::Revenue || currentChartMode == ChartMode::MovieRevenue || currentChartMode == ChartMode::ComboRevenue);
    const float paddingLeft = isRevenueMode ? 100.f : 60.f;
    const float paddingRight = 30.f;
    const float paddingTop = 50.f;
    const float paddingBottom = 80.f;
    const float chartWidth = std::max(60.f, chartCard.getSize().x - paddingLeft - paddingRight);
    const float chartHeight = std::max(80.f, chartCard.getSize().y - paddingTop - paddingBottom);
    const float originX = chartCard.getPosition().x + paddingLeft;
    const float originY = chartCard.getPosition().y + chartCard.getSize().y - paddingBottom;

    const int gridLines = 5;
    for (int i = 0; i <= gridLines; ++i) {
        const float ratio = static_cast<float>(i) / static_cast<float>(gridLines);
        const float y = originY - ratio * chartHeight;

        sf::RectangleShape line({chartWidth, 1.f});
        line.setFillColor(Color(229, 233, 240));
        line.setPosition(sf::Vector2f(originX, y));
        target.draw(line);

        const double rawValue = maxValue * ratio;
        std::string labelStr;
        if (isRevenueMode) {
            long long value = static_cast<long long>(std::round(rawValue));
            labelStr = formatCurrency(value);
        } else {
            labelStr = std::to_string(static_cast<int>(std::round(rawValue)));
        }

        sf::Text label(font, toSfString(labelStr), 12);
        label.setFillColor(Color(120, 130, 140));
        FloatRect bounds = label.getLocalBounds();
        label.setPosition(sf::Vector2f(
            originX - bounds.size.x - 8.f - bounds.position.x,
            y - bounds.size.y / 2.f - bounds.position.y
        ));
        target.draw(label);
    }

    sf::RectangleShape axis({chartWidth, 2.f});
    axis.setFillColor(Color(203, 213, 225));
    axis.setPosition(sf::Vector2f(originX, originY));
    target.draw(axis);

    // Use wider bars to avoid too much empty space
    float gap = 12.f;
    float barWidth = (chartWidth - gap * (static_cast<float>(barCount) + 1.f)) / static_cast<float>(barCount);
    if (barWidth < 28.f) {
        gap = 8.f;
        barWidth = (chartWidth - gap * (static_cast<float>(barCount) + 1.f)) / static_cast<float>(barCount);
    }
    if (barWidth > 120.f) barWidth = 120.f;
    if (barWidth < 24.f) barWidth = 24.f;

    float totalBarsWidth = barCount * barWidth + (barCount + 1) * gap;
    float startX = originX + (chartWidth - totalBarsWidth) / 2.f + gap;

    for (std::size_t i = 0; i < barCount; ++i) {
        const float x = startX + static_cast<float>(i) * (barWidth + gap);
        double rawValue = 0.0;
        if (currentChartMode == ChartMode::ComboRevenue) {
            rawValue = static_cast<double>(comboStats[i].revenue);
        } else if (currentChartMode == ChartMode::Revenue) {
            rawValue = static_cast<double>(movieStats[i].totalRevenue());
        } else if (currentChartMode == ChartMode::MovieRevenue) {
            rawValue = static_cast<double>(movieStats[i].ticketRevenue);
        } else {
            rawValue = static_cast<double>(movieStats[i].ticketCount);
        }
        float barHeight = static_cast<float>((rawValue / maxValue) * chartHeight);
        if (barHeight < 4.f) {
            barHeight = 4.f;
        }

        sf::RectangleShape bar({barWidth, barHeight});
        if (currentChartMode == ChartMode::ComboRevenue) {
            bar.setFillColor(Color(221, 180, 67));
        } else if (currentChartMode == ChartMode::MovieRevenue) {
            bar.setFillColor(Color(13, 148, 136));
        } else if (currentChartMode == ChartMode::Revenue) {
            bar.setFillColor(Color(236, 72, 153));
        } else {
            bar.setFillColor(Color(59, 130, 246));
        }
        bar.setPosition(sf::Vector2f(x, originY - barHeight));
        target.draw(bar);

        // Giá trị nằm bên trên cột (dễ đọc hơn)
        std::string valueStr;
        if (isRevenueMode) {
            long long val = static_cast<long long>(std::round(rawValue));
            valueStr = formatCurrency(val);
        } else {
            valueStr = std::to_string(static_cast<int>(std::round(rawValue)));
        }

        sf::Text valueText(font, toSfString(valueStr), 12);
        valueText.setStyle(sf::Text::Bold);
        FloatRect valueBounds = valueText.getLocalBounds();

        float valueY = originY - barHeight - valueBounds.size.y - 6.f - valueBounds.position.y;
        valueText.setFillColor(Color(27, 38, 59));
        const float minY = chartCard.getPosition().y + paddingTop;
        if (valueY < minY) {
            // If there is no space above, place inside the bar
            valueY = originY - barHeight + 6.f - valueBounds.position.y;
            valueText.setFillColor(Color::White);
        }

        valueText.setPosition(sf::Vector2f(
            x + (barWidth - valueBounds.size.x) / 2.f - valueBounds.position.x,
            valueY
        ));
        target.draw(valueText);

        // Nhãn phim (giống Tổng quan: ngang, tên dài -> "...")
        const int maxChars = std::max(6, static_cast<int>(std::floor(barWidth / 7.f)));
        const std::string labelText = (currentChartMode == ChartMode::ComboRevenue)
            ? ellipsize(comboStats[i].label, static_cast<std::size_t>(maxChars))
            : ellipsize(movieStats[i].title, static_cast<std::size_t>(maxChars));
        sf::Text label(font, toSfString(labelText), 12);
        label.setFillColor(Color(94, 106, 123));
        FloatRect labelBounds = label.getLocalBounds();
        label.setOrigin(sf::Vector2f(
            labelBounds.position.x + labelBounds.size.x * 0.5f,
            labelBounds.position.y
        ));
        label.setPosition(sf::Vector2f(x + barWidth * 0.5f, originY + 10.f));
        target.draw(label);
    }
}

void MovieRevenuePanel::setPosition(const sf::Vector2f& pos) {
    position = pos;
    layoutComponents();
}

void MovieRevenuePanel::handleEvent(const Event& event, const RenderWindow& window) {
    const sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));

    const bool wasDropdownOpen = isAnyDropdownOpen();

    auto handleDateGroup = [&](std::unique_ptr<DropdownBox>& year,
                               std::unique_ptr<DropdownBox>& month,
                               std::unique_ptr<DropdownBox>& day) {
        if (!year || !month || !day) {
            return;
        }

        const int prevYearIdx = year->getSelectedIndex();
        year->handleEvent(event, mousePos);
        if (year->getSelectedIndex() != prevYearIdx) {
            populateMonths(*month, *year);
            populateDays(*day, *year, *month);
        }

        const int prevMonthIdx = month->getSelectedIndex();
        month->handleEvent(event, mousePos);
        if (month->getSelectedIndex() != prevMonthIdx) {
            populateDays(*day, *year, *month);
        }

        day->handleEvent(event, mousePos);
    };

    handleDateGroup(fromYearDropdown, fromMonthDropdown, fromDayDropdown);
    handleDateGroup(toYearDropdown, toMonthDropdown, toDayDropdown);
    updateDropdownEnabling();

    // If a dropdown was open at the start of this event, prevent click-through to other controls.
    if (wasDropdownOpen) {
        if (const auto* mousePressed = event.getIf<Event::MouseButtonPressed>()) {
            if (mousePressed->button == sf::Mouse::Button::Left) {
                suppressClicksUntilMouseRelease = true;
            }
        }
        return;
    }

    if (const auto* mousePressed = event.getIf<Event::MouseButtonPressed>()) {
        if (mousePressed->button == sf::Mouse::Button::Left) {
            if (ticketsToggle.shape.getGlobalBounds().contains(mousePos)) {
                setChartMode(ChartMode::Tickets);
            } else if (revenueToggle.shape.getGlobalBounds().contains(mousePos)) {
                setChartMode(ChartMode::Revenue);
            } else if (movieOnlyToggle.shape.getGlobalBounds().contains(mousePos)) {
                setChartMode(ChartMode::MovieRevenue);
            } else if (comboToggle.shape.getGlobalBounds().contains(mousePos)) {
                setChartMode(ChartMode::ComboRevenue);
            }
        }
    }
}

void MovieRevenuePanel::update(const sf::Vector2f& mousePos, bool mouseDown) {
    if (!mouseDown) {
        suppressClicksUntilMouseRelease = false;
    }

    const bool blockClicks = suppressClicksUntilMouseRelease || isAnyDropdownOpen();

    const sf::Color primary = Color(59, 130, 246);
    const sf::Color hover = Color(37, 99, 235);
    const sf::Color active = Color(29, 78, 216);

    loadButton.update(mousePos, mouseDown, hover, active);
    if (!blockClicks && loadButton.isClicked(mousePos, mouseDown)) {
        // Reload from source-of-truth then apply selected date range
        loadMovies();
        loadShowtimePrices();
        loadComboPrices();
        loadTickets();
        applySelection();
    }

    auto updateToggle = [&](ModeToggle& toggle, bool isActive) {
        const bool hovered = toggle.shape.getGlobalBounds().contains(mousePos);
        if (isActive) {
            toggle.shape.setFillColor(primary);
            toggle.label.setFillColor(Color::White);
        } else {
            toggle.shape.setFillColor(hovered ? Color(241, 245, 249) : Color(229, 231, 235));
            toggle.label.setFillColor(Color(71, 85, 105));
        }
    };

    updateToggle(ticketsToggle, currentChartMode == ChartMode::Tickets);
    updateToggle(revenueToggle, currentChartMode == ChartMode::Revenue);
    updateToggle(movieOnlyToggle, currentChartMode == ChartMode::MovieRevenue);
    updateToggle(comboToggle, currentChartMode == ChartMode::ComboRevenue);

    if (fromYearDropdown) fromYearDropdown->update();
    if (fromMonthDropdown) fromMonthDropdown->update();
    if (fromDayDropdown) fromDayDropdown->update();
    if (toYearDropdown) toYearDropdown->update();
    if (toMonthDropdown) toMonthDropdown->update();
    if (toDayDropdown) toDayDropdown->update();
}

void MovieRevenuePanel::render(RenderTarget& target) const {
    target.draw(background);
    target.draw(titleText);

    totalRevenueCard.render(target);
    movieRevenueCard.render(target);
    comboRevenueCard.render(target);

    target.draw(fromLabel);
    target.draw(toLabel);

    target.draw(ticketsToggle.shape);
    target.draw(ticketsToggle.label);
    target.draw(revenueToggle.shape);
    target.draw(revenueToggle.label);
    target.draw(movieOnlyToggle.shape);
    target.draw(movieOnlyToggle.label);
    target.draw(comboToggle.shape);
    target.draw(comboToggle.label);

    loadButton.draw(target);
    drawChart(target);

    if (auto* window = dynamic_cast<RenderWindow*>(&target)) {
        if (fromYearDropdown) fromYearDropdown->draw(*window);
        if (fromMonthDropdown) fromMonthDropdown->draw(*window);
        if (fromDayDropdown) fromDayDropdown->draw(*window);
        if (toYearDropdown) toYearDropdown->draw(*window);
        if (toMonthDropdown) toMonthDropdown->draw(*window);
        if (toDayDropdown) toDayDropdown->draw(*window);
    }
}

void MovieRevenuePanel::refresh() {
    loadData();
}
