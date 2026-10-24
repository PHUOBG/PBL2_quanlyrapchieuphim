#include "UI/components/Admin/UserPanel.h"
#include "UI/components/Admin/RoundedRectRenderer.h"

#include <algorithm>
#include <array>
#include <iomanip>
#include <sstream>

using sf::Event;
using sf::FloatRect;
using sf::RenderWindow;
using sf::Vector2f;

namespace {
    constexpr float BUTTON_SPACING = 16.f;
    constexpr float BUTTON_RADIUS = 6.f;
    constexpr float POPUP_W = 760.f;
    constexpr float POPUP_H_VIEW = 520.f;
    constexpr float POPUP_H_CONFIRM = 320.f;
}

UserPanel::UserPanel(Font& font, float width, float height)
    : font(font), width(width), height(height) {
    repository = make_unique<UserRepository>();
    setupUI();
    refreshData();
}

void UserPanel::setupUI() {
    background.setSize(Vector2f(width, height));
    background.setFillColor(bgColor);

    const std::string titleUtf8 = "Quản lý khách hàng";
    titleText = make_unique<Text>(font, sf::String::fromUtf8(titleUtf8.begin(), titleUtf8.end()), 26);
    titleText->setFillColor(Color(27, 38, 59));
    titleText->setStyle(sf::Text::Bold);

    tableBodyBg.setSize(Vector2f(TABLE_WIDTH, TABLE_HEIGHT));
    tableBodyBg.setFillColor(Color::White);
    tableBodyBg.setOutlineColor(borderColor);
    tableBodyBg.setOutlineThickness(1.f);

    tableHeaderBg.setSize(Vector2f(TABLE_WIDTH, HEADER_HEIGHT));
    tableHeaderBg.setFillColor(headerColor);

    setupButton(btnViewInfo, "Xem thông tin", Color(20, 118, 172), Color(34, 156, 218), {172.f, 44.f});
    setupButton(btnDelete, "Xóa tài khoản", Color(211, 47, 47), Color(226, 83, 83), {176.f, 44.f});
    setupButton(btnRefresh, "", Color(20, 118, 172), Color(30, 138, 192), {48.f, 48.f});

    if (!reloadTexture.loadFromFile("../assets/elements/reload.png")) {
        const std::string fallback = "Refresh";
        btnRefresh.label = make_unique<Text>(font, sf::String::fromUtf8(fallback.begin(), fallback.end()), 14);
        btnRefresh.label->setFillColor(Color::White);
    } else {
        reloadTexture.setSmooth(true);
        reloadSprite.emplace(reloadTexture);
    }

    notificationBg.setSize(Vector2f(400.f, 60.f));
    notificationBg.setFillColor(notificationColor);
    notificationBg.setOutlineThickness(0.f);

    notificationText = make_unique<Text>(font, "", 18);
    notificationText->setFillColor(Color::White);

    popupOverlay.setSize(Vector2f(1728.f, 972.f));
    popupOverlay.setFillColor(Color(0, 0, 0, 90));

    popupPanel.setSize(Vector2f(POPUP_W, POPUP_H_VIEW));
    popupPanel.setFillColor(Color::White);
    popupPanel.setOutlineThickness(1.f);
    popupPanel.setOutlineColor(borderColor);

    popupTitle = make_unique<Text>(font, "", 24);
    popupTitle->setFillColor(Color(20, 118, 172));
    popupTitle->setStyle(sf::Text::Bold);

    setupButton(popupBtnPrimary, "", Color(20, 118, 172), Color(17, 98, 144), {240.f, 56.f});
    setupButton(popupBtnSecondary, "", Color(201, 206, 214), Color(170, 176, 186), {240.f, 56.f});
    if (popupBtnSecondary.label) {
        popupBtnSecondary.label->setFillColor(Color(33, 37, 41));
    }

    layoutElements();
}

