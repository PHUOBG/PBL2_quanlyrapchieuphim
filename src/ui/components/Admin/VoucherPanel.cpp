#include "UI/components/Admin/VoucherPanel.h"
#include "UI/components/Admin/RoundedRectRenderer.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>

using namespace std;
using namespace sf;

namespace {
    inline sf::String toUtf8(const std::string& text) {
        return sf::String::fromUtf8(text.begin(), text.end());
    }
}

VoucherPanel::VoucherPanel(Font& f, float w, float h)
    : font(f), width(w), height(h) {
    repository = make_unique<AdminVoucherRepository>();
    userSelectPopup_ = make_unique<UserSelectPopup>(font);
    setupUI();
    refreshData();
}

void VoucherPanel::setupUI() {
    // Background
    background.setSize(Vector2f(width, height));
    background.setFillColor(bgColor);
    
    // Title
    titleText = make_unique<Text>(font, toUtf8("Quản lý Voucher"), 26);
    titleText->setFillColor(Color(27, 38, 59));
    titleText->setStyle(Text::Bold);
    
    // Left panel (Voucher Definitions)
    leftPanelBg.setSize(Vector2f(LEFT_PANEL_WIDTH, height - 100));
    leftPanelBg.setFillColor(panelBgColor);
    leftPanelBg.setOutlineColor(borderColor);
    leftPanelBg.setOutlineThickness(1.f);
    
    leftPanelTitle = make_unique<Text>(font, toUtf8("Danh sách Voucher"), 18);
    leftPanelTitle->setFillColor(tableHeaderColor);
    leftPanelTitle->setStyle(Text::Bold);
    
    // Right panel (User Vouchers)
    rightPanelBg.setSize(Vector2f(RIGHT_PANEL_WIDTH, height - 100));
    rightPanelBg.setFillColor(panelBgColor);
    rightPanelBg.setOutlineColor(borderColor);
    rightPanelBg.setOutlineThickness(1.f);
    
    rightPanelTitle = make_unique<Text>(font, toUtf8("Chi tiết người nhận"), 18);
    rightPanelTitle->setFillColor(tableHeaderColor);
    rightPanelTitle->setStyle(Text::Bold);
    
    // Table headers
    defTableHeader.setSize(Vector2f(LEFT_PANEL_WIDTH - 40, HEADER_HEIGHT));
    defTableHeader.setFillColor(headerColor);
    
    userTableHeader.setSize(Vector2f(RIGHT_PANEL_WIDTH - 40, HEADER_HEIGHT));
    userTableHeader.setFillColor(headerColor);
    
    // Setup buttons - Left panel (requested order/colors)
    // Thêm mới (Xanh lá) - Chỉnh sửa (Vàng) - Xóa (Đỏ) - Phát đồng loạt (Xanh dương) - Reload (giữ nguyên)
    const float btnW = 150.f;
    setupButton(btnAddVoucher, "Thêm mới", Color(40, 167, 69), Color(60, 187, 89), {btnW, BUTTON_HEIGHT});
    setupButton(btnEditVoucher, "Chỉnh sửa", Color(233, 164, 0), Color(247, 186, 40), {btnW, BUTTON_HEIGHT});
    setupButton(btnDeleteVoucher, "Xóa", Color(211, 47, 47), Color(226, 83, 83), {btnW, BUTTON_HEIGHT});
    setupButton(btnDistribute, "Phát đồng loạt", Color(20, 118, 172), Color(34, 156, 218), {btnW, BUTTON_HEIGHT});
    setupButton(btnViewUsers, "Xem người nhận", Color(108, 117, 125), Color(128, 137, 145), {btnW, BUTTON_HEIGHT});
    setupButton(btnRefresh, "", Color(20, 118, 172), Color(30, 138, 192), {BUTTON_HEIGHT, BUTTON_HEIGHT});
    
    // Setup buttons - Right panel (widen Add/Remove to avoid text overflow)
    setupButton(btnBackToList, "← Quay lại", Color(108, 117, 125), Color(128, 137, 145), {150.f, BUTTON_HEIGHT});
    setupButton(btnAddToUser, "Thêm người nhận", Color(40, 167, 69), Color(60, 187, 89), {190.f, BUTTON_HEIGHT});
    setupButton(btnRemoveFromUser, "Thu hồi voucher", Color(211, 47, 47), Color(226, 83, 83), {190.f, BUTTON_HEIGHT});
    
    // Reload icon
    if (!reloadTexture.loadFromFile("../assets/elements/reload.png")) {
        btnRefresh.label = make_unique<Text>(font, toUtf8("↻"), 18);
        btnRefresh.label->setFillColor(Color::White);
    } else {
        reloadTexture.setSmooth(true);
        reloadSprite.emplace(reloadTexture);
    }
    
    // Notification
    notificationBg.setSize(Vector2f(400.f, 60.f));
    notificationText = make_unique<Text>(font, "", 18);
    notificationText->setFillColor(Color::White);
    
    // Popup
    popupOverlay.setFillColor(Color(0, 0, 0, 150));
    popupBackground.setFillColor(Color::White);
    popupBackground.setOutlineColor(borderColor);
    popupBackground.setOutlineThickness(2.f);
    
    popupTitle = make_unique<Text>(font, "", 20);
    popupTitle->setFillColor(tableHeaderColor);
    popupTitle->setStyle(Text::Bold);
    
    layoutElements();
}

void VoucherPanel::setupButton(ActionButton& button, const string& labelUtf8, 
                               const Color& base, const Color& hover, Vector2f size) {
    button.box.setSize(size);
    button.box.setFillColor(base);
    button.baseColor = base;
    button.hoverColor = hover;
    
    if (!labelUtf8.empty()) {
        button.label = make_unique<Text>(font, toUtf8(labelUtf8), 16);
        button.label->setFillColor(Color::White);
    }
}

