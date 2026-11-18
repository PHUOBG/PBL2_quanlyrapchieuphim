#include "UI/components/Admin/AdminSidebar.h"
#include "UI/components/Admin/FontManager.h"
#include <cmath>

AdminSidebar::AdminSidebar(float w, float h, const string& uName)
    : width(w), height(h), position(0.f, 0.f),
      font(FontManager::instance().regular()),
      boldFont(FontManager::instance().bold()),
      userName(uName),
      activeState(AppState::ADMIN_MOVIES),
      hoveredIndex(-1),
      logoutHovered(false)
{
    // Cấu trúc danh mục và chức năng
    elements = {
        // Section 1: Quản lý hệ thống
        {SidebarElementType::SECTION_HEADER, L"QU\u1ea2N L\u00dd H\u1ec6 TH\u1ed0NG", AppState::ADMIN_MOVIES, {}},
        {SidebarElementType::MENU_ITEM, L"Qu\u1ea3n l\u00fd phim", AppState::ADMIN_MOVIES, {}},
        {SidebarElementType::MENU_ITEM, L"Qu\u1ea3n l\u00fd ph\u00f2ng chi\u1ebfu", AppState::ADMIN_ROOMS, {}},
        {SidebarElementType::MENU_ITEM, L"Qu\u1ea3n l\u00fd su\u1ea5t chi\u1ebfu", AppState::ADMIN_SHOWTIMES, {}},
        {SidebarElementType::MENU_ITEM, L"Qu\u1ea3n l\u00fd v\u00e9", AppState::ADMIN_TICKETS, {}},
        {SidebarElementType::MENU_ITEM, L"Qu\u1ea3n l\u00fd combo b\u1eafp n\u01b0\u1edbc", AppState::ADMIN_COMBOS, {}},
        {SidebarElementType::MENU_ITEM, L"Qu\u1ea3n l\u00fd voucher", AppState::ADMIN_VOUCHERS, {}},

        // Section 2: Thống kê & báo cáo
        {SidebarElementType::SECTION_HEADER, L"TH\u1ed0NG K\u00ca & B\u00c1O C\u00c1O", AppState::ADMIN_DASHBOARD, {}},
        {SidebarElementType::MENU_ITEM, L"T\u1ed5ng quan", AppState::ADMIN_DASHBOARD, {}},
        {SidebarElementType::MENU_ITEM, L"Doanh thu", AppState::ADMIN_REVENUE, {}},

        // Section 3: Tài khoản
        {SidebarElementType::SECTION_HEADER, L"T\u00c0I KHO\u1ea2N", AppState::ADMIN_CUSTOMERS, {}},
        {SidebarElementType::MENU_ITEM, L"Qu\u1ea3n l\u00fd kh\u00e1ch h\u00e0ng", AppState::ADMIN_CUSTOMERS, {}},

        // Section 4: Hệ thống
        {SidebarElementType::SECTION_HEADER, L"H\u1ec6 TH\u1ed0NG", AppState::ADMIN_CHANGE_PASSWORD, {}},
        {SidebarElementType::MENU_ITEM, L"Th\u00f4ng tin c\u00e1 nh\u00e2n", AppState::ADMIN_CHANGE_PASSWORD, {}}
    };

    if (logoTexture.loadFromFile("../assets/elements/cinexine_logo.png") ||
        logoTexture.loadFromFile("assets/elements/cinexine_logo.png") ||
        logoTexture.loadFromFile("../assets/elements/icon.png") ||
        logoTexture.loadFromFile("assets/elements/icon.png")) {
        logoTexture.setSmooth(true);
        logoSprite.emplace(logoTexture);
    }

    recalculateLayout();
}

float AdminSidebar::getWidth() const {
    return width;
}

void AdminSidebar::setPosition(Vector2f pos) {
    position = pos;
    recalculateLayout();
}

void AdminSidebar::setOnStateChange(function<void(AppState)> callback) {
    onStateChange = callback;
}

void AdminSidebar::setOnLogout(function<void()> callback) {
    onLogout = callback;
}

void AdminSidebar::setOnToggleCollapse(function<void(bool)> callback) {
    onToggleCollapse = callback;
}

