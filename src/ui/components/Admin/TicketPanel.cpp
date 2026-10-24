#include "UI/components/Admin/TicketPanel.h"
#include <algorithm>
#include <array>
#include <fstream>
#include <sstream>
#include "UI/components/Admin/AdminSidebar.h"
#include <vector>
#include <regex>
#include <unordered_map>

using sf::Color;
using sf::Event;
using sf::FloatRect;
using sf::RenderWindow;
using sf::Vector2f;
using std::string;
using std::to_string;

namespace {
const Color BG_COLOR(244, 246, 250);
const Color HEADER_COLOR(20, 118, 172);
const Color ROW_COLOR(255, 255, 255);
const Color HOVER_COLOR(229, 241, 251);
const Color SELECTED_COLOR(204, 228, 247);
const Color BORDER_COLOR(201, 206, 214);
const Color TEXT_COLOR(34, 34, 34);
}

sf::String TicketPanel::utf8(const string& text) {
    return sf::String::fromUtf8(text.begin(), text.end());
}

static string formatDateToDDMMYYYY(const string& input) {
    if (input.empty()) return input;
    if (input.find('/') != string::npos) {
        vector<string> parts;
        stringstream ss(input);
        string tok;
        while (getline(ss, tok, '/')) parts.push_back(tok);
        if (parts.size() == 3 && parts[0].size() == 4) {
            return parts[2] + "/" + parts[1] + "/" + parts[0];
        }
        return input;
    }
    regex r(R"((\d{4})-(\d{2})-(\d{2}))");
    smatch m;
    if (regex_match(input, m, r) && m.size() == 4) {
        return m[3].str() + "/" + m[2].str() + "/" + m[1].str();
    }
    return input;
}

static string trimCopy(const string& source) {
    size_t start = source.find_first_not_of(" \t\n\r");
    size_t end = source.find_last_not_of(" \t\n\r");
    if (start == string::npos) return "";
    return source.substr(start, end - start + 1);
}

static long long parseBookedKey(const std::string& bookedDate, const std::string& bookedTime) {
    // bookedDate: dd/mm/yyyy
    // bookedTime: HH:MM:SS
    if (bookedDate.size() < 10 || bookedTime.size() < 8) return 0;
    try {
        int day = std::stoi(bookedDate.substr(0, 2));
        int month = std::stoi(bookedDate.substr(3, 2));
        int year = std::stoi(bookedDate.substr(6, 4));
        int hour = std::stoi(bookedTime.substr(0, 2));
        int minute = std::stoi(bookedTime.substr(3, 2));
        int second = std::stoi(bookedTime.substr(6, 2));

        long long dateKey = static_cast<long long>(year) * 10000LL + static_cast<long long>(month) * 100LL + day;
        long long timeKey = static_cast<long long>(hour) * 10000LL + static_cast<long long>(minute) * 100LL + second;
        return dateKey * 1000000LL + timeKey;
    } catch (...) {
        return 0;
    }
}

static std::unordered_map<std::string, std::string> loadComboIdToName() {
    std::unordered_map<std::string, std::string> out;
    std::ifstream file("../data/combo.txt", std::ios::binary);
    if (!file.is_open()) return out;

    std::string data((std::istreambuf_iterator<char>(file)), {});
    file.close();

    if (data.size() >= 3 &&
        (unsigned char)data[0] == 0xEF &&
        (unsigned char)data[1] == 0xBB &&
        (unsigned char)data[2] == 0xBF) {
        data.erase(0, 3);
    }

    std::stringstream ss(data);
    std::string line;
    std::getline(ss, line); // header
    while (std::getline(ss, line)) {
        if (line.empty()) continue;
        std::stringstream ls(line);
        std::string id, name, price;
        std::getline(ls, id, '|');
        std::getline(ls, name, '|');
        std::getline(ls, price, '|');
        if (!id.empty() && !name.empty()) {
            out[id] = name;
        }
    }
    return out;
}

static string formatComboTokenForAdminDisplay(const string& tokenRaw,
                                              const std::unordered_map<std::string, std::string>& idToName) {
    string token = trimCopy(tokenRaw);
    if (token.empty()) return token;
    if (token == "Không có") return token;

    size_t pos = token.find(":x");
    if (pos != string::npos && pos > 0) {
        string id = token.substr(0, pos);
        string qtyStr = token.substr(pos + 2);
        int qty = 0;
        try {
            qty = std::stoi(qtyStr);
        } catch (...) {
            qty = 0;
        }
        auto it = idToName.find(id);
        if (it != idToName.end()) {
            if (qty > 0) return it->second + " x" + std::to_string(qty);
            return it->second;
        }
    }

    return token;
}