void VoucherPanel::layoutElements() {
    background.setPosition(position);
    
    // Title
    if (titleText) {
        titleText->setPosition(Vector2f(position.x + 40.f, position.y + 20.f));
    }
    
    // Panels (dynamic sizing to match MoviePanel scale and avoid button overlap)
    float panelY = position.y + 70.f;
    const float availableWidth = std::max(0.f, width - PANEL_MARGIN * 3.f);
    const float minRightWidth = 560.f;
    const float minLeftWidth = 640.f;

    float leftWidth = std::max(minLeftWidth, availableWidth * 0.56f);
    if (availableWidth - leftWidth < minRightWidth) {
        leftWidth = std::max(0.f, availableWidth - minRightWidth);
    }
    float rightWidth = std::max(0.f, availableWidth - leftWidth);
    if (rightWidth < minRightWidth && availableWidth >= minRightWidth) {
        rightWidth = minRightWidth;
        leftWidth = std::max(0.f, availableWidth - rightWidth);
    }

    leftPanelBg.setSize(Vector2f(leftWidth, height - 100.f));
    rightPanelBg.setSize(Vector2f(rightWidth, height - 100.f));

    leftPanelBg.setPosition(Vector2f(position.x + PANEL_MARGIN, panelY));
    rightPanelBg.setPosition(Vector2f(leftPanelBg.getPosition().x + leftWidth + PANEL_MARGIN, panelY));
    
    // Panel titles
    if (leftPanelTitle) {
        leftPanelTitle->setPosition(Vector2f(leftPanelBg.getPosition().x + 20.f, panelY + 15.f));
    }
    if (rightPanelTitle) {
        rightPanelTitle->setPosition(Vector2f(rightPanelBg.getPosition().x + 20.f, panelY + 15.f));
    }
    
    // Table headers
    defTableHeader.setSize(Vector2f(std::max(0.f, leftWidth - 40.f), HEADER_HEIGHT));
    userTableHeader.setSize(Vector2f(std::max(0.f, rightWidth - 40.f), HEADER_HEIGHT));
    defTableHeader.setPosition(Vector2f(leftPanelBg.getPosition().x + 20.f, panelY + 100.f));
    userTableHeader.setPosition(Vector2f(rightPanelBg.getPosition().x + 20.f, panelY + 100.f));
    
    // Left panel buttons (single row like MoviePanel; ensure no overlap with refresh)
    const float spacing = 18.f;
    const float leftBtnY = panelY + 50.f;
    const float leftBtnX0 = leftPanelBg.getPosition().x + 20.f;

    const float refreshX = leftPanelBg.getPosition().x + leftWidth - 20.f - btnRefresh.box.getSize().x;
    const float maxRight = std::max(leftBtnX0, refreshX - spacing);
    const float availableForButtons = std::max(0.f, maxRight - leftBtnX0);
    const int buttonCount = 4;
    const float desiredW = btnAddVoucher.box.getSize().x;
    float btnW = desiredW;
    if (buttonCount > 0) {
        const float needed = buttonCount * desiredW + (buttonCount - 1) * spacing;
        if (needed > availableForButtons) {
            btnW = std::max(110.f, (availableForButtons - (buttonCount - 1) * spacing) / static_cast<float>(buttonCount));
        }
    }

    btnAddVoucher.box.setSize({btnW, BUTTON_HEIGHT});
    btnEditVoucher.box.setSize({btnW, BUTTON_HEIGHT});
    btnDeleteVoucher.box.setSize({btnW, BUTTON_HEIGHT});
    btnDistribute.box.setSize({btnW, BUTTON_HEIGHT});

    btnAddVoucher.box.setPosition(Vector2f(leftBtnX0, leftBtnY));
    btnEditVoucher.box.setPosition(Vector2f(leftBtnX0 + (btnW + spacing) * 1.f, leftBtnY));
    btnDeleteVoucher.box.setPosition(Vector2f(leftBtnX0 + (btnW + spacing) * 2.f, leftBtnY));
    btnDistribute.box.setPosition(Vector2f(leftBtnX0 + (btnW + spacing) * 3.f, leftBtnY));

    btnRefresh.box.setPosition(Vector2f(refreshX, leftBtnY));
    
    // Right panel buttons (keep Back size, widen Add/Remove to avoid text overflow)
    const float rightBtnY = panelY + 50.f;
    const float rightBtnX0 = rightPanelBg.getPosition().x + 20.f;
    const float rightAvail = std::max(0.f, rightWidth - 40.f);
    const float backW = 150.f;

    float actionW = 190.f;
    {
        const float remaining = std::max(0.f, rightAvail - backW - spacing * 2.f);
        if (remaining > 0.f) {
            actionW = std::max(160.f, remaining / 2.f);
        }
    }

    btnBackToList.box.setSize({backW, BUTTON_HEIGHT});
    btnAddToUser.box.setSize({actionW, BUTTON_HEIGHT});
    btnRemoveFromUser.box.setSize({actionW, BUTTON_HEIGHT});

    btnBackToList.box.setPosition(Vector2f(rightBtnX0, rightBtnY));
    btnAddToUser.box.setPosition(Vector2f(rightBtnX0 + backW + spacing, rightBtnY));
    btnRemoveFromUser.box.setPosition(Vector2f(btnAddToUser.box.getPosition().x + actionW + spacing, rightBtnY));
    
    // Center button labels
    auto centerLabel = [](ActionButton& btn) {
        if (!btn.label) return;
        FloatRect bounds = btn.label->getLocalBounds();
        btn.label->setPosition(Vector2f(
            btn.box.getPosition().x + (btn.box.getSize().x - bounds.size.x) / 2.f - bounds.position.x,
            btn.box.getPosition().y + (btn.box.getSize().y - bounds.size.y) / 2.f - bounds.position.y
        ));
    };
    
    centerLabel(btnAddVoucher);
    centerLabel(btnEditVoucher);
    centerLabel(btnDeleteVoucher);
    centerLabel(btnDistribute);
    centerLabel(btnViewUsers);
    centerLabel(btnRefresh);
    centerLabel(btnAddToUser);
    centerLabel(btnRemoveFromUser);
    centerLabel(btnBackToList);
    
    // Reload sprite
    if (reloadSprite) {
        FloatRect bounds = reloadSprite->getLocalBounds();
        float scale = 0.1f;
        reloadSprite->setScale(Vector2f(scale, scale));
        reloadSprite->setPosition(Vector2f(
            btnRefresh.box.getPosition().x + (btnRefresh.box.getSize().x - bounds.size.x * scale) / 2.f,
            btnRefresh.box.getPosition().y + (btnRefresh.box.getSize().y - bounds.size.y * scale) / 2.f
        ));
        reloadSprite->setColor(Color::White);
    }
}

void VoucherPanel::setPosition(Vector2f pos) {
    position = pos;
    layoutElements();
}

void VoucherPanel::refreshData() {
    repository->reload();
    repository->cleanupExpiredVouchers();
    selectedVoucherIndex = -1;
    selectedVoucherCode = "";
    currentVoucherUsers.clear();
    defScrollOffset = 0;
    userScrollOffset = 0;
}

void VoucherPanel::loadUsersForVoucher(const string& code) {
    selectedVoucherCode = code;
    currentVoucherUsers = repository->getUsersWithVoucher(code);
    selectedUserIndex = -1;
    userScrollOffset = 0;
    
    // Update right panel title
    if (rightPanelTitle) {
        string title = "Người nhận: " + code + " (" + to_string(currentVoucherUsers.size()) + ")";
        rightPanelTitle->setString(toUtf8(title));
    }
}

void VoucherPanel::updateButton(ActionButton& button, Vector2f mousePos) {
    button.hovered = button.box.getGlobalBounds().contains(mousePos);
    button.box.setFillColor(button.hovered ? button.hoverColor : button.baseColor);
}