void UserPanel::setupButton(ActionButton& button, const std::string& labelUtf8, const Color& base, const Color& hover, Vector2f size) {
    button.box.setSize(size);
    button.box.setFillColor(base);
    button.box.setOutlineThickness(0.f);
    button.baseColor = base;
    button.hoverColor = hover;

    if (!labelUtf8.empty()) {
        button.label = make_unique<Text>(font, sf::String::fromUtf8(labelUtf8.begin(), labelUtf8.end()), 16);
        button.label->setFillColor(Color::White);
    } else {
        button.label.reset();
    }
}

void UserPanel::layoutElements() {
    background.setPosition(position);

    if (titleText) {
        titleText->setPosition(Vector2f(position.x + 40.f, position.y + 20.f));
    }

    const float buttonRowY = position.y + TABLE_Y - 64.f;
    float nextX = position.x + TABLE_X;

    btnViewInfo.box.setPosition(Vector2f(nextX, buttonRowY));
    nextX += btnViewInfo.box.getSize().x + BUTTON_SPACING;

    btnDelete.box.setPosition(Vector2f(nextX, buttonRowY));

    const float refreshX = position.x + TABLE_X + TABLE_WIDTH - btnRefresh.box.getSize().x;
    btnRefresh.box.setPosition(Vector2f(refreshX, buttonRowY - 4.f));

    auto centerLabel = [](ActionButton& btn) {
        if (!btn.label) return;
        const FloatRect bounds = btn.label->getLocalBounds();
        btn.label->setPosition(Vector2f(
            btn.box.getPosition().x + (btn.box.getSize().x - bounds.size.x) / 2.f - bounds.position.x,
            btn.box.getPosition().y + (btn.box.getSize().y - bounds.size.y) / 2.f - bounds.position.y
        ));
    };

    centerLabel(btnViewInfo);
    centerLabel(btnDelete);
    centerLabel(btnRefresh);

    if (reloadSprite) {
        const FloatRect bounds = reloadSprite->getLocalBounds();
        const float scale = 0.1f;
        reloadSprite->setScale(Vector2f(scale, scale));
        reloadSprite->setPosition(Vector2f(
            btnRefresh.box.getPosition().x + (btnRefresh.box.getSize().x - bounds.size.x * scale) / 2.f,
            btnRefresh.box.getPosition().y + (btnRefresh.box.getSize().y - bounds.size.y * scale) / 2.f
        ));
        reloadSprite->setColor(Color::White);
    }

    tableBodyBg.setPosition(Vector2f(position.x + TABLE_X, position.y + TABLE_Y));
    tableHeaderBg.setPosition(tableBodyBg.getPosition());

    popupOverlay.setPosition(Vector2f(0.f, 0.f));
}

void UserPanel::setPosition(Vector2f pos) {
    position = pos;
    layoutElements();
}

void UserPanel::refreshData() {
    repository->loadFromFile();
    userList = repository->getAllUsers();
    userList.erase(std::remove_if(userList.begin(), userList.end(), [](const User& u) {
        return u.getRole() == AppRole::Admin;
    }), userList.end());
    std::sort(userList.begin(), userList.end(), [](const User& a, const User& b) {
        if (a.getRegisteredAt() != b.getRegisteredAt()) return a.getRegisteredAt() > b.getRegisteredAt();
        return a.getEmail() < b.getEmail();
    });
    selectedRow = -1;
    hoveredRow = -1;
    scrollOffset = 0;
    closePopup();
}

void UserPanel::updateButton(ActionButton& button, Vector2f mousePos) {
    button.hovered = button.box.getGlobalBounds().contains(mousePos);
    button.box.setFillColor(button.hovered ? button.hoverColor : button.baseColor);
}

string UserPanel::formatDate(time_t timestamp) const {
    if (timestamp <= 0) return "-";

    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &timestamp);
#else
    localtime_r(&timestamp, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%d/%m/%Y");
    return oss.str();
}

