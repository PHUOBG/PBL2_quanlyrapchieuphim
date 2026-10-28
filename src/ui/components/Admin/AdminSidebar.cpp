#include "UI/components/Admin/AdminSidebar.h"
#include <cmath>

AdminSidebar::AdminSidebar(float w, float h, const string& uName)
    : width(w), height(h), position(0.f, 0.f),
      font("../assets/fonts/segoeui.ttf"),
      boldFont("../assets/fonts/segoeuib.ttf"),
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

void AdminSidebar::setActiveState(AppState state) {
    activeState = state;
}

void AdminSidebar::recalculateLayout() {
    const float padX = position.x + 16.f;
    const float itemW = width - 32.f;
    float curY = position.y + 72.f;

    for (size_t i = 0; i < elements.size(); ++i) {
        if (elements[i].type == SidebarElementType::SECTION_HEADER) {
            if (i > 0) curY += 14.f; // Khoảng cách giữa các nhóm chức năng
            elements[i].bounds = FloatRect({padX, curY}, {itemW, 22.f});
            curY += 26.f;
        } else {
            elements[i].bounds = FloatRect({padX, curY}, {itemW, 36.f});
            curY += 40.f;
        }
    }

    // Nút đăng xuất ở đáy thanh điều hướng
    logoutBounds = FloatRect({padX, position.y + height - 56.f}, {itemW, 40.f});
}

