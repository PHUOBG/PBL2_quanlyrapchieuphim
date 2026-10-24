#include "UI/components/Admin/UserSelectPopup.h"
#include "UI/components/Admin/RoundedRectRenderer.h"
#include <algorithm>
#include <cstdint>

UserInfo::UserInfo(const std::string& e, const std::string& n)
    : email(e), name(n) {}

UserSelectPopup::UserSelectPopup(sf::Font& font)
    : font_(font)
    , titleText_(std::make_unique<sf::Text>(font))
    , selectedCountText_(std::make_unique<sf::Text>(font))
{
    setupUI();
}

bool UserSelectPopup::isOpen() const {
    return isOpen_;
}

size_t UserSelectPopup::getSelectedCount() const {
    return selectedIndices_.size();
}

void UserSelectPopup::setOnConfirm(ConfirmCallback callback) {
    onConfirm_ = std::move(callback);
}

void UserSelectPopup::setOnCancel(CancelCallback callback) {
    onCancel_ = std::move(callback);
}

void UserSelectPopup::setupUI() {
    // Overlay
    overlay_.setFillColor(sf::Color(0, 0, 0, 150));
    
    // Background
    background_.setSize({POPUP_WIDTH, POPUP_HEIGHT});
    background_.setFillColor(bgColor_);
    background_.setOutlineColor(borderColor_);
    background_.setOutlineThickness(2.f);
    
    // Title
    titleText_->setCharacterSize(18);
    titleText_->setFillColor(headerColor_);
    titleText_->setStyle(sf::Text::Bold);
    
    // Selected count text
    selectedCountText_->setCharacterSize(13);
    selectedCountText_->setFillColor(accentColor_);
    selectedCountText_->setStyle(sf::Text::Bold);
    
    // Search box
    UIInputBox::Config searchConfig;
    searchConfig.width = POPUP_WIDTH - 2 * PADDING - 130.f; // Leave room for label
    searchConfig.height = 36.f;
    searchBox_ = std::make_unique<UIInputBox>(font_, searchConfig);
    searchBox_->setPlaceholder("Tìm theo email hoặc tên...");
    
    // List background
    listBackground_.setSize({POPUP_WIDTH - 2 * PADDING, LIST_HEIGHT});
    listBackground_.setFillColor(sf::Color(248, 250, 252));
    listBackground_.setOutlineColor(borderColor_);
    listBackground_.setOutlineThickness(1.f);
    
    // Buttons
    btnSelectAll_ = std::make_unique<Button>(font_, toUtf8("Chọn tất cả"), sf::Vector2f(120.f, 32.f), 13);
    btnSelectAll_->setColors(accentColor_, sf::Color(40, 138, 192), sf::Color::White);
    
    btnConfirm_ = std::make_unique<Button>(font_, toUtf8("Xác nhận"), sf::Vector2f(100.f, 40.f), 15);
    btnConfirm_->setColors(successColor_, sf::Color(60, 187, 89), sf::Color::White);
    
    btnCancel_ = std::make_unique<Button>(font_, toUtf8("Hủy"), sf::Vector2f(100.f, 40.f), 15);
    btnCancel_->setColors(grayColor_, sf::Color(128, 137, 145), sf::Color::White);
}

void UserSelectPopup::setUsers(const std::vector<UserInfo>& users) {
    allUsers_ = users;
    selectedIndices_.clear();
    searchFilter_.clear();
    scrollOffset_ = 0.f;
    
    // Initialize filtered list with all users
    filteredIndices_.clear();
    for (size_t i = 0; i < allUsers_.size(); ++i) {
        filteredIndices_.push_back(i);
    }
    
    if (searchBox_) {
        searchBox_->setText("");
    }
}

void UserSelectPopup::setTitle(const std::string& title) {
    title_ = title;
    if (titleText_) {
        titleText_->setString(toUtf8(title_));
    }
}

void UserSelectPopup::setExtraFields(const std::vector<std::pair<std::string, std::string>>& fields) {
    extraFields_.clear();
    
    UIInputBox::Config fieldConfig;
    fieldConfig.width = 80.f;
    fieldConfig.height = 36.f;
    
    for (const auto& [label, defaultValue] : fields) {
        auto inputBox = std::make_unique<UIInputBox>(font_, fieldConfig);
        inputBox->setText(defaultValue);
        extraFields_.emplace_back(label, std::move(inputBox));
    }
}