void UserPanel::renderTable(RenderWindow& window) {
    window.draw(tableBodyBg);
    window.draw(tableHeaderBg);

    static const std::array<std::string, 5> headers = {
        "STT", "Email", "Họ và tên", "Số điện thoại", "Ngày đăng ký"
    };
    static const std::array<float, 5> columnWidths = {
        70.f, 300.f, 260.f, 190.f, 280.f
    };

    std::array<float, 5> columnLefts{};
    columnLefts[0] = tableHeaderBg.getPosition().x;
    for (size_t i = 1; i < columnLefts.size(); ++i) {
        columnLefts[i] = columnLefts[i - 1] + columnWidths[i - 1];
    }

    for (size_t i = 0; i < headers.size(); ++i) {
        Text headerText(font, sf::String::fromUtf8(headers[i].begin(), headers[i].end()), 16);
        headerText.setFillColor(Color::White);
        headerText.setStyle(sf::Text::Bold);

        const FloatRect bounds = headerText.getLocalBounds();
        headerText.setPosition(Vector2f(
            columnLefts[i] + (columnWidths[i] - bounds.size.x) / 2.f - bounds.position.x,
            tableHeaderBg.getPosition().y + (HEADER_HEIGHT - bounds.size.y) / 2.f - bounds.position.y
        ));
        window.draw(headerText);
    }

    // White header dividers (like TicketPanel)
    float divX = tableHeaderBg.getPosition().x;
    for (size_t i = 0; i < headers.size(); ++i) {
        divX += columnWidths[i];
        RectangleShape divider(Vector2f(1.f, HEADER_HEIGHT));
        divider.setPosition(Vector2f(divX, tableHeaderBg.getPosition().y));
        divider.setFillColor(Color(255, 255, 255, 70));
        window.draw(divider);
    }

    const float rowsStartY = tableHeaderBg.getPosition().y + HEADER_HEIGHT;
    const int maxRows = static_cast<int>((TABLE_HEIGHT - HEADER_HEIGHT) / ROW_HEIGHT);
    const int maxScroll = std::max(0, static_cast<int>(userList.size()) - maxRows);
    if (scrollOffset > maxScroll) {
        scrollOffset = maxScroll;
    }

    const int endIndex = std::min<int>(static_cast<int>(userList.size()), scrollOffset + maxRows);

    // Vertical separators only down to the last rendered row (avoid looking like an extra blank row)
    const float visibleHeight = HEADER_HEIGHT + static_cast<float>(std::max(0, endIndex - scrollOffset)) * ROW_HEIGHT;
    if (visibleHeight > HEADER_HEIGHT) {
        RectangleShape separator(Vector2f(1.f, visibleHeight));
        separator.setFillColor(borderColor);
        for (size_t i = 1; i < headers.size(); ++i) {
            separator.setPosition(Vector2f(columnLefts[i], tableHeaderBg.getPosition().y));
            window.draw(separator);
        }
    }

    for (int idx = scrollOffset; idx < endIndex; ++idx) {
        const float rowY = rowsStartY + static_cast<float>(idx - scrollOffset) * ROW_HEIGHT;
        RectangleShape rowBg(Vector2f(TABLE_WIDTH, ROW_HEIGHT));
        rowBg.setPosition(Vector2f(tableHeaderBg.getPosition().x, rowY));

        if (idx == selectedRow) {
            rowBg.setFillColor(selectedColor);
        } else if (idx == hoveredRow) {
            rowBg.setFillColor(hoverColor);
        } else {
            rowBg.setFillColor(idx % 2 == 0 ? Color::White : Color(248, 250, 252));
        }
        window.draw(rowBg);

        if (idx < endIndex - 1) {
            RectangleShape rowBottom(Vector2f(TABLE_WIDTH, 1.f));
            rowBottom.setPosition(Vector2f(tableHeaderBg.getPosition().x, rowY + ROW_HEIGHT));
            rowBottom.setFillColor(borderColor);
            window.draw(rowBottom);
        }

        const User& user = userList[idx];
        std::array<std::string, 5> values = {
            std::to_string(idx + 1),
            user.getEmail(),
            user.getFullName(),
            user.getPhone(),
            formatDate(user.getRegisteredAt())
        };

        for (size_t col = 0; col < values.size(); ++col) {
            Color cellColor = textColor;

            Text cell(font, sf::String::fromUtf8(values[col].begin(), values[col].end()), 14);
            cell.setFillColor(cellColor);

            auto textBounds = cell.getLocalBounds();
            float textX = columnLefts[col] + 14.f - textBounds.position.x;
            if (textBounds.size.x > columnWidths[col] - 28.f) {
                cell.setCharacterSize(13);
                textBounds = cell.getLocalBounds();
            }

            cell.setPosition(Vector2f(
                textX,
                rowY + (ROW_HEIGHT - textBounds.size.y) / 2.f - textBounds.position.y
            ));
            window.draw(cell);
        }
    }

    // Table border
    RectangleShape border;
    border.setSize(Vector2f(TABLE_WIDTH, TABLE_HEIGHT));
    border.setPosition(Vector2f(tableHeaderBg.getPosition().x, tableHeaderBg.getPosition().y));
    border.setFillColor(Color::Transparent);
    border.setOutlineThickness(1.f);
    border.setOutlineColor(borderColor);
    window.draw(border);
}