void AdminSidebar::setCollapsed(bool c) {
    collapsed = c;
    recalculateLayout();
}

void AdminSidebar::setActiveState(AppState state) {
    activeState = state;
}

void AdminSidebar::recalculateLayout() {
    if (collapsed) {
        toggleBtnBounds = FloatRect({8.f, 18.f}, {28.f, 28.f});
        return;
    }

    const float padX = position.x + 16.f;
    const float itemW = width - 32.f;
    float curY = position.y + 100.f; // Kéo toàn bộ các mục xuống thấp hơn cho cân đối

    toggleBtnBounds = FloatRect({position.x + width - 42.f, position.y + 23.f}, {26.f, 26.f});

    for (size_t i = 0; i < elements.size(); ++i) {
        if (elements[i].type == SidebarElementType::SECTION_HEADER) {
            if (i > 0) curY += 24.f; // Khoảng cách rộng rãi giữa các nhóm chức năng
            elements[i].bounds = FloatRect({padX, curY}, {itemW, 24.f});
            curY += 30.f;
        } else {
            elements[i].bounds = FloatRect({padX, curY}, {itemW, 42.f});
            curY += 48.f;
        }
    }

    // Nút đăng xuất ở đáy thanh điều hướng
    logoutBounds = FloatRect({padX, position.y + height - 64.f}, {itemW, 44.f});
}

void AdminSidebar::handleEvent(const Event& event, const RenderWindow& win) {
    if (const auto* mouseBtn = event.getIf<Event::MouseButtonPressed>()) {
        if (mouseBtn->button == Mouse::Button::Left) {
            Vector2f mousePos = win.mapPixelToCoords(Mouse::getPosition(win));

            // Click nút thu gọn / mở rộng thanh điều hướng
            if (toggleBtnBounds.contains(mousePos)) {
                setCollapsed(!collapsed);
                if (onToggleCollapse) {
                    onToggleCollapse(collapsed);
                }
                return;
            }

            if (collapsed) return;

            // Click nút Đăng xuất
            if (logoutBounds.contains(mousePos)) {
                if (onLogout) onLogout();
                return;
            }

            // Click menu item
            for (const auto& elem : elements) {
                if (elem.type == SidebarElementType::MENU_ITEM && elem.bounds.contains(mousePos)) {
                    activeState = elem.targetState;
                    if (onStateChange) {
                        onStateChange(elem.targetState);
                    }
                    break;
                }
            }
        }
    }
}

void AdminSidebar::update(const RenderWindow& win) {
    Vector2f mousePos = win.mapPixelToCoords(Mouse::getPosition(win));

    toggleHovered = toggleBtnBounds.contains(mousePos);

    if (collapsed) {
        hoveredIndex = -1;
        logoutHovered = false;
        return;
    }

    logoutHovered = logoutBounds.contains(mousePos);

    hoveredIndex = -1;
    for (size_t i = 0; i < elements.size(); ++i) {
        if (elements[i].type == SidebarElementType::MENU_ITEM && elements[i].bounds.contains(mousePos)) {
            hoveredIndex = static_cast<int>(i);
            break;
        }
    }
}

void AdminSidebar::drawLogoutIcon(RenderTarget& target, Vector2f pos, Color col) {
    RectangleShape stem({8.f, 2.f});
    stem.setFillColor(col);
    stem.setPosition({pos.x + 2.f, pos.y + 5.f});
    target.draw(stem);

    ConvexShape tip(3);
    tip.setPoint(0, {pos.x + 10.f, pos.y + 1.f});
    tip.setPoint(1, {pos.x + 14.f, pos.y + 6.f});
    tip.setPoint(2, {pos.x + 10.f, pos.y + 11.f});
    tip.setFillColor(col);
    target.draw(tip);

    RectangleShape frameTop({7.f, 2.f});
    frameTop.setFillColor(col);
    frameTop.setPosition({pos.x + 4.f, pos.y});
    target.draw(frameTop);

    RectangleShape frameLeft({2.f, 12.f});
    frameLeft.setFillColor(col);
    frameLeft.setPosition({pos.x + 4.f, pos.y});
    target.draw(frameLeft);

    RectangleShape frameBtm({7.f, 2.f});
    frameBtm.setFillColor(col);
    frameBtm.setPosition({pos.x + 4.f, pos.y + 10.f});
    target.draw(frameBtm);
}