void VoucherPanel::handleEvent(const Event& event, const RenderWindow& window) {
    // Handle UserSelectPopup if open
    if (userSelectPopup_ && userSelectPopup_->isOpen()) {
        userSelectPopup_->handleEvent(event, window);
        return;
    }
    
    // Handle scroll
    if (const auto* scroll = event.getIf<Event::MouseWheelScrolled>()) {
        Vector2f mousePos(static_cast<float>(scroll->position.x), static_cast<float>(scroll->position.y));
        
        // Scroll in voucher definitions
        if (leftPanelBg.getGlobalBounds().contains(mousePos)) {
            auto defs = repository->getAllDefinitions();
            const float tableBottom = leftPanelBg.getPosition().y + leftPanelBg.getSize().y - 20.f;
            const float rowY = defTableHeader.getPosition().y + HEADER_HEIGHT;
            const float bodyHeight = max(0.f, tableBottom - rowY);
            const int visibleRows = max(1, static_cast<int>(bodyHeight / ROW_HEIGHT));
            int maxScroll = max(0, static_cast<int>(defs.size()) - visibleRows);
            if (scroll->delta > 0) {
                defScrollOffset = max(0, defScrollOffset - 1);
            } else {
                defScrollOffset = min(maxScroll, defScrollOffset + 1);
            }
        }
        
        // Scroll in user vouchers
        if (rightPanelBg.getGlobalBounds().contains(mousePos)) {
            const float tableBottom = rightPanelBg.getPosition().y + rightPanelBg.getSize().y - 20.f;
            const float rowY = userTableHeader.getPosition().y + HEADER_HEIGHT;
            const float bodyHeight = max(0.f, tableBottom - rowY);
            const int visibleRows = max(1, static_cast<int>(bodyHeight / ROW_HEIGHT));
            int maxScroll = max(0, static_cast<int>(currentVoucherUsers.size()) - visibleRows);
            if (scroll->delta > 0) {
                userScrollOffset = max(0, userScrollOffset - 1);
            } else {
                userScrollOffset = min(maxScroll, userScrollOffset + 1);
            }
        }
        return;
    }
    
    // Handle popup events
    if (currentPopup != PopupType::NONE) {
        // Tab navigation between input boxes (Add/Edit/Distribute/etc)
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
                    // Focus first editable box
                    for (int i = 0; i < static_cast<int>(inputBoxes.size()); ++i) {
                        if (inputBoxes[static_cast<size_t>(i)] && inputBoxes[static_cast<size_t>(i)]->isEditable()) {
                            focusOnly(i);
                            break;
                        }
                    }
                } else {
                    int nextIndex = focusedIndex;
                    int total = static_cast<int>(inputBoxes.size());
                    for (int attempt = 0; attempt < total; ++attempt) {
                        nextIndex = (nextIndex + direction + total) % total;
                        if (inputBoxes[static_cast<size_t>(nextIndex)] && inputBoxes[static_cast<size_t>(nextIndex)]->isEditable()) {
                            break;
                        }
                    }
                    focusOnly(nextIndex);
                }
                return;
            }
        }

        // Handle mouse click for input focus
        if (const auto* click = event.getIf<Event::MouseButtonPressed>()) {
            if (click->button == Mouse::Button::Left) {
                Vector2f mousePos(static_cast<float>(click->position.x), static_cast<float>(click->position.y));
                
                // Handle TextBox focus on click
                for (auto& textBox : inputBoxes) {
                    if (textBox) {
                        textBox->update(mousePos, true);  // Pass mousePressed=true
                    }
                }
                
                // Handle search box focus on click
                if (currentPopup == PopupType::ADD_TO_USER && searchBox) {
                    searchBox->update(mousePos, true);
                }
            }
        }
        
        // Handle textbox text input
        for (auto& textBox : inputBoxes) {
            if (textBox) {
                textBox->handleEvent(event, window);
            }
        }
        
        // Handle search box input for ADD_TO_USER popup
        if (currentPopup == PopupType::ADD_TO_USER && searchBox) {
            searchBox->handleEvent(event, window);
            
            // Check if search text changed and filter users
            string newFilter = searchBox->getText();
            if (newFilter != userSearchFilter) {
                userSearchFilter = newFilter;
                filterUsers(userSearchFilter);
            }
        }
        
        if (const auto* click = event.getIf<Event::MouseButtonPressed>()) {
            if (click->button == Mouse::Button::Left) {
                Vector2f mousePos(static_cast<float>(click->position.x), static_cast<float>(click->position.y));
                
                // Handle save button
                if (btnPopupSave && btnPopupSave->isClicked(mousePos)) {
                    switch (currentPopup) {
                        case PopupType::ADD_VOUCHER: handleAddVoucher(); break;
                        case PopupType::EDIT_VOUCHER: handleEditVoucher(); break;
                        case PopupType::DELETE_VOUCHER: handleDeleteVoucher(); break;
                        case PopupType::DISTRIBUTE_VOUCHER: handleDistribute(); break;
                        case PopupType::ADD_TO_USER: handleAddToUser(); break;
                        default: break;
                    }
                    return;
                }
                
                // Handle cancel button
                if (btnPopupCancel && btnPopupCancel->isClicked(mousePos)) {
                    closePopup();
                    return;
                }
                
                // Handle user selection in ADD_TO_USER popup (checkbox toggle)
                if (currentPopup == PopupType::ADD_TO_USER && userListBg.getGlobalBounds().contains(mousePos)) {
                    float localY = mousePos.y - userListBg.getPosition().y;
                    int visualIdx = static_cast<int>(localY / 38.f);  // Visual index in the list
                    int actualListIdx = visualIdx + availableUserScrollOffset;  // Actual index in filtered list
                    
                    if (actualListIdx >= 0 && actualListIdx < static_cast<int>(filteredAvailableUsers.size())) {
                        int userIdx = filteredAvailableUsers[actualListIdx];
                        // Toggle selection
                        if (selectedUserSet.find(userIdx) != selectedUserSet.end()) {
                            selectedUserSet.erase(userIdx);
                        } else {
                            selectedUserSet.insert(userIdx);
                        }
                    }
                }
                
                // Handle Select All button
                if (currentPopup == PopupType::ADD_TO_USER && btnSelectAll && btnSelectAll->isClicked(mousePos)) {
                    if (selectedUserSet.size() == filteredAvailableUsers.size()) {
                        // Deselect all
                        selectedUserSet.clear();
                        btnSelectAll->setText(toUtf8("Chọn tất cả"));
                    } else {
                        // Select all
                        for (int idx : filteredAvailableUsers) {
                            selectedUserSet.insert(idx);
                        }
                        btnSelectAll->setText(toUtf8("Bỏ chọn tất cả"));
                    }
                }
            }
        }
        
        // Handle scroll in user selection popup
        if (currentPopup == PopupType::ADD_TO_USER) {
            if (const auto* scroll = event.getIf<Event::MouseWheelScrolled>()) {
                Vector2f mousePos(static_cast<float>(scroll->position.x), static_cast<float>(scroll->position.y));
                if (userListBg.getGlobalBounds().contains(mousePos)) {
                    int maxScroll = max(0, static_cast<int>(filteredAvailableUsers.size()) - 6);
                    if (scroll->delta > 0) {
                        availableUserScrollOffset = max(0, availableUserScrollOffset - 1);
                    } else {
                        availableUserScrollOffset = min(maxScroll, availableUserScrollOffset + 1);
                    }
                }
            }
        }
        
        return;
    }
    
    // Handle main panel clicks
    if (const auto* click = event.getIf<Event::MouseButtonPressed>()) {
        if (click->button == Mouse::Button::Left) {
            Vector2f mousePos(static_cast<float>(click->position.x), static_cast<float>(click->position.y));
            
            // Left panel buttons
            if (btnAddVoucher.box.getGlobalBounds().contains(mousePos)) {
                openAddVoucherPopup();
                return;
            }
            
            if (btnEditVoucher.box.getGlobalBounds().contains(mousePos)) {
                if (selectedVoucherIndex >= 0) {
                    openEditVoucherPopup();
                } else {
                    showNotification("Vui lòng chọn voucher cần chỉnh sửa", dangerColor);
                }
                return;
            }
            
            if (btnDeleteVoucher.box.getGlobalBounds().contains(mousePos)) {
                if (selectedVoucherIndex >= 0) {
                    openDeleteVoucherPopup();
                } else {
                    showNotification("Vui lòng chọn voucher cần xóa", dangerColor);
                }
                return;
            }
            
            if (btnDistribute.box.getGlobalBounds().contains(mousePos)) {
                if (selectedVoucherIndex >= 0) {
                    openDistributePopup();
                } else {
                    showNotification("Vui lòng chọn voucher để phát", dangerColor);
                }
                return;
            }
            
            if (btnRefresh.box.getGlobalBounds().contains(mousePos)) {
                refreshData();
                showNotification("Đã tải lại dữ liệu", dangerColor);
                return;
            }
            
            // Right panel buttons
            if (btnAddToUser.box.getGlobalBounds().contains(mousePos)) {
                if (!selectedVoucherCode.empty()) {
                    openAddToUserPopup();
                } else {
                    showNotification("Vui lòng chọn voucher trước", dangerColor);
                }
                return;
            }
            
            if (btnRemoveFromUser.box.getGlobalBounds().contains(mousePos)) {
                if (selectedUserIndex >= 0 && selectedUserIndex < static_cast<int>(currentVoucherUsers.size())) {
                    handleRemoveFromUser();
                } else {
                    showNotification("Vui lòng chọn người dùng cần thu hồi", dangerColor);
                }
                return;
            }
            
            if (btnBackToList.box.getGlobalBounds().contains(mousePos)) {
                selectedVoucherCode = "";
                currentVoucherUsers.clear();
                if (rightPanelTitle) {
                    rightPanelTitle->setString(toUtf8("Chi tiết người nhận"));
                }
                return;
            }
            
            // Click on voucher definition table
            const float defsBottom = leftPanelBg.getPosition().y + leftPanelBg.getSize().y - 20.f;
            const float defsTop = defTableHeader.getPosition().y + HEADER_HEIGHT;
            FloatRect defTableBounds(
                Vector2f(defTableHeader.getPosition().x, defsTop),
                Vector2f(defTableHeader.getSize().x, std::max(0.f, defsBottom - defsTop))
            );
            
            if (defTableBounds.contains(mousePos)) {
                float localY = mousePos.y - defTableBounds.position.y;
                int idx = static_cast<int>(localY / ROW_HEIGHT) + defScrollOffset;
                auto defs = repository->getAllDefinitions();
                if (idx >= 0 && idx < static_cast<int>(defs.size())) {
                    selectedVoucherIndex = idx;
                    loadUsersForVoucher(defs[idx].code);
                }
                return;
            }
            
            // Click on user voucher table
            const float usersBottom = rightPanelBg.getPosition().y + rightPanelBg.getSize().y - 20.f;
            const float usersTop = userTableHeader.getPosition().y + HEADER_HEIGHT;
            FloatRect userTableBounds(
                Vector2f(userTableHeader.getPosition().x, usersTop),
                Vector2f(userTableHeader.getSize().x, std::max(0.f, usersBottom - usersTop))
            );
            
            if (userTableBounds.contains(mousePos)) {
                float localY = mousePos.y - userTableBounds.position.y;
                int idx = static_cast<int>(localY / ROW_HEIGHT) + userScrollOffset;
                if (idx >= 0 && idx < static_cast<int>(currentVoucherUsers.size())) {
                    selectedUserIndex = idx;
                }
                return;
            }
        }
    }
}

