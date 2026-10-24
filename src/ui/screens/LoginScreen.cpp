#include "UI/screens/LoginScreen.h"
#include "UI/components/Admin/RoundedRectRenderer.h"
#include "core/AppState.h"
#include "core/AppRole.h"
#include <SFML/Graphics.hpp>
#include <cmath>

using namespace sf;
using namespace std;

LoginScreen::LoginScreen(const Font& fontRef, AuthService& authRef)
    : auth(authRef),
      font(fontRef),
      statusColor(Color(239, 68, 68)),
      forgotMsgColor(Color(239, 68, 68))
{
    // Pre-populate with default admin credentials for quick convenience if desired,
    // or start blank as per standard login UI.
    // We start blank with clear placeholders.
}

wstring LoginScreen::bullets(size_t n) {
    return wstring(n, L'\u2022');
}

void LoginScreen::drawVectorEye(RenderTarget& target, Vector2f pos, bool open, bool hovered) {
    Color col = hovered ? Color(56, 189, 248) : Color(148, 163, 184);

    // Eye contour using convex shape or circles
    CircleShape outer(9.f, 24);
    outer.setFillColor(Color::Transparent);
    outer.setOutlineThickness(1.8f);
    outer.setOutlineColor(col);
    outer.setScale({1.3f, 0.75f});
    outer.setPosition({pos.x - 11.f, pos.y - 7.f});
    target.draw(outer);

    // Pupil
    CircleShape pupil(3.5f, 16);
    pupil.setFillColor(col);
    pupil.setPosition({pos.x - 3.5f, pos.y - 3.5f});
    target.draw(pupil);

    if (!open) {
        // Diagonal slash through the eye
        RectangleShape slash({24.f, 2.f});
        slash.setFillColor(col);
        slash.setRotation(degrees(-45.f));
        slash.setPosition({pos.x - 8.f, pos.y + 8.f});
        target.draw(slash);
    }
}

void LoginScreen::drawVectorLock(RenderTarget& target, Vector2f pos, Color color) {
    // Shackle (hollow arc/box above body)
    RoundedRectRenderer::draw(target, {pos.x + 2.f, pos.y - 6.f}, {10.f, 8.f}, 4.f, Color::Transparent, 1.8f, color);

    // Body
    RoundedRectRenderer::draw(target, {pos.x, pos.y}, {14.f, 12.f}, 3.f, color);

    // Keyhole
    CircleShape dot(1.5f, 8);
    dot.setFillColor(Color(10, 16, 28));
    dot.setPosition({pos.x + 5.5f, pos.y + 3.f});
    target.draw(dot);

    RectangleShape stem({1.5f, 3.5f});
    stem.setFillColor(Color(10, 16, 28));
    stem.setPosition({pos.x + 6.25f, pos.y + 5.f});
    target.draw(stem);
}

void LoginScreen::drawVectorAt(RenderTarget& target, Vector2f pos, Color color) {
    Text atText(font);
    atText.setString(L"@");
    atText.setCharacterSize(16);
    atText.setFillColor(color);
    atText.setStyle(Text::Bold);
    atText.setPosition({pos.x, pos.y - 4.f});
    target.draw(atText);
}

void LoginScreen::drawVectorCinema(RenderTarget& target, Vector2f pos, Color color) {
    // Projector / Film screen icon
    RoundedRectRenderer::draw(target, pos, {22.f, 16.f}, 3.f, Color::Transparent, 1.8f, color);
    
    // Lens / grid inside
    RectangleShape bar1({4.f, 8.f});
    bar1.setFillColor(color);
    bar1.setPosition({pos.x + 4.f, pos.y + 4.f});
    target.draw(bar1);

    RectangleShape bar2({4.f, 8.f});
    bar2.setFillColor(color);
    bar2.setPosition({pos.x + 10.f, pos.y + 4.f});
    target.draw(bar2);

    RectangleShape bar3({2.f, 8.f});
    bar3.setFillColor(color);
    bar3.setPosition({pos.x + 16.f, pos.y + 4.f});
    target.draw(bar3);
}

void LoginScreen::drawVectorChart(RenderTarget& target, Vector2f pos, Color color) {
    // 3 vertical bars representing analytics
    RectangleShape b1({4.f, 8.f});
    b1.setFillColor(color);
    b1.setPosition({pos.x + 2.f, pos.y + 10.f});
    target.draw(b1);

    RectangleShape b2({4.f, 16.f});
    b2.setFillColor(color);
    b2.setPosition({pos.x + 9.f, pos.y + 2.f});
    target.draw(b2);

    RectangleShape b3({4.f, 12.f});
    b3.setFillColor(color);
    b3.setPosition({pos.x + 16.f, pos.y + 6.f});
    target.draw(b3);
}

void LoginScreen::drawVectorShield(RenderTarget& target, Vector2f pos, Color color) {
    // Shield outer
    ConvexShape shield(5);
    shield.setPoint(0, {pos.x, pos.y});
    shield.setPoint(1, {pos.x + 18.f, pos.y});
    shield.setPoint(2, {pos.x + 18.f, pos.y + 10.f});
    shield.setPoint(3, {pos.x + 9.f, pos.y + 18.f});
    shield.setPoint(4, {pos.x, pos.y + 10.f});
    shield.setFillColor(Color::Transparent);
    shield.setOutlineThickness(1.8f);
    shield.setOutlineColor(color);
    target.draw(shield);

    // Inner checkmark
    RectangleShape check1({6.f, 1.8f});
    check1.setFillColor(color);
    check1.setRotation(degrees(45.f));
    check1.setPosition({pos.x + 5.f, pos.y + 9.f});
    target.draw(check1);

    RectangleShape check2({9.f, 1.8f});
    check2.setFillColor(color);
    check2.setRotation(degrees(-55.f));
    check2.setPosition({pos.x + 8.5f, pos.y + 13.f});
    target.draw(check2);
}