const User* UserPanel::getPopupUser() const {
    if (popupUserIndex < 0 || popupUserIndex >= static_cast<int>(userList.size())) return nullptr;
    return &userList[popupUserIndex];
}

void UserPanel::closePopup() {
    activePopup = PopupType::None;
    popupUserIndex = -1;
    detailEntries.clear();
    if (popupTitle) popupTitle->setString("");
}

void UserPanel::rebuildDetailTexts(const User& user) {
    detailEntries.clear();

    struct Row { std::string label; std::string value; };
    std::vector<Row> rows = {
        {"Email", user.getEmail()},
        {"Họ và tên", user.getFullName()},
        {"Ngày sinh", user.getBirthDate().empty() ? "-" : user.getBirthDate()},
        {"Số điện thoại", user.getPhone()},
        {"Ngày đăng ký", formatDate(user.getRegisteredAt())},
    };

    detailEntries.reserve(rows.size());
    for (const auto& r : rows) {
        DetailEntry entry(font);
        entry.label.setCharacterSize(18);
        entry.label.setFillColor(Color(33, 37, 41));
        entry.label.setStyle(Text::Bold);
        entry.label.setString(sf::String::fromUtf8(r.label.begin(), r.label.end()));

        entry.value.setCharacterSize(18);
        entry.value.setFillColor(Color(33, 37, 41));
        entry.value.setString(sf::String::fromUtf8(r.value.begin(), r.value.end()));

        detailEntries.push_back(entry);
    }
}

void UserPanel::openViewPopup() {
    if (selectedRow < 0 || selectedRow >= static_cast<int>(userList.size())) {
        showNotification("Vui lòng chọn một khách hàng trước.", Color(211, 47, 47));
        return;
    }

    popupUserIndex = selectedRow;
    const User& user = userList[popupUserIndex];
    activePopup = PopupType::ViewInfo;

    popupPanel.setSize(Vector2f(POPUP_W, POPUP_H_VIEW));
    popupPanel.setPosition(Vector2f((1728.f - POPUP_W) / 2.f, (972.f - POPUP_H_VIEW) / 2.f + 15.f));

    if (popupTitle) {
        const std::string title = "Thông tin khách hàng";
        popupTitle->setString(sf::String::fromUtf8(title.begin(), title.end()));
        FloatRect tb = popupTitle->getLocalBounds();
        popupTitle->setPosition(Vector2f(
            popupPanel.getPosition().x + (popupPanel.getSize().x - tb.size.x) / 2.f - tb.position.x,
            popupPanel.getPosition().y + 32.f
        ));
    }

    rebuildDetailTexts(user);

    setupButton(popupBtnPrimary, "Đã hiểu", Color(20, 118, 172), Color(17, 98, 144), {260.f, 56.f});
    popupBtnSecondary.label.reset();
}