void VoucherPanel::update(Vector2f mousePos, bool mousePressed) {
    (void)mousePressed;
    
    // Update UserSelectPopup if open
    if (userSelectPopup_ && userSelectPopup_->isOpen()) {
        userSelectPopup_->update(mousePos, mousePressed);
        return;
    }
    
    // Update buttons
    updateButton(btnAddVoucher, mousePos);
    updateButton(btnEditVoucher, mousePos);
    updateButton(btnDeleteVoucher, mousePos);
    updateButton(btnDistribute, mousePos);
    updateButton(btnViewUsers, mousePos);
    updateButton(btnRefresh, mousePos);
    updateButton(btnAddToUser, mousePos);
    updateButton(btnRemoveFromUser, mousePos);
    updateButton(btnBackToList, mousePos);
    
    // Update hover states for tables
    hoveredVoucherIndex = -1;
    hoveredUserIndex = -1;
    
    // Voucher definition hover
    const float defsBottom = leftPanelBg.getPosition().y + leftPanelBg.getSize().y - 20.f;
    const float defsTop = defTableHeader.getPosition().y + HEADER_HEIGHT;
    FloatRect defTableBounds(
        Vector2f(defTableHeader.getPosition().x, defsTop),
        Vector2f(defTableHeader.getSize().x, std::max(0.f, defsBottom - defsTop))
    );
    
    if (defTableBounds.contains(mousePos)) {
        float localY = mousePos.y - defTableBounds.position.y;
        int idx = static_cast<int>(localY / ROW_HEIGHT) + defScrollOffset;
        auto defs = repository->getAllDefinitions();
        if (idx >= 0 && idx < static_cast<int>(defs.size())) {
            hoveredVoucherIndex = idx;
        }
    }
    
    // User voucher hover
    const float usersBottom = rightPanelBg.getPosition().y + rightPanelBg.getSize().y - 20.f;
    const float usersTop = userTableHeader.getPosition().y + HEADER_HEIGHT;
    FloatRect userTableBounds(
        Vector2f(userTableHeader.getPosition().x, usersTop),
        Vector2f(userTableHeader.getSize().x, std::max(0.f, usersBottom - usersTop))
    );
    
    if (userTableBounds.contains(mousePos)) {
        float localY = mousePos.y - userTableBounds.position.y;
        int idx = static_cast<int>(localY / ROW_HEIGHT) + userScrollOffset;
        if (idx >= 0 && idx < static_cast<int>(currentVoucherUsers.size())) {
            hoveredUserIndex = idx;
        }
    }
    
    // Update popup buttons
    if (currentPopup != PopupType::NONE) {
        if (btnPopupSave) btnPopupSave->update(mousePos);
        if (btnPopupCancel) btnPopupCancel->update(mousePos);
        
        for (auto& textBox : inputBoxes) {
            if (textBox) textBox->update(mousePos);
        }
        
        // Update hover in user selection
        if (currentPopup == PopupType::ADD_TO_USER) {
            hoveredAvailableUserIndex = -1;
            if (userListBg.getGlobalBounds().contains(mousePos)) {
                float localY = mousePos.y - userListBg.getPosition().y;
                int visualIdx = static_cast<int>(localY / 38.f);  // Visual index
                int actualListIdx = visualIdx + availableUserScrollOffset;
                if (actualListIdx >= 0 && actualListIdx < static_cast<int>(filteredAvailableUsers.size())) {
                    hoveredAvailableUserIndex = actualListIdx;
                }
            }
            
            // Update Select All button
            if (btnSelectAll) btnSelectAll->update(mousePos);
            
            // Update search box (hover only, click handled in handleEvent)
            if (searchBox) searchBox->update(mousePos);
        }
    }
}

void VoucherPanel::render(RenderWindow& window) {
    window.draw(background);
    
    if (titleText) window.draw(*titleText);
    
    renderLeftPanel(window);
    renderRightPanel(window);
    renderNotification(window);
    
    if (currentPopup != PopupType::NONE) {
        renderPopup(window);
    }
    
    // Render UserSelectPopup (with its own overlay)
    if (userSelectPopup_ && userSelectPopup_->isOpen()) {
        userSelectPopup_->render(window);
    }
}

void VoucherPanel::renderLeftPanel(RenderWindow& window) {
    window.draw(leftPanelBg);
    if (leftPanelTitle) window.draw(*leftPanelTitle);
    
    // Render buttons
    auto drawButton = [&](ActionButton& btn) {
        RoundedRectRenderer::draw(window, btn.box.getPosition(), btn.box.getSize(), 
                                  BUTTON_RADIUS, btn.box.getFillColor());
        if (btn.label) window.draw(*btn.label);
    };
    
    drawButton(btnAddVoucher);
    drawButton(btnEditVoucher);
    drawButton(btnDeleteVoucher);
    drawButton(btnDistribute);
    
    RoundedRectRenderer::draw(window, btnRefresh.box.getPosition(), btnRefresh.box.getSize(),
                              BUTTON_RADIUS, btnRefresh.box.getFillColor());
    if (reloadSprite) {
        window.draw(*reloadSprite);
    } else if (btnRefresh.label) {
        window.draw(*btnRefresh.label);
    }
    
    renderVoucherDefinitions(window);
}

void VoucherPanel::renderRightPanel(RenderWindow& window) {
    window.draw(rightPanelBg);
    if (rightPanelTitle) window.draw(*rightPanelTitle);
    
    // Render buttons
    auto drawButton = [&](ActionButton& btn) {
        RoundedRectRenderer::draw(window, btn.box.getPosition(), btn.box.getSize(), 
                                  BUTTON_RADIUS, btn.box.getFillColor());
        if (btn.label) window.draw(*btn.label);
    };
    
    drawButton(btnBackToList);
    drawButton(btnAddToUser);
    drawButton(btnRemoveFromUser);
    
    renderUserVouchers(window);
}

void VoucherPanel::renderVoucherDefinitions(RenderWindow& window) {
    // Table header
    window.draw(defTableHeader);
    
    // Header columns
    vector<string> headers = {"Mã", "Loại", "Giá trị", "Tối thiểu", "Người nhận"};
    const float totalW = defTableHeader.getSize().x;
    vector<float> colWidths;
    colWidths.reserve(headers.size());
    colWidths.push_back(totalW * 0.20f); // Mã
    colWidths.push_back(totalW * 0.14f); // Loại
    colWidths.push_back(totalW * 0.22f); // Giá trị
    colWidths.push_back(totalW * 0.22f); // Tối thiểu
    colWidths.push_back(std::max(0.f, totalW - (colWidths[0] + colWidths[1] + colWidths[2] + colWidths[3]))); // Người nhận

    vector<float> colLefts(headers.size(), defTableHeader.getPosition().x);
    for (size_t i = 1; i < colLefts.size(); ++i) {
        colLefts[i] = colLefts[i - 1] + colWidths[i - 1];
    }

    const float headerY = defTableHeader.getPosition().y;
    // White separators between header columns
    {
        RectangleShape headerSep(Vector2f(1.f, HEADER_HEIGHT));
        headerSep.setFillColor(Color::White);
        for (size_t i = 0; i + 1 < headers.size(); ++i) {
            headerSep.setPosition(Vector2f(colLefts[i] + colWidths[i], headerY));
            window.draw(headerSep);
        }
    }

    for (size_t i = 0; i < headers.size(); i++) {
        Text headerText(font, toUtf8(headers[i]), 18);
        headerText.setFillColor(Color::White);
        headerText.setStyle(Text::Bold);

        const FloatRect bounds = headerText.getLocalBounds();
        headerText.setPosition(Vector2f(
            colLefts[i] + (colWidths[i] - bounds.size.x) / 2.f - bounds.position.x,
            headerY + (HEADER_HEIGHT - bounds.size.y) / 2.f - bounds.position.y
        ));
        window.draw(headerText);
    }
    
    // Table rows
    auto defs = repository->getAllDefinitions();
    float rowY = defTableHeader.getPosition().y + HEADER_HEIGHT;
    const float tableBottom = leftPanelBg.getPosition().y + leftPanelBg.getSize().y - 20.f;
    const float bodyHeight = std::max(0.f, tableBottom - rowY);
    int maxRows = std::max(1, static_cast<int>(bodyHeight / ROW_HEIGHT));
    int endIdx = min(static_cast<int>(defs.size()), defScrollOffset + maxRows);
    
    for (int i = defScrollOffset; i < endIdx; i++) {
        const auto& def = defs[i];
        float currentRowY = rowY + (i - defScrollOffset) * ROW_HEIGHT;
        
        // Row background
        RectangleShape rowBg(Vector2f(defTableHeader.getSize().x, ROW_HEIGHT - 2));
        rowBg.setPosition(Vector2f(defTableHeader.getPosition().x, currentRowY));
        
        if (i == selectedVoucherIndex) {
            rowBg.setFillColor(selectedColor);
        } else if (i == hoveredVoucherIndex) {
            rowBg.setFillColor(hoverColor);
        } else {
            rowBg.setFillColor(i % 2 == 0 ? rowColor : rowAltColor);
        }
        window.draw(rowBg);

        // Row data (center-aligned per column)
        auto drawCenteredCell = [&](size_t colIndex, const std::string& value, unsigned int fontSize, const Color& color, bool bold = false) {
            Text text(font, toUtf8(value), fontSize);
            text.setFillColor(color);
            text.setStyle(bold ? Text::Bold : Text::Regular);
            const FloatRect b = text.getLocalBounds();
            const float x = colLefts[colIndex] + (colWidths[colIndex] - b.size.x) / 2.f - b.position.x;
            const float y = currentRowY + (ROW_HEIGHT - b.size.y) / 2.f - b.position.y;
            text.setPosition(Vector2f(x, y));
            window.draw(text);
        };

        // Code
        drawCenteredCell(0, def.code, 16, accentColor, true);

        // Type (display numeric 1/2 like voucher_defs.txt)
        drawCenteredCell(1, to_string(def.type), 16, textColor);

        // Value
        string valueStr = def.type == 1 ? formatCurrency(def.value) : to_string(static_cast<int>(def.value)) + "%";
        drawCenteredCell(2, valueStr, 16, successColor);

        // Min bill
        drawCenteredCell(3, formatCurrency(def.minBill), 16, textColor);

        // User count
        int userCount = repository->countUsersWithVoucher(def.code);
        drawCenteredCell(4, to_string(userCount), 16, userCount > 0 ? accentColor : textColor);
    }
    
    // Empty state
    if (defs.empty()) {
        Text emptyText(font, toUtf8("Chưa có voucher nào"), 16);
        emptyText.setFillColor(Color(150, 150, 150));
        FloatRect bounds = emptyText.getLocalBounds();
        emptyText.setPosition(Vector2f(
            defTableHeader.getPosition().x + (defTableHeader.getSize().x - bounds.size.x) / 2.f,
            defTableHeader.getPosition().y + HEADER_HEIGHT + 50.f
        ));
        window.draw(emptyText);
    }
}