std::string UserSelectPopup::getExtraFieldValue(size_t index) const {
    if (index < extraFields_.size() && extraFields_[index].second) {
        return extraFields_[index].second->getText();
    }
    return "";
}

void UserSelectPopup::open() {
    isOpen_ = true;
    scrollOffset_ = 0.f;
    hoveredIndex_ = -1;
}

void UserSelectPopup::close() {
    isOpen_ = false;
}

std::vector<UserInfo> UserSelectPopup::getSelectedUsers() const {
    std::vector<UserInfo> result;
    for (size_t idx : selectedIndices_) {
        if (idx < allUsers_.size()) {
            result.push_back(allUsers_[idx]);
        }
    }
    return result;
}

void UserSelectPopup::filterUsers() {
    filteredIndices_.clear();
    scrollOffset_ = 0.f;
    
    std::string lowerQuery = searchFilter_;
    std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), ::tolower);
    
    for (size_t i = 0; i < allUsers_.size(); ++i) {
        std::string email = allUsers_[i].email;
        std::string name = allUsers_[i].name;
        
        std::transform(email.begin(), email.end(), email.begin(), ::tolower);
        std::transform(name.begin(), name.end(), name.begin(), ::tolower);
        
        if (searchFilter_.empty() || 
            email.find(lowerQuery) != std::string::npos || 
            name.find(lowerQuery) != std::string::npos) {
            filteredIndices_.push_back(i);
        }
    }
}

void UserSelectPopup::toggleSelection(size_t index) {
    if (selectedIndices_.count(index)) {
        selectedIndices_.erase(index);
    } else {
        selectedIndices_.insert(index);
    }
}

void UserSelectPopup::selectAll() {
    for (size_t idx : filteredIndices_) {
        selectedIndices_.insert(idx);
    }
}

void UserSelectPopup::deselectAll() {
    selectedIndices_.clear();
}

void UserSelectPopup::handleEvent(const sf::Event& event, const sf::RenderWindow& window) {
    if (!isOpen_) return;
    
    // Calculate popup position
    sf::Vector2f popupPos(
        (static_cast<float>(window.getSize().x) - POPUP_WIDTH) / 2.f,
        (static_cast<float>(window.getSize().y) - POPUP_HEIGHT) / 2.f
    );
    
    // Handle mouse click
    if (const auto* click = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (click->button == sf::Mouse::Button::Left) {
            sf::Vector2f mousePos(static_cast<float>(click->position.x), 
                                  static_cast<float>(click->position.y));
            
            // Handle input box clicks
            if (searchBox_) {
                searchBox_->update(mousePos, true);
            }
            
            for (auto& [label, inputBox] : extraFields_) {
                if (inputBox) {
                    inputBox->update(mousePos, true);
                }
            }
            
            // Handle list click (checkbox toggle)
            if (listBackground_.getGlobalBounds().contains(mousePos)) {
                float localY = mousePos.y - listBackground_.getPosition().y;
                int visualIdx = static_cast<int>(localY / ITEM_HEIGHT);
                int actualIdx = visualIdx + static_cast<int>(scrollOffset_ / ITEM_HEIGHT);
                
                if (actualIdx >= 0 && actualIdx < static_cast<int>(filteredIndices_.size())) {
                    size_t userIdx = filteredIndices_[static_cast<size_t>(actualIdx)];
                    toggleSelection(userIdx);
                }
            }
            
            // Handle Select All button
            if (btnSelectAll_ && btnSelectAll_->isClicked(mousePos)) {
                if (selectedIndices_.size() == filteredIndices_.size()) {
                    deselectAll();
                    btnSelectAll_->setText(toUtf8("Chọn tất cả"));
                } else {
                    selectAll();
                    btnSelectAll_->setText(toUtf8("Bỏ chọn"));
                }
            }
            
            // Handle Confirm button
            if (btnConfirm_ && btnConfirm_->isClicked(mousePos)) {
                if (onConfirm_) {
                    onConfirm_(getSelectedUsers());
                }
                close();
            }
            
            // Handle Cancel button
            if (btnCancel_ && btnCancel_->isClicked(mousePos)) {
                if (onCancel_) {
                    onCancel_();
                }
                close();
            }
        }
    }
    
    // Handle text input
    if (searchBox_) {
        searchBox_->handleEvent(event);
        
        std::string newFilter = searchBox_->getText();
        if (newFilter != searchFilter_) {
            searchFilter_ = newFilter;
            filterUsers();
        }
    }
    
    for (auto& [label, inputBox] : extraFields_) {
        if (inputBox) {
            inputBox->handleEvent(event);
        }
    }
    
    // Handle scroll
    if (const auto* scroll = event.getIf<sf::Event::MouseWheelScrolled>()) {
        sf::Vector2f mousePos(static_cast<float>(scroll->position.x), 
                              static_cast<float>(scroll->position.y));
        
        if (listBackground_.getGlobalBounds().contains(mousePos)) {
            float maxScroll = std::max(0.f, 
                static_cast<float>(filteredIndices_.size()) * ITEM_HEIGHT - LIST_HEIGHT);
            
            scrollOffset_ -= scroll->delta * ITEM_HEIGHT;
            scrollOffset_ = std::max(0.f, std::min(scrollOffset_, maxScroll));
        }
    }
}

