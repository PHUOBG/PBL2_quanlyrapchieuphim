#include "UI/components/Admin/ComboPanel.h"
#include "UI/components/Admin/RoundedRectRenderer.h"
#include <algorithm>
#include <sstream>
#include <array>
#include <cstring>

using namespace sf;
using namespace std;

namespace {

sf::String utf8(const char* text) {
    const char* end = text + std::strlen(text);
    return sf::String::fromUtf8(text, end);
}

string formatPrice(const string& value) {
    if (value.empty()) return "";
    bool numeric = all_of(value.begin(), value.end(), [](unsigned char ch){ return std::isdigit(ch); });
    if (!numeric) return value;

    long long number = 0;
    try {
        number = stoll(value);
    } catch (...) {
        return value;
    }

    string digits = to_string(number);
    string formatted;
    int count = 0;
    for (auto it = digits.rbegin(); it != digits.rend(); ++it) {
        if (count == 3) {
            formatted.push_back('.');
            count = 0;
        }
        formatted.push_back(*it);
        ++count;
    }
    reverse(formatted.begin(), formatted.end());
    formatted += " đ";
    return formatted;
}
}

ComboPanel::ComboPanel(Font& f, float w, float h)
    : font(f), width(w), height(h), 
    reloadTexture("../assets/elements/reload.png"),
    reloadSprite(reloadTexture) 
{
    repository = make_unique<AdminComboRepository>("../data/combo.txt");
    setupUI();
}

void ComboPanel::setupUI() {
    background.setSize(Vector2f(width, height));
    background.setFillColor(bgColor);

    titleText = make_unique<Text>(font);
    titleText->setCharacterSize(26);
    titleText->setString(utf8("Quản lý combo bắp nước"));
    titleText->setFillColor(Color(27, 38, 59));
    titleText->setStyle(Text::Bold);

    tableHeaderBg.setSize(Vector2f(TABLE_WIDTH, HEADER_HEIGHT));
    tableHeaderBg.setFillColor(headerColor);

    const float btnW = 150.f;
    const float btnH = 44.f;

    btnAddBg.setSize(Vector2f(btnW, btnH));
    btnAddBg.setFillColor(Color(40, 167, 69));
    btnEditBg.setSize(Vector2f(btnW, btnH));
    btnEditBg.setFillColor(Color(233, 164, 0));
    btnDeleteBg.setSize(Vector2f(btnW, btnH));
    btnDeleteBg.setFillColor(Color(211, 47, 47));
    reloadButtonBg.setSize(Vector2f(btnH, btnH));
    reloadButtonBg.setFillColor(Color(20, 118, 172));

    btnAddText = make_unique<Text>(font);
    btnAddText->setString(utf8("Thêm combo"));
    btnAddText->setCharacterSize(16);
    btnAddText->setFillColor(Color::White);

    btnEditText = make_unique<Text>(font);
    btnEditText->setString(utf8("Sửa combo"));
    btnEditText->setCharacterSize(16);
    btnEditText->setFillColor(Color::White);

    btnDeleteText = make_unique<Text>(font);
    btnDeleteText->setString(utf8("Xóa combo"));
    btnDeleteText->setCharacterSize(16);
    btnDeleteText->setFillColor(Color::White);

    reloadTexture.setSmooth(true);
    reloadSprite.setTexture(reloadTexture);
    reloadSprite.setScale({0.1, 0.1});

    notificationBg.setSize(Vector2f(400, 60));
    notificationBg.setFillColor(Color(211, 47, 47, 230));
    notificationTextObj = make_unique<Text>(font);
    notificationTextObj->setCharacterSize(18);
    notificationTextObj->setFillColor(Color::White);
}

