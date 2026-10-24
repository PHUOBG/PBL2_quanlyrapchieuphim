#include "UI/components/Admin/AdminSidebar.h"
#include <iostream>

Vector2f SidebarRoundRectButton::getSize() const {
    return size;
}

AdminSidebar::MenuItem::MenuItem(const string& l, AppState state, bool header, bool logout)
    : label(l), targetState(state), isGroupHeader(header), isLogout(logout) {}

SidebarRoundRectButton::SidebarRoundRectButton(Font& f, const string& text, Vector2f sz, float r)
    : font(f), size(sz), position(Vector2f()), radius(r), baseColor(Color::White),
      hoverColor(Color::White), hovered(false) {
    label = make_unique<Text>(font);
    label->setCharacterSize(27);
    label->setFillColor(Color::White);
    label->setString(String::fromUtf8(text.begin(), text.end()));
}

void SidebarRoundRectButton::setPosition(Vector2f pos) {
    position = pos;
    if (label) {
        FloatRect bounds = label->getLocalBounds();
        label->setPosition(Vector2f(
            position.x + (size.x - bounds.size.x) / 2.f - bounds.position.x,
            position.y + (size.y - bounds.size.y) / 2.f - bounds.position.y
        ));
    }
}

void SidebarRoundRectButton::setColors(const Color& base, const Color& hover) {
    baseColor = base;
    hoverColor = hover;
}

void SidebarRoundRectButton::setTextSize(unsigned int size) {
    if (label) {
        label->setCharacterSize(size);
        setPosition(position);
    }
}

void SidebarRoundRectButton::update(Vector2f mousePos) {
    FloatRect bounds(position, size);
    hovered = bounds.contains(mousePos);
}

bool SidebarRoundRectButton::contains(Vector2f point) const {
    FloatRect bounds(position, size);
    return bounds.contains(point);
}

void SidebarRoundRectButton::draw(RenderWindow& window) const {
    const Color& color = hovered ? hoverColor : baseColor;
    RoundedRectRenderer::draw(window, position, size, radius, color);
    if (label) {
        window.draw(*label);
    }
}

AdminSidebar::AdminSidebar(float w, float h, const string& uName)
    : width(w), height(h), hoveredIndex(-1), activeIndex(0),
    font("../assets/fonts/AMPLE_MEDIUM.ttf"),
    montserratFont("../assets/fonts/Montserrat_SemiBold.ttf"),
    userName(uName) {
    
    // Colors
    bgColor = Color(13, 27, 42);         // #0D1B2A
    textColor = Color(233, 238, 245);    // #E9EEF5
    hoverColor = Color(27, 38, 59);      // #1B263B
    activeColor = Color(20, 118, 172);   // #1476AC
    groupHeaderColor = Color(50, 60, 80);
    
    initializeMenuItems();
    setupGraphics();
}

float AdminSidebar::getWidth() const {
    return width;
}

void AdminSidebar::initializeMenuItems() {
    menuItems.clear();
    
    // Header (user info area - will be handled separately)
    
    // Group 1: QUáº¢N LÃ Há»† THá»NG
    menuItems.push_back(MenuItem("QUẢN LÝ HỆ THỐNG", AppState::ADMIN_DASHBOARD, true, false));
    menuItems.push_back(MenuItem("Quản lý phim", AppState::ADMIN_MOVIES));
    menuItems.push_back(MenuItem("Quản lý phòng chiếu", AppState::ADMIN_ROOMS));
    menuItems.push_back(MenuItem("Quản lý suất chiếu", AppState::ADMIN_SHOWTIMES));
    menuItems.push_back(MenuItem("Quản lý vé", AppState::ADMIN_TICKETS));
    menuItems.push_back(MenuItem("Quản lý combo bắp nước", AppState::ADMIN_COMBOS));
    menuItems.push_back(MenuItem("Quản lý voucher", AppState::ADMIN_VOUCHERS));
    
    // Group 2: THá»NG KÃŠ & BÃO CÃO
    menuItems.push_back(MenuItem("THỐNG KÊ & BÁO CÁO", AppState::ADMIN_DASHBOARD, true, false));
    menuItems.push_back(MenuItem("Tổng quan", AppState::ADMIN_REVENUE));
    menuItems.push_back(MenuItem("Báo cáo theo phim", AppState::ADMIN_SOLD_TICKETS));
    
    // Group 3: TÃ€I KHOáº¢N
    menuItems.push_back(MenuItem("TÀI KHOẢN", AppState::ADMIN_DASHBOARD, true, false));
    menuItems.push_back(MenuItem("Quản lý khách hàng", AppState::ADMIN_CUSTOMERS));
    
    // Group 4: Há»† THá»NG
    menuItems.push_back(MenuItem("HỆ THỐNG", AppState::ADMIN_DASHBOARD, true, false));
    menuItems.push_back(MenuItem("Thông tin cá nhân", AppState::ADMIN_CHANGE_PASSWORD));
}