void AdminSidebar::drawSidebarToggleIcon(RenderWindow& window, Vector2f center, bool pointRight, Color col) {
    // Khung bo góc nhỏ đại diện cho cửa sổ + thanh điều hướng (giống hình mẫu 2)
    const Vector2f boxSize(13.f, 11.f);
    const Vector2f boxPos(center.x - boxSize.x / 2.f, center.y - boxSize.y / 2.f);
    RoundedRectRenderer::draw(window, boxPos, boxSize, 2.2f, Color::Transparent, 1.3f, col);

    // Vạch dọc chia đôi phần sidebar bên trái
    RectangleShape divider({1.3f, boxSize.y});
    divider.setFillColor(col);
    divider.setPosition({boxPos.x + 4.f, boxPos.y});
    window.draw(divider);

    // Mũi tên chevron nhỏ (< hoặc >) nằm trong phần khung bên phải
    const float chevX = boxPos.x + 8.5f;
    const float chevY = center.y;
    if (!pointRight) {
        // Mũi tên '<' (thu gọn)
        RectangleShape arm1({3.6f, 1.3f});
        arm1.setOrigin({0.f, 0.65f});
        arm1.setFillColor(col);
        arm1.setPosition({chevX - 1.3f, chevY});
        arm1.setRotation(degrees(-42.f));
        window.draw(arm1);

        RectangleShape arm2({3.6f, 1.3f});
        arm2.setOrigin({0.f, 0.65f});
        arm2.setFillColor(col);
        arm2.setPosition({chevX - 1.3f, chevY});
        arm2.setRotation(degrees(42.f));
        window.draw(arm2);
    } else {
        // Mũi tên '>' (mở rộng)
        RectangleShape arm1({3.6f, 1.3f});
        arm1.setOrigin({0.f, 0.65f});
        arm1.setFillColor(col);
        arm1.setPosition({chevX + 1.3f, chevY});
        arm1.setRotation(degrees(-138.f));
        window.draw(arm1);

        RectangleShape arm2({3.6f, 1.3f});
        arm2.setOrigin({0.f, 0.65f});
        arm2.setFillColor(col);
        arm2.setPosition({chevX + 1.3f, chevY});
        arm2.setRotation(degrees(138.f));
        window.draw(arm2);
    }
}