void VoucherPanel::renderUserVouchers(RenderWindow& window) {
    // Table header
    window.draw(userTableHeader);
    
    // Header columns
    vector<string> headers = {"Email", "Tên", "SL"};
    const float totalW = userTableHeader.getSize().x;
    vector<float> colWidths;
    colWidths.reserve(headers.size());
    colWidths.push_back(totalW * 0.46f); // Email
    colWidths.push_back(totalW * 0.42f); // Tên (wider)
    colWidths.push_back(std::max(0.f, totalW - (colWidths[0] + colWidths[1]))); // SL

    vector<float> colLefts(headers.size(), userTableHeader.getPosition().x);
    for (size_t i = 1; i < colLefts.size(); ++i) {
        colLefts[i] = colLefts[i - 1] + colWidths[i - 1];
    }

    const float headerY = userTableHeader.getPosition().y;
    // White separators between header columns
    {
        RectangleShape headerSep(Vector2f(1.f, HEADER_HEIGHT));
        headerSep.setFillColor(Color::White);
        for (size_t i = 0; i + 1 < headers.size(); ++i) {
            headerSep.setPosition(Vector2f(colLefts[i] + colWidths[i], headerY));
            window.draw(headerSep);
        }
    }
    for (size_t i = 0; i < headers.size(); i++) {
        Text headerText(font, toUtf8(headers[i]), 18);
        headerText.setFillColor(Color::White);
        headerText.setStyle(Text::Bold);
        const FloatRect bounds = headerText.getLocalBounds();
        headerText.setPosition(Vector2f(
            colLefts[i] + (colWidths[i] - bounds.size.x) / 2.f - bounds.position.x,
            headerY + (HEADER_HEIGHT - bounds.size.y) / 2.f - bounds.position.y
        ));
        window.draw(headerText);
    }
    
    // Table rows
    float rowY = userTableHeader.getPosition().y + HEADER_HEIGHT;
    const float tableBottom = rightPanelBg.getPosition().y + rightPanelBg.getSize().y - 20.f;
    const float bodyHeight = std::max(0.f, tableBottom - rowY);
    int maxRows = std::max(1, static_cast<int>(bodyHeight / ROW_HEIGHT));
    int endIdx = min(static_cast<int>(currentVoucherUsers.size()), userScrollOffset + maxRows);
    
    for (int i = userScrollOffset; i < endIdx; i++) {
        const auto& user = currentVoucherUsers[i];
        float currentRowY = rowY + (i - userScrollOffset) * ROW_HEIGHT;
        
        // Row background
        RectangleShape rowBg(Vector2f(userTableHeader.getSize().x, ROW_HEIGHT - 2));
        rowBg.setPosition(Vector2f(userTableHeader.getPosition().x, currentRowY));
        
        if (i == selectedUserIndex) {
            rowBg.setFillColor(selectedColor);
        } else if (i == hoveredUserIndex) {
            rowBg.setFillColor(hoverColor);
        } else {
            rowBg.setFillColor(i % 2 == 0 ? rowColor : rowAltColor);
        }
        window.draw(rowBg);

        const float cellPaddingX = 10.f;
        auto fitToWidth = [&](const std::string& raw, float maxWidth, unsigned int fontSize) -> sf::String {
            sf::String s = toUtf8(raw);
            sf::Text t(font, s, fontSize);
            if (t.getLocalBounds().size.x <= maxWidth) return s;

            const sf::String ellipsis = toUtf8("...");
            // If even ellipsis doesn't fit, return empty.
            t.setString(ellipsis);
            if (t.getLocalBounds().size.x > maxWidth) return sf::String();

            while (s.getSize() > 0) {
                s = s.substring(0, s.getSize() - 1);
                t.setString(s + ellipsis);
                if (t.getLocalBounds().size.x <= maxWidth) {
                    return s + ellipsis;
                }
            }
            return ellipsis;
        };

        auto drawLeftCellFit = [&](size_t colIndex, const std::string& value, unsigned int fontSize, const Color& color, bool bold = false) {
            float x = colLefts[colIndex] + cellPaddingX;
            float maxWidth = std::max(0.f, colWidths[colIndex] - 2.f * cellPaddingX);
            sf::String fitted = fitToWidth(value, maxWidth, fontSize);

            Text text(font, fitted, fontSize);
            text.setFillColor(color);
            text.setStyle(bold ? Text::Bold : Text::Regular);
            const FloatRect b = text.getLocalBounds();
            text.setPosition(Vector2f(x, currentRowY + (ROW_HEIGHT - b.size.y) / 2.f - b.position.y));
            window.draw(text);
        };

        auto drawCenteredCell = [&](size_t colIndex, const std::string& value, unsigned int fontSize, const Color& color, bool bold = false) {
            sf::String s = toUtf8(value);
            Text text(font, s, fontSize);
            text.setFillColor(color);
            text.setStyle(bold ? Text::Bold : Text::Regular);
            const FloatRect b = text.getLocalBounds();
            float x = colLefts[colIndex] + (colWidths[colIndex] - b.size.x) / 2.f - b.position.x;
            float y = currentRowY + (ROW_HEIGHT - b.size.y) / 2.f - b.position.y;
            text.setPosition(Vector2f(x, y));
            window.draw(text);
        };
        
        // Email (left, fitted)
        drawLeftCellFit(0, user.email, 16, textColor);

        // Name (left, fitted; wider column)
        drawLeftCellFit(1, user.fullName, 16, textColor);

        // Quantity (centered)
        drawCenteredCell(2, to_string(user.quantity), 16, successColor);
    }
    
    // Empty state
    if (currentVoucherUsers.empty()) {
        string emptyMsg = selectedVoucherCode.empty() ? 
            "Chọn voucher để xem người nhận" : 
            "Chưa có người nhận voucher này";
        Text emptyText(font, toUtf8(emptyMsg), 16);
        emptyText.setFillColor(Color(150, 150, 150));
        FloatRect bounds = emptyText.getLocalBounds();
        emptyText.setPosition(Vector2f(
            userTableHeader.getPosition().x + (userTableHeader.getSize().x - bounds.size.x) / 2.f,
            userTableHeader.getPosition().y + HEADER_HEIGHT + 50.f
        ));
        window.draw(emptyText);
    }
}