void ComboPanel::setPosition(Vector2f pos) {
    position = pos;
    background.setPosition(pos);

    if (titleText) titleText->setPosition(Vector2f(pos.x + 40, pos.y + 20));

    tableHeaderBg.setPosition(Vector2f(pos.x + TABLE_X, pos.y + TABLE_Y));

    const float buttonRowY = TABLE_Y - 64.f;
    const float spacing = 18.f;

    btnAddBg.setPosition(Vector2f(pos.x + TABLE_X, pos.y + buttonRowY));
    btnEditBg.setPosition(Vector2f(btnAddBg.getPosition().x + btnAddBg.getSize().x + spacing, pos.y + buttonRowY));
    btnDeleteBg.setPosition(Vector2f(btnEditBg.getPosition().x + btnEditBg.getSize().x + spacing, pos.y + buttonRowY));
    reloadButtonBg.setPosition(Vector2f(pos.x + TABLE_X + TABLE_WIDTH - reloadButtonBg.getSize().x, pos.y + buttonRowY));

    auto centerText = [](unique_ptr<Text>& textObj, const RectangleShape& rect) {
        if (!textObj) return;
        FloatRect tb = textObj->getLocalBounds();
        Vector2f posRect = rect.getPosition();
        textObj->setPosition(Vector2f(
            posRect.x + (rect.getSize().x - tb.size.x) / 2.f - tb.position.x,
            posRect.y + (rect.getSize().y - tb.size.y) / 2.f - tb.position.y
        ));
    };

    centerText(btnAddText, btnAddBg);
    centerText(btnEditText, btnEditBg);
    centerText(btnDeleteText, btnDeleteBg);
}

void ComboPanel::renderTable(RenderWindow& window) {
    window.draw(tableHeaderBg);

    struct ColumnSpec {
        string label;
        float ratio;
    };

    static const array<ColumnSpec, 3> columns = {{{"Mã combo", 0.18f},
        {"Tên combo", 0.52f}, {"Giá bán", 0.30f}}};

    vector<float> columnWidths;
    columnWidths.reserve(columns.size());
    for (const auto& col : columns) {
        columnWidths.push_back(TABLE_WIDTH * col.ratio);
    }

    vector<float> columnLefts(columns.size(), position.x + TABLE_X);
    for (size_t i = 1; i < columns.size(); ++i) {
        columnLefts[i] = columnLefts[i - 1] + columnWidths[i - 1];
    }

    auto allData = repository->getAllData();
    float headerTop = position.y + TABLE_Y;
    float startY = headerTop + HEADER_HEIGHT;
    float totalHeight = HEADER_HEIGHT + static_cast<float>(allData.size()) * ROW_HEIGHT;

    RectangleShape separator(Vector2f(1.f, totalHeight));
    separator.setFillColor(borderColor);
    separator.setPosition(Vector2f(position.x + TABLE_X, headerTop));
    window.draw(separator);
    for (size_t i = 0; i < columns.size(); ++i) {
        separator.setPosition(Vector2f(columnLefts[i] + columnWidths[i], headerTop));
        window.draw(separator);
    }

    for (size_t i = 0; i < columns.size(); ++i) {
        Text text(font, String::fromUtf8(columns[i].label.begin(), columns[i].label.end()), 18);
        text.setFillColor(Color::White);
        text.setStyle(Text::Bold);
        FloatRect bounds = text.getLocalBounds();
        text.setPosition(Vector2f(
            columnLefts[i] + (columnWidths[i] - bounds.size.x) / 2.f - bounds.position.x,
            headerTop + (HEADER_HEIGHT - bounds.size.y) / 2.f - bounds.position.y
        ));
        window.draw(text);
    }

    for (size_t i = 0; i < allData.size(); ++i) {
        float rowY = startY + static_cast<float>(i) * ROW_HEIGHT;

        RectangleShape rowBg(Vector2f(TABLE_WIDTH, ROW_HEIGHT));
        rowBg.setPosition(Vector2f(position.x + TABLE_X, rowY));
        if (static_cast<int>(i) == selectedRow) {
            rowBg.setFillColor(selectedColor);
        } else if (static_cast<int>(i) == hoveredRow) {
            rowBg.setFillColor(hoverColor);
        } else {
            rowBg.setFillColor(rowColor);
        }
        window.draw(rowBg);

        RectangleShape border(Vector2f(TABLE_WIDTH, 1.f));
        border.setPosition(Vector2f(position.x + TABLE_X, rowY + ROW_HEIGHT));
        border.setFillColor(borderColor);
        window.draw(border);

        const auto& row = allData[i];
        if (row.size() < 3) continue;

        array<string, 3> values = {row[0], row[1], formatPrice(row[2])};
        for (size_t col = 0; col < columns.size(); ++col) {
            Text text(font, String::fromUtf8(values[col].begin(), values[col].end()), 16);
            text.setFillColor(textColor);
            FloatRect bounds = text.getLocalBounds();
            text.setPosition(Vector2f(
                columnLefts[col] + 12.f - bounds.position.x,
                rowY + (ROW_HEIGHT - bounds.size.y) / 2.f - bounds.position.y
            ));
            window.draw(text);
        }
    }
}