void UserSelectPopup::update(sf::Vector2f mousePos, bool mousePressed) {
    if (!isOpen_) return;
    
    (void)mousePressed;
    
    // Update hover for list
    hoveredIndex_ = -1;
    if (listBackground_.getGlobalBounds().contains(mousePos)) {
        float localY = mousePos.y - listBackground_.getPosition().y;
        int visualIdx = static_cast<int>(localY / ITEM_HEIGHT);
        int actualIdx = visualIdx + static_cast<int>(scrollOffset_ / ITEM_HEIGHT);
        
        if (actualIdx >= 0 && actualIdx < static_cast<int>(filteredIndices_.size())) {
            hoveredIndex_ = actualIdx;
        }
    }
    
    // Update buttons
    if (btnSelectAll_) btnSelectAll_->update(mousePos);
    if (btnConfirm_) btnConfirm_->update(mousePos);
    if (btnCancel_) btnCancel_->update(mousePos);
    
    // Update input boxes
    if (searchBox_) searchBox_->update(mousePos);
    for (auto& [label, inputBox] : extraFields_) {
        if (inputBox) inputBox->update(mousePos);
    }
}

void UserSelectPopup::render(sf::RenderWindow& window) {
    if (!isOpen_) return;
    
    // Draw overlay
    overlay_.setSize(sf::Vector2f(static_cast<float>(window.getSize().x), 
                                   static_cast<float>(window.getSize().y)));
    overlay_.setPosition({0, 0});
    window.draw(overlay_);
    
    // Calculate popup position
    sf::Vector2f popupPos(
        (static_cast<float>(window.getSize().x) - POPUP_WIDTH) / 2.f,
        (static_cast<float>(window.getSize().y) - POPUP_HEIGHT) / 2.f
    );
    
    // Draw background
    RoundedRectRenderer::draw(window, popupPos, {POPUP_WIDTH, POPUP_HEIGHT}, 12.f, bgColor_);
    
    // Draw title
    titleText_->setString(toUtf8(title_));
    titleText_->setPosition({popupPos.x + PADDING, popupPos.y + 20.f});
    window.draw(*titleText_);
    
    // Content start position
    float contentY = popupPos.y + 60.f;
    
    // Draw extra fields (horizontal layout)
    float fieldX = popupPos.x + PADDING;
    for (auto& [label, inputBox] : extraFields_) {
        sf::Text labelText(font_, toUtf8(label), 14);
        labelText.setFillColor(textColor_);
        labelText.setPosition({fieldX, contentY + 8.f});
        window.draw(labelText);
        
        fieldX += labelText.getLocalBounds().size.x + 10.f;
        inputBox->setPosition({fieldX, contentY});
        inputBox->render(window);
        
        fieldX += inputBox->getWidth() + 25.f;
    }
    
    // Search box
    sf::Text searchLabel(font_, toUtf8("Tìm kiếm:"), 14);
    searchLabel.setFillColor(textColor_);
    searchLabel.setPosition({popupPos.x + PADDING, contentY + 55.f});
    window.draw(searchLabel);
    
    searchBox_->setPosition({popupPos.x + PADDING + 80.f, contentY + 50.f});
    searchBox_->render(window);
    
    // User list label
    sf::Text listLabel(font_, toUtf8("Chọn người dùng:"), 14);
    listLabel.setFillColor(textColor_);
    listLabel.setStyle(sf::Text::Bold);
    listLabel.setPosition({popupPos.x + PADDING, contentY + 100.f});
    window.draw(listLabel);
    
    // Position list background
    listBackground_.setPosition({popupPos.x + PADDING, contentY + 125.f});
    window.draw(listBackground_);
    
    // Render user list
    renderUserList(window);
    
    // Selected count and Select All button (below/outside the list frame)
    const float listBottomY = listBackground_.getPosition().y + listBackground_.getSize().y;
    const float actionsTopY = listBottomY + 10.f;

    // Left: selected count
    std::string countStr = "Đã chọn: " + std::to_string(selectedIndices_.size()) + " người";
    selectedCountText_->setString(toUtf8(countStr));
    selectedCountText_->setPosition({popupPos.x + PADDING, actionsTopY + 6.f});
    window.draw(*selectedCountText_);

    // Center: select all / deselect button
    const float btnW = btnSelectAll_->getGlobalBounds().size.x;
    btnSelectAll_->setPosition({popupPos.x + (POPUP_WIDTH - btnW) / 2.f, actionsTopY});
    btnSelectAll_->draw(window);
    
    // Scroll indicator
    if (filteredIndices_.size() > MAX_VISIBLE_ITEMS) {
        int startIdx = static_cast<int>(scrollOffset_ / ITEM_HEIGHT);
        int endIdx = std::min(startIdx + MAX_VISIBLE_ITEMS, static_cast<int>(filteredIndices_.size()));
        std::string scrollInfo = std::to_string(startIdx + 1) + "-" + 
                                 std::to_string(endIdx) + "/" + 
                                 std::to_string(filteredIndices_.size());
        sf::Text scrollText(font_, toUtf8(scrollInfo), 11);
        scrollText.setFillColor(sf::Color(120, 120, 120));
        scrollText.setPosition({listBackground_.getPosition().x + listBackground_.getSize().x - 60.f,
                               listBackground_.getPosition().y + listBackground_.getSize().y + 5.f});
        window.draw(scrollText);
    }
    
    // Buttons at bottom
    float buttonY = popupPos.y + POPUP_HEIGHT - 60.f;
    float buttonSpacing = 15.f;
    float totalBtnWidth = 100.f + 100.f + buttonSpacing;
    float startX = popupPos.x + (POPUP_WIDTH - totalBtnWidth) / 2.f;
    
    btnConfirm_->setPosition({startX, buttonY});
    btnCancel_->setPosition({startX + 100.f + buttonSpacing, buttonY});
    
    btnConfirm_->draw(window);
    btnCancel_->draw(window);
}