void LoginScreen::drawVectorUser(RenderTarget& target, Vector2f pos, Color color) {
    // Head
    CircleShape head(4.f, 16);
    head.setFillColor(color);
    head.setPosition({pos.x + 5.f, pos.y});
    target.draw(head);

    // Shoulders
    RoundedRectRenderer::draw(target, {pos.x + 1.f, pos.y + 10.f}, {16.f, 8.f}, 4.f, color);
}

void LoginScreen::drawVectorGoogle(RenderTarget& target, Vector2f pos) {
    Text gText(font);
    gText.setString(L"G");
    gText.setCharacterSize(16);
    gText.setStyle(Text::Bold);
    gText.setFillColor(Color(234, 67, 53)); // Google Red
    gText.setPosition(pos);
    target.draw(gText);
}

void LoginScreen::drawVectorScreen(RenderTarget& target, Vector2f pos, Color color) {
    // TV / Screen monitor icon
    RoundedRectRenderer::draw(target, pos, {18.f, 13.f}, 2.5f, Color::Transparent, 1.8f, color);

    // Stand
    RectangleShape stand({3.f, 4.f});
    stand.setFillColor(color);
    stand.setPosition({pos.x + 7.5f, pos.y + 13.f});
    target.draw(stand);

    RectangleShape base({11.f, 2.f});
    base.setFillColor(color);
    base.setPosition({pos.x + 3.5f, pos.y + 16.f});
    target.draw(base);
}

void LoginScreen::handleEvent(const Event& event, AppState& state, string& user, string& email, bool& successFlag) {
    (void)user;
    (void)email;
    (void)state;

    if (const auto* keyEvent = event.getIf<Event::KeyPressed>()) {
        auto code = keyEvent->code;
        if (code == Keyboard::Key::Escape) {
            if (isForgotMode) {
                isForgotMode = false;
                forgotMsg = L"";
                return;
            }
        }
        if (!isForgotMode) {
            if (code == Keyboard::Key::Tab) {
                bool e = emailActive, p = passActive;
                emailActive = !e && !p ? true : !e;
                passActive  = !emailActive;
                caretVisible = true;
                caretClock.restart();
            } 
            else if (code == Keyboard::Key::Backspace) {
                if (emailActive && !emailInput.empty()) {
                    emailInput.pop_back();
                    caretVisible = true;
                    caretClock.restart();
                }
                else if (passActive && !passInput.empty()) {
                    passInput.pop_back();
                    caretVisible = true;
                    caretClock.restart();
                }
            } 
            else if (code == Keyboard::Key::Enter) {
                string uStr(emailInput.begin(), emailInput.end());
                string pStr(passInput.begin(), passInput.end());
                
                if (auth.login(uStr, pStr)) {
                    User* uObj = auth.getUser(uStr);
                    if (uObj && uObj->getRole() == AppRole::Admin) {
                        statusColor = Color(16, 185, 129);
                        statusMessage = L"Đăng nhập Quản trị viên thành công!";
                        loginSuccess = true;
                        successFlag = true;
                        loggedUser = uStr;
                        loginClock.restart();
                    } else {
                        statusColor = Color(239, 68, 68);
                        statusMessage = L"Tài khoản không có quyền Quản trị viên.";
                    }
                } 
                else {
                    statusColor = Color(239, 68, 68);
                    statusMessage = L"Sai thông tin email hoặc mật khẩu.";
                }
            }
        } else {
            // Forgot password modal
            if (code == Keyboard::Key::Tab) {
                bool e = forgotEmailActive, p = forgotPassActive;
                forgotEmailActive = !e && !p ? true : !e;
                forgotPassActive  = !forgotEmailActive;
            }
            else if (code == Keyboard::Key::Backspace) {
                if (forgotEmailActive && !forgotEmailInput.empty()) forgotEmailInput.pop_back();
                else if (forgotPassActive && !forgotPassInput.empty()) forgotPassInput.pop_back();
            }
            else if (code == Keyboard::Key::Enter) {
                string fEmail(forgotEmailInput.begin(), forgotEmailInput.end());
                string fPass(forgotPassInput.begin(), forgotPassInput.end());
                if (fEmail.empty() || fPass.empty()) {
                    forgotMsgColor = Color(239, 68, 68);
                    forgotMsg = L"Vui lòng nhập đầy đủ thông tin.";
                } else {
                    User* userObj = auth.getUser(fEmail);
                    if (userObj) {
                        userObj->setPasswordHash(PasswordHasher::hashPassword(fPass));
                        if (auth.getRepository()) auth.getRepository()->saveToFile();
                        forgotMsgColor = Color(16, 185, 129);
                        forgotMsg = L"Đặt lại mật khẩu thành công! Vui lòng đăng nhập.";
                    } else {
                        forgotMsgColor = Color(239, 68, 68);
                        forgotMsg = L"Email không tồn tại trong hệ thống.";
                    }
                }
            }
        }
    }
    
    if (const auto* textEvent = event.getIf<Event::TextEntered>()) {
        char32_t unicode = textEvent->unicode;
        if (unicode >= 32 && unicode != 127) {
            if (!isForgotMode) {
                if (emailActive && emailInput.size() < 48) {
                    emailInput.push_back((wchar_t)unicode);
                    caretVisible = true;
                    caretClock.restart();
                }
                else if (passActive && passInput.size() < 48) {
                    passInput.push_back((wchar_t)unicode);
                    caretVisible = true;
                    caretClock.restart();
                }
            } else {
                if (forgotEmailActive && forgotEmailInput.size() < 48) forgotEmailInput.push_back((wchar_t)unicode);
                else if (forgotPassActive && forgotPassInput.size() < 48) forgotPassInput.push_back((wchar_t)unicode);
            }
        }
    }
}