static string formatCurrency(int amount) {
    if (amount <= 0) return "0 đ";
    string digits = to_string(amount);
    string result;
    int counter = 0;
    for (int i = static_cast<int>(digits.size()) - 1; i >= 0; --i) {
        result.push_back(digits[i]);
        ++counter;
        if (counter == 3 && i != 0) {
            result.push_back('.');
            counter = 0;
        }
    }
    reverse(result.begin(), result.end());
    result += " đ";
    return result;
}

static string formatSeatList(const string& seatsRaw) {
    if (seatsRaw.empty()) return "-";
    stringstream ss(seatsRaw);
    string token;
    string result;
    bool first = true;
    while (getline(ss, token, ',')) {
        string seat = trimCopy(token);
        if (seat.empty()) continue;
        if (!first) result += ", ";
        result += seat;
        first = false;
    }
    return result.empty() ? "-" : result;
}

static string formatComboDisplay(const string& combosRaw) {
    string trimmed = trimCopy(combosRaw);
    if (trimmed.empty() || trimmed == "Không có") return "Không có";

    static const std::unordered_map<std::string, std::string> comboIdToName = loadComboIdToName();

    stringstream ss(trimmed);
    string token;
    vector<string> items;
    while (getline(ss, token, ',')) {
        string item = formatComboTokenForAdminDisplay(token, comboIdToName);
        if (!item.empty()) items.push_back(item);
    }
    if (items.empty()) return "Không có";

    string result;
    for (size_t i = 0; i < items.size(); ++i) {
        if (i > 0) {
            result += (i % 2 == 0) ? "\n" : ", ";
        }
        result += items[i];
    }
    return result;
}

static string wrapTextToWidth(const string& input, const Font& font,
                                   unsigned int charSize, float maxWidth) {
    if (input.empty()) return "-";

    stringstream lineStream(input);
    string line;
    string wrapped;
    while (getline(lineStream, line)) {
        string trimmedLine = trimCopy(line);
        if (trimmedLine.empty()) {
            continue;
        }

        stringstream words(trimmedLine);
        string word;
        string currentLine;
        while (words >> word) {
            string candidate = currentLine.empty() ? word : currentLine + " " + word;
            Text measure(font, sf::String::fromUtf8(candidate.begin(), candidate.end()), charSize);
            if (measure.getLocalBounds().size.x > maxWidth && !currentLine.empty()) {
                if (!wrapped.empty()) wrapped += "\n";
                wrapped += currentLine;
                currentLine = word;
            } else {
                currentLine = candidate;
            }
        }

        if (!currentLine.empty()) {
            if (!wrapped.empty()) wrapped += "\n";
            wrapped += currentLine;
        }
    }

    return wrapped.empty() ? "-" : wrapped;
}

TicketPanel::TicketPanel(Font& f, float w, float h)
    : font(f), width(w), height(h), repository("../data/tickets.txt"),
    reloadTexture("../assets/elements/reload.png"),
    reloadSprite(reloadTexture) {
    setupUI();
    refreshTickets();
}