void VoucherPanel::renderPopup(RenderWindow& window) {
    // Overlay
    popupOverlay.setSize(Vector2f(static_cast<float>(window.getSize().x), static_cast<float>(window.getSize().y)));
    popupOverlay.setPosition(Vector2f(0, 0));
    window.draw(popupOverlay);
    
    // Popup size based on type
    Vector2f popupSize(450.f, 400.f);
    if (currentPopup == PopupType::ADD_VOUCHER || currentPopup == PopupType::EDIT_VOUCHER) {
        // More room to avoid text overlap/truncation
        popupSize = Vector2f(560.f, 460.f);
    }
    if (currentPopup == PopupType::ADD_TO_USER) {
        popupSize = Vector2f(520.f, 520.f);
    } else if (currentPopup == PopupType::DELETE_VOUCHER) {
        popupSize = Vector2f(400.f, 200.f);
    } else if (currentPopup == PopupType::DISTRIBUTE_VOUCHER) {
        popupSize = Vector2f(450.f, 280.f);
    }
    
    Vector2f popupPos(
        (window.getSize().x - popupSize.x) / 2.f,
        (window.getSize().y - popupSize.y) / 2.f
    );
    
    popupBackground.setSize(popupSize);
    popupBackground.setPosition(popupPos);
    RoundedRectRenderer::draw(window, popupPos, popupSize, 12.f, Color::White);
    
    // Title
    if (popupTitle) {
        popupTitle->setPosition(Vector2f(popupPos.x + 24.f, popupPos.y + 20.f));
        window.draw(*popupTitle);
    }
    
    // Position and render input labels and boxes based on popup type
    float contentStartY = popupPos.y + 60.f;
    float labelX = popupPos.x + 24.f;
    float inputX = popupPos.x + 140.f;
    float rowHeight = 50.f;
    
    if (currentPopup == PopupType::ADD_TO_USER) {
        // Position inputs for ADD_TO_USER popup - horizontal layout
        if (inputLabels.size() >= 2 && inputBoxes.size() >= 2) {
            // Days to expire
            inputLabels[0]->setPosition(Vector2f(labelX, contentStartY + 8.f));
            inputBoxes[0]->setPosition(Vector2f(inputX, contentStartY));
            
            // Quantity - next to days
            inputLabels[1]->setPosition(Vector2f(popupPos.x + 260.f, contentStartY + 8.f));
            inputBoxes[1]->setPosition(Vector2f(popupPos.x + 340.f, contentStartY));
        }
        
        // Search box
        if (searchBox) {
            Text searchLabel(font, toUtf8("Tìm kiếm:"), 14);
            searchLabel.setFillColor(textColor);
            searchLabel.setPosition(Vector2f(labelX, contentStartY + 50.f + 8.f));
            window.draw(searchLabel);
            
            searchBox->setPosition(Vector2f(inputX, contentStartY + 50.f));
            searchBox->draw(window);
        }
        
        // User list label
        Text userListLabel(font, toUtf8("Chọn người dùng:"), 14);
        userListLabel.setFillColor(textColor);
        userListLabel.setStyle(Text::Bold);
        userListLabel.setPosition(Vector2f(labelX, contentStartY + 100.f));
        window.draw(userListLabel);
        
        // Position and render user list
        userListBg.setPosition(Vector2f(labelX, contentStartY + 125.f));
        userListBg.setSize(Vector2f(popupSize.x - 48.f, 230.f));
        window.draw(userListBg);
        
        // === Render user list with clipping using sf::View ===
        // Save current view
        View defaultView = window.getView();
        
        // Calculate list area
        FloatRect listArea(
            Vector2f(userListBg.getPosition().x, userListBg.getPosition().y),
            Vector2f(userListBg.getSize().x, userListBg.getSize().y)
        );
        
        // Create clipping view
        View clippedView;
        clippedView.setViewport(FloatRect(
            Vector2f(listArea.position.x / window.getSize().x, listArea.position.y / window.getSize().y),
            Vector2f(listArea.size.x / window.getSize().x, listArea.size.y / window.getSize().y)
        ));
        clippedView.setCenter(Vector2f(listArea.position.x + listArea.size.x / 2.f, 
                                       listArea.position.y + listArea.size.y / 2.f));
        clippedView.setSize(Vector2f(listArea.size.x, listArea.size.y));
        
        window.setView(clippedView);
        
        // Render user list items
        float listY = userListBg.getPosition().y;
        int totalUsers = static_cast<int>(filteredAvailableUsers.size());
        int maxVisible = 6;
        int endIdx = min(totalUsers, availableUserScrollOffset + maxVisible);
        
        for (int i = availableUserScrollOffset; i < endIdx; i++) {
            int visualIdx = i - availableUserScrollOffset;
            float itemY = listY + visualIdx * 38.f;
            int userIdx = filteredAvailableUsers[i];
            
            RectangleShape itemBg(Vector2f(userListBg.getSize().x - 4, 36.f));
            itemBg.setPosition(Vector2f(userListBg.getPosition().x + 2, itemY + 1));
            
            // Check if this user is selected (checkbox selected)
            bool isSelected = (selectedUserSet.find(userIdx) != selectedUserSet.end());
            
            if (isSelected) {
                itemBg.setFillColor(selectedColor);
            } else if (i == hoveredAvailableUserIndex) {
                itemBg.setFillColor(hoverColor);
            } else {
                itemBg.setFillColor(visualIdx % 2 == 0 ? rowColor : rowAltColor);
            }
            window.draw(itemBg);
            
            // Checkbox
            RectangleShape checkbox(Vector2f(18.f, 18.f));
            checkbox.setPosition(Vector2f(userListBg.getPosition().x + 10.f, itemY + 9.f));
            checkbox.setFillColor(isSelected ? accentColor : Color::White);
            checkbox.setOutlineColor(isSelected ? accentColor : borderColor);
            checkbox.setOutlineThickness(1.5f);
            window.draw(checkbox);
            
            // Checkmark if selected
            if (isSelected) {
                Text checkmark(font, toUtf8("✓"), 14);
                checkmark.setFillColor(Color::White);
                checkmark.setStyle(Text::Bold);
                checkmark.setPosition(Vector2f(checkbox.getPosition().x + 2.f, checkbox.getPosition().y - 1.f));
                window.draw(checkmark);
            }
            
            // Email
            string email = availableUsers[userIdx].first;
            if (email.length() > 28) {
                email = email.substr(0, 25) + "...";
            }
            Text emailText(font, toUtf8(email), 13);
            emailText.setFillColor(textColor);
            emailText.setPosition(Vector2f(userListBg.getPosition().x + 38.f, itemY + 10.f));
            window.draw(emailText);
            
            // Name
            string name = availableUsers[userIdx].second;
            if (name.length() > 20) {
                name = name.substr(0, 17) + "...";
            }
            Text nameText(font, toUtf8(name), 11);
            nameText.setFillColor(Color(100, 100, 100));
            nameText.setPosition(Vector2f(userListBg.getPosition().x + 300.f, itemY + 11.f));
            window.draw(nameText);
        }
        
        // Restore default view after clipped rendering
        window.setView(defaultView);
        
        if (filteredAvailableUsers.empty()) {
            Text emptyText(font, toUtf8("Không có người dùng khả dụng"), 14);
            emptyText.setFillColor(Color(150, 150, 150));
            FloatRect bounds = emptyText.getLocalBounds();
            emptyText.setPosition(Vector2f(
                userListBg.getPosition().x + (userListBg.getSize().x - bounds.size.x) / 2.f,
                userListBg.getPosition().y + 100.f
            ));
            window.draw(emptyText);
        }
        
        // Scroll indicator (show if more items exist)
        if (totalUsers > maxVisible) {
            string scrollInfo = to_string(availableUserScrollOffset + 1) + "-" + 
                               to_string(min(availableUserScrollOffset + maxVisible, totalUsers)) + 
                               "/" + to_string(totalUsers);
            Text scrollText(font, toUtf8(scrollInfo), 11);
            scrollText.setFillColor(Color(120, 120, 120));
            scrollText.setPosition(Vector2f(
                userListBg.getPosition().x + userListBg.getSize().x - 60.f,
                userListBg.getPosition().y + userListBg.getSize().y + 5.f
            ));
            window.draw(scrollText);
        }
        
        // Selected count label
        string countStr = "Đã chọn: " + to_string(selectedUserSet.size()) + " người";
        Text countText(font, toUtf8(countStr), 13);
        countText.setFillColor(accentColor);
        countText.setStyle(Text::Bold);
        countText.setPosition(Vector2f(labelX, contentStartY + 360.f));
        window.draw(countText);
        
        // "Chọn tất cả" button
        if (btnSelectAll) {
            btnSelectAll->setPosition(Vector2f(popupPos.x + 200.f, contentStartY + 355.f));
            btnSelectAll->draw(window);
        }
        
    } else {
        // Standard input layout for other popups
        for (size_t i = 0; i < inputLabels.size(); i++) {
            if (inputLabels[i]) {
                float currentY = contentStartY + i * rowHeight;
                inputLabels[i]->setPosition(Vector2f(labelX, currentY + 8.f));
            }
        }
        for (size_t i = 0; i < inputBoxes.size(); i++) {
            if (inputBoxes[i]) {
                float currentY = contentStartY + i * rowHeight;
                inputBoxes[i]->setPosition(Vector2f(inputX, currentY));
            }
        }
    }
    
    // Render input labels and boxes
    for (size_t i = 0; i < inputLabels.size(); i++) {
        if (inputLabels[i]) window.draw(*inputLabels[i]);
    }
    for (auto& textBox : inputBoxes) {
        if (textBox) textBox->draw(window);
    }
    
    // Position and render buttons at the bottom of popup
    float buttonY = popupPos.y + popupSize.y - 60.f;
    float buttonSpacing = 15.f;
    
    if (btnPopupSave && btnPopupCancel) {
        float totalBtnWidth = btnPopupSave->getGlobalBounds().size.x + btnPopupCancel->getGlobalBounds().size.x + buttonSpacing;
        float startX = popupPos.x + (popupSize.x - totalBtnWidth) / 2.f;
        
        btnPopupSave->setPosition(Vector2f(startX, buttonY));
        btnPopupCancel->setPosition(Vector2f(startX + btnPopupSave->getGlobalBounds().size.x + buttonSpacing, buttonY));
        
        btnPopupSave->draw(window);
        btnPopupCancel->draw(window);
    }
}