void ComboPanel::openAddPopup() {
    currentPopup = ADD;

    popupOverlay.setSize(Vector2f(1728, 972));
    popupOverlay.setFillColor(Color(0, 0, 0, 160));
    popupOverlay.setPosition(Vector2f(0, 0));

    const float popupW = 520.f;
    const float popupH = 360.f;
    const float popupX = (1728.f - popupW) / 2.f;
    const float popupY = (972.f - popupH) / 2.f;

    popupBackground.setSize(Vector2f(popupW, popupH));
    popupBackground.setPosition(Vector2f(popupX, popupY));
    popupBackground.setFillColor(Color(255, 255, 255));

    popupTitle = make_unique<Text>(font);
    popupTitle->setCharacterSize(22);
    popupTitle->setStyle(Text::Bold);
    popupTitle->setFillColor(Color(34, 34, 34));
    popupTitle->setString(utf8("Thêm combo mới"));
    popupTitle->setPosition(Vector2f(popupX + 30, popupY + 20));

    inputBoxes.clear();
    float inputW = popupW - 60.f;
    float inputH = 44.f;
    float startY = popupY + 80;

    auto nameBox = make_unique<TextBox>(font, "Tên combo*", popupX + 30, startY, inputW, inputH);
    nameBox->setPlaceholder("Ví dụ: 2 nước + 1 bắp");

    auto priceBox = make_unique<TextBox>(font, "Giá bán (VND)*", popupX + 30, startY + 90.f, inputW, inputH);
    priceBox->setPlaceholder("Ví dụ: 120000");

    inputBoxes.push_back(move(nameBox));
    inputBoxes.push_back(move(priceBox));

    const float btnW = 160.f;
    const float btnH = 46.f;
    const float spacing = 24.f;
    float btnStartX = popupX + (popupW - (btnW * 2 + spacing)) / 2.f;
    float btnY = popupY + popupH - btnH - 24.f;

    btnPopupSave = make_unique<Button>(font, utf8("Lưu"), btnW, btnH, 18);
    btnPopupSave->setPosition(Vector2f(btnStartX, btnY));
    btnPopupSave->setFillColor(Color(20, 118, 172));
    btnPopupSave->setTextColor(Color::White);

    btnPopupCancel = make_unique<Button>(font, utf8("Hủy"), btnW, btnH, 18);
    btnPopupCancel->setPosition(Vector2f(btnStartX + btnW + spacing, btnY));
    btnPopupCancel->setFillColor(Color(90, 90, 90));
    btnPopupCancel->setTextColor(Color::White);
}