void TicketPanel::setupUI() {
    background.setSize(Vector2f(width, height));
    background.setFillColor(BG_COLOR);

    titleText = make_unique<Text>(font);
    titleText->setCharacterSize(26);
    titleText->setFillColor(Color(27, 38, 59));
    titleText->setStyle(Text::Bold);
    titleText->setString(utf8("Quản lý vé"));

    tableHeaderBg.setSize(Vector2f(TABLE_WIDTH, HEADER_HEIGHT));
    tableHeaderBg.setFillColor(HEADER_COLOR);

    btnView = make_unique<SidebarRoundRectButton>(font, "Xem thông tin", Vector2f(160.f, 44.f), 8.f);
    btnView->setTextSize(16);
    btnView->setColors(Color(142, 169, 204), Color(17, 98, 144));

    btnDelete = make_unique<SidebarRoundRectButton>(font, "Xóa vé", Vector2f(160.f, 44.f), 8.f);
    btnDelete->setTextSize(16);
    btnDelete->setColors(Color(189, 99, 99), Color(171, 36, 36));

    btnReload = make_unique<SidebarRoundRectButton>(font, "", Vector2f(44.f, 44.f), 8.f);
    btnReload->setTextSize(16);
    btnReload->setColors(Color(20, 118, 172), Color(15, 90, 135));

    reloadSprite.setScale({0.1f, 0.1f});
    reloadSprite.setColor(Color::White);

    overlay.setSize(Vector2f(1728.f, 972.f));
    overlay.setFillColor(Color(0, 0, 0, 190));

    detailPanel.setSize(Vector2f(1100.f, 860.f));
    detailPanel.setFillColor(Color(1, 3, 33, 240));
    detailPanel.setOutlineThickness(1.f);
    detailPanel.setOutlineColor(Color::White);

    detailTitle = make_unique<Text>(font);
    detailTitle->setCharacterSize(28);
    detailTitle->setFillColor(Color::White);
    detailTitle->setStyle(Text::Bold);
    detailTitle->setOutlineThickness(1.f);
    detailTitle->setOutlineColor(Color(20, 118, 172));
    detailTitle->setString(utf8("Thông tin vé"));

    detailCloseBg.setSize(Vector2f(260.f, 56.f));
    detailCloseBg.setFillColor(Color(20, 118, 172));
    detailCloseBg.setOutlineThickness(1.f);
    detailCloseBg.setOutlineColor(Color::White);
    detailCloseText = make_unique<Text>(font);
    detailCloseText->setCharacterSize(20);
    detailCloseText->setFillColor(Color::White);
    detailCloseText->setString(utf8("Đã hiểu"));

    notificationBg.setSize(Vector2f(400.f, 60.f));
    notificationBg.setFillColor(Color(211, 47, 47, 230));
    notificationText = make_unique<Text>(font);
    notificationText->setCharacterSize(18);
    notificationText->setFillColor(Color::White);
}

void TicketPanel::setPosition(Vector2f pos) {
    position = pos;
    background.setPosition(pos);
    if (titleText) {
        titleText->setPosition(Vector2f(pos.x + 40.f, pos.y + 20.f));
    }
    tableHeaderBg.setPosition(Vector2f(pos.x + TABLE_X, pos.y + TABLE_Y));

    float buttonRowY = pos.y + TABLE_Y - 64.f;
    Vector2f viewPos(pos.x + TABLE_X, buttonRowY);
    btnView->setPosition(viewPos);

    Vector2f deletePos(viewPos.x + btnView->getSize().x + 18.f, buttonRowY);
    btnDelete->setPosition(deletePos);

    Vector2f reloadPos(pos.x + TABLE_X + TABLE_WIDTH - btnReload->getSize().x, buttonRowY);
    btnReload->setPosition(reloadPos);

    // center reload sprite in reload button
    FloatRect spriteBounds = reloadSprite.getLocalBounds();
    float scaledW = spriteBounds.size.x * reloadSprite.getScale().x;
    float scaledH = spriteBounds.size.y * reloadSprite.getScale().y;
    reloadSprite.setPosition(Vector2f(
        reloadPos.x + (btnReload->getSize().x - scaledW) / 2.f,
        reloadPos.y + (btnReload->getSize().y - scaledH) / 2.f
    ));

    overlay.setPosition(Vector2f(0.f, 0.f));
    detailPanel.setPosition(Vector2f((1728.f - detailPanel.getSize().x) / 2.f,(972.f - detailPanel.getSize().y) / 2.f + 15.f));

    if (detailTitle) {
        FloatRect titleBounds = detailTitle->getLocalBounds();
        detailTitle->setPosition(Vector2f(
            detailPanel.getPosition().x + (detailPanel.getSize().x - titleBounds.size.x) / 2.f - titleBounds.position.x,
            detailPanel.getPosition().y + 40.f
        ));
    }
    detailCloseBg.setPosition(Vector2f(
        detailPanel.getPosition().x + (detailPanel.getSize().x - detailCloseBg.getSize().x) / 2.f,
        detailPanel.getPosition().y + detailPanel.getSize().y - 95.f
    ));

    FloatRect closeBounds = detailCloseText->getLocalBounds();
    detailCloseText->setPosition(Vector2f(
        detailCloseBg.getPosition().x + (detailCloseBg.getSize().x - closeBounds.size.x) / 2.f - closeBounds.position.x,
        detailCloseBg.getPosition().y + (detailCloseBg.getSize().y - closeBounds.size.y) / 2.f - closeBounds.position.y
    ));

    notificationBg.setPosition(Vector2f(pos.x + width - notificationBg.getSize().x - 30.f, pos.y + 20.f));
}