void UserPanel::openDeletePopup() {
    if (selectedRow < 0 || selectedRow >= static_cast<int>(userList.size())) {
        showNotification("Chưa chọn khách hàng nào.", Color(211, 47, 47));
        return;
    }
    popupUserIndex = selectedRow;
    activePopup = PopupType::DeleteConfirm;

    popupPanel.setSize(Vector2f(POPUP_W, POPUP_H_CONFIRM));
    popupPanel.setPosition(Vector2f((1728.f - POPUP_W) / 2.f, (972.f - POPUP_H_CONFIRM) / 2.f + 15.f));

    if (popupTitle) {
        const std::string title = "Xác nhận xóa";
        popupTitle->setString(sf::String::fromUtf8(title.begin(), title.end()));
        FloatRect tb = popupTitle->getLocalBounds();
        popupTitle->setPosition(Vector2f(
            popupPanel.getPosition().x + (popupPanel.getSize().x - tb.size.x) / 2.f - tb.position.x,
            popupPanel.getPosition().y + 32.f
        ));
    }

    setupButton(popupBtnPrimary, "Xóa tài khoản", Color(211, 47, 47), Color(171, 36, 36), {260.f, 56.f});
    setupButton(popupBtnSecondary, "Hủy", Color(201, 206, 214), Color(170, 176, 186), {260.f, 56.f});
    if (popupBtnSecondary.label) popupBtnSecondary.label->setFillColor(Color(33, 37, 41));
}

void UserPanel::renderPopup(RenderWindow& window) {
    if (activePopup == PopupType::None) return;

    window.draw(popupOverlay);
    window.draw(popupPanel);
    if (popupTitle) window.draw(*popupTitle);

    const User* user = getPopupUser();
    if (!user) return;

    if (activePopup == PopupType::ViewInfo) {
        float xLabel = popupPanel.getPosition().x + 72.f;
        float xValue = popupPanel.getPosition().x + 320.f;
        float y = popupPanel.getPosition().y + 110.f;
        float gap = 52.f;

        for (auto& entry : detailEntries) {
            entry.label.setPosition(Vector2f(xLabel, y));
            entry.value.setPosition(Vector2f(xValue, y));
            window.draw(entry.label);
            window.draw(entry.value);
            y += gap;
        }

        // Primary button centered
        popupBtnPrimary.box.setPosition(Vector2f(
            popupPanel.getPosition().x + (popupPanel.getSize().x - popupBtnPrimary.box.getSize().x) / 2.f,
            popupPanel.getPosition().y + popupPanel.getSize().y - 95.f
        ));
        if (popupBtnPrimary.label) {
            FloatRect b = popupBtnPrimary.label->getLocalBounds();
            popupBtnPrimary.label->setPosition(Vector2f(
                popupBtnPrimary.box.getPosition().x + (popupBtnPrimary.box.getSize().x - b.size.x) / 2.f - b.position.x,
                popupBtnPrimary.box.getPosition().y + (popupBtnPrimary.box.getSize().y - b.size.y) / 2.f - b.position.y
            ));
        }
        RoundedRectRenderer::draw(window, popupBtnPrimary.box.getPosition(), popupBtnPrimary.box.getSize(), BUTTON_RADIUS, popupBtnPrimary.box.getFillColor());
        if (popupBtnPrimary.label) window.draw(*popupBtnPrimary.label);
        return;
    }

    // Confirm popups
    std::string message;
    message = "Bạn có chắc muốn xóa tài khoản\n" + user->getEmail() + " ?";

    Text msg(font, sf::String::fromUtf8(message.begin(), message.end()), 20);
    msg.setFillColor(Color(33, 37, 41));
    msg.setStyle(Text::Bold);
    FloatRect mb = msg.getLocalBounds();
    msg.setPosition(Vector2f(
        popupPanel.getPosition().x + (popupPanel.getSize().x - mb.size.x) / 2.f - mb.position.x,
        popupPanel.getPosition().y + 120.f
    ));
    window.draw(msg);

    float buttonsY = popupPanel.getPosition().y + popupPanel.getSize().y - 95.f;
    float leftX = popupPanel.getPosition().x + (popupPanel.getSize().x - (popupBtnPrimary.box.getSize().x + 22.f + popupBtnSecondary.box.getSize().x)) / 2.f;
    popupBtnPrimary.box.setPosition(Vector2f(leftX, buttonsY));
    popupBtnSecondary.box.setPosition(Vector2f(leftX + popupBtnPrimary.box.getSize().x + 22.f, buttonsY));

    auto drawPopupButton = [&](ActionButton& btn) {
        if (btn.label) {
            FloatRect b = btn.label->getLocalBounds();
            btn.label->setPosition(Vector2f(
                btn.box.getPosition().x + (btn.box.getSize().x - b.size.x) / 2.f - b.position.x,
                btn.box.getPosition().y + (btn.box.getSize().y - b.size.y) / 2.f - b.position.y
            ));
        }
        RoundedRectRenderer::draw(window, btn.box.getPosition(), btn.box.getSize(), BUTTON_RADIUS, btn.box.getFillColor());
        if (btn.label) window.draw(*btn.label);
    };

    drawPopupButton(popupBtnPrimary);
    drawPopupButton(popupBtnSecondary);
}

