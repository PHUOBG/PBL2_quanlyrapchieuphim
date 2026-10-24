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
}

wstring LoginScreen::bullets(size_t n) {
    return wstring(n, L'\u2022');
}

void LoginScreen::drawDotGrid(RenderTarget& target, float W, float H) {
    const float step = 28.f;
    CircleShape dot(1.2f, 8);
    dot.setFillColor(Color(203, 213, 225, 120)); // Soft slate dot

    for (float y = 50.f; y < H - 40.f; y += step) {
        for (float x = 20.f; x < W - 20.f; x += step) {
            dot.setPosition({x, y});
            target.draw(dot);
        }
    }
}

void LoginScreen::drawVectorEye(RenderTarget& target, Vector2f pos, bool open, bool hovered) {
    Color col = hovered ? Color(2, 132, 199) : Color(148, 163, 184);

    CircleShape outer(8.f, 24);
    outer.setFillColor(Color::Transparent);
    outer.setOutlineThickness(1.6f);
    outer.setOutlineColor(col);
    outer.setScale({1.3f, 0.75f});
    outer.setPosition({pos.x - 10.f, pos.y - 6.f});
    target.draw(outer);

    CircleShape pupil(3.f, 16);
    pupil.setFillColor(col);
    pupil.setPosition({pos.x - 3.f, pos.y - 3.f});
    target.draw(pupil);

    if (!open) {
        RectangleShape slash({22.f, 1.8f});
        slash.setFillColor(col);
        slash.setRotation(degrees(-45.f));
        slash.setPosition({pos.x - 7.f, pos.y + 7.f});
        target.draw(slash);
    }
}

void LoginScreen::drawVectorLock(RenderTarget& target, Vector2f pos, Color color) {
    // Shackle
    RoundedRectRenderer::draw(target, {pos.x + 2.f, pos.y - 6.f}, {10.f, 8.f}, 4.f, Color::Transparent, 1.8f, color);

    // Body
    RoundedRectRenderer::draw(target, {pos.x, pos.y}, {14.f, 11.f}, 2.5f, color);

    // Keyhole
    CircleShape dot(1.5f, 8);
    dot.setFillColor(Color::White);
    dot.setPosition({pos.x + 5.5f, pos.y + 2.5f});
    target.draw(dot);

    RectangleShape stem({1.5f, 3.5f});
    stem.setFillColor(Color::White);
    stem.setPosition({pos.x + 6.25f, pos.y + 4.5f});
    target.draw(stem);
}

void LoginScreen::drawVectorUser(RenderTarget& target, Vector2f pos, Color color) {
    CircleShape head(4.f, 16);
    head.setFillColor(Color::Transparent);
    head.setOutlineThickness(1.6f);
    head.setOutlineColor(color);
    head.setPosition({pos.x + 4.f, pos.y - 1.f});
    target.draw(head);

    RoundedRectRenderer::draw(target, {pos.x, pos.y + 9.f}, {16.f, 7.f}, 3.5f, Color::Transparent, 1.6f, color);
}

void LoginScreen::drawVectorClapper(RenderTarget& target, Vector2f pos, Color color) {
    RoundedRectRenderer::draw(target, pos, {24.f, 20.f}, 3.f, color);

    RectangleShape cut1({3.f, 6.f});
    cut1.setFillColor(Color(0, 119, 182));
    cut1.setPosition({pos.x + 4.f, pos.y});
    target.draw(cut1);

    RectangleShape cut2({3.f, 6.f});
    cut2.setFillColor(Color(0, 119, 182));
    cut2.setPosition({pos.x + 11.f, pos.y});
    target.draw(cut2);

    RectangleShape cut3({3.f, 6.f});
    cut3.setFillColor(Color(0, 119, 182));
    cut3.setPosition({pos.x + 18.f, pos.y});
    target.draw(cut3);
}

void LoginScreen::drawVectorFilm(RenderTarget& target, Vector2f pos, Color color) {
    RoundedRectRenderer::draw(target, pos, {18.f, 14.f}, 2.5f, Color::Transparent, 1.5f, color);
    RectangleShape line1({3.f, 8.f});
    line1.setFillColor(color);
    line1.setPosition({pos.x + 3.f, pos.y + 3.f});
    target.draw(line1);

    RectangleShape line2({3.f, 8.f});
    line2.setFillColor(color);
    line2.setPosition({pos.x + 8.f, pos.y + 3.f});
    target.draw(line2);

    RectangleShape line3({3.f, 8.f});
    line3.setFillColor(color);
    line3.setPosition({pos.x + 13.f, pos.y + 3.f});
    target.draw(line3);
}

void LoginScreen::drawVectorCalendar(RenderTarget& target, Vector2f pos, Color color) {
    RoundedRectRenderer::draw(target, pos, {18.f, 15.f}, 2.5f, Color::Transparent, 1.5f, color);
    
    RectangleShape bar({18.f, 3.5f});
    bar.setFillColor(color);
    bar.setPosition(pos);
    target.draw(bar);

    RectangleShape pin1({2.f, 2.5f});
    pin1.setFillColor(color);
    pin1.setPosition({pos.x + 4.f, pos.y - 2.f});
    target.draw(pin1);

    RectangleShape pin2({2.f, 2.5f});
    pin2.setFillColor(color);
    pin2.setPosition({pos.x + 12.f, pos.y - 2.f});
    target.draw(pin2);
}