void ComboPanel::openEditPopup() {
    if (selectedRow < 0) {
        showNotification("Vui lòng chọn combo cần sửa");
        return;
    }
    currentPopup = EDIT;

    popupOverlay.setSize(Vector2f(1728, 972));
    popupOverlay.setFillColor(Color(0, 0, 0, 160));
    popupOverlay.setPosition(Vector2f(0, 0));

    const float popupW = 520.f;
    const float popupH = 360.f;
    const float popupX = (1728.f - popupW) / 2.f;
    const float popupY = (972.f - popupH) / 2.f;

    popupBackground.setSize(Vector2f(popupW, popupH));
    popupBackground.setPosition(Vector2f(popupX, popupY));
    popupBackground.setFillColor(Color(255, 255, 255));

    popupTitle = make_unique<Text>(font);
    popupTitle->setCharacterSize(22);
    popupTitle->setStyle(Text::Bold);
    popupTitle->setFillColor(Color(34, 34, 34));
    popupTitle->setString(utf8("Chỉnh sửa combo"));
    popupTitle->setPosition(Vector2f(popupX + 30, popupY + 20));

    inputBoxes.clear();
    float inputW = popupW - 60.f;
    float inputH = 44.f;
    float startY = popupY + 80;

    auto record = repository->getRecord(selectedRow);

    auto nameBox = make_unique<TextBox>(font, "Tên combo*", popupX + 30, startY, inputW, inputH);
    nameBox->setPlaceholder("Ví dụ: 2 nước + 1 bắp");
    if (record.size() >= 2) nameBox->setValue(record[1]);

    auto priceBox = make_unique<TextBox>(font, "Giá bán (VND)*", popupX + 30, startY + 90.f, inputW, inputH);
    priceBox->setPlaceholder("Ví dụ: 120000");
    if (record.size() >= 3) priceBox->setValue(record[2]);

    inputBoxes.push_back(move(nameBox));
    inputBoxes.push_back(move(priceBox));

    const float btnW = 160.f;
    const float btnH = 46.f;
    const float spacing = 24.f;
    float btnStartX = popupX + (popupW - (btnW * 2 + spacing)) / 2.f;
    float btnY = popupY + popupH - btnH - 24.f;

    btnPopupSave = make_unique<Button>(font, utf8("Lưu"), btnW, btnH, 18);
    btnPopupSave->setPosition(Vector2f(btnStartX, btnY));
    btnPopupSave->setFillColor(Color(20, 118, 172));
    btnPopupSave->setTextColor(Color::White);

    btnPopupCancel = make_unique<Button>(font, utf8("Hủy"), btnW, btnH, 18);
    btnPopupCancel->setPosition(Vector2f(btnStartX + btnW + spacing, btnY));
    btnPopupCancel->setFillColor(Color(90, 90, 90));
    btnPopupCancel->setTextColor(Color::White);
}

void ComboPanel::openDeletePopup() {
    if (selectedRow < 0) {
        showNotification("Vui lòng chọn combo cần xóa");
        return;
    }

    currentPopup = DELETE_CONFIRM;

    popupOverlay.setSize(Vector2f(1728, 972));
    popupOverlay.setFillColor(Color(0, 0, 0, 160));
    popupOverlay.setPosition(Vector2f(0, 0));

    const float popupW = 420.f;
    const float popupH = 200.f;
    const float popupX = (1728.f - popupW) / 2.f;
    const float popupY = (972.f - popupH) / 2.f;

    popupBackground.setSize(Vector2f(popupW, popupH));
    popupBackground.setPosition(Vector2f(popupX, popupY));
    popupBackground.setFillColor(Color(255, 255, 255));

    popupTitle = make_unique<Text>(font);
    popupTitle->setCharacterSize(20);
    popupTitle->setStyle(Text::Bold);
    popupTitle->setFillColor(Color(34, 34, 34));
    popupTitle->setString(utf8("Bạn có chắc chắn muốn xóa?"));
    popupTitle->setPosition(Vector2f(popupX + 30, popupY + 30));

    const float btnW = 140.f;
    const float btnH = 46.f;
    const float spacing = 24.f;
    float btnStartX = popupX + (popupW - (btnW * 2 + spacing)) / 2.f;
    float btnY = popupY + popupH - btnH - 24.f;

    btnPopupSave = make_unique<Button>(font, utf8("Xóa"), btnW, btnH, 18);
    btnPopupSave->setPosition(Vector2f(btnStartX, btnY));
    btnPopupSave->setFillColor(Color(211, 47, 47));
    btnPopupSave->setTextColor(Color::White);

    btnPopupCancel = make_unique<Button>(font, utf8("Hủy"), btnW, btnH, 18);
    btnPopupCancel->setPosition(Vector2f(btnStartX + btnW + spacing, btnY));
    btnPopupCancel->setFillColor(Color(160, 160, 160));
    btnPopupCancel->setTextColor(Color::White);
}