void VoucherPanel::renderNotification(RenderWindow& window) {
    if (!notificationVisible) return;
    
    if (notificationClock.getElapsedTime().asSeconds() > 3.f) {
        notificationVisible = false;
        return;
    }
    
    Vector2f notifPos(position.x + width - notificationBg.getSize().x - 30.f, position.y + 20.f);
    notificationBg.setPosition(notifPos);
    notificationBg.setFillColor(notificationColor);

    // Keep notification style consistent with other admin states (no rounded frame).
    window.draw(notificationBg);
    
    if (notificationText) {
        FloatRect bounds = notificationText->getLocalBounds();
        notificationText->setPosition(Vector2f(
            notifPos.x + (notificationBg.getSize().x - bounds.size.x) / 2.f - bounds.position.x,
            notifPos.y + (notificationBg.getSize().y - bounds.size.y) / 2.f - bounds.position.y
        ));
        window.draw(*notificationText);
    }
}

// ===== POPUP METHODS =====

void VoucherPanel::openAddVoucherPopup() {
    currentPopup = PopupType::ADD_VOUCHER;
    
    popupTitle->setString(toUtf8("Thêm Voucher mới"));
    
    inputBoxes.clear();
    inputLabels.clear();

    float inputWidth = 360.f;
    
    vector<string> labels = {"Mã voucher:", "Loại (1/2):", "Giá trị:", "HĐ tối thiểu:", "Mô tả:"};
    
    for (size_t i = 0; i < labels.size(); i++) {
        auto label = make_unique<Text>(font, toUtf8(labels[i]), 14);
        label->setFillColor(textColor);
        inputLabels.push_back(move(label));
        
        auto textBox = make_unique<TextBox>(font, inputWidth, 36.f);
        // Sample text / placeholder
        if (i == 0) textBox->setPlaceholder("Ví dụ: SALE10");
        if (i == 1) textBox->setPlaceholder("Nhập 1 hoặc 2");
        if (i == 2) textBox->setPlaceholder("Ví dụ: 50000 (type 1) hoặc 10 (type 2)");
        if (i == 3) textBox->setPlaceholder("Ví dụ: 200000");
        if (i == 4) textBox->setPlaceholder("Ví dụ: Giảm 10% cho đơn từ 200k");
        inputBoxes.push_back(move(textBox));
    }
    
    // Popup buttons
    btnPopupSave = make_unique<Button>(font, toUtf8("Lưu"), Vector2f(100.f, 40.f), 15);
    btnPopupSave->setColors(successColor, Color(60, 187, 89), Color::White);
    
    btnPopupCancel = make_unique<Button>(font, toUtf8("Hủy"), Vector2f(100.f, 40.f), 15);
    btnPopupCancel->setColors(Color(108, 117, 125), Color(128, 137, 145), Color::White);
    
    // Position elements (will be set in render based on popup position)
}

void VoucherPanel::openEditVoucherPopup() {
    auto defs = repository->getAllDefinitions();
    if (selectedVoucherIndex < 0 || selectedVoucherIndex >= static_cast<int>(defs.size())) return;
    
    const auto& def = defs[selectedVoucherIndex];
    
    currentPopup = PopupType::EDIT_VOUCHER;
    popupTitle->setString(toUtf8("Chỉnh sửa Voucher"));
    
    inputBoxes.clear();
    inputLabels.clear();
    
    vector<string> labels = {"Mã voucher:", "Loại (1/2):", "Giá trị:", "HĐ tối thiểu:", "Mô tả:"};
    vector<string> values = {
        def.code,
        to_string(def.type),
        to_string(static_cast<int>(def.value)),
        to_string(static_cast<int>(def.minBill)),
        def.description
    };
    
    float inputWidth = 360.f;
    
    for (size_t i = 0; i < labels.size(); i++) {
        auto label = make_unique<Text>(font, toUtf8(labels[i]), 14);
        label->setFillColor(textColor);
        inputLabels.push_back(move(label));
        
        auto textBox = make_unique<TextBox>(font, inputWidth, 36.f);
        textBox->setText(values[i]);
        if (i == 0) textBox->setEditable(false); // Code is not editable
        inputBoxes.push_back(move(textBox));
    }
    
    btnPopupSave = make_unique<Button>(font, toUtf8("Cập nhật"), Vector2f(100.f, 40.f), 15);
    btnPopupSave->setColors(accentColor, Color(40, 138, 192), Color::White);
    
    btnPopupCancel = make_unique<Button>(font, toUtf8("Hủy"), Vector2f(100.f, 40.f), 15);
    btnPopupCancel->setColors(Color(108, 117, 125), Color(128, 137, 145), Color::White);
}

void VoucherPanel::openDeleteVoucherPopup() {
    auto defs = repository->getAllDefinitions();
    if (selectedVoucherIndex < 0 || selectedVoucherIndex >= static_cast<int>(defs.size())) return;
    
    currentPopup = PopupType::DELETE_VOUCHER;
    
    string msg = "Xác nhận xóa voucher: " + defs[selectedVoucherIndex].code + "?";
    popupTitle->setString(toUtf8(msg));
    
    inputBoxes.clear();
    inputLabels.clear();
    
    btnPopupSave = make_unique<Button>(font, toUtf8("Xóa"), Vector2f(100.f, 40.f), 15);
    btnPopupSave->setColors(dangerColor, Color(240, 73, 89), Color::White);
    
    btnPopupCancel = make_unique<Button>(font, toUtf8("Hủy"), Vector2f(100.f, 40.f), 15);
    btnPopupCancel->setColors(Color(108, 117, 125), Color(128, 137, 145), Color::White);
}

void VoucherPanel::openDistributePopup() {
    auto defs = repository->getAllDefinitions();
    if (selectedVoucherIndex < 0 || selectedVoucherIndex >= static_cast<int>(defs.size())) return;
    
    currentPopup = PopupType::DISTRIBUTE_VOUCHER;
    
    string title = "Phát voucher: " + defs[selectedVoucherIndex].code;
    popupTitle->setString(toUtf8(title));
    
    inputBoxes.clear();
    inputLabels.clear();
    
    vector<string> labels = {"Số ngày hết hạn:", "Số lượng/người:"};
    vector<string> defaults = {"30", "1"};
    float inputWidth = 200.f;
    
    for (size_t i = 0; i < labels.size(); i++) {
        auto label = make_unique<Text>(font, toUtf8(labels[i]), 14);
        label->setFillColor(textColor);
        inputLabels.push_back(move(label));
        
        auto textBox = make_unique<TextBox>(font, inputWidth, 36.f);
        textBox->setText(defaults[i]);
        inputBoxes.push_back(move(textBox));
    }
    
    // Info text
    auto activeUsers = repository->getActiveUsers();
    string infoStr = "Sẽ phát cho " + to_string(activeUsers.size()) + " người dùng active";
    auto infoLabel = make_unique<Text>(font, toUtf8(infoStr), 13);
    infoLabel->setFillColor(accentColor);
    inputLabels.push_back(move(infoLabel));
    
    btnPopupSave = make_unique<Button>(font, toUtf8("Phát ngay"), Vector2f(120.f, 40.f), 15);
    btnPopupSave->setColors(warningColor, Color(255, 172, 40), Color::White);
    
    btnPopupCancel = make_unique<Button>(font, toUtf8("Hủy"), Vector2f(100.f, 40.f), 15);
    btnPopupCancel->setColors(Color(108, 117, 125), Color(128, 137, 145), Color::White);
}

void VoucherPanel::openAddToUserPopup() {
    // Use the new UserSelectPopup component
    if (!userSelectPopup_) {
        userSelectPopup_ = make_unique<UserSelectPopup>(font);
    }
    
    userSelectPopup_->setTitle("Thêm người nhận voucher: " + selectedVoucherCode);
    
    // Set extra fields (days to expire, quantity)
    userSelectPopup_->setExtraFields({
        {"Số ngày HH:", "30"},
        {"Số lượng:", "1"}
    });
    
    // Load available users (exclude those who already have this voucher)
    auto allUsers = repository->getActiveUsers();
    auto existingUsers = repository->getUsersWithVoucher(selectedVoucherCode);
    
    vector<UserInfo> filteredUsers;
    for (const auto& user : allUsers) {
        bool alreadyHas = false;
        for (const auto& existing : existingUsers) {
            if (existing.email == user.first) {
                alreadyHas = true;
                break;
            }
        }
        if (!alreadyHas) {
            filteredUsers.emplace_back(user.first, user.second);
        }
    }
    
    userSelectPopup_->setUsers(filteredUsers);
    
    // Set callbacks
    userSelectPopup_->setOnConfirm([this](const vector<UserInfo>& selectedUsers) {
        if (selectedUsers.empty()) {
            showNotification("Vui lòng chọn ít nhất một người dùng", dangerColor);
            return;
        }
        
        try {
            int daysToExpire = stoi(userSelectPopup_->getExtraFieldValue(0));
            int quantity = stoi(userSelectPopup_->getExtraFieldValue(1));
            
            if (daysToExpire <= 0 || quantity <= 0) {
                showNotification("Giá trị phải lớn hơn 0", dangerColor);
                return;
            }
            
            int successCount = 0;
            for (const auto& user : selectedUsers) {
                if (repository->giveVoucherToUser(user.email, selectedVoucherCode, daysToExpire, quantity)) {
                    successCount++;
                }
            }
            
            if (successCount > 0) {
                showNotification("Đã thêm voucher cho " + to_string(successCount) + " người", successColor);
                loadUsersForVoucher(selectedVoucherCode);
            } else {
                showNotification("Không thể thêm voucher", dangerColor);
            }
        } catch (...) {
            showNotification("Dữ liệu không hợp lệ", dangerColor);
        }
    });
    
    userSelectPopup_->setOnCancel([this]() {
        // Nothing to do on cancel
    });
    
    userSelectPopup_->open();
    
    // Keep the old popup type set to NONE since we're using the new component
    currentPopup = PopupType::NONE;
}