bool LoginScreen::update(Vector2f mouse, bool mousePressed, string& currentUser, string& currentUserEmail, AppState& state) {
    const float cardW = 1160.f;
    const float cardH = 640.f;
    const float cardX = (1728.f - cardW) / 2.f; // 284.f
    const float cardY = (972.f - cardH) / 2.f;  // 166.f
    const float formX = cardX + 460.f + 110.f;  // 854.f
    const float formW = 460.f;

    FloatRect emailBoxBounds({formX, cardY + 138.f}, {formW, 46.f});
    FloatRect passBoxBounds({formX, cardY + 222.f}, {formW, 46.f});
    FloatRect eyeBounds({formX + formW - 42.f, cardY + 222.f}, {40.f, 46.f});
    FloatRect checkBounds({formX, cardY + 282.f}, {150.f, 22.f});
    FloatRect forgotBounds({formX + formW - 120.f, cardY + 282.f}, {120.f, 22.f});
    FloatRect btnBounds({formX, cardY + 316.f}, {formW, 48.f});
    FloatRect linkRegBounds({formX + 70.f, cardY + 386.f}, {320.f, 24.f});
    FloatRect posBtnBounds({formX, cardY + 450.f}, {222.f, 42.f});
    FloatRect googleBtnBounds({formX + 238.f, cardY + 450.f}, {222.f, 42.f});

    // Hover states
    eyeHovered = eyeBounds.contains(mouse);
    checkHovered = checkBounds.contains(mouse);
    linkForgotHovered = forgotBounds.contains(mouse);
    btnHovered = btnBounds.contains(mouse);
    linkCreateHovered = linkRegBounds.contains(mouse);
    posBtnHovered = posBtnBounds.contains(mouse);
    googleBtnHovered = googleBtnBounds.contains(mouse);

    if (isForgotMode) {
        FloatRect forgotCardBounds({(1728.f - 520.f) / 2.f, (972.f - 380.f) / 2.f}, {520.f, 380.f});
        FloatRect fEmailBox({forgotCardBounds.position.x + 40.f, forgotCardBounds.position.y + 110.f}, {440.f, 44.f});
        FloatRect fPassBox({forgotCardBounds.position.x + 40.f, forgotCardBounds.position.y + 180.f}, {440.f, 44.f});
        FloatRect fCloseBtn({forgotCardBounds.position.x + 40.f, forgotCardBounds.position.y + 270.f}, {210.f, 44.f});
        FloatRect fSubmitBtn({forgotCardBounds.position.x + 270.f, forgotCardBounds.position.y + 270.f}, {210.f, 44.f});

        forgotCloseHovered = fCloseBtn.contains(mouse);
        forgotBtnHovered = fSubmitBtn.contains(mouse);

        if (mousePressed) {
            forgotEmailActive = fEmailBox.contains(mouse);
            forgotPassActive  = fPassBox.contains(mouse);

            if (forgotCloseHovered) {
                isForgotMode = false;
                forgotMsg = L"";
            }
            else if (forgotBtnHovered) {
                string fEmail(forgotEmailInput.begin(), forgotEmailInput.end());
                string fPass(forgotPassInput.begin(), forgotPassInput.end());
                if (fEmail.empty() || fPass.empty()) {
                    forgotMsgColor = Color(239, 68, 68);
                    forgotMsg = L"Vui lòng nhập đầy đủ thông tin.";
                } else {
                    User* userObj = auth.getUser(fEmail);
                    if (userObj) {
                        userObj->setPasswordHash(PasswordHasher::hashPassword(fPass));
                        if (auth.getRepository()) auth.getRepository()->saveToFile();
                        forgotMsgColor = Color(16, 185, 129);
                        forgotMsg = L"Đặt lại mật khẩu thành công! Vui lòng đăng nhập.";
                    } else {
                        forgotMsgColor = Color(239, 68, 68);
                        forgotMsg = L"Email không tồn tại trong hệ thống.";
                    }
                }
            }
        }
        return false;
    }

    if (mousePressed) {
        if (eyeHovered) {
            showPassword = !showPassword;
        } else if (emailBoxBounds.contains(mouse)) {
            emailActive = true;
            passActive = false;
            caretVisible = true;
            caretClock.restart();
        } else if (passBoxBounds.contains(mouse)) {
            passActive = true;
            emailActive = false;
            caretVisible = true;
            caretClock.restart();
        } else {
            emailActive = false;
            passActive = false;
        }

        if (checkHovered) {
            rememberMe = !rememberMe;
        }

        if (btnHovered) {
            string u(emailInput.begin(), emailInput.end());
            string p(passInput.begin(), passInput.end());
            if (auth.login(u, p)) {
                User* uObj = auth.getUser(u);
                if (uObj && uObj->getRole() == AppRole::Admin) {
                    statusColor = Color(16, 185, 129);
                    statusMessage = L"Đăng nhập Quản trị viên thành công!";
                    loginSuccess = true;
                    loggedUser = u;
                    loginClock.restart();
                } else {
                    statusColor = Color(239, 68, 68);
                    statusMessage = L"Tài khoản không có quyền Quản trị viên.";
                }
            } else {
                statusColor = Color(239, 68, 68);
                statusMessage = L"Sai thông tin email hoặc mật khẩu.";
            }
        }

        if (linkCreateHovered) {
            state = AppState::REGISTER;
        }

        if (linkForgotHovered) {
            isForgotMode = true;
            forgotEmailInput = emailInput;
            forgotPassInput = L"";
            forgotMsg = L"";
        }

        // Quick login helpers for developer/demo
        if (posBtnHovered) {
            emailInput = L"admin@cinexine.vn";
            passInput = L"admin123";
            statusColor = Color(56, 189, 248);
            statusMessage = L"Đã nạp sẵn tài khoản Admin POS!";
        }

        if (googleBtnHovered) {
            emailInput = L"admin@cinema.com";
            passInput = L"admin123";
            statusColor = Color(56, 189, 248);
            statusMessage = L"Đã nạp sẵn tài khoản Admin Workspace!";
        }
    }

    // Caret blink
    if (caretClock.getElapsedTime().asSeconds() >= 0.5f) {
        caretVisible = !caretVisible;
        caretClock.restart();
    }

    // Transition after login success
    if (loginSuccess && loginClock.getElapsedTime().asSeconds() >= 0.35f) {
        currentUserEmail = loggedUser;
        User* userObj = auth.getUser(loggedUser);
        if (userObj) {
            currentUser = userObj->getUsername();
            state = AppState::ADMIN_DASHBOARD;
            loginSuccess = false;
        }
        return true; 
    }

    return false;
}