void LoginScreen::drawVectorTicket(RenderTarget& target, Vector2f pos, Color color) {
    RoundedRectRenderer::draw(target, pos, {18.f, 13.f}, 2.5f, Color::Transparent, 1.5f, color);

    CircleShape dot1(1.5f, 8);
    dot1.setFillColor(color);
    dot1.setPosition({pos.x + 4.f, pos.y + 5.f});
    target.draw(dot1);

    RectangleShape dash({6.f, 1.5f});
    dash.setFillColor(color);
    dash.setPosition({pos.x + 8.f, pos.y + 6.f});
    target.draw(dash);
}

void LoginScreen::drawVectorCup(RenderTarget& target, Vector2f pos, Color color) {
    ConvexShape cup(4);
    cup.setPoint(0, {pos.x + 2.f, pos.y + 3.f});
    cup.setPoint(1, {pos.x + 16.f, pos.y + 3.f});
    cup.setPoint(2, {pos.x + 13.f, pos.y + 16.f});
    cup.setPoint(3, {pos.x + 5.f, pos.y + 16.f});
    cup.setFillColor(Color::Transparent);
    cup.setOutlineThickness(1.5f);
    cup.setOutlineColor(color);
    target.draw(cup);

    CircleShape top1(3.f, 10);
    top1.setFillColor(color);
    top1.setPosition({pos.x + 3.f, pos.y - 1.f});
    target.draw(top1);

    CircleShape top2(3.f, 10);
    top2.setFillColor(color);
    top2.setPosition({pos.x + 8.f, pos.y - 2.f});
    target.draw(top2);

    CircleShape top3(3.f, 10);
    top3.setFillColor(color);
    top3.setPosition({pos.x + 12.f, pos.y - 1.f});
    target.draw(top3);
}

void LoginScreen::drawVectorChart(RenderTarget& target, Vector2f pos, Color color) {
    RoundedRectRenderer::draw(target, pos, {18.f, 15.f}, 2.5f, Color::Transparent, 1.5f, color);

    RectangleShape b1({2.5f, 5.f});
    b1.setFillColor(color);
    b1.setPosition({pos.x + 3.5f, pos.y + 7.f});
    target.draw(b1);

    RectangleShape b2({2.5f, 9.f});
    b2.setFillColor(color);
    b2.setPosition({pos.x + 7.5f, pos.y + 3.f});
    target.draw(b2);

    RectangleShape b3({2.5f, 7.f});
    b3.setFillColor(color);
    b3.setPosition({pos.x + 11.5f, pos.y + 5.f});
    target.draw(b3);
}

void LoginScreen::drawVectorPos(RenderTarget& target, Vector2f pos, Color color) {
    RoundedRectRenderer::draw(target, pos, {18.f, 14.f}, 2.5f, Color::Transparent, 1.6f, color);

    CircleShape head(2.f, 10);
    head.setFillColor(color);
    head.setPosition({pos.x + 3.5f, pos.y + 4.f});
    target.draw(head);

    RectangleShape line1({6.f, 1.5f});
    line1.setFillColor(color);
    line1.setPosition({pos.x + 9.f, pos.y + 4.f});
    target.draw(line1);

    RectangleShape line2({6.f, 1.5f});
    line2.setFillColor(color);
    line2.setPosition({pos.x + 9.f, pos.y + 8.f});
    target.draw(line2);
}

void LoginScreen::drawVectorHeaderScreen(RenderTarget& target, Vector2f pos, Color color) {
    RoundedRectRenderer::draw(target, pos, {18.f, 13.f}, 2.5f, Color::Transparent, 1.6f, color);

    RectangleShape stand({3.f, 4.f});
    stand.setFillColor(color);
    stand.setPosition({pos.x + 7.5f, pos.y + 13.f});
    target.draw(stand);

    RectangleShape base({11.f, 2.f});
    base.setFillColor(color);
    base.setPosition({pos.x + 3.5f, pos.y + 16.f});
    target.draw(base);
}