void TicketPanel::refreshTickets() {
    tickets = repository.loadAll();

    vector<Ticket> ordered;
    ordered.reserve(tickets.getSize());
    for (int i = 0; i < tickets.getSize(); ++i) {
        ordered.push_back(tickets[i]);
    }
    sort(ordered.begin(), ordered.end(), [](const Ticket& a, const Ticket& b) {
        long long ka = parseBookedKey(a.bookedDate, a.bookedTime);
        long long kb = parseBookedKey(b.bookedDate, b.bookedTime);
        if (ka != 0 && kb != 0 && ka != kb) {
            return ka > kb;
        }
        return a.ticketId > b.ticketId;
    });

    DLL<Ticket> sortedList;
    for (const auto& ticket : ordered) {
        sortedList.push_back(ticket);
    }
    tickets = move(sortedList);

    selectedRow = -1;
    hoveredRow = -1;
    scrollOffset = 0;
    detailVisible = false;
    updateButtonStates();
}

void TicketPanel::updateButtonStates() {
    // Keep buttons visually active at all times; actions still validate selection
    // and show notifications when required.
    btnView->setColors(Color(20, 118, 172), Color(17, 98, 144));
    btnDelete->setColors(Color(211, 47, 47), Color(171, 36, 36));
}

void TicketPanel::handleReload() {
    refreshTickets();
    showNotification("Đã tải lại dữ liệu");
}

void TicketPanel::handleDelete() {
    if (selectedRow < 0 || selectedRow >= tickets.getSize()) {
        showNotification("Vui lòng chọn vé cần xóa");
        return;
    }
    string ticketId = tickets[selectedRow].ticketId;
    if (repository.deleteTicket(ticketId)) {
        showNotification("Đã xóa vé " + ticketId);
    } else {
        showNotification("Không thể xóa vé này");
    }
    refreshTickets();
}

void TicketPanel::openDetailPopup() {
    if (selectedRow < 0 || selectedRow >= tickets.getSize()) {
        showNotification("Hãy chọn vé để xem chi tiết");
        return;
    }
    rebuildDetailTexts(tickets[selectedRow]);
    detailVisible = true;
}

void TicketPanel::closeDetailPopup() {
    detailVisible = false;
}

void TicketPanel::rebuildDetailTexts(const Ticket& ticket) {
    detailEntries.clear();
    detailEntries.reserve(12);

    const float padding = 70.f;
    const float labelX = detailPanel.getPosition().x + padding;
    const float valueX = labelX + 320.f;
    const float maxValueWidth = max(120.f, detailPanel.getPosition().x + detailPanel.getSize().x - valueX - padding);
    float currentY = detailPanel.getPosition().y + 100.f;
    const float minSpacing = 54.f;

    auto pushEntry = [&](const string& label, const string& rawValue, bool highlight = false) {
        string safeValue = rawValue.empty() ? "-" : rawValue;
        unsigned int valueSize = highlight ? 24u : 20u;
        string wrappedValue = wrapTextToWidth(safeValue, font, valueSize, maxValueWidth);

        DetailEntry entry(font);
        entry.label = Text(font, utf8(label + ":"), 19);
        entry.label.setFillColor(Color(180, 180, 180));
        entry.label.setPosition(Vector2f(labelX, currentY));

        entry.value = Text(font, utf8(wrappedValue), valueSize);
        entry.value.setFillColor(highlight ? Color(255, 215, 0) : Color::White);
        entry.value.setLineSpacing(1.2f);
        entry.value.setPosition(Vector2f(valueX, currentY));

        detailEntries.push_back(entry);

        size_t newlineCount = count(wrappedValue.begin(), wrappedValue.end(), '\n');
        float blockHeight = minSpacing + static_cast<float>(newlineCount) * (valueSize + 6.f);
        currentY += blockHeight;
    };

    pushEntry("Mã vé", ticket.ticketId);
    pushEntry("ID suất chiếu", ticket.showtimeId);
    pushEntry("Khách hàng", ticket.fullName);
    pushEntry("Email", ticket.email);
    pushEntry("Phim", ticket.title);
    pushEntry("Phòng", ticket.roomName);
    pushEntry("Ngày & giờ chiếu", formatDateToDDMMYYYY(ticket.date) + " - " + ticket.time);
    pushEntry("Ghế", formatSeatList(ticket.booked));
    pushEntry("Combo", formatComboDisplay(ticket.comboName));
    pushEntry("Tổng hóa đơn", formatCurrency(ticket.price), true);
    pushEntry("Ngày đặt vé", formatDateToDDMMYYYY(ticket.bookedDate) + " - " + ticket.bookedTime);
}