void AdminSidebar::setupGraphics() {
    // Background
    background.setSize(Vector2f(width, height));
    background.setFillColor(bgColor);
    
    // User info background
    userInfoBg.setSize(Vector2f(width, 80));
    userInfoBg.setFillColor(Color(10, 20, 35)); // Slightly darker
    
    // User name text - SFML 3 requires font in constructor, use unique_ptr
    userNameText = make_unique<Text>(font);
    userNameText->setCharacterSize(16);
    userNameText->setFillColor(textColor);
    userNameText->setString(String::fromUtf8(userName.begin(), userName.end()));
    
    // Header text uses Montserrat SemiBold
    headerText = make_unique<Text>(montserratFont);
    headerText->setCharacterSize(24);
    headerText->setFillColor(textColor);
    headerText->setString(L"PBL2 - ADMIN CINEMA");
    headerText->setStyle(Text::Bold);
    
    // Create item backgrounds and texts
    itemBackgrounds.clear();
    itemTexts.clear();
    
    float yOffset = 100; // Start after user info
    float itemHeight = 45;
    float groupHeaderHeight = 35;
    
    for (int i = 0; i < menuItems.getSize(); i++) {
        const auto& item = menuItems[i];
        
        float currentHeight = item.isGroupHeader ? groupHeaderHeight : itemHeight;
        
        // Background
        RectangleShape itemBg;
        itemBg.setSize(Vector2f(width - 20, currentHeight - 5));
        itemBg.setPosition(Vector2f(10, yOffset)); // SFML 3: use Vector2f
        itemBg.setFillColor(Color::Transparent);
        
        if (item.isGroupHeader) {
            itemBg.setFillColor(groupHeaderColor);
        }
        
        itemBackgrounds.push_back(itemBg);
        
        // Text - SFML 3 requires font in constructor, use unique_ptr
        Font& itemFont = item.isGroupHeader ? montserratFont : font;
        auto itemText = make_unique<Text>(itemFont);
        itemText->setCharacterSize(item.isGroupHeader ? 19 : 21);
        itemText->setFillColor(textColor);
        
        string displayText = item.label;
        itemText->setString(String::fromUtf8(displayText.begin(), displayText.end()));
        
        // SFML 3: getLocalBounds() returns FloatRect, use .size.y instead of .height
        itemText->setPosition(Vector2f(
            item.isGroupHeader ? 15.f : 25.f,
            yOffset + (currentHeight - itemText->getLocalBounds().size.y) / 2 - 5
        ));
        
        if (item.isGroupHeader) {
            itemText->setStyle(Text::Bold);
        }
        
        itemTexts.push_back(std::move(itemText));
        
        yOffset += currentHeight;
    }

    logoutButton = make_unique<SidebarRoundRectButton>(font, "Đăng xuất", Vector2f(width - 40.f, 50.f), 12.f);
    if (logoutButton) {
        logoutButton->setColors(Color(72, 201, 176), Color(102, 231, 206));
    }
}