void UserPanel::renderButtons(RenderWindow& window) {
    auto drawButton = [&](ActionButton& button) {
        RoundedRectRenderer::draw(window, button.box.getPosition(), button.box.getSize(), BUTTON_RADIUS, button.box.getFillColor());
        if (button.label) {
            window.draw(*button.label);
        }
    };

    drawButton(btnViewInfo);
    drawButton(btnDelete);

    RoundedRectRenderer::draw(window, btnRefresh.box.getPosition(), btnRefresh.box.getSize(), BUTTON_RADIUS, btnRefresh.box.getFillColor());
    if (reloadSprite) {
        window.draw(*reloadSprite);
    } else if (btnRefresh.label) {
        window.draw(*btnRefresh.label);
    }
}

void UserPanel::renderNotification(RenderWindow& window) {
    if (!notificationVisible) return;

    if (notificationClock.getElapsedTime().asSeconds() > 3.0f) {
        notificationVisible = false;
        return;
    }

    const Vector2f size = notificationBg.getSize();
    Vector2f pos(window.getSize().x - size.x - 32.f, position.y + 24.f);
    notificationBg.setPosition(pos);
    notificationBg.setFillColor(notificationColor);

    window.draw(notificationBg);

    if (notificationText) {
        const FloatRect bounds = notificationText->getLocalBounds();
        notificationText->setPosition(Vector2f(
            pos.x + (size.x - bounds.size.x) / 2.f - bounds.position.x,
            pos.y + (size.y - bounds.size.y) / 2.f - bounds.position.y
        ));
        window.draw(*notificationText);
    }
}

void UserPanel::showNotification(const string& message, const Color& color) {
    notificationMessage = message;
    notificationColor = color;
    notificationVisible = true;
    notificationClock.restart();

    if (notificationText) {
        notificationText->setString(sf::String::fromUtf8(message.begin(), message.end()));
    }
}