void VoucherPanel::closePopup() {
    currentPopup = PopupType::NONE;
    inputBoxes.clear();
    inputLabels.clear();
    btnPopupSave.reset();
    btnPopupCancel.reset();
    btnSelectAll.reset();
    searchBox.reset();
    availableUsers.clear();
    filteredAvailableUsers.clear();
    selectedUserSet.clear();
    selectedAvailableUserIndex = -1;
    userSearchFilter = "";
}

// ===== ACTION HANDLERS =====

void VoucherPanel::handleAddVoucher() {
    if (inputBoxes.size() < 5) return;
    
    string code = inputBoxes[0]->getText();
    string typeStr = inputBoxes[1]->getText();
    string valueStr = inputBoxes[2]->getText();
    string minBillStr = inputBoxes[3]->getText();
    string desc = inputBoxes[4]->getText();
    
    if (code.empty() || typeStr.empty() || valueStr.empty()) {
        showNotification("Vui lòng điền đầy đủ thông tin", dangerColor);
        return;
    }
    
    try {
        VoucherDefinition def;
        def.code = code;
        def.type = stoi(typeStr);
        def.value = stod(valueStr);
        def.minBill = minBillStr.empty() ? 0 : stod(minBillStr);
        def.description = desc;
        
        if (def.type != 1 && def.type != 2) {
            showNotification("Loại voucher phải là 1 hoặc 2", dangerColor);
            return;
        }
        
        if (repository->addDefinition(def)) {
            showNotification("Đã thêm voucher thành công", successColor);
            closePopup();
            refreshData();
        } else {
            showNotification("Mã voucher đã tồn tại", dangerColor);
        }
    } catch (...) {
        showNotification("Dữ liệu không hợp lệ", dangerColor);
    }
}

void VoucherPanel::handleEditVoucher() {
    if (inputBoxes.size() < 5) return;
    
    string code = inputBoxes[0]->getText();
    string typeStr = inputBoxes[1]->getText();
    string valueStr = inputBoxes[2]->getText();
    string minBillStr = inputBoxes[3]->getText();
    string desc = inputBoxes[4]->getText();
    
    try {
        VoucherDefinition def;
        def.code = code;
        def.type = stoi(typeStr);
        def.value = stod(valueStr);
        def.minBill = minBillStr.empty() ? 0 : stod(minBillStr);
        def.description = desc;
        
        if (repository->updateDefinition(def)) {
            showNotification("Đã cập nhật voucher", successColor);
            closePopup();
            refreshData();
        } else {
            showNotification("Không thể cập nhật voucher", dangerColor);
        }
    } catch (...) {
        showNotification("Dữ liệu không hợp lệ", dangerColor);
    }
}

void VoucherPanel::handleDeleteVoucher() {
    auto defs = repository->getAllDefinitions();
    if (selectedVoucherIndex < 0 || selectedVoucherIndex >= static_cast<int>(defs.size())) return;
    
    if (repository->deleteDefinition(defs[selectedVoucherIndex].code)) {
        showNotification("Đã xóa voucher", successColor);
        closePopup();
        refreshData();
    } else {
        showNotification("Không thể xóa voucher", dangerColor);
    }
}

void VoucherPanel::handleDistribute() {
    auto defs = repository->getAllDefinitions();
    if (selectedVoucherIndex < 0 || selectedVoucherIndex >= static_cast<int>(defs.size())) return;
    if (inputBoxes.size() < 2) return;
    
    try {
        int daysToExpire = stoi(inputBoxes[0]->getText());
        int quantity = stoi(inputBoxes[1]->getText());
        
        if (daysToExpire <= 0 || quantity <= 0) {
            showNotification("Giá trị phải lớn hơn 0", dangerColor);
            return;
        }
        
        int count = repository->giveVoucherToAllActiveUsers(
            defs[selectedVoucherIndex].code, 
            daysToExpire, 
            quantity
        );
        
        showNotification("Đã phát voucher cho " + to_string(count) + " người", successColor);
        closePopup();
        loadUsersForVoucher(defs[selectedVoucherIndex].code);
    } catch (...) {
        showNotification("Dữ liệu không hợp lệ", dangerColor);
    }
}

void VoucherPanel::handleAddToUser() {
    if (selectedUserSet.empty()) {
        showNotification("Vui lòng chọn ít nhất một người dùng", dangerColor);
        return;
    }
    
    if (inputBoxes.size() < 2) return;
    
    try {
        int daysToExpire = stoi(inputBoxes[0]->getText());
        int quantity = stoi(inputBoxes[1]->getText());
        
        if (daysToExpire <= 0 || quantity <= 0) {
            showNotification("Giá trị phải lớn hơn 0", dangerColor);
            return;
        }
        
        int successCount = 0;
        for (int userIdx : selectedUserSet) {
            if (userIdx >= 0 && userIdx < static_cast<int>(availableUsers.size())) {
                string email = availableUsers[userIdx].first;
                if (repository->giveVoucherToUser(email, selectedVoucherCode, daysToExpire, quantity)) {
                    successCount++;
                }
            }
        }
        
        if (successCount > 0) {
            showNotification("Đã thêm voucher cho " + to_string(successCount) + " người", successColor);
            closePopup();
            loadUsersForVoucher(selectedVoucherCode);
        } else {
            showNotification("Không thể thêm voucher", dangerColor);
        }
    } catch (...) {
        showNotification("Dữ liệu không hợp lệ", dangerColor);
    }
}

void VoucherPanel::handleRemoveFromUser() {
    if (selectedUserIndex < 0 || selectedUserIndex >= static_cast<int>(currentVoucherUsers.size())) return;
    
    string email = currentVoucherUsers[selectedUserIndex].email;
    
    if (repository->removeVoucherFromUser(email, selectedVoucherCode)) {
        showNotification("Đã thu hồi voucher", successColor);
        loadUsersForVoucher(selectedVoucherCode);
        selectedUserIndex = -1;
    } else {
        showNotification("Không thể thu hồi voucher", dangerColor);
    }
}

// ===== HELPERS =====

void VoucherPanel::showNotification(const string& message, const Color& color) {
    notificationMessage = message;
    notificationColor = color;
    notificationVisible = true;
    notificationClock.restart();
    
    if (notificationText) {
        notificationText->setString(toUtf8(message));
    }
}

string VoucherPanel::formatCurrency(double amount) const {
    long long value = static_cast<long long>(round(amount));
    string digits = to_string(abs(value));
    string formatted;
    
    for (size_t i = 0; i < digits.size(); i++) {
        if (i > 0 && (digits.size() - i) % 3 == 0) {
            formatted.push_back('.');
        }
        formatted.push_back(digits[i]);
    }
    
    if (value < 0) formatted.insert(formatted.begin(), '-');
    formatted += "đ";
    
    return formatted;
}

string VoucherPanel::formatDate(const string& raw) const {
    if (raw.size() != 8) return raw;
    return raw.substr(6, 2) + "/" + raw.substr(4, 2) + "/" + raw.substr(0, 4);
}

string VoucherPanel::getTypeString(int type) const {
    return type == 1 ? "Cố định" : "Phần %";
}

void VoucherPanel::filterUsers(const string& query) {
    filteredAvailableUsers.clear();
    availableUserScrollOffset = 0;
    
    string lowerQuery = query;
    transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), ::tolower);
    
    for (int i = 0; i < static_cast<int>(availableUsers.size()); i++) {
        string email = availableUsers[i].first;
        string name = availableUsers[i].second;
        
        // Convert to lowercase for case-insensitive search
        transform(email.begin(), email.end(), email.begin(), ::tolower);
        transform(name.begin(), name.end(), name.begin(), ::tolower);
        
        if (query.empty() || email.find(lowerQuery) != string::npos || name.find(lowerQuery) != string::npos) {
            filteredAvailableUsers.push_back(i);
        }
    }
}