void ComboPanel::closePopup() {
    currentPopup = NONE;
    inputBoxes.clear();
    btnPopupSave.reset();
    btnPopupCancel.reset();
    popupTitle.reset();
}

bool ComboPanel::isDigits(const string& value) {
    return !value.empty() && all_of(value.begin(), value.end(), [](unsigned char ch){ return std::isdigit(ch); });
}

void ComboPanel::handleAdd() {
    if (inputBoxes.size() < 2) return;

    string name = inputBoxes[0]->getValue();
    string price = inputBoxes[1]->getValue();

    if (name.empty()) {
        showNotification("Tên combo không được để trống");
        return;
    }
    if (!isDigits(price)) {
        showNotification("Giá combo chỉ nhận số");
        return;
    }

    vector<string> record = {"", name, price};
    repository->addRecord(record);
    repository->saveToFile();
    repository->loadFromFile();

    closePopup();
    showNotification("Đã thêm combo mới");
}

void ComboPanel::handleEdit() {
    if (selectedRow < 0 || inputBoxes.size() < 2) return;

    string name = inputBoxes[0]->getValue();
    string price = inputBoxes[1]->getValue();

    if (name.empty()) {
        showNotification("Tên combo không được để trống");
        return;
    }
    if (!isDigits(price)) {
        showNotification("Giá combo chỉ nhận số");
        return;
    }

    vector<string> record = {"", name, price};
    repository->updateRecord(selectedRow, record);
    repository->saveToFile();
    repository->loadFromFile();

    closePopup();
    showNotification("Đã cập nhật combo");
}

void ComboPanel::handleDelete() {
    if (selectedRow < 0) return;
    repository->deleteRecord(selectedRow);
    repository->saveToFile();
    repository->loadFromFile();
    selectedRow = -1;
    closePopup();
    showNotification("Đã xóa combo");
}

void ComboPanel::handleReload() {
    repository->loadFromFile();
    selectedRow = -1;
    hoveredRow = -1;
    showNotification("Đã tải lại dữ liệu");
}

void ComboPanel::showNotification(const string& message) {
    notificationText = message;
    notificationClock.restart();
}

void ComboPanel::renderPopup(RenderWindow& window) {
    if (currentPopup == NONE) return;

    window.draw(popupOverlay);
    window.draw(popupBackground);
    if (popupTitle) window.draw(*popupTitle);

    for (auto& box : inputBoxes) {
        box->render(window);
    }

    if (btnPopupSave) btnPopupSave->draw(window);
    if (btnPopupCancel) btnPopupCancel->draw(window);
}

void ComboPanel::renderNotification(RenderWindow& window) {
    if (notificationText.empty()) return;
    if (notificationClock.getElapsedTime().asSeconds() > 3.f) return;

    notificationBg.setPosition(Vector2f(position.x + width - notificationBg.getSize().x - 30.f, position.y + 20.f));
    window.draw(notificationBg);

    notificationTextObj->setString(String::fromUtf8(notificationText.begin(), notificationText.end()));
    FloatRect bounds = notificationTextObj->getLocalBounds();
    notificationTextObj->setPosition(Vector2f(
        notificationBg.getPosition().x + (notificationBg.getSize().x - bounds.size.x) / 2.f - bounds.position.x,
        notificationBg.getPosition().y + (notificationBg.getSize().y - bounds.size.y) / 2.f - bounds.position.y
    ));
    window.draw(*notificationTextObj);
}