void UserSelectPopup::renderUserList(sf::RenderWindow& window) {
    // Save current view
    sf::View defaultView = window.getView();
    
    // Create clipped view for list
    sf::FloatRect listArea(listBackground_.getPosition(), listBackground_.getSize());
    
    sf::View clippedView;
    clippedView.setViewport(sf::FloatRect(
        sf::Vector2f(listArea.position.x / static_cast<float>(window.getSize().x),
                     listArea.position.y / static_cast<float>(window.getSize().y)),
        sf::Vector2f(listArea.size.x / static_cast<float>(window.getSize().x),
                     listArea.size.y / static_cast<float>(window.getSize().y))
    ));
    clippedView.setCenter({listArea.position.x + listArea.size.x / 2.f,
                          listArea.position.y + listArea.size.y / 2.f});
    clippedView.setSize(listArea.size);
    
    window.setView(clippedView);
    
    // Calculate visible range
    int startIdx = static_cast<int>(scrollOffset_ / ITEM_HEIGHT);
    int endIdx = std::min(startIdx + MAX_VISIBLE_ITEMS + 1, static_cast<int>(filteredIndices_.size()));
    
    for (int i = startIdx; i < endIdx; ++i) {
        size_t userIdx = filteredIndices_[static_cast<size_t>(i)];
        const UserInfo& user = allUsers_[userIdx];
        
        float itemY = listArea.position.y + static_cast<float>(i - startIdx) * ITEM_HEIGHT;
        
        // Item background
        sf::RectangleShape itemBg({listArea.size.x - 4.f, ITEM_HEIGHT - 2.f});
        itemBg.setPosition({listArea.position.x + 2.f, itemY + 1.f});
        
        bool isSelected = selectedIndices_.count(userIdx) > 0;
        
        if (isSelected) {
            itemBg.setFillColor(selectedColor_);
        } else if (i == hoveredIndex_) {
            itemBg.setFillColor(hoverColor_);
        } else {
            itemBg.setFillColor((i % 2 == 0) ? rowColor_ : rowAltColor_);
        }
        window.draw(itemBg);
        
        // Selection indicator (circle)
        // Avoid glyph-based checkmarks to prevent font/encoding issues.
        const float indicatorBoxSize = 18.f;
        const float indicatorRadius = 7.f;
        const sf::Vector2f indicatorBoxPos(listArea.position.x + 10.f, itemY + 9.f);
        sf::CircleShape indicator(indicatorRadius);
        indicator.setPosition({
            indicatorBoxPos.x + (indicatorBoxSize - 2.f * indicatorRadius) / 2.f,
            indicatorBoxPos.y + (indicatorBoxSize - 2.f * indicatorRadius) / 2.f
        });
        indicator.setFillColor(isSelected ? successColor_ : sf::Color::White);
        indicator.setOutlineColor(isSelected ? successColor_ : borderColor_);
        indicator.setOutlineThickness(1.5f);
        window.draw(indicator);
        
        // Email
        std::string email = user.email;
        if (email.length() > 28) {
            email = email.substr(0, 25) + "...";
        }
        sf::Text emailText(font_, toUtf8(email), 13);
        emailText.setFillColor(textColor_);
        emailText.setPosition({listArea.position.x + 38.f, itemY + 10.f});
        window.draw(emailText);
        
        // Name
        std::string name = user.name;
        if (name.length() > 20) {
            name = name.substr(0, 17) + "...";
        }
        sf::Text nameText(font_, toUtf8(name), 11);
        nameText.setFillColor(sf::Color(100, 100, 100));
        nameText.setPosition({listArea.position.x + 300.f, itemY + 11.f});
        window.draw(nameText);
    }
    
    // Restore default view
    window.setView(defaultView);
    
    // Empty state
    if (filteredIndices_.empty()) {
        sf::Text emptyText(font_, toUtf8("Không có người dùng khả dụng"), 14);
        emptyText.setFillColor(sf::Color(150, 150, 150));
        sf::FloatRect bounds = emptyText.getLocalBounds();
        emptyText.setPosition({
            listBackground_.getPosition().x + (listBackground_.getSize().x - bounds.size.x) / 2.f,
            listBackground_.getPosition().y + 100.f
        });
        window.draw(emptyText);
    }
}