void AdminSidebar::handleEvent(const Event& event, const RenderWindow& win) {
    if (const auto* mouseBtn = event.getIf<Event::MouseButtonPressed>()) {
        if (mouseBtn->button == Mouse::Button::Left) {
            Vector2f mousePos = win.mapPixelToCoords(Mouse::getPosition(win));

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

void AdminSidebar::render(RenderWindow& window) {
    // 1. Nền thanh điều hướng: Ocean Blue (#0077B6)
    RectangleShape bg({width, height});
    bg.setPosition(position);
    bg.setFillColor(Color(0, 119, 182));
    window.draw(bg);

    // Đường viền ngăn cách bên phải (mờ nhẹ)
    RectangleShape rightBorder({1.f, height});
    rightBorder.setPosition({position.x + width - 1.f, position.y});
    rightBorder.setFillColor(Color(0, 95, 146));
    window.draw(rightBorder);

    const float padX = position.x + 16.f;

    // 2. Tiêu đề thương hiệu (Logo + CineCore + PBL2 PORTAL + ADMIN badge)
    if (logoSprite) {
        float targetH = 36.f;
        float scale = targetH / (float)logoTexture.getSize().y;
        logoSprite->setScale({scale, scale});
        logoSprite->setPosition({padX, position.y + 18.f});
        window.draw(*logoSprite);

        float brandTextX = padX + (float)logoTexture.getSize().x * scale + 10.f;

        Text brandLogo(boldFont, L"CineCore", 19);
        brandLogo.setFillColor(Color::White);
        brandLogo.setPosition({brandTextX, position.y + 16.f});
        window.draw(brandLogo);

        Text brandPortal(boldFont, L"PBL2 PORTAL", 11);
        brandPortal.setFillColor(Color(186, 230, 253));
        brandPortal.setPosition({brandTextX + 1.f, position.y + 38.f});
        window.draw(brandPortal);
    } else {
        RoundedRectRenderer::draw(window, {padX, position.y + 18.f}, {36.f, 36.f}, 8.f, Color(255, 255, 255, 45));
        RectangleShape prj1({6.f, 18.f}); prj1.setFillColor(Color::White); prj1.setPosition({padX + 9.f, position.y + 27.f}); window.draw(prj1);
        RectangleShape prj2({6.f, 18.f}); prj2.setFillColor(Color::White); prj2.setPosition({padX + 21.f, position.y + 27.f}); window.draw(prj2);

        Text brandLogo(boldFont, L"CineCore", 19);
        brandLogo.setFillColor(Color::White);
        brandLogo.setPosition({padX + 44.f, position.y + 16.f});
        window.draw(brandLogo);

        Text brandPortal(boldFont, L"PBL2 PORTAL", 11);
        brandPortal.setFillColor(Color(186, 230, 253));
        brandPortal.setPosition({padX + 45.f, position.y + 38.f});
        window.draw(brandPortal);
    }

    // Badge ADMIN
    Vector2f bPos(position.x + width - 68.f, position.y + 24.f);
    Vector2f bSize(52.f, 22.f);
    RoundedRectRenderer::draw(window, bPos, bSize, 11.f, Color(0, 95, 146));
    Text admTxt(boldFont, L"ADMIN", 11);
    admTxt.setFillColor(Color(224, 242, 254));
    FloatRect ab = admTxt.getLocalBounds();
    admTxt.setPosition({
        bPos.x + (bSize.x - ab.size.x) / 2.f - ab.position.x,
        bPos.y + (bSize.y - ab.size.y) / 2.f - ab.position.y - 1.f
    });
    window.draw(admTxt);

    // 3. Render các nhóm và mục chức năng
    for (size_t i = 0; i < elements.size(); ++i) {
        const auto& elem = elements[i];

        if (elem.type == SidebarElementType::SECTION_HEADER) {
            // Tiêu đề nhóm (Text in hoa màu xanh phấn nhạt, không có nền hộp)
            Text secTitle(boldFont, elem.label, 12);
            secTitle.setFillColor(Color(170, 222, 255));
            FloatRect tb = secTitle.getLocalBounds();
            secTitle.setPosition({
                elem.bounds.position.x + 6.f - tb.position.x,
                elem.bounds.position.y + (elem.bounds.size.y - tb.size.y) / 2.f - tb.position.y
            });
            window.draw(secTitle);
        }
        else {
            // Menu Item
            bool isActive = (elem.targetState == activeState);
            bool isHover = (static_cast<int>(i) == hoveredIndex);

            if (isActive) {
                // Khung chữ nhật bo góc màu TRẮNG tinh nổi bật khi được chọn (Pill card)
                RoundedRectRenderer::draw(window, elem.bounds.position, elem.bounds.size, 8.f, Color::White);
            }
            else if (isHover) {
                // Nền hover trắng mờ nhẹ
                RoundedRectRenderer::draw(window, elem.bounds.position, elem.bounds.size, 8.f, Color(255, 255, 255, 40));
            }

            // Chữ hiển thị
            Text itemText(isActive ? boldFont : boldFont, elem.label, 14);
            if (isActive) {
                // Chữ màu xanh dương đậm thương hiệu trên nền trắng
                itemText.setFillColor(Color(0, 102, 160));
            } else if (isHover) {
                itemText.setFillColor(Color::White);
            } else {
                itemText.setFillColor(Color(245, 250, 255));
            }

            FloatRect tb = itemText.getLocalBounds();
            itemText.setPosition({
                elem.bounds.position.x + 14.f - tb.position.x,
                elem.bounds.position.y + (elem.bounds.size.y - tb.size.y) / 2.f - tb.position.y - 1.f
            });
            window.draw(itemText);

            if (elem.targetState == AppState::ADMIN_CUSTOMERS) {
                Vector2f bPos(elem.bounds.position.x + elem.bounds.size.x - 38.f, elem.bounds.position.y + (elem.bounds.size.y - 20.f) / 2.f);
                RoundedRectRenderer::draw(window, bPos, {26.f, 20.f}, 10.f, isActive ? Color(224, 242, 254) : Color(0, 80, 130));
                Text bTxt(boldFont, L"15", 11.5f);
                bTxt.setFillColor(isActive ? Color(0, 102, 160) : Color(224, 242, 254));
                FloatRect bb = bTxt.getLocalBounds();
                bTxt.setPosition({bPos.x + 13.f - bb.size.x / 2.f, bPos.y + 2.f});
                window.draw(bTxt);
            }
        }
    }

    // 4. Trạng thái node hệ thống & Nút Đăng xuất
    float statY = logoutBounds.position.y - 24.f;
    Text nodeTxt(boldFont, L"NODE - VN - DN601", 11);
    nodeTxt.setFillColor(Color(186, 230, 253));
    nodeTxt.setPosition({padX + 2.f, statY});
    window.draw(nodeTxt);

    CircleShape liveDot(3.5f);
    liveDot.setFillColor(Color(34, 197, 94));
    liveDot.setPosition({position.x + width - 60.f, statY + 4.f});
    window.draw(liveDot);

    Text liveTxt(boldFont, L"LIVE", 11);
    liveTxt.setFillColor(Color(34, 197, 94));
    liveTxt.setPosition({position.x + width - 50.f, statY});
    window.draw(liveTxt);

    Color logBg = logoutHovered ? Color(255, 255, 255, 45) : Color(255, 255, 255, 18);
    Color logBorder = logoutHovered ? Color(255, 255, 255, 140) : Color(255, 255, 255, 70);

    RoundedRectRenderer::draw(window, logoutBounds.position, logoutBounds.size, 8.f, logBg, 1.2f, logBorder);

    drawLogoutIcon(window, {logoutBounds.position.x + 14.f, logoutBounds.position.y + 14.f}, Color::White);

    Text logText(boldFont, L"\u0110\u0103ng xu\u1ea5t h\u1ec7 th\u1ed1ng", 13);
    logText.setFillColor(Color::White);
    FloatRect logTb = logText.getLocalBounds();
    logText.setPosition({
        logoutBounds.position.x + 38.f,
        logoutBounds.position.y + (logoutBounds.size.y - logTb.size.y) / 2.f - logTb.position.y - 1.f
    });
    window.draw(logText);
}