void AdminSidebar::setPosition(Vector2f pos) {
    position = pos;
    background.setPosition(pos);
    userInfoBg.setPosition(Vector2f(pos.x, pos.y + 10)); // SFML 3: use Vector2f
    if (headerText) headerText->setPosition(Vector2f(pos.x + 15, pos.y + 20));
    if (userNameText) userNameText->setPosition(Vector2f(pos.x + 15, pos.y + 55));
    
    // Update item positions
    for (int i = 0; i < itemBackgrounds.getSize(); i++) {
        Vector2f itemPos = itemBackgrounds[i].getPosition();
        itemBackgrounds[i].setPosition(Vector2f(pos.x + itemPos.x, pos.y + itemPos.y));
        
        if (i < itemTexts.getSize() && itemTexts[i]) {
            Vector2f textPos = itemTexts[i]->getPosition();
            itemTexts[i]->setPosition(Vector2f(pos.x + textPos.x, pos.y + textPos.y));
        }
    }

    if (logoutButton) {
        float buttonHeight = logoutButton->getSize().y;
        float bottomPadding = 20.f;
        logoutButton->setPosition(Vector2f(pos.x + 20.f, pos.y + height - buttonHeight - bottomPadding));
    }
}

void AdminSidebar::setOnStateChange(function<void(AppState)> callback) {
    onStateChange = callback;
}

void AdminSidebar::setOnLogout(function<void()> callback) {
    onLogout = callback;
}

void AdminSidebar::setActiveState(AppState state) {
    for (int i = 0; i < menuItems.getSize(); i++) {
        if (menuItems[i].targetState == state && !menuItems[i].isGroupHeader) {
            activeIndex = i;
            break;
        }
    }
}

int AdminSidebar::getItemAtPosition(Vector2f mousePos) {
    for (int i = 0; i < itemBackgrounds.getSize(); i++) {
        if (menuItems[i].isGroupHeader) continue;
        
        FloatRect bounds = itemBackgrounds[i].getGlobalBounds();
        if (bounds.contains(mousePos)) {
            return i;
        }
    }
    return -1;
}

void AdminSidebar::handleEvent(const Event& event, const RenderWindow& window) {
    // SFML 3: Use event.is<Event::Type>() instead of event.type
    if (const auto* mouseButtonEvent = event.getIf<Event::MouseButtonPressed>()) {
        if (mouseButtonEvent->button == Mouse::Button::Left) { // SFML 3: Mouse::Button::Left
            Vector2f mousePos = window.mapPixelToCoords(
                Mouse::getPosition(window)
            );

            if (logoutButton && logoutButton->contains(mousePos)) {
                if (onLogout) {
                    onLogout();
                }
                return;
            }
            
            int clickedIndex = getItemAtPosition(mousePos);
            
            if (clickedIndex >= 0) {
                const auto& item = menuItems[clickedIndex];
                
                // Change state
                activeIndex = clickedIndex;
                if (onStateChange) {
                    onStateChange(item.targetState);
                }
            }
        }
    }
}

void AdminSidebar::update(const RenderWindow& window) {
    Vector2f mousePos = window.mapPixelToCoords(Mouse::getPosition(window));
    hoveredIndex = getItemAtPosition(mousePos);
    
    // Update item backgrounds based on hover/active state
    for (int i = 0; i < itemBackgrounds.getSize(); i++) {
        if (menuItems[i].isGroupHeader) {
            itemBackgrounds[i].setFillColor(groupHeaderColor);
        } else if (i == activeIndex) {
            itemBackgrounds[i].setFillColor(activeColor);
        } else if (i == hoveredIndex) {
            itemBackgrounds[i].setFillColor(hoverColor);
        } else {
            itemBackgrounds[i].setFillColor(Color::Transparent);
        }
    }

    if (logoutButton) {
        logoutButton->update(mousePos);
    }
}

void AdminSidebar::render(RenderWindow& window) {
    // Draw background
    window.draw(background);
    
    // Draw user info section
    window.draw(userInfoBg);
    if (headerText) window.draw(*headerText);
    // if (userNameText) window.draw(*userNameText);
    
    // Draw menu items
    for (int i = 0; i < itemBackgrounds.getSize(); i++) {
        window.draw(itemBackgrounds[i]);
        if (i < itemTexts.getSize() && itemTexts[i]) {
            window.draw(*itemTexts[i]);
        }
    }

    if (logoutButton) {
        logoutButton->draw(window);
    }
}