void UserPanel::handleEvent(const Event& event, const RenderWindow& window) {
    (void)window;
    if (const auto* scroll = event.getIf<Event::MouseWheelScrolled>()) {
        if (scroll->wheel == sf::Mouse::Wheel::Vertical) {
            const int maxRows = static_cast<int>((TABLE_HEIGHT - HEADER_HEIGHT) / ROW_HEIGHT);
            const int maxScroll = std::max(0, static_cast<int>(userList.size()) - maxRows);
            if (scroll->delta > 0) {
                scrollOffset = std::max(0, scrollOffset - 1);
            } else {
                scrollOffset = std::min(maxScroll, scrollOffset + 1);
            }
        }
        return;
    }

    if (const auto* click = event.getIf<Event::MouseButtonPressed>()) {
        if (click->button == sf::Mouse::Button::Left) {
            Vector2f mousePos(static_cast<float>(click->position.x), static_cast<float>(click->position.y));

            // Popup interaction blocks everything underneath
            if (activePopup != PopupType::None) {
                if (popupBtnPrimary.box.getGlobalBounds().contains(mousePos)) {
                    const User* user = getPopupUser();
                    if (!user) {
                        closePopup();
                        return;
                    }

                    if (activePopup == PopupType::ViewInfo) {
                        closePopup();
                        return;
                    }

                    if (activePopup == PopupType::DeleteConfirm) {
                        bool ok = repository->deleteUser(user->getEmail());
                        closePopup();
                        refreshData();
                        showNotification(ok ? "Đã xóa tài khoản" : "Không thể xóa tài khoản", ok ? Color(20, 118, 172) : Color(211, 47, 47));
                        return;
                    }
                }

                if (popupBtnSecondary.box.getGlobalBounds().contains(mousePos)) {
                    closePopup();
                    return;
                }

                // Click outside: ignore
                return;
            }

            if (btnViewInfo.box.getGlobalBounds().contains(mousePos)) {
                openViewPopup();
                return;
            }

            if (btnDelete.box.getGlobalBounds().contains(mousePos)) {
                openDeletePopup();
                return;
            }

            if (btnRefresh.box.getGlobalBounds().contains(mousePos)) {
                refreshData();
                showNotification("Đã tải lại dữ liệu", Color(20, 118, 172));
                return;
            }

            const FloatRect tableBounds(tableBodyBg.getPosition(), tableBodyBg.getSize());
            if (tableBounds.contains(mousePos)) {
                const float localY = mousePos.y - (tableHeaderBg.getPosition().y + HEADER_HEIGHT);
                if (localY >= 0.f) {
                    const int rowIndex = static_cast<int>(localY / ROW_HEIGHT) + scrollOffset;
                    if (rowIndex >= 0 && rowIndex < static_cast<int>(userList.size())) {
                        selectedRow = rowIndex;
                    }
                }
            }
        }
    }
}

void UserPanel::update(Vector2f mousePos, bool mousePressed) {
    (void)mousePressed;

    if (activePopup != PopupType::None) {
        updateButton(popupBtnPrimary, mousePos);
        updateButton(popupBtnSecondary, mousePos);
        return;
    }

    updateButton(btnViewInfo, mousePos);
    updateButton(btnDelete, mousePos);
    updateButton(btnRefresh, mousePos);

    hoveredRow = -1;
    const FloatRect tableBounds(Vector2f(tableBodyBg.getPosition().x, tableBodyBg.getPosition().y + HEADER_HEIGHT),
                                Vector2f(tableBodyBg.getSize().x, tableBodyBg.getSize().y - HEADER_HEIGHT));
    if (tableBounds.contains(mousePos)) {
        const float localY = mousePos.y - tableBounds.position.y;
        const int rowIndex = static_cast<int>(localY / ROW_HEIGHT) + scrollOffset;
        if (rowIndex >= 0 && rowIndex < static_cast<int>(userList.size())) {
            hoveredRow = rowIndex;
        }
    }
}

void UserPanel::render(RenderWindow& window) {
    window.draw(background);
    if (titleText) {
        window.draw(*titleText);
    }

    renderButtons(window);
    renderTable(window);
    renderNotification(window);
    renderPopup(window);
}