void LoginScreen::draw(RenderWindow& window) {
    const Vector2u winSize = window.getSize();
    const float W = (float)winSize.x;
    const float H = (float)winSize.y;

    // 1. Dark canvas background
    RectangleShape bgCanvas({W, H});
    bgCanvas.setFillColor(Color(8, 13, 24)); // #080D18
    window.draw(bgCanvas);

    // Subtle ambient glow behind card (radial/soft ellipses)
    CircleShape glow1(280.f, 36);
    glow1.setFillColor(Color(2, 132, 199, 14)); // Cyan/blue aura
    glow1.setOrigin({280.f, 280.f});
    glow1.setPosition({W * 0.45f, H * 0.5f});
    window.draw(glow1);

    CircleShape glow2(240.f, 36);
    glow2.setFillColor(Color(56, 189, 248, 12));
    glow2.setOrigin({240.f, 240.f});
    glow2.setPosition({W * 0.65f, H * 0.48f});
    window.draw(glow2);

    // 2. Top Header Bar
    drawVectorScreen(window, {24.f, 20.f}, Color(245, 158, 11)); // Gold screen icon

    Text headerTitle(font);
    headerTitle.setString(L"PBL2 \u2013 H\u1ec6 TH\u1ed0NG QU\u1ea2N L\u00dd R\u1ea0P CHI\u1ebeU PHIM & TH\u1ed0NG K\u00ca DOANH THU");
    headerTitle.setCharacterSize(13);
    headerTitle.setStyle(Text::Bold);
    headerTitle.setFillColor(Color(180, 195, 215));
    headerTitle.setPosition({52.f, 19.f});
    window.draw(headerTitle);

    // Top-right version pill badge
    RoundedRectRenderer::draw(window, {W - 220.f, 15.f}, {145.f, 26.f}, 13.f, Color(15, 23, 42), 1.f, Color(30, 41, 59));
    Text verText(font);
    verText.setString(L"Phi\u00ean b\u1ea3n Desktop v2.4");
    verText.setCharacterSize(11);
    verText.setFillColor(Color(148, 163, 184));
    verText.setPosition({W - 212.f, 20.f});
    window.draw(verText);

    // Window control dots
    CircleShape dotRed(4.5f, 12);
    dotRed.setFillColor(Color(239, 68, 68, 180));
    dotRed.setPosition({W - 55.f, 24.f});
    window.draw(dotRed);

    CircleShape dotYellow(4.5f, 12);
    dotYellow.setFillColor(Color(245, 158, 11, 180));
    dotYellow.setPosition({W - 40.f, 24.f});
    window.draw(dotYellow);

    CircleShape dotGreen(4.5f, 12);
    dotGreen.setFillColor(Color(16, 185, 129, 180));
    dotGreen.setPosition({W - 25.f, 24.f});
    window.draw(dotGreen);

    // 3. Central Login Modal Card
    const float cardW = 1160.f;
    const float cardH = 640.f;
    const float cardX = (W - cardW) / 2.f; // 284.f
    const float cardY = (H - cardH) / 2.f; // 166.f

    // Main Card background & border
    RoundedRectRenderer::draw(window, {cardX, cardY}, {cardW, cardH}, 22.f, Color(12, 19, 36), 1.5f, Color(30, 45, 72));

    // Vertical Divider Line between Left & Right Column
    const float dividerX = cardX + 460.f;
    RectangleShape vLine({1.f, cardH});
    vLine.setFillColor(Color(25, 36, 58));
    vLine.setPosition({dividerX, cardY});
    window.draw(vLine);

    // ==========================================
    // 4. LEFT COLUMN: CineAdmin Portal Showcase
    // ==========================================
    const float leftPadX = cardX + 40.f;

    // Status Pill Badge: "Hệ Thống Trực Tuyến 24/7"
    RoundedRectRenderer::draw(window, {leftPadX, cardY + 40.f}, {176.f, 30.f}, 15.f, Color(7, 34, 58), 1.2f, Color(2, 132, 199));
    
    CircleShape statusDot(4.f, 12);
    statusDot.setFillColor(Color(16, 185, 129));
    statusDot.setPosition({leftPadX + 14.f, cardY + 51.f});
    window.draw(statusDot);

    Text badgeText(font);
    badgeText.setString(L"H\u1ec7 Th\u1ed1ng Tr\u1ef1c Tuy\u1ebfn 24/7");
    badgeText.setCharacterSize(11);
    badgeText.setStyle(Text::Bold);
    badgeText.setFillColor(Color(56, 189, 248));
    badgeText.setPosition({leftPadX + 28.f, cardY + 47.f});
    window.draw(badgeText);

    // Headline: CineAdmin Portal
    Text mainTitle1(font);
    mainTitle1.setString(L"CineAdmin Portal");
    mainTitle1.setCharacterSize(28);
    mainTitle1.setStyle(Text::Bold);
    mainTitle1.setFillColor(Color::White);
    mainTitle1.setPosition({leftPadX, cardY + 86.f});
    window.draw(mainTitle1);

    // Sub-headline: Quản Trị Rạp Hiện Đại
    Text mainTitle2(font);
    mainTitle2.setString(L"Qu\u1ea3n Tr\u1ecb R\u1ea1p Hi\u1ec7n \u0110\u1ea1i");
    mainTitle2.setCharacterSize(28);
    mainTitle2.setStyle(Text::Bold);
    mainTitle2.setFillColor(Color(56, 189, 248)); // Sky/Cyan Blue
    mainTitle2.setPosition({leftPadX, cardY + 122.f});
    window.draw(mainTitle2);

    // Description text
    Text descText(font);
    descText.setString(
        L"Gi\u1ea3i ph\u00e1p s\u1ed1 h\u00f3a to\u00e0n di\u1ec7n \u0111i\u1ec1u ph\u1ed1i ph\u00f2ng chi\u1ebfu,\n"
        L"\u0111i\u1ec1u h\u00f2a l\u1ecbch phim, xu\u1ea5t v\u00e9 POS v\u00e0 ph\u00e2n t\u00edch th\u1ed1ng k\u00ea\n"
        L"doanh thu theo th\u1eddi gian th\u1ef1c."
    );
    descText.setCharacterSize(12);
    descText.setFillColor(Color(148, 163, 184));
    descText.setLineSpacing(1.35f);
    descText.setPosition({leftPadX, cardY + 166.f});
    window.draw(descText);

    // 3 Feature Highlight Cards
    const float featW = 380.f;
    const float featH = 58.f;
    const float featYStart = cardY + 238.f;

    // Feature Card 1: 12 Phòng Chiếu IMAX
    RoundedRectRenderer::draw(window, {leftPadX, featYStart}, {featW, featH}, 12.f, Color(16, 26, 46), 1.f, Color(26, 38, 62));
    RoundedRectRenderer::draw(window, {leftPadX + 10.f, featYStart + 11.f}, {36.f, 36.f}, 8.f, Color(8, 47, 73));
    drawVectorCinema(window, {leftPadX + 17.f, featYStart + 21.f}, Color(56, 189, 248));

    Text f1Title(font, L"Qu\u1ea3n l\u00fd 12 Ph\u00f2ng Chi\u1ebfu Chu\u1ea9n IMAX", 13);
    f1Title.setStyle(Text::Bold);
    f1Title.setFillColor(Color(248, 250, 252));
    f1Title.setPosition({leftPadX + 56.f, featYStart + 11.f});
    window.draw(f1Title);

    Text f1Sub(font, L"T\u1ef1 \u0111\u1ed9ng \u0111\u1ed3ng b\u1ed9 gh\u1ebf ng\u1ed3i & l\u1ecbch xu\u1ea5t chi\u1ebfu", 11);
    f1Sub.setFillColor(Color(100, 116, 139));
    f1Sub.setPosition({leftPadX + 56.f, featYStart + 31.f});
    window.draw(f1Sub);

    // Feature Card 2: Báo Cáo Doanh Thu Tự Động
    const float f2Y = featYStart + 68.f;
    RoundedRectRenderer::draw(window, {leftPadX, f2Y}, {featW, featH}, 12.f, Color(16, 26, 46), 1.f, Color(26, 38, 62));
    RoundedRectRenderer::draw(window, {leftPadX + 10.f, f2Y + 11.f}, {36.f, 36.f}, 8.f, Color(69, 26, 3));
    drawVectorChart(window, {leftPadX + 17.f, f2Y + 19.f}, Color(245, 158, 11));

    Text f2Title(font, L"B\u00e1o C\u00e1o Doanh Thu T\u1ef1 \u0110\u1ed9ng", 13);
    f2Title.setStyle(Text::Bold);
    f2Title.setFillColor(Color(248, 250, 252));
    f2Title.setPosition({leftPadX + 56.f, f2Y + 11.f});
    window.draw(f2Title);

    Text f2Sub(font, L"Bi\u1ec3u \u0111\u1ed3 t\u1ed5ng k\u1ebft doanh s\u1ed1 v\u00e9, b\u1eafp n\u01b0\u1edbc (F&B)", 11);
    f2Sub.setFillColor(Color(100, 116, 139));
    f2Sub.setPosition({leftPadX + 56.f, f2Y + 31.f});
    window.draw(f2Sub);

    // Feature Card 3: Bảo Mật Quyền Truy Cập Cao Cấp
    const float f3Y = featYStart + 136.f;
    RoundedRectRenderer::draw(window, {leftPadX, f3Y}, {featW, featH}, 12.f, Color(16, 26, 46), 1.f, Color(26, 38, 62));
    RoundedRectRenderer::draw(window, {leftPadX + 10.f, f3Y + 11.f}, {36.f, 36.f}, 8.f, Color(6, 78, 59));
    drawVectorShield(window, {leftPadX + 19.f, f3Y + 20.f}, Color(16, 185, 129));

    Text f3Title(font, L"B\u1ea3o M\u1eadt Quy\u1ec1n Truy C\u1eadp Cao C\u1ea5p", 13);
    f3Title.setStyle(Text::Bold);
    f3Title.setFillColor(Color(248, 250, 252));
    f3Title.setPosition({leftPadX + 56.f, f3Y + 11.f});
    window.draw(f3Title);

    Text f3Sub(font, L"Ph\u00e2n quy\u1ec1n chi ti\u1ebft: Ban gi\u00e1m \u0111\u1ed1c & Qu\u1ea3n tr\u1ecb vi\u00ean", 11);
    f3Sub.setFillColor(Color(100, 116, 139));
    f3Sub.setPosition({leftPadX + 56.f, f3Y + 31.f});
    window.draw(f3Sub);

    // Bottom Status Row in Left Column
    Text projectText(font, L"D\u1ef1 \u00e1n: K\u1ef9 Thu\u1eadt Ph\u1ea7n M\u1ec1m PBL2", 11);
    projectText.setFillColor(Color(100, 116, 139));
    projectText.setPosition({leftPadX, cardY + cardH - 45.f});
    window.draw(projectText);

    CircleShape dbDot(3.5f, 10);
    dbDot.setFillColor(Color(16, 185, 129));
    dbDot.setPosition({leftPadX + 295.f, cardY + cardH - 40.f});
    window.draw(dbDot);

    Text dbReadyText(font, L"DB  Ready", 11);
    dbReadyText.setStyle(Text::Bold);
    dbReadyText.setFillColor(Color(16, 185, 129));
    dbReadyText.setPosition({leftPadX + 306.f, cardY + cardH - 45.f});
    window.draw(dbReadyText);

    // ==========================================
    // 5. RIGHT COLUMN: Form Đăng Nhập Quản Trị
    // ==========================================
    const float formX = cardX + 460.f + 110.f; // 854.f
    const float formW = 460.f;

    // Heading: ĐĂNG NHẬP QUẢN TRỊ VIÊN
    Text formTitle(font, L"\u0110\u0102NG NH\u1eacP QU\u1ea2N TR\u1eca VI\u00caN", 25);
    formTitle.setStyle(Text::Bold);
    formTitle.setFillColor(Color::White);
    formTitle.setPosition({formX, cardY + 40.f});
    window.draw(formTitle);

    // Subtitle
    Text formSubtitle(font, L"Vui l\u00f2ng nh\u1eadp th\u00f4ng tin x\u00e1c th\u1ef1c \u0111\u1ec3 truy c\u1eadp b\u1ea3ng \u0111i\u1ec1u khi\u1ec3n r\u1ea1p chi\u1ebfu phim.", 12);
    formSubtitle.setFillColor(Color(124, 139, 161));
    formSubtitle.setPosition({formX, cardY + 76.f});
    window.draw(formSubtitle);

    // Field 1: TÀI KHOẢN / EMAIL QUẢN TRỊ VIÊN
    Text emailLabel(font, L"T\u00c0I KHO\u1ea2N / EMAIL QU\u1ea2N TR\u1eca VI\u00caN", 11);
    emailLabel.setStyle(Text::Bold);
    emailLabel.setFillColor(Color(148, 163, 184));
    emailLabel.setPosition({formX, cardY + 118.f});
    window.draw(emailLabel);

    // Email Input Box
    Color emailBorderCol = emailActive ? Color(2, 132, 199) : Color(30, 43, 69);
    RoundedRectRenderer::draw(window, {formX, cardY + 138.f}, {formW, 46.f}, 10.f, Color(9, 14, 26), 1.5f, emailBorderCol);
    drawVectorAt(window, {formX + 14.f, cardY + 151.f}, Color(100, 116, 139));

    if (emailInput.empty() && !emailActive) {
        Text phEmail(font, L"Nh\u1eadp email qu\u1ea3n tr\u1ecb vi\u00ean", 13);
        phEmail.setFillColor(Color(71, 85, 105));
        phEmail.setPosition({formX + 42.f, cardY + 150.f});
        window.draw(phEmail);
    } else {
        Text valEmail(font, emailInput, 13);
        valEmail.setFillColor(Color(248, 250, 252));
        valEmail.setPosition({formX + 42.f, cardY + 150.f});
        window.draw(valEmail);

        if (emailActive && caretVisible) {
            FloatRect eb = valEmail.getLocalBounds();
            RectangleShape caretLine({1.8f, 18.f});
            caretLine.setFillColor(Color(56, 189, 248));
            caretLine.setPosition({formX + 44.f + (emailInput.empty() ? 0.f : eb.size.x), cardY + 152.f});
            window.draw(caretLine);
        }
    }

    // Field 2: MẬT KHẨU BẢO MẬT
    Text passLabel(font, L"M\u1eacT KH\u1ea8U B\u1ea2O M\u1eacT", 11);
    passLabel.setStyle(Text::Bold);
    passLabel.setFillColor(Color(148, 163, 184));
    passLabel.setPosition({formX, cardY + 202.f});
    window.draw(passLabel);

    // Password Input Box
    Color passBorderCol = passActive ? Color(2, 132, 199) : Color(30, 43, 69);
    RoundedRectRenderer::draw(window, {formX, cardY + 222.f}, {formW, 46.f}, 10.f, Color(9, 14, 26), 1.5f, passBorderCol);
    drawVectorLock(window, {formX + 14.f, cardY + 239.f}, Color(100, 116, 139));

    if (passInput.empty() && !passActive) {
        Text phPass(font, L"Nh\u1eadp m\u1eadt kh\u1ea9u", 13);
        phPass.setFillColor(Color(71, 85, 105));
        phPass.setPosition({formX + 42.f, cardY + 234.f});
        window.draw(phPass);
    } else {
        wstring dispPass = showPassword ? passInput : bullets(passInput.size());
        Text valPass(font, dispPass, 13);
        valPass.setFillColor(Color(248, 250, 252));
        valPass.setPosition({formX + 42.f, cardY + 234.f});
        window.draw(valPass);

        if (passActive && caretVisible) {
            FloatRect pb = valPass.getLocalBounds();
            RectangleShape caretLine({1.8f, 18.f});
            caretLine.setFillColor(Color(56, 189, 248));
            caretLine.setPosition({formX + 44.f + (passInput.empty() ? 0.f : pb.size.x), cardY + 236.f});
            window.draw(caretLine);
        }
    }

    // Eye toggle button (neat vector graphic, properly sized)
    drawVectorEye(window, {formX + formW - 22.f, cardY + 245.f}, showPassword, eyeHovered);

    // Options Row: Checkbox "Ghi nhớ đăng nhập" & "Quên mật khẩu?"
    // Checkbox
    Color cbBorder = checkHovered ? Color(56, 189, 248) : Color(51, 65, 85);
    RoundedRectRenderer::draw(window, {formX + 1.f, cardY + 285.f}, {16.f, 16.f}, 4.f, Color(11, 18, 32), 1.2f, cbBorder);
    if (rememberMe) {
        // Small checkmark
        RectangleShape c1({4.f, 1.8f});
        c1.setFillColor(Color(56, 189, 248));
        c1.setRotation(degrees(45.f));
        c1.setPosition({formX + 4.f, cardY + 293.f});
        window.draw(c1);

        RectangleShape c2({8.f, 1.8f});
        c2.setFillColor(Color(56, 189, 248));
        c2.setRotation(degrees(-55.f));
        c2.setPosition({formX + 7.f, cardY + 296.f});
        window.draw(c2);
    }

    Text remText(font, L"Ghi nh\u1edb \u0111\u0103ng nh\u1eadp", 12);
    remText.setFillColor(Color(148, 163, 184));
    remText.setPosition({formX + 24.f, cardY + 284.f});
    window.draw(remText);

    // "Quên mật khẩu?" Link
    Text forgotText(font, L"Qu\u00ean m\u1eadt kh\u1ea9u?", 12);
    forgotText.setStyle(linkForgotHovered ? Text::Underlined : Text::Regular);
    forgotText.setFillColor(linkForgotHovered ? Color(252, 165, 165) : Color(239, 68, 68));
    forgotText.setPosition({formX + formW - forgotText.getLocalBounds().size.x, cardY + 284.f});
    window.draw(forgotText);

    // Main Submit Button: ĐĂNG NHẬP ➔
    Color btnCol = btnHovered ? Color(14, 165, 233) : Color(2, 132, 199);
    RoundedRectRenderer::draw(window, {formX, cardY + 316.f}, {formW, 48.f}, 10.f, btnCol);

    Text submitText(font, L"\u0110\u0102NG NH\u1eacP  \u2794", 15);
    submitText.setStyle(Text::Bold);
    submitText.setFillColor(Color::White);
    FloatRect stb = submitText.getLocalBounds();
    submitText.setPosition({formX + (formW - stb.size.x) / 2.f, cardY + 330.f});
    window.draw(submitText);

    // Status / Error message
    if (!statusMessage.empty()) {
        Text msgText(font, statusMessage, 12);
        msgText.setStyle(Text::Bold);
        msgText.setFillColor(statusColor);
        FloatRect mb = msgText.getLocalBounds();
        msgText.setPosition({formX + (formW - mb.size.x) / 2.f, cardY + 368.f});
        window.draw(msgText);
    }

    // Register Link: Chưa có tài khoản? Đăng ký Admin ngay
    Text regPrefix(font, L"Ch\u01b0a c\u00f3 t\u00e0i kho\u1ea3n? ", 12);
    regPrefix.setFillColor(Color(100, 116, 139));

    Text regLink(font, L"\u0110\u0103ng k\u00fd Admin ngay", 12);
    regLink.setStyle(Text::Bold | (linkCreateHovered ? Text::Underlined : Text::Regular));
    regLink.setFillColor(linkCreateHovered ? Color(125, 211, 252) : Color(56, 189, 248));

    float fullRegW = regPrefix.getLocalBounds().size.x + regLink.getLocalBounds().size.x;
    float regStartX = formX + (formW - fullRegW) / 2.f;
    regPrefix.setPosition({regStartX, cardY + 392.f});
    regLink.setPosition({regStartX + regPrefix.getLocalBounds().size.x, cardY + 392.f});
    window.draw(regPrefix);
    window.draw(regLink);

    // Horizontal Divider with text: HOẶC ĐĂNG NHẬP NHANH BẰNG
    const float divY = cardY + 430.f;
    RectangleShape hLine({formW, 1.f});
    hLine.setFillColor(Color(30, 41, 59));
    hLine.setPosition({formX, divY});
    window.draw(hLine);

    // Text in middle of divider
    Text orText(font, L"HO\u1eb6C \u0110\u0102NG NH\u1eacP NHANH B\u1eb0NG", 10);
    orText.setStyle(Text::Bold);
    orText.setFillColor(Color(100, 116, 139));
    FloatRect otb = orText.getLocalBounds();
    float otX = formX + (formW - otb.size.x) / 2.f;

    // Small mask under text so line doesn't cross text
    RectangleShape orMask({otb.size.x + 16.f, 16.f});
    orMask.setFillColor(Color(12, 19, 36));
    orMask.setPosition({otX - 8.f, divY - 8.f});
    window.draw(orMask);

    orText.setPosition({otX, divY - 6.f});
    window.draw(orText);

    // Quick Login Buttons (POS & Google)
    const float qBtnW = 222.f;
    const float qBtnH = 42.f;
    const float qBtnY = cardY + 452.f;

    // 1. POS Button
    Color posBg = posBtnHovered ? Color(20, 32, 54) : Color(11, 18, 32);
    RoundedRectRenderer::draw(window, {formX, qBtnY}, {qBtnW, qBtnH}, 10.f, posBg, 1.f, Color(30, 41, 59));
    drawVectorUser(window, {formX + 22.f, qBtnY + 12.f}, Color(56, 189, 248));

    Text posText(font, L"M\u00e3 Nh\u00e2n Vi\u00ean (POS)", 12);
    posText.setStyle(Text::Bold);
    posText.setFillColor(Color(203, 213, 225));
    posText.setPosition({formX + 48.f, qBtnY + 13.f});
    window.draw(posText);

    // 2. Google Workspace Button
    Color gBg = googleBtnHovered ? Color(20, 32, 54) : Color(11, 18, 32);
    RoundedRectRenderer::draw(window, {formX + 238.f, qBtnY}, {qBtnW, qBtnH}, 10.f, gBg, 1.f, Color(30, 41, 59));
    drawVectorGoogle(window, {formX + 258.f, qBtnY + 11.f});

    Text gText(font, L"Google Workspace", 12);
    gText.setStyle(Text::Bold);
    gText.setFillColor(Color(203, 213, 225));
    gText.setPosition({formX + 282.f, qBtnY + 13.f});
    window.draw(gText);

    // 6. Bottom Footer Bar
    Text footLeft(font, L"\u00a9 2024 PBL2 Cinema Management System. B\u1ea3n quy\u1ec1n thu\u1ed9c B\u1ed9 m\u00f4n K\u1ef9 thu\u1eadt ph\u1ea7n m\u1ec1m.", 11);
    footLeft.setFillColor(Color(71, 85, 105));
    footLeft.setPosition({cardX, H - 32.f});
    window.draw(footLeft);

    Text footRight(font, L"\u0110i\u1ec1u kho\u1ea3n d\u1ecbch v\u1ee5   \u2022   Tr\u1ee3 gi\u00fap k\u1ef9 thu\u1eadt", 11);
    footRight.setFillColor(Color(71, 85, 105));
    FloatRect frb = footRight.getLocalBounds();
    footRight.setPosition({cardX + cardW - frb.size.x, H - 32.f});
    window.draw(footRight);

    // ==========================================
    // 7. FORGOT PASSWORD MODAL (if open)
    // ==========================================
    if (isForgotMode) {
        // Dark translucent overlay
        RectangleShape fOverlay({W, H});
        fOverlay.setFillColor(Color(0, 0, 0, 200));
        window.draw(fOverlay);

        const float fCardW = 520.f;
        const float fCardH = 380.f;
        const float fCardX = (W - fCardW) / 2.f;
        const float fCardY = (H - fCardH) / 2.f;

        // Modal Card
        RoundedRectRenderer::draw(window, {fCardX, fCardY}, {fCardW, fCardH}, 16.f, Color(14, 23, 42), 1.5f, Color(30, 48, 80));

        Text fTitle(font, L"QU\u00caN M\u1eacT KH\u1ea8U ADMIN", 20);
        fTitle.setStyle(Text::Bold);
        fTitle.setFillColor(Color::White);
        fTitle.setPosition({fCardX + 36.f, fCardY + 30.f});
        window.draw(fTitle);

        Text fSub(font, L"Nh\u1eadp email qu\u1ea3n tr\u1ecb v\u00e0 m\u1eadt kh\u1ea9u m\u1edbi \u0111\u1ec3 \u0111\u1eb7t l\u1ea1i m\u1eadt kh\u1ea9u.", 12);
        fSub.setFillColor(Color(148, 163, 184));
        fSub.setPosition({fCardX + 36.f, fCardY + 62.f});
        window.draw(fSub);

        // Email input
        Color fEBorder = forgotEmailActive ? Color(2, 132, 199) : Color(30, 41, 59);
        RoundedRectRenderer::draw(window, {fCardX + 36.f, fCardY + 104.f}, {448.f, 44.f}, 8.f, Color(8, 14, 26), 1.2f, fEBorder);
        Text fEmailText(font, forgotEmailInput.empty() ? L"Nh\u1eadp email qu\u1ea3n tr\u1ecb vi\u00ean" : forgotEmailInput, 13);
        fEmailText.setFillColor(forgotEmailInput.empty() ? Color(71, 85, 105) : Color::White);
        fEmailText.setPosition({fCardX + 50.f, fCardY + 117.f});
        window.draw(fEmailText);

        // New password input
        Color fPBorder = forgotPassActive ? Color(2, 132, 199) : Color(30, 41, 59);
        RoundedRectRenderer::draw(window, {fCardX + 36.f, fCardY + 168.f}, {448.f, 44.f}, 8.f, Color(8, 14, 26), 1.2f, fPBorder);
        Text fPassText(font, forgotPassInput.empty() ? L"Nh\u1eadp m\u1eadt kh\u1ea9u m\u1edbi" : bullets(forgotPassInput.size()), 13);
        fPassText.setFillColor(forgotPassInput.empty() ? Color(71, 85, 105) : Color::White);
        fPassText.setPosition({fCardX + 50.f, fCardY + 181.f});
        window.draw(fPassText);

        // Status message
        if (!forgotMsg.empty()) {
            Text fMsgDisplay(font, forgotMsg, 12);
            fMsgDisplay.setStyle(Text::Bold);
            fMsgDisplay.setFillColor(forgotMsgColor);
            fMsgDisplay.setPosition({fCardX + 36.f, fCardY + 228.f});
            window.draw(fMsgDisplay);
        }

        // Buttons
        Color fCloseBg = forgotCloseHovered ? Color(30, 41, 59) : Color(15, 23, 42);
        RoundedRectRenderer::draw(window, {fCardX + 36.f, fCardY + 268.f}, {214.f, 44.f}, 8.f, fCloseBg, 1.f, Color(51, 65, 85));
        Text fCloseText(font, L"\u0110\u00f3ng", 14);
        fCloseText.setStyle(Text::Bold);
        fCloseText.setFillColor(Color(203, 213, 225));
        FloatRect ctb = fCloseText.getLocalBounds();
        fCloseText.setPosition({fCardX + 36.f + (214.f - ctb.size.x) / 2.f, fCardY + 280.f});
        window.draw(fCloseText);

        Color fSubBg = forgotBtnHovered ? Color(14, 165, 233) : Color(2, 132, 199);
        RoundedRectRenderer::draw(window, {fCardX + 270.f, fCardY + 268.f}, {214.f, 44.f}, 8.f, fSubBg);
        Text fSubText(font, L"\u0110\u1eb7t l\u1ea1i MK", 14);
        fSubText.setStyle(Text::Bold);
        fSubText.setFillColor(Color::White);
        FloatRect stb2 = fSubText.getLocalBounds();
        fSubText.setPosition({fCardX + 270.f + (214.f - stb2.size.x) / 2.f, fCardY + 280.f});
        window.draw(fSubText);
    }
}