sf::String UserSelectPopup::toUtf8(const std::string& text) {
    sf::String result;
    size_t i = 0;
    while (i < text.size()) {
        uint32_t codepoint = 0;
        unsigned char c = text[i];
        
        if ((c & 0x80) == 0) {
            codepoint = c;
            i += 1;
        } else if ((c & 0xE0) == 0xC0) {
            codepoint = (c & 0x1F) << 6;
            if (i + 1 < text.size()) {
                codepoint |= (text[i + 1] & 0x3F);
            }
            i += 2;
        } else if ((c & 0xF0) == 0xE0) {
            codepoint = (c & 0x0F) << 12;
            if (i + 1 < text.size()) {
                codepoint |= (text[i + 1] & 0x3F) << 6;
            }
            if (i + 2 < text.size()) {
                codepoint |= (text[i + 2] & 0x3F);
            }
            i += 3;
        } else if ((c & 0xF8) == 0xF0) {
            codepoint = (c & 0x07) << 18;
            if (i + 1 < text.size()) {
                codepoint |= (text[i + 1] & 0x3F) << 12;
            }
            if (i + 2 < text.size()) {
                codepoint |= (text[i + 2] & 0x3F) << 6;
            }
            if (i + 3 < text.size()) {
                codepoint |= (text[i + 3] & 0x3F);
            }
            i += 4;
        } else {
            i += 1;
            continue;
        }
        
        result += static_cast<char32_t>(codepoint);
    }
    return result;
}