void TicketPanel::handleEvent(const Event& event, const RenderWindow& window) {
    if (const auto* mouseEvent = event.getIf<Event::MouseButtonPressed>()) {
        if (mouseEvent->button == Mouse::Button::Left) {
            Vector2f mousePos = window.mapPixelToCoords(Mouse::getPosition(window));
            if (detailVisible) {
                if (detailCloseBg.getGlobalBounds().contains(mousePos)) {
                    closeDetailPopup();
                }
                return;
            }

            if (btnView && btnView->contains(mousePos)) {
                openDetailPopup();
                return;
            }
            if (btnDelete && btnDelete->contains(mousePos)) {
                handleDelete();
                return;
            }
            if (btnReload && btnReload->contains(mousePos)) {
                handleReload();
                return;
            }

            int hitRow = hitTestRow(mousePos);
            if (hitRow >= 0) {
                selectedRow = hitRow;
                updateButtonStates();
            }
        }
    }

    if (detailVisible) {
        return;
    }

    if (const auto* wheelEvent = event.getIf<Event::MouseWheelScrolled>()) {
        Vector2f mousePos = window.mapPixelToCoords(Mouse::getPosition(window));
        if (getTableBounds().contains(mousePos)) {
            scrollOffset -= static_cast<int>(wheelEvent->delta);
            int visibleRows = static_cast<int>((TABLE_HEIGHT - HEADER_HEIGHT) / ROW_HEIGHT);
            int maxOffset = max(0, tickets.getSize() - visibleRows);
            scrollOffset = clamp(scrollOffset, 0, maxOffset);
        }
    }

    if (event.getIf<Event::MouseMoved>()) {
        Vector2f mp = window.mapPixelToCoords(Mouse::getPosition(window));
        hoveredRow = hitTestRow(mp);
        if (btnView) btnView->update(mp);
        if (btnDelete) btnDelete->update(mp);
        if (btnReload) btnReload->update(mp);
    }
}

int TicketPanel::hitTestRow(Vector2f mousePos) const {
    FloatRect tableBounds = getTableBounds();
    if (!tableBounds.contains(mousePos)) {
        return -1;
    }
    float relativeY = mousePos.y - tableBounds.position.y;
    if (relativeY < HEADER_HEIGHT) {
        return -1;
    }
    int rowIndex = static_cast<int>((relativeY - HEADER_HEIGHT) / ROW_HEIGHT);
    int visibleRows = static_cast<int>((TABLE_HEIGHT - HEADER_HEIGHT) / ROW_HEIGHT);
    if (rowIndex < 0 || rowIndex >= visibleRows) {
        return -1;
    }
    int actualIndex = scrollOffset + rowIndex;
    if (actualIndex >= tickets.getSize()) {
        return -1;
    }
    return actualIndex;
}

FloatRect TicketPanel::getTableBounds() const {
    return FloatRect({position.x + TABLE_X, position.y + TABLE_Y},
                     {TABLE_WIDTH, TABLE_HEIGHT});
}

void TicketPanel::update(Vector2f mousePos, bool /*mousePressed*/) {
    if (detailVisible) {
        detailCloseHover = detailCloseBg.getGlobalBounds().contains(mousePos);
        detailCloseBg.setFillColor(detailCloseHover ? Color(30, 138, 192) : Color(20, 118, 172));
        return;
    }

    if (btnView) btnView->update(mousePos);
    if (btnDelete) btnDelete->update(mousePos);
    if (btnReload) {
        btnReload->update(mousePos);
        reloadSprite.setColor(btnReload->contains(mousePos) ? Color(220, 236, 255) : Color::White);
    }

    hoveredRow = hitTestRow(mousePos);

    if (notificationVisible && notificationClock.getElapsedTime().asSeconds() > 3.0f) {
        notificationVisible = false;
    }
}

void TicketPanel::showNotification(const string& message) {
    notificationVisible = true;
    notificationClock.restart();
    if (notificationText) {
        notificationText->setString(utf8(message));
    }
}

void TicketPanel::renderButtons(RenderWindow& window) {
    if (btnView) btnView->draw(window);
    if (btnDelete) btnDelete->draw(window);
    if (btnReload) {
        btnReload->draw(window);
        window.draw(reloadSprite);
    }
}