void ComboPanel::handleEvent(const Event& event, const RenderWindow& window) {
    (void)window;
    if (currentPopup != NONE) {
        // Tab navigation between input fields
        if (const auto* keyEvent = event.getIf<Event::KeyPressed>()) {
            if (keyEvent->code == Keyboard::Key::Tab && !inputBoxes.empty()) {
                int direction = keyEvent->shift ? -1 : 1;
                int focusedIndex = -1;
                for (int i = 0; i < static_cast<int>(inputBoxes.size()); ++i) {
                    if (inputBoxes[static_cast<size_t>(i)] && inputBoxes[static_cast<size_t>(i)]->getFocus()) {
                        focusedIndex = i;
                        break;
                    }
                }

                auto focusOnly = [&](int idx) {
                    for (int i = 0; i < static_cast<int>(inputBoxes.size()); ++i) {
                        if (inputBoxes[static_cast<size_t>(i)]) {
                            inputBoxes[static_cast<size_t>(i)]->setFocus(i == idx);
                        }
                    }
                };

                if (focusedIndex < 0) {
                    // Focus first box
                    focusOnly(0);
                } else {
                    int total = static_cast<int>(inputBoxes.size());
                    int nextIndex = (focusedIndex + direction + total) % total;
                    focusOnly(nextIndex);
                }
                return;
            }
        }

        for (auto& box : inputBoxes) {
            box->handleEvent(event);
        }
    }

    if (const auto* mouseEvent = event.getIf<Event::MouseButtonPressed>()) {
        if (mouseEvent->button == Mouse::Button::Left) {
            Vector2f mousePos(static_cast<float>(mouseEvent->position.x), static_cast<float>(mouseEvent->position.y));

            if (currentPopup == NONE) {
                float startY = position.y + TABLE_Y + HEADER_HEIGHT;
                auto total = repository->getRecordCount();

                for (int i = 0; i < total; ++i) {
                    FloatRect bounds(Vector2f(position.x + TABLE_X, startY + i * ROW_HEIGHT), Vector2f(TABLE_WIDTH, ROW_HEIGHT));
                    if (bounds.contains(mousePos)) {
                        selectedRow = i;
                        break;
                    }
                }
            } else {
                if (btnPopupSave && btnPopupSave->isClicked(mousePos, true)) {
                    if (currentPopup == ADD) handleAdd();
                    else if (currentPopup == EDIT) handleEdit();
                    else if (currentPopup == DELETE_CONFIRM) handleDelete();
                } else if (btnPopupCancel && btnPopupCancel->isClicked(mousePos, true)) {
                    closePopup();
                }
            }
        }
    }
}