void AdminSidebar::render(RenderWindow& window) {
    if (collapsed) {
        // Viền nhấn màu xanh Ocean Blue sát mép trái khi đã ẩn thanh điều hướng
        RectangleShape leftAccent({4.f, height});
        leftAccent.setPosition({0.f, position.y});
        leftAccent.setFillColor(Color(18, 59, 93));
        window.draw(leftAccent);

        // Nút tròn mở lại thanh điều hướng ở góc trên bên trái
        Color btnBg = toggleHovered ? Color(8, 125, 181) : Color(18, 59, 93);
        RoundedRectRenderer::draw(window, {toggleBtnBounds.position.x, toggleBtnBounds.position.y + 1.5f}, toggleBtnBounds.size, 14.f, Color(0, 0, 0, 28));
        RoundedRectRenderer::draw(window, toggleBtnBounds.position, toggleBtnBounds.size, 14.f, btnBg);
        drawSidebarToggleIcon(
            window,
            {toggleBtnBounds.position.x + toggleBtnBounds.size.x / 2.f, toggleBtnBounds.position.y + toggleBtnBounds.size.y / 2.f},
            true,
            Color(224, 242, 254)
        );
        return;
    }

    // 1. Nền thanh điều hướng: Primary Navy (#123B5D)
    RectangleShape bg({width, height});
    bg.setPosition(position);
    bg.setFillColor(Color(18, 59, 93));
    window.draw(bg);

    // Đường viền ngăn cách bên phải (mờ nhẹ)
    RectangleShape rightBorder({1.f, height});
    rightBorder.setPosition({position.x + width - 1.f, position.y});
    rightBorder.setFillColor(Color(14, 46, 74));
    window.draw(rightBorder);

    const float padX = position.x + 16.f;

    // 2. Tiêu đề thương hiệu (Logo + CineCore + PBL2 PORTAL + ADMIN badge)
    if (logoSprite) {
        float targetH = 38.f;
        float scale = targetH / (float)logoTexture.getSize().y;
        logoSprite->setScale({scale, scale});
        logoSprite->setPosition({padX, position.y + 18.f});
        window.draw(*logoSprite);

        float brandTextX = padX + (float)logoTexture.getSize().x * scale + 10.f;

        Text brandLogo(boldFont, L"CineCore", 24);
        brandLogo.setFillColor(Color::White);
        brandLogo.setPosition({std::round(brandTextX), std::round(position.y + 14.f)});
        window.draw(brandLogo);

        Text brandPortal(boldFont, L"PBL2 PORTAL", 14);
        brandPortal.setFillColor(Color(186, 230, 253));
        brandPortal.setPosition({std::round(brandTextX + 1.f), std::round(position.y + 43.f)});
        window.draw(brandPortal);
    } else {
        RoundedRectRenderer::draw(window, {padX, position.y + 18.f}, {38.f, 38.f}, 8.f, Color(255, 255, 255, 45));
        RectangleShape prj1({6.f, 18.f}); prj1.setFillColor(Color::White); prj1.setPosition({padX + 9.f, position.y + 27.f}); window.draw(prj1);
        RectangleShape prj2({6.f, 18.f}); prj2.setFillColor(Color::White); prj2.setPosition({padX + 21.f, position.y + 27.f}); window.draw(prj2);

        Text brandLogo(boldFont, L"CineCore", 24);
        brandLogo.setFillColor(Color::White);
        brandLogo.setPosition({std::round(padX + 46.f), std::round(position.y + 14.f)});
        window.draw(brandLogo);

        Text brandPortal(boldFont, L"PBL2 PORTAL", 14);
        brandPortal.setFillColor(Color(186, 230, 253));
        brandPortal.setPosition({std::round(padX + 47.f), std::round(position.y + 43.f)});
        window.draw(brandPortal);
    }

    // Badge ADMIN
    Vector2f bPos(position.x + width - 108.f, position.y + 24.f);
    Vector2f bSize(60.f, 24.f);
    RoundedRectRenderer::draw(window, bPos, bSize, 12.f, Color(8, 125, 181));
    Text admTxt(boldFont, L"ADMIN", 14);
    admTxt.setFillColor(Color::White);
    FloatRect ab = admTxt.getLocalBounds();
    admTxt.setPosition({
        std::round(bPos.x + (bSize.x - ab.size.x) / 2.f - ab.position.x),
        std::round(bPos.y + (bSize.y - ab.size.y) / 2.f - ab.position.y)
    });
    window.draw(admTxt);

    // Nút tròn thu gọn thanh điều hướng nằm ngay bên phải badge ADMIN
    Color toggleBg = toggleHovered ? Color(8, 125, 181) : Color(14, 46, 74);
    RoundedRectRenderer::draw(window, toggleBtnBounds.position, toggleBtnBounds.size, 13.f, toggleBg);
    drawSidebarToggleIcon(
        window,
        {toggleBtnBounds.position.x + toggleBtnBounds.size.x / 2.f, toggleBtnBounds.position.y + toggleBtnBounds.size.y / 2.f},
        false,
        Color(224, 242, 254)
    );

    // Đường kẻ phân cách dưới khu vực thương hiệu
    RectangleShape headerDivider({width - 32.f, 1.f});
    headerDivider.setPosition({padX, position.y + 74.f});
    headerDivider.setFillColor(Color(255, 255, 255, 35));
    window.draw(headerDivider);

    // 3. Render các nhóm và mục chức năng
    for (size_t i = 0; i < elements.size(); ++i) {
        const auto& elem = elements[i];

        if (elem.type == SidebarElementType::SECTION_HEADER) {
            if (i > 0) {
                RectangleShape secDivider({width - 44.f, 1.f});
                secDivider.setPosition({padX + 6.f, elem.bounds.position.y - 12.f});
                secDivider.setFillColor(Color(255, 255, 255, 22));
                window.draw(secDivider);
            }
            // Tiêu đề nhóm (Text in hoa màu xanh phấn nhạt, không có nền hộp)
            Text secTitle(boldFont, elem.label, 14);
            secTitle.setFillColor(Color(148, 197, 233));
            FloatRect tb = secTitle.getLocalBounds();
            secTitle.setPosition({
                std::round(elem.bounds.position.x + 8.f - tb.position.x),
                std::round(elem.bounds.position.y + (elem.bounds.size.y - tb.size.y) / 2.f - tb.position.y)
            });
            window.draw(secTitle);
        }
        else {
            // Menu Item
            bool isActive = (elem.targetState == activeState);
            bool isHover = (static_cast<int>(i) == hoveredIndex);

            if (isActive) {
                RoundedRectRenderer::draw(window, elem.bounds.position, elem.bounds.size, 8.f, Color::White);
                RoundedRectRenderer::draw(window, {elem.bounds.position.x, elem.bounds.position.y + 6.f}, {4.f, elem.bounds.size.y - 12.f}, 2.f, Color(8, 125, 181));
            }
            else if (isHover) {
                RoundedRectRenderer::draw(window, elem.bounds.position, elem.bounds.size, 8.f, Color(255, 255, 255, 28));
            }

            // Chữ hiển thị
            Text itemText(isActive ? boldFont : font, elem.label, 16);
            if (isActive) {
                itemText.setFillColor(Color(18, 59, 93));
            } else {
                itemText.setFillColor(Color(241, 245, 249));
            }

            FloatRect tb = itemText.getLocalBounds();
            itemText.setPosition({
                std::round(elem.bounds.position.x + 16.f - tb.position.x),
                std::round(elem.bounds.position.y + (elem.bounds.size.y - tb.size.y) / 2.f - tb.position.y)
            });
            window.draw(itemText);

            if (elem.targetState == AppState::ADMIN_CUSTOMERS) {
                Vector2f badgeSize(32.f, 22.f);
                Vector2f badgePos(elem.bounds.position.x + elem.bounds.size.x - 44.f, elem.bounds.position.y + (elem.bounds.size.y - badgeSize.y) / 2.f);
                RoundedRectRenderer::draw(window, badgePos, badgeSize, 11.f, isActive ? Color(224, 242, 254) : Color(8, 125, 181));
                Text bTxt(boldFont, L"15", 14);
                bTxt.setFillColor(isActive ? Color(18, 59, 93) : Color::White);
                FloatRect bb = bTxt.getLocalBounds();
                bTxt.setPosition({
                    std::round(badgePos.x + (badgeSize.x - bb.size.x) / 2.f - bb.position.x),
                    std::round(badgePos.y + (badgeSize.y - bb.size.y) / 2.f - bb.position.y)
                });
                window.draw(bTxt);
            }
        }
    }

    // 4. Nút Đăng xuất
    Color logBg = logoutHovered ? Color(255, 255, 255, 36) : Color(255, 255, 255, 16);
    Color logBorder = logoutHovered ? Color(255, 255, 255, 120) : Color(255, 255, 255, 60);

    RoundedRectRenderer::draw(window, logoutBounds.position, logoutBounds.size, 8.f, logBg, 1.f, logBorder);

    drawLogoutIcon(window, {logoutBounds.position.x + 16.f, logoutBounds.position.y + (logoutBounds.size.y - 12.f) / 2.f}, Color::White);

    Text logText(boldFont, L"\u0110\u0103ng xu\u1ea5t h\u1ec7 th\u1ed1ng", 16);
    logText.setFillColor(Color::White);
    FloatRect logTb = logText.getLocalBounds();
    logText.setPosition({
        std::round(logoutBounds.position.x + 40.f - logTb.position.x),
        std::round(logoutBounds.position.y + (logoutBounds.size.y - logTb.size.y) / 2.f - logTb.position.y)
    });
    window.draw(logText);
}