void LoginScreen::drawVectorArrow(RenderTarget& target, Vector2f pos, Color color) {
    // Arrow shaft
    RectangleShape line({13.f, 2.2f});
    line.setFillColor(color);
    line.setPosition({pos.x, pos.y + 1.f});
    target.draw(line);

    // Arrow tip
    ConvexShape tip(3);
    tip.setPoint(0, {pos.x + 13.f, pos.y - 4.f});
    tip.setPoint(1, {pos.x + 18.5f, pos.y + 2.1f});
    tip.setPoint(2, {pos.x + 13.f, pos.y + 8.2f});
    tip.setFillColor(color);
    target.draw(tip);
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
    // ENLARGED DIMENSIONS (Phóng to tràn viền, hiện đại & thoáng đãng)
    const float cardW = 1520.f;
    const float cardH = 820.f;
    const float cardX = (1728.f - cardW) / 2.f; // 104.f
    const float cardY = (972.f - cardH) / 2.f;  // 76.f
    const float leftW = 490.f;
    const float rightX = cardX + leftW;         // 594.f
    const float rightW = cardW - leftW;         // 1030.f
    const float formW = 550.f;
    const float formX = rightX + (rightW - formW) / 2.f; // 834.f

    FloatRect emailBoxBounds({formX, cardY + 224.f}, {formW, 52.f});
    FloatRect passBoxBounds({formX, cardY + 324.f}, {formW, 52.f});
    FloatRect eyeBounds({formX + formW - 46.f, cardY + 324.f}, {44.f, 52.f});
    FloatRect checkBounds({formX, cardY + 394.f}, {180.f, 26.f});
    FloatRect forgotBounds({formX + formW - 140.f, cardY + 394.f}, {140.f, 26.f});
    FloatRect btnBounds({formX, cardY + 438.f}, {formW, 54.f});
    FloatRect posBtnBounds({formX, cardY + 570.f}, {formW, 50.f});
    FloatRect linkRegBounds({formX + 40.f, cardY + 648.f}, {470.f, 28.f});

    // Hover states
    eyeHovered = eyeBounds.contains(mouse);
    checkHovered = checkBounds.contains(mouse);
    linkForgotHovered = forgotBounds.contains(mouse);
    btnHovered = btnBounds.contains(mouse);
    posBtnHovered = posBtnBounds.contains(mouse);
    linkCreateHovered = linkRegBounds.contains(mouse);

    if (isForgotMode) {
        FloatRect forgotCardBounds({(1728.f - 580.f) / 2.f, (972.f - 420.f) / 2.f}, {580.f, 420.f});
        FloatRect fEmailBox({forgotCardBounds.position.x + 40.f, forgotCardBounds.position.y + 118.f}, {500.f, 48.f});
        FloatRect fPassBox({forgotCardBounds.position.x + 40.f, forgotCardBounds.position.y + 196.f}, {500.f, 48.f});
        FloatRect fCloseBtn({forgotCardBounds.position.x + 40.f, forgotCardBounds.position.y + 304.f}, {240.f, 48.f});
        FloatRect fSubmitBtn({forgotCardBounds.position.x + 300.f, forgotCardBounds.position.y + 304.f}, {240.f, 48.f});

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

        if (posBtnHovered) {
            emailInput = L"admin@cinexine.vn";
            passInput = L"admin123";
            statusColor = Color(2, 132, 199);
            statusMessage = L"Đã nạp sẵn tài khoản Admin POS!";
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

    // 1. Off-white canvas background (#F4F6F9)
    RectangleShape bgCanvas({W, H});
    bgCanvas.setFillColor(Color(244, 246, 249));
    window.draw(bgCanvas);

    // Modern subtle dot grid
    drawDotGrid(window, W, H);

    // 2. Top Header Bar
    drawVectorHeaderScreen(window, {24.f, 20.f}, Color(30, 41, 59));

    Text headerTitle(font);
    headerTitle.setString(L"PBL2 \u2013 H\u1ec6 TH\u1ed0NG QU\u1ea2N L\u00dd R\u1ea0P CHI\u1ebeU PHIM & TH\u1ed0NG K\u00ca DOANH THU");
    headerTitle.setCharacterSize(13);
    headerTitle.setStyle(Text::Bold);
    headerTitle.setFillColor(Color(30, 41, 59));
    headerTitle.setPosition({52.f, 19.f});
    window.draw(headerTitle);

    // Top-right version pill badge
    RoundedRectRenderer::draw(window, {W - 220.f, 15.f}, {145.f, 26.f}, 13.f, Color(226, 232, 240), 1.f, Color(203, 213, 225));
    Text verText(font);
    verText.setString(L"Phi\u00ean b\u1ea3n Desktop v2.4");
    verText.setCharacterSize(11);
    verText.setFillColor(Color(100, 116, 139));
    verText.setPosition({W - 212.f, 20.f});
    window.draw(verText);

    // Window control dots
    CircleShape dotRed(4.5f, 12);
    dotRed.setFillColor(Color(239, 68, 68));
    dotRed.setPosition({W - 55.f, 24.f});
    window.draw(dotRed);

    CircleShape dotYellow(4.5f, 12);
    dotYellow.setFillColor(Color(245, 158, 11));
    dotYellow.setPosition({W - 40.f, 24.f});
    CircleShape dotGreen(4.5f, 12);
    dotGreen.setFillColor(Color(16, 185, 129));
    dotGreen.setPosition({W - 25.f, 24.f});
    window.draw(dotGreen);

    // 3. Central Login Modal Card (ENLARGED & EXPANDED BORDER-TO-BORDER)
    const float cardW = 1520.f;
    const float cardH = 820.f;
    const float cardX = (W - cardW) / 2.f; // 104.f
    const float cardY = (H - cardH) / 2.f; // 76.f
    const float leftW = 490.f;
    const float rightX = cardX + leftW;
    const float rightW = cardW - leftW; // 1030.f

    // Soft drop shadow under the card
    RoundedRectRenderer::draw(window, {cardX - 5.f, cardY + 5.f}, {cardW + 10.f, cardH + 8.f}, 24.f, Color(0, 0, 0, 10));
    RoundedRectRenderer::draw(window, {cardX - 2.f, cardY + 2.f}, {cardW + 4.f, cardH + 4.f}, 22.f, Color(0, 0, 0, 15));

    // Right card body: Pure White (#FFFFFF)
    RoundedRectRenderer::draw(window, {cardX, cardY}, {cardW, cardH}, 22.f, Color::White, 1.f, Color(226, 232, 240));

    // Left Section: Ocean Blue Panel (#0077B6 / #0084C7)
    RoundedRectRenderer::draw(window, {cardX, cardY}, {leftW, cardH}, 22.f, Color(0, 119, 182));
    RectangleShape leftFiller({22.f, cardH});
    leftFiller.setFillColor(Color(0, 119, 182));
    leftFiller.setPosition({cardX + leftW - 22.f, cardY});
    window.draw(leftFiller);

    // ==========================================
    // 4. LEFT SECTION (Ocean Blue Brand Panel)
    // ==========================================
    const float leftPadX = cardX + 36.f; // 140.f
    const float featW = 418.f;
    const float featH = 58.f;

    // Top brand icon container
    RoundedRectRenderer::draw(window, {leftPadX, cardY + 42.f}, {50.f, 50.f}, 12.f, Color(255, 255, 255, 38));
    drawVectorClapper(window, {leftPadX + 13.f, cardY + 57.f}, Color::White);

    Text brandTop(font, L"CINEADMIN ENTERPRISE", 11);
    brandTop.setStyle(Text::Bold);
    brandTop.setFillColor(Color(255, 255, 255, 230));
    brandTop.setPosition({leftPadX + 64.f, cardY + 44.f});
    window.draw(brandTop);

    Text brandMain(font, L"PBL2 Portal", 24);
    brandMain.setStyle(Text::Bold);
    brandMain.setFillColor(Color::White);
    brandMain.setPosition({leftPadX + 64.f, cardY + 60.f});
    window.draw(brandMain);

    // Thin white divider
    RectangleShape brandLine({featW, 1.f});
    brandLine.setFillColor(Color(255, 255, 255, 50));
    brandLine.setPosition({leftPadX, cardY + 110.f});
    window.draw(brandLine);

    // Section title
    Text modelTitle(font, L"M\u00d4 H\u00ccNH V\u1eacN H\u00c0NH C\u1ed0T L\u00d5I", 11);
    modelTitle.setStyle(Text::Bold);
    modelTitle.setFillColor(Color(255, 255, 255, 220));
    modelTitle.setPosition({leftPadX, cardY + 126.f});
    window.draw(modelTitle);

    // 5 Feature Highlight Items (Translucent rounded pills with centered icons)
    const float fY0 = cardY + 154.f;
    const float fSpacing = 68.f;

    // Item 1: Phim & 12 Phòng Chiếu
    RoundedRectRenderer::draw(window, {leftPadX, fY0}, {featW, featH}, 12.f, Color(255, 255, 255, 28), 1.f, Color(255, 255, 255, 42));
    RoundedRectRenderer::draw(window, {leftPadX + 12.f, fY0 + 11.f}, {36.f, 36.f}, 8.f, Color(255, 255, 255, 36));
    drawVectorFilm(window, {leftPadX + 21.f, fY0 + 22.f}, Color::White);
    Text itm1Title(font, L"Qu\u1ea3n l\u00fd Phim & 12 Ph\u00f2ng Chi\u1ebfu", 13);
    itm1Title.setStyle(Text::Bold);
    itm1Title.setFillColor(Color::White);
    itm1Title.setPosition({leftPadX + 58.f, fY0 + 10.f});
    window.draw(itm1Title);
    Text itm1Sub(font, L"\u0110i\u1ec1u ph\u1ed1i \u0111\u1ecbnh d\u1ea1ng chu\u1ea9n IMAX, 2D & 3D", 11);
    itm1Sub.setFillColor(Color(255, 255, 255, 215));
    itm1Sub.setPosition({leftPadX + 58.f, fY0 + 31.f});
    window.draw(itm1Sub);

    // Item 2: Suất Chiếu & Lịch Trình
    const float fY1 = fY0 + fSpacing;
    RoundedRectRenderer::draw(window, {leftPadX, fY1}, {featW, featH}, 12.f, Color(255, 255, 255, 28), 1.f, Color(255, 255, 255, 42));
    RoundedRectRenderer::draw(window, {leftPadX + 12.f, fY1 + 11.f}, {36.f, 36.f}, 8.f, Color(255, 255, 255, 36));
    drawVectorCalendar(window, {leftPadX + 21.f, fY1 + 21.5f}, Color::White);
    Text itm2Title(font, L"Qu\u1ea3n l\u00fd Su\u1ea5t Chi\u1ebfu & L\u1ecbch Tr\u00ecnh", 13);
    itm2Title.setStyle(Text::Bold);
    itm2Title.setFillColor(Color::White);
    itm2Title.setPosition({leftPadX + 58.f, fY1 + 10.f});
    window.draw(itm2Title);
    Text itm2Sub(font, L"Ph\u00e2n b\u1ed5 khung gi\u1edd v\u00e0ng & l\u1ecbch v\u1eadn h\u00e0nh t\u1ef1 \u0111\u1ed9ng", 11);
    itm2Sub.setFillColor(Color(255, 255, 255, 215));
    itm2Sub.setPosition({leftPadX + 58.f, fY1 + 31.f});
    window.draw(itm2Sub);

    // Item 3: Vé & Thông tin Khách Hàng
    const float fY2 = fY1 + fSpacing;
    RoundedRectRenderer::draw(window, {leftPadX, fY2}, {featW, featH}, 12.f, Color(255, 255, 255, 28), 1.f, Color(255, 255, 255, 42));
    RoundedRectRenderer::draw(window, {leftPadX + 12.f, fY2 + 11.f}, {36.f, 36.f}, 8.f, Color(255, 255, 255, 36));
    drawVectorTicket(window, {leftPadX + 21.f, fY2 + 22.5f}, Color::White);
    Text itm3Title(font, L"Qu\u1ea3n l\u00fd V\u00e9 & Th\u00f4ng tin Kh\u00e1ch H\u00e0ng", 13);
    itm3Title.setStyle(Text::Bold);
    itm3Title.setFillColor(Color::White);
    itm3Title.setPosition({leftPadX + 58.f, fY2 + 10.f});
    window.draw(itm3Title);
    Text itm3Sub(font, L"Qu\u1ea3n tr\u1ecb h\u1ed9i vi\u00ean, t\u00edch l\u0169y \u0111i\u1ec3m v\u00e0 h\u1ea1ng th\u1ebb VIP", 11);
    itm3Sub.setFillColor(Color(255, 255, 255, 215));
    itm3Sub.setPosition({leftPadX + 58.f, fY2 + 31.f});
    window.draw(itm3Sub);

    // Item 4: Combo Bắp Nước & Khuyến Mãi
    const float fY3 = fY2 + fSpacing;
    RoundedRectRenderer::draw(window, {leftPadX, fY3}, {featW, featH}, 12.f, Color(255, 255, 255, 28), 1.f, Color(255, 255, 255, 42));
    RoundedRectRenderer::draw(window, {leftPadX + 12.f, fY3 + 11.f}, {36.f, 36.f}, 8.f, Color(255, 255, 255, 36));
    drawVectorCup(window, {leftPadX + 21.f, fY3 + 20.f}, Color::White);
    Text itm4Title(font, L"Qu\u1ea3n l\u00fd Combo B\u1eafp N\u01b0\u1edbc & Khuy\u1ebfn M\u00e3i", 13);
    itm4Title.setStyle(Text::Bold);
    itm4Title.setFillColor(Color::White);
    itm4Title.setPosition({leftPadX + 58.f, fY3 + 10.f});
    window.draw(itm4Title);
    Text itm4Sub(font, L"\u0110\u1ecbnh l\u01b0\u1ee3ng t\u1ed3n kho b\u1ebfp n\u01b0\u1edbc v\u00e0 m\u00e3 voucher gi\u1ea3m gi\u00e1", 11);
    itm4Sub.setFillColor(Color(255, 255, 255, 215));
    itm4Sub.setPosition({leftPadX + 58.f, fY3 + 31.f});
    window.draw(itm4Sub);

    // Item 5: Thống Kê Doanh Thu
    const float fY4 = fY3 + fSpacing;
    RoundedRectRenderer::draw(window, {leftPadX, fY4}, {featW, featH}, 12.f, Color(255, 255, 255, 28), 1.f, Color(255, 255, 255, 42));
    RoundedRectRenderer::draw(window, {leftPadX + 12.f, fY4 + 11.f}, {36.f, 36.f}, 8.f, Color(255, 255, 255, 36));
    drawVectorChart(window, {leftPadX + 21.f, fY4 + 21.5f}, Color::White);
    Text itm5Title(font, L"Th\u1ed1ng k\u00ea Doanh Thu", 13);
    itm5Title.setStyle(Text::Bold);
    itm5Title.setFillColor(Color::White);
    itm5Title.setPosition({leftPadX + 58.f, fY4 + 10.f});
    window.draw(itm5Title);
    Text itm5Sub(font, L"B\u00e1o c\u00e1o \u0111a chi\u1ec1u theo Ng\u00e0y / Th\u00e1ng / T\u1eebng phim", 11);
    itm5Sub.setFillColor(Color(255, 255, 255, 215));
    itm5Sub.setPosition({leftPadX + 58.f, fY4 + 31.f});
    window.draw(itm5Sub);

    // Bottom Status Container (Dark Ocean Blue #004B78)
    const float btmBoxY = cardY + cardH - 88.f;
    RoundedRectRenderer::draw(window, {leftPadX, btmBoxY}, {featW, 60.f}, 10.f, Color(0, 75, 120));

    // Green dot + Terminal Node Online
    CircleShape tNodeDot(4.f, 12);
    tNodeDot.setFillColor(Color(52, 211, 153));
    tNodeDot.setPosition({leftPadX + 16.f, btmBoxY + 16.f});
    window.draw(tNodeDot);

    Text tNodeText(font, L"Terminal Node Online", 11);
    tNodeText.setFillColor(Color(241, 245, 249));
    tNodeText.setPosition({leftPadX + 30.f, btmBoxY + 12.f});
    window.draw(tNodeText);

    // Cyan dot + DB: Connected
    CircleShape dbDot(4.f, 12);
    dbDot.setFillColor(Color(56, 189, 248));
    dbDot.setPosition({leftPadX + 16.f, btmBoxY + 36.f});
    window.draw(dbDot);

    Text dbConnText(font, L"DB: Connected", 11);
    dbConnText.setFillColor(Color(241, 245, 249));
    dbConnText.setPosition({leftPadX + 30.f, btmBoxY + 32.f});
    window.draw(dbConnText);

    // Right tags in status box
    Text portText(font, L"PORT : 8088", 11);
    portText.setStyle(Text::Bold);
    portText.setFillColor(Color(56, 189, 248));
    portText.setPosition({leftPadX + featW - portText.getLocalBounds().size.x - 16.f, btmBoxY + 12.f});
    window.draw(portText);

    Text sqlText(font, L"MS-SQL Server", 11);
    sqlText.setFillColor(Color(203, 213, 225));
    sqlText.setPosition({leftPadX + featW - sqlText.getLocalBounds().size.x - 16.f, btmBoxY + 32.f});
    window.draw(sqlText);

    // ==========================================
    // 5. RIGHT SECTION (Pure White Form - ENLARGED)
    // ==========================================
    const float formW = 550.f;
    const float formX = rightX + (rightW - formW) / 2.f; // 834.f

    // Top Badge: "CỔNG QUẢN TRỊ NỘI BỘ"
    RoundedRectRenderer::draw(window, {formX, cardY + 52.f}, {180.f, 32.f}, 16.f, Color(239, 246, 255), 1.f, Color(186, 230, 253));
    
    // Checkmark icon
    RectangleShape ck1({4.5f, 2.f});
    ck1.setFillColor(Color(2, 132, 199));
    ck1.setRotation(degrees(45.f));
    ck1.setPosition({formX + 16.f, cardY + 68.f});
    window.draw(ck1);

    RectangleShape ck2({9.f, 2.f});
    ck2.setFillColor(Color(2, 132, 199));
    ck2.setRotation(degrees(-55.f));
    ck2.setPosition({formX + 19.5f, cardY + 71.f});
    window.draw(ck2);

    Text topBadgeText(font, L"C\u1ed4NG QU\u1ea2N TR\u1eca N\u1ed8I B\u1ed8", 11);
    topBadgeText.setStyle(Text::Bold);
    topBadgeText.setFillColor(Color(2, 132, 199));
    topBadgeText.setPosition({formX + 33.f, cardY + 59.f});
    window.draw(topBadgeText);

    // Heading: ĐĂNG NHẬP QUẢN TRỊ VIÊN
    Text formHeading(font, L"\u0110\u0102NG NH\u1eacP QU\u1ea2N TR\u1eca VI\u00caN", 28);
    formHeading.setStyle(Text::Bold);
    formHeading.setFillColor(Color(15, 23, 42)); // Dark Navy #0F172A
    formHeading.setPosition({formX, cardY + 102.f});
    window.draw(formHeading);

    // Subtitle
    Text formSub(font, L"Vui l\u00f2ng x\u00e1c th\u1ef1c t\u00e0i kho\u1ea3n \u0111\u1ec3 truy c\u1eadp b\u1ea3ng \u0111i\u1ec1u khi\u1ec3n r\u1ea1p chi\u1ebfu phim", 13);
    formSub.setFillColor(Color(100, 116, 139));
    formSub.setPosition({formX, cardY + 148.f});
    window.draw(formSub);

    // Field 1: TÀI KHOẢN QUẢN TRỊ VIÊN / MÃ ĐỊNH DANH
    Text lblEmail(font, L"T\u00c0I KHO\u1ea2N QU\u1ea2N TR\u1eca VI\u00caN / M\u00c3 \u0110\u1ecaNH DANH", 11);
    lblEmail.setStyle(Text::Bold);
    lblEmail.setFillColor(Color(51, 65, 85));
    lblEmail.setPosition({formX, cardY + 196.f});
    window.draw(lblEmail);

    Color eBorder = emailActive ? Color(2, 132, 199) : Color(226, 232, 240);
    RoundedRectRenderer::draw(window, {formX, cardY + 224.f}, {formW, 52.f}, 10.f, Color(248, 250, 252), 1.2f, eBorder);
    drawVectorUser(window, {formX + 18.f, cardY + 242.f}, Color(148, 163, 184));

    if (emailInput.empty() && !emailActive) {
        Text phEmail(font, L"VD: admin_danang01 ho\u1eb7c ADM-9842", 13);
        phEmail.setFillColor(Color(148, 163, 184));
        phEmail.setPosition({formX + 48.f, cardY + 239.f});
        window.draw(phEmail);
    } else {
        Text valEmail(font, emailInput, 14);
        valEmail.setFillColor(Color(15, 23, 42));
        valEmail.setPosition({formX + 48.f, cardY + 238.f});
        window.draw(valEmail);

        if (emailActive && caretVisible) {
            FloatRect eb = valEmail.getLocalBounds();
            RectangleShape caretLine({1.8f, 20.f});
            caretLine.setFillColor(Color(2, 132, 199));
            caretLine.setPosition({formX + 50.f + (emailInput.empty() ? 0.f : eb.size.x), cardY + 240.f});
            window.draw(caretLine);
        }
    }

    // Field 2: MẬT KHẨU TRUY CẬP
    Text lblPass(font, L"M\u1eacT KH\u1ea8U TRUY C\u1eacP", 11);
    lblPass.setStyle(Text::Bold);
    lblPass.setFillColor(Color(51, 65, 85));
    lblPass.setPosition({formX, cardY + 296.f});
    window.draw(lblPass);

    Color pBorder = passActive ? Color(2, 132, 199) : Color(226, 232, 240);
    RoundedRectRenderer::draw(window, {formX, cardY + 324.f}, {formW, 52.f}, 10.f, Color(248, 250, 252), 1.2f, pBorder);
    drawVectorLock(window, {formX + 19.f, cardY + 345.f}, Color(148, 163, 184));

    if (passInput.empty() && !passActive) {
        Text phPass(font, L"Nh\u1eadp m\u1eadt kh\u1ea9u an to\u00e0n", 13);
        phPass.setFillColor(Color(148, 163, 184));
        phPass.setPosition({formX + 48.f, cardY + 339.f});
        window.draw(phPass);
    } else {
        wstring dispPass = showPassword ? passInput : bullets(passInput.size());
        Text valPass(font, dispPass, 14);
        valPass.setFillColor(Color(15, 23, 42));
        valPass.setPosition({formX + 48.f, cardY + 338.f});
        window.draw(valPass);

        if (passActive && caretVisible) {
            FloatRect pb = valPass.getLocalBounds();
            RectangleShape caretLine({1.8f, 20.f});
            caretLine.setFillColor(Color(2, 132, 199));
            caretLine.setPosition({formX + 50.f + (passInput.empty() ? 0.f : pb.size.x), cardY + 340.f});
            window.draw(caretLine);
        }
    }

    // Eye toggle button
    drawVectorEye(window, {formX + formW - 24.f, cardY + 350.f}, showPassword, eyeHovered);

    // Options Row: Checkbox "Ghi nhớ đăng nhập" & Link "Quên mật khẩu?"
    Color cbBorder = checkHovered ? Color(2, 132, 199) : Color(203, 213, 225);
    RoundedRectRenderer::draw(window, {formX + 1.f, cardY + 397.f}, {18.f, 18.f}, 4.f, Color(248, 250, 252), 1.2f, cbBorder);
    if (rememberMe) {
        RectangleShape c1({4.5f, 2.f});
        c1.setFillColor(Color(2, 132, 199));
        c1.setRotation(degrees(45.f));
        c1.setPosition({formX + 4.f, cardY + 406.f});
        window.draw(c1);

        RectangleShape c2({9.f, 2.f});
        c2.setFillColor(Color(2, 132, 199));
        c2.setRotation(degrees(-55.f));
        c2.setPosition({formX + 7.5f, cardY + 409.f});
        window.draw(c2);
    }

    Text remText(font, L"Ghi nh\u1edb \u0111\u0103ng nh\u1eadp", 12);
    remText.setFillColor(Color(71, 85, 105));
    remText.setPosition({formX + 28.f, cardY + 396.f});
    window.draw(remText);

    Text forgotText(font, L"Qu\u00ean m\u1eadt kh\u1ea9u?", 12);
    forgotText.setStyle(Text::Bold | (linkForgotHovered ? Text::Underlined : Text::Regular));
    forgotText.setFillColor(Color(2, 132, 199));
    forgotText.setPosition({formX + formW - forgotText.getLocalBounds().size.x, cardY + 396.f});
    window.draw(forgotText);

    // Main Submit Button: ĐĂNG NHẬP QUẢN TRỊ ->
    Color btnCol = btnHovered ? Color(3, 105, 161) : Color(0, 119, 182);
    RoundedRectRenderer::draw(window, {formX, cardY + 438.f}, {formW, 54.f}, 10.f, btnCol);

    Text submitText(font, L"\u0110\u0102NG NH\u1eacP QU\u1ea2N TR\u1eca", 15);
    submitText.setStyle(Text::Bold);
    submitText.setFillColor(Color::White);
    FloatRect stb = submitText.getLocalBounds();
    float totalBtnContentW = stb.size.x + 28.f;
    float btnContentStartX = formX + (formW - totalBtnContentW) / 2.f;
    submitText.setPosition({btnContentStartX, cardY + 454.f});
    window.draw(submitText);

    // Draw crisp vector arrow right after the button text (No missing font glyphs!)
    drawVectorArrow(window, {btnContentStartX + stb.size.x + 12.f, cardY + 465.f}, Color::White);

    // Status Message
    if (!statusMessage.empty()) {
        Text msgText(font, statusMessage, 12);
        msgText.setStyle(Text::Bold);
        msgText.setFillColor(statusColor);
        FloatRect mb = msgText.getLocalBounds();
        msgText.setPosition({formX + (formW - mb.size.x) / 2.f, cardY + 504.f});
        window.draw(msgText);
    }

    // Divider line with "XÁC THỰC NỘI BỘ"
    const float divY = cardY + 538.f;
    RectangleShape hLine({formW, 1.f});
    hLine.setFillColor(Color(226, 232, 240));
    hLine.setPosition({formX, divY});
    window.draw(hLine);

    Text orText(font, L"X\u00c1C TH\u1ef0C N\u1ed8I B\u1ed8", 11);
    orText.setStyle(Text::Bold);
    orText.setFillColor(Color(148, 163, 184));
    FloatRect otb = orText.getLocalBounds();
    float otX = formX + (formW - otb.size.x) / 2.f;

    RectangleShape orMask({otb.size.x + 18.f, 18.f});
    orMask.setFillColor(Color::White);
    orMask.setPosition({otX - 9.f, divY - 9.f});
    window.draw(orMask);

    orText.setPosition({otX, divY - 7.f});
    window.draw(orText);

    // POS / Terminal Key Button
    Color posBg = posBtnHovered ? Color(241, 245, 249) : Color(248, 250, 252);
    RoundedRectRenderer::draw(window, {formX, cardY + 570.f}, {formW, 50.f}, 10.f, posBg, 1.2f, Color(226, 232, 240));
    drawVectorPos(window, {formX + 76.f, cardY + 587.f}, Color(2, 132, 199));

    Text posText(font, L"\u0110\u0103ng nh\u1eadp b\u1eb1ng M\u00e3 Nh\u00e2n Vi\u00ean POS / Terminal Key", 13);
    posText.setStyle(Text::Bold);
    posText.setFillColor(Color(51, 65, 85));
    posText.setPosition({formX + 104.f, cardY + 585.f});
    window.draw(posText);

    // Register Link Row: Chưa có tài khoản quản trị? Đăng ký Admin mới  →
    Text regPrefix(font, L"Ch\u01b0a c\u00f3 t\u00e0i kho\u1ea3n qu\u1ea3n tr\u1ecb? ", 13);
    regPrefix.setFillColor(Color(100, 116, 139));

    Text regLink(font, L"\u0110\u0103ng k\u00fd Admin m\u1edbi \u2192", 13);
    regLink.setStyle(Text::Bold | (linkCreateHovered ? Text::Underlined : Text::Regular));
    regLink.setFillColor(Color(2, 132, 199));

    float fullRegW = regPrefix.getLocalBounds().size.x + regLink.getLocalBounds().size.x;
    float regStartX = formX + (formW - fullRegW) / 2.f;
    regPrefix.setPosition({regStartX, cardY + 648.f});
    regLink.setPosition({regStartX + regPrefix.getLocalBounds().size.x, cardY + 648.f});
    window.draw(regPrefix);
    window.draw(regLink);

    // 6. Bottom Footer Bar
    Text footLeft(font, L"\u00a9 2024 PBL2 Cinema Management System. B\u1ea3n quy\u1ec1n thu\u1ed9c B\u1ed9 m\u00f4n K\u1ef9 thu\u1eadt ph\u1ea7n m\u1ec1m.", 11);
    footLeft.setFillColor(Color(100, 116, 139));
    footLeft.setPosition({cardX, H - 32.f});
    window.draw(footLeft);

    Text footRight(font, L"\u0110i\u1ec1u kho\u1ea3n d\u1ecbch v\u1ee5   \u2022   Tr\u1ee3 gi\u00fap k\u1ef9 thu\u1eadt", 11);
    footRight.setFillColor(Color(100, 116, 139));
    FloatRect frb = footRight.getLocalBounds();
    footRight.setPosition({cardX + cardW - frb.size.x, H - 32.f});
    window.draw(footRight);

    // ==========================================
    // 7. FORGOT PASSWORD MODAL (if open)
    // ==========================================
    if (isForgotMode) {
        RectangleShape fOverlay({W, H});
        fOverlay.setFillColor(Color(15, 23, 42, 140));
        window.draw(fOverlay);

        const float fCardW = 560.f;
        const float fCardH = 400.f;
        const float fCardX = (W - fCardW) / 2.f;
        const float fCardY = (H - fCardH) / 2.f;

        RoundedRectRenderer::draw(window, {fCardX - 3.f, fCardY + 3.f}, {fCardW + 6.f, fCardH + 6.f}, 18.f, Color(0, 0, 0, 30));
        RoundedRectRenderer::draw(window, {fCardX, fCardY}, {fCardW, fCardH}, 16.f, Color::White, 1.f, Color(226, 232, 240));

        Text fTitle(font, L"QU\u00caN M\u1eacT KH\u1ea8U ADMIN", 21);
        fTitle.setStyle(Text::Bold);
        fTitle.setFillColor(Color(15, 23, 42));
        fTitle.setPosition({fCardX + 40.f, fCardY + 32.f});
        window.draw(fTitle);

        Text fSub(font, L"Nh\u1eadp email qu\u1ea3n tr\u1ecb v\u00e0 m\u1eadt kh\u1ea9u m\u1edbi \u0111\u1ec3 \u0111\u1eb7t l\u1ea1i m\u1eadt kh\u1ea9u.", 13);
        fSub.setFillColor(Color(100, 116, 139));
        fSub.setPosition({fCardX + 40.f, fCardY + 66.f});
        window.draw(fSub);

        // Email input
        Color fEBorder = forgotEmailActive ? Color(2, 132, 199) : Color(203, 213, 225);
        RoundedRectRenderer::draw(window, {fCardX + 40.f, fCardY + 112.f}, {480.f, 46.f}, 8.f, Color(248, 250, 252), 1.2f, fEBorder);
        Text fEmailText(font, forgotEmailInput.empty() ? L"Nh\u1eadp email qu\u1ea3n tr\u1ecb vi\u00ean" : forgotEmailInput, 13);
        fEmailText.setFillColor(forgotEmailInput.empty() ? Color(148, 163, 184) : Color(15, 23, 42));
        fEmailText.setPosition({fCardX + 54.f, fCardY + 126.f});
        window.draw(fEmailText);

        // New password input
        Color fPBorder = forgotPassActive ? Color(2, 132, 199) : Color(203, 213, 225);
        RoundedRectRenderer::draw(window, {fCardX + 40.f, fCardY + 180.f}, {480.f, 46.f}, 8.f, Color(248, 250, 252), 1.2f, fPBorder);
        Text fPassText(font, forgotPassInput.empty() ? L"Nh\u1eadp m\u1eadt kh\u1ea9u m\u1edbi" : bullets(forgotPassInput.size()), 13);
        fPassText.setFillColor(forgotPassInput.empty() ? Color(148, 163, 184) : Color(15, 23, 42));
        fPassText.setPosition({fCardX + 54.f, fCardY + 194.f});
        window.draw(fPassText);

        // Status message
        if (!forgotMsg.empty()) {
            Text fMsgDisplay(font, forgotMsg, 12);
            fMsgDisplay.setStyle(Text::Bold);
            fMsgDisplay.setFillColor(forgotMsgColor);
            fMsgDisplay.setPosition({fCardX + 40.f, fCardY + 246.f});
            window.draw(fMsgDisplay);
        }

        // Buttons
        Color fCloseBg = forgotCloseHovered ? Color(203, 213, 225) : Color(226, 232, 240);
        RoundedRectRenderer::draw(window, {fCardX + 40.f, fCardY + 290.f}, {230.f, 46.f}, 8.f, fCloseBg);
        Text fCloseText(font, L"\u0110\u00f3ng", 14);
        fCloseText.setStyle(Text::Bold);
        fCloseText.setFillColor(Color(51, 65, 85));
        FloatRect ctb = fCloseText.getLocalBounds();
        fCloseText.setPosition({fCardX + 40.f + (230.f - ctb.size.x) / 2.f, fCardY + 303.f});
        window.draw(fCloseText);

        Color fSubBg = forgotBtnHovered ? Color(3, 105, 161) : Color(0, 119, 182);
        RoundedRectRenderer::draw(window, {fCardX + 290.f, fCardY + 290.f}, {230.f, 46.f}, 8.f, fSubBg);
        Text fSubText(font, L"\u0110\u1eb7t l\u1ea1i MK", 14);
        fSubText.setStyle(Text::Bold);
        fSubText.setFillColor(Color::White);
        FloatRect stb2 = fSubText.getLocalBounds();
        fSubText.setPosition({fCardX + 290.f + (230.f - stb2.size.x) / 2.f, fCardY + 303.f});
        window.draw(fSubText);
    }
}