void TicketPanel::renderTable(RenderWindow& window) {
    window.draw(tableHeaderBg);

    float headerX = tableHeaderBg.getPosition().x;
    float headerY = tableHeaderBg.getPosition().y;
    float currentX = headerX;

    for (const auto& column : COLUMNS) {
        RectangleShape separator;
        separator.setSize(Vector2f(column.width, HEADER_HEIGHT));
        separator.setPosition(Vector2f(currentX, headerY));
        separator.setFillColor(Color::Transparent);
        separator.setOutlineThickness(0.f);
        window.draw(separator);

        Text headerText(font);
        headerText.setString(utf8(column.label));
        headerText.setFillColor(Color::White);
        headerText.setCharacterSize(19);
        headerText.setStyle(Text::Bold);
        FloatRect bounds = headerText.getLocalBounds();
        headerText.setPosition(Vector2f(
            currentX + (column.width - bounds.size.x) / 2.f - bounds.position.x,
            headerY + (HEADER_HEIGHT - bounds.size.y) / 2.f - bounds.position.y
        ));
        window.draw(headerText);
        currentX += column.width;

        RectangleShape divider(Vector2f(1.f, HEADER_HEIGHT));
        divider.setPosition(Vector2f(currentX, headerY));
        divider.setFillColor(Color(255, 255, 255, 70));
        window.draw(divider);
    }

    float rowStartY = headerY + HEADER_HEIGHT;
    int visibleRows = static_cast<int>((TABLE_HEIGHT - HEADER_HEIGHT) / ROW_HEIGHT);
    for (int row = 0; row < visibleRows; ++row) {
        int ticketIndex = scrollOffset + row;
        if (ticketIndex >= tickets.getSize()) {
            break;
        }
        float rowY = rowStartY + row * ROW_HEIGHT;
        RectangleShape rowBg(Vector2f(TABLE_WIDTH, ROW_HEIGHT - 2.f));
        rowBg.setPosition(Vector2f(tableHeaderBg.getPosition().x, rowY));

        if (ticketIndex == selectedRow) {
            rowBg.setFillColor(SELECTED_COLOR);
        } else if (ticketIndex == hoveredRow) {
            rowBg.setFillColor(HOVER_COLOR);
        } else {
            rowBg.setFillColor(ROW_COLOR);
        }
        window.draw(rowBg);

        currentX = tableHeaderBg.getPosition().x;
        const Ticket& ticket = tickets[ticketIndex];
        array<string, 5> values = {
            ticket.ticketId,
            ticket.showtimeId,
            ticket.email,
            formatDateToDDMMYYYY(ticket.bookedDate),
            ticket.bookedTime
        };

        for (size_t col = 0; col < COLUMNS.size(); ++col) {
            Text cell(font);
            cell.setCharacterSize(15);
            cell.setFillColor(TEXT_COLOR);
            cell.setString(utf8(values[col]));
            cell.setPosition(Vector2f(currentX + 14.f, rowY + 6.f));
            window.draw(cell);
            currentX += COLUMNS[col].width;
        }
    }

    RectangleShape border;
    border.setSize(Vector2f(TABLE_WIDTH, TABLE_HEIGHT));
    border.setPosition(Vector2f(tableHeaderBg.getPosition().x, tableHeaderBg.getPosition().y));
    border.setFillColor(Color::Transparent);
    border.setOutlineThickness(1.f);
    border.setOutlineColor(BORDER_COLOR);
    window.draw(border);
}

void TicketPanel::renderNotification(RenderWindow& window) {
    if (!notificationVisible || !notificationText) return;
    window.draw(notificationBg);

    FloatRect bounds = notificationText->getLocalBounds();
    notificationText->setPosition(Vector2f(
        notificationBg.getPosition().x + (notificationBg.getSize().x - bounds.size.x) / 2.f - bounds.position.x,
        notificationBg.getPosition().y + (notificationBg.getSize().y - bounds.size.y) / 2.f - bounds.position.y
    ));
    window.draw(*notificationText);
}

void TicketPanel::renderDetailPopup(RenderWindow& window) {
    if (!detailVisible) return;
    window.draw(overlay);
    window.draw(detailPanel);
    if (detailTitle) window.draw(*detailTitle);
    for (const auto& entry : detailEntries) {
        window.draw(entry.label);
        window.draw(entry.value);
    }
    window.draw(detailCloseBg);
    if (detailCloseText) window.draw(*detailCloseText);
}

void TicketPanel::render(RenderWindow& window) {
    window.draw(background);
    if (titleText) window.draw(*titleText);

    renderButtons(window);
    renderTable(window);
    renderNotification(window);
    renderDetailPopup(window);
}