void ComboPanel::update(Vector2f mousePos, bool mousePressed) {
    if (currentPopup == NONE) {
        FloatRect addBounds = btnAddBg.getGlobalBounds();
        FloatRect editBounds = btnEditBg.getGlobalBounds();
        FloatRect delBounds = btnDeleteBg.getGlobalBounds();
        FloatRect reloadBounds = reloadButtonBg.getGlobalBounds();

        btnAddHover = addBounds.contains(mousePos);
        btnEditHover = editBounds.contains(mousePos);
        btnDeleteHover = delBounds.contains(mousePos);
        btnReloadHover = reloadBounds.contains(mousePos);

        auto brighten = [](const Color& c, float pct) {
            auto clamp = [](int v) { return v < 0 ? 0 : (v > 255 ? 255 : v); };
            int r = clamp(static_cast<int>(c.r + (255 - c.r) * pct));
            int g = clamp(static_cast<int>(c.g + (255 - c.g) * pct));
            int b = clamp(static_cast<int>(c.b + (255 - c.b) * pct));
            return Color(r, g, b);
        };

        Color addBase(40, 167, 69);
        Color editBase(233, 164, 0);
        Color delBase(211, 47, 47);
        Color reloadBase(20, 118, 172);

        btnAddBg.setFillColor(btnAddHover ? brighten(addBase, 0.12f) : addBase);
        btnEditBg.setFillColor(btnEditHover ? brighten(editBase, 0.12f) : editBase);
        btnDeleteBg.setFillColor(btnDeleteHover ? brighten(delBase, 0.12f) : delBase);
        reloadButtonBg.setFillColor(btnReloadHover ? brighten(reloadBase, 0.12f) : reloadBase);
        reloadSprite.setColor(btnReloadHover ? Color(200, 230, 255) : Color::White);

        if (mousePressed) {
            if (btnAddHover) btnAddPressed = true;
            else if (btnEditHover) btnEditPressed = true;
            else if (btnDeleteHover) btnDeletePressed = true;
            else if (btnReloadHover) btnReloadPressed = true;
        } else {
            if (btnAddPressed && btnAddHover) openAddPopup();
            if (btnEditPressed && btnEditHover) openEditPopup();
            if (btnDeletePressed && btnDeleteHover) openDeletePopup();
            if (btnReloadPressed && btnReloadHover) handleReload();
            btnAddPressed = btnEditPressed = btnDeletePressed = btnReloadPressed = false;
        }

        float startY = position.y + TABLE_Y + HEADER_HEIGHT;
        hoveredRow = -1;
        auto count = repository->getRecordCount();
        for (int i = 0; i < count; ++i) {
            FloatRect rowBounds(Vector2f(position.x + TABLE_X, startY + i * ROW_HEIGHT), Vector2f(TABLE_WIDTH, ROW_HEIGHT));
            if (rowBounds.contains(mousePos)) {
                hoveredRow = i;
                break;
            }
        }
    } else {
        for (auto& box : inputBoxes) {
            box->update(mousePos, mousePressed);
        }

        if (btnPopupSave && btnPopupCancel) {
            Color primary(Color(20, 118, 172));
            Color primaryHover(Color(30, 138, 192));
            Color danger(Color(211, 47, 47));
            Color dangerHover(Color(231, 67, 67));
            Color neutral(Color(160, 160, 160));
            Color neutralHover(Color(190, 190, 190));

            if (currentPopup == DELETE_CONFIRM) {
                btnPopupSave->update(mousePos, mousePressed, dangerHover, danger);
                btnPopupCancel->update(mousePos, mousePressed, neutralHover, neutral);
            } else {
                btnPopupSave->update(mousePos, mousePressed, primaryHover, primary);
                btnPopupCancel->update(mousePos, mousePressed, neutralHover, neutral);
            }

            if (mousePressed) {
                if (btnPopupSave->isClicked(mousePos, true)) {
                    if (currentPopup == ADD) handleAdd();
                    else if (currentPopup == EDIT) handleEdit();
                    else if (currentPopup == DELETE_CONFIRM) handleDelete();
                } else if (btnPopupCancel->isClicked(mousePos, true)) {
                    closePopup();
                }
            }
        }
    }
}

void ComboPanel::render(RenderWindow& window) {
    window.draw(background);
    if (titleText) window.draw(*titleText);

    renderTable(window);

    RoundedRectRenderer::draw(window, btnAddBg.getPosition(), btnAddBg.getSize(), 6.f, btnAddBg.getFillColor());
    if (btnAddText) window.draw(*btnAddText);

    RoundedRectRenderer::draw(window, btnEditBg.getPosition(), btnEditBg.getSize(), 6.f, btnEditBg.getFillColor());
    if (btnEditText) window.draw(*btnEditText);

    RoundedRectRenderer::draw(window, btnDeleteBg.getPosition(), btnDeleteBg.getSize(), 6.f, btnDeleteBg.getFillColor());
    if (btnDeleteText) window.draw(*btnDeleteText);

    RoundedRectRenderer::draw(window, reloadButtonBg.getPosition(), reloadButtonBg.getSize(), 6.f, reloadButtonBg.getFillColor());
    FloatRect spriteBounds = reloadSprite.getLocalBounds();
    Vector2f scale = reloadSprite.getScale();
    float scaledW = spriteBounds.size.x * scale.x;
    float scaledH = spriteBounds.size.y * scale.y;
    reloadSprite.setPosition({reloadButtonBg.getPosition().x + (reloadButtonBg.getSize().x - scaledW) / 2.f, reloadButtonBg.getPosition().y + (reloadButtonBg.getSize().y - scaledH) / 2.f});
    window.draw(reloadSprite);

    renderPopup(window);
    renderNotification(window);
}
