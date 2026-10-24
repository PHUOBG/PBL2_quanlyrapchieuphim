#include "UI/screens/LoginScreen.h"    
#include "core/AppState.h"
#include "core/AppRole.h"
#include <SFML/Graphics.hpp>

using namespace sf;

LoginScreen::LoginScreen(const Font& font, AuthService& authRef)
    : auth(authRef),
      overlay({0.f, 0.f}),
      card({480.f, 400.f}),
      emailBox({380.f, 44.f}),
      passBox({380.f, 44.f}),
      btn({380.f, 44.f}),
      eyeToggleArea({40.f, 44.f}),
      caret({0.f, 0.f}),
      title(font, L"ĐĂNG NHẬP QUẢN TRỊ VIÊN", 24),
      emailPH(font, L"Nhập email quản trị viên", 16),
      passPH(font, L"Nhập mật khẩu", 16),
      btnText(font, L"Đăng nhập", 18),
      linkCreate(font, L"Chưa có tài khoản? Đăng ký Admin ngay", 15),
      linkForgot(font, L"Quên mật khẩu?", 15),
      closeX(font, L"X", 22),
      emailDisplay(font, L"", 16),
      passDisplay(font, L"", 16),
      msg(font, L"", 15),
      // Forgot password modal
      forgotCard({480.f, 360.f}),
      forgotEmailBox({380.f, 44.f}),
      forgotNewPassBox({380.f, 44.f}),
      forgotBtn({180.f, 44.f}),
      forgotCloseBtn({180.f, 44.f}),
      forgotTitle(font, L"QUÊN MẬT KHẨU ADMIN", 22),
      forgotEmailPH(font, L"Nhập email tài khoản admin", 15),
      forgotNewPassPH(font, L"Nhập mật khẩu mới", 15),
      forgotBtnText(font, L"Đặt lại MK", 16),
      forgotCloseBtnText(font, L"Đóng", 16),
      forgotMsg(font, L"", 15),
      forgotEmailDisplay(font, L"", 15),
      forgotPassDisplay(font, L"", 15)
{
    overlay.setFillColor(Color(0, 0, 0, 180));

    card.setFillColor(Color(255, 255, 255, 250));
    card.setOrigin(card.getSize() / 2.f);

    title.setFillColor(Color(13, 27, 42));
    title.setStyle(Text::Bold);

    emailBox.setFillColor(Color(240, 243, 246));
    emailBox.setOutlineThickness(1.5f);
    emailBox.setOutlineColor(Color(200, 205, 215));
    emailPH.setFillColor(Color(120, 125, 135));

    passBox = emailBox;
    passPH.setFillColor(Color(120, 125, 135));

    btn.setFillColor(Color(20, 118, 172));
    btnText.setFillColor(Color::White);
    btnText.setStyle(Text::Bold);

    linkCreate.setFillColor(Color(20, 118, 172));
    linkForgot.setFillColor(Color(180, 50, 50));
    closeX.setFillColor(Color(90, 90, 90));

    emailDisplay.setFillColor(Color::Black);
    passDisplay.setFillColor(Color::Black);

    msg.setFillColor(Color(200, 60, 60));

    caret.setSize({2.f, 22.f});
    caret.setFillColor(Color::Black);

    eyeToggleArea.setFillColor(Color::Transparent);
    if (eyeOpenTexture.loadFromFile("../assets/elements/eye-open.png")) {
        eyeOpenTexture.setSmooth(true);
    }
    if (eyeClosedTexture.loadFromFile("../assets/elements/eye-close.png")) {
        eyeClosedTexture.setSmooth(true);
    }
    eyeSprite = make_unique<Sprite>(eyeClosedTexture);

    // Setup forgot password components
    forgotCard.setFillColor(Color(255, 255, 255, 255));
    forgotCard.setOrigin(forgotCard.getSize() / 2.f);
    forgotTitle.setFillColor(Color(13, 27, 42));
    forgotTitle.setStyle(Text::Bold);
    forgotEmailBox = emailBox;
    forgotNewPassBox = passBox;
    forgotEmailPH.setFillColor(Color(120, 125, 135));
    forgotNewPassPH.setFillColor(Color(120, 125, 135));
    forgotBtn.setFillColor(Color(20, 118, 172));
    forgotBtnText.setFillColor(Color::White);
    forgotCloseBtn.setFillColor(Color(150, 155, 165));
    forgotCloseBtnText.setFillColor(Color::White);
    forgotEmailDisplay.setFillColor(Color::Black);
    forgotPassDisplay.setFillColor(Color::Black);
    forgotMsg.setFillColor(Color(200, 60, 60));
}

wstring LoginScreen::bullets(size_t n) { 
    return wstring(n, L'\u2022'); 
}

void LoginScreen::handleEvent(const Event& event, AppState& state, string& user, string& email, bool& successFlag) {
    (void)user;
    (void)email;

    if (const auto* keyEvent = event.getIf<Event::KeyPressed>()) {
        auto code = keyEvent->code;
        if (code == Keyboard::Key::Escape) {
            if (isForgotMode) {
                isForgotMode = false;
                return;
            }
        }
        if (!isForgotMode) {
            if (code == Keyboard::Key::Tab) {
                bool e = emailActive, p = passActive;
                emailActive = !e && !p ? true : !e;
                passActive  = !emailActive;
            } 
            else if (code == Keyboard::Key::Backspace) {
                if (emailActive && !emailInput.empty()) emailInput.pop_back();
                else if (passActive && !passInput.empty()) passInput.pop_back();
            } 
            else if (code == Keyboard::Key::Enter) {
                string uStr(emailInput.begin(), emailInput.end());
                string pStr(passInput.begin(), passInput.end());
                
                if (auth.login(uStr, pStr)) {
                    User* uObj = auth.getUser(uStr);
                    if (uObj && uObj->getRole() == AppRole::Admin) {
                        msg.setFillColor(Color(40, 160, 60));
                        msg.setString(L"Đăng nhập Quản trị viên thành công!");
                        loginSuccess = true;
                        successFlag = true;
                        loggedUser = uStr;
                        loginClock.restart();
                    } else {
                        msg.setFillColor(Color(200, 60, 60));
                        msg.setString(L"Tài khoản không có quyền Quản trị viên.");
                    }
                } 
                else {
                    msg.setFillColor(Color(200, 60, 60));
                    msg.setString(L"Sai thông tin email hoặc mật khẩu.");
                }
            }
        } else {
            // Forgot password mode
            if (code == Keyboard::Key::Tab) {
                bool e = forgotEmailActive, p = forgotPassActive;
                forgotEmailActive = !e && !p ? true : !e;
                forgotPassActive  = !forgotEmailActive;
            }
            else if (code == Keyboard::Key::Backspace) {
                if (forgotEmailActive && !forgotEmailInput.empty()) forgotEmailInput.pop_back();
                else if (forgotPassActive && !forgotPassInput.empty()) forgotPassInput.pop_back();
            }
        }
    }
    
    if (const auto* textEvent = event.getIf<Event::TextEntered>()) {
        char32_t unicode = textEvent->unicode;
        if (unicode >= 32 && unicode != 127) {
            if (!isForgotMode) {
                if (emailActive && emailInput.size() < 48) emailInput.push_back((wchar_t)unicode);
                else if (passActive && passInput.size() < 48) passInput.push_back((wchar_t)unicode);
            } else {
                if (forgotEmailActive && forgotEmailInput.size() < 48) forgotEmailInput.push_back((wchar_t)unicode);
                else if (forgotPassActive && forgotPassInput.size() < 48) forgotPassInput.push_back((wchar_t)unicode);
            }
        }
    }
}

bool LoginScreen::update(Vector2f mouse, bool mousePressed, string& currentUser, string& currentUserEmail, AppState& state) {
    const Vector2f center(864.f, 486.f);
    card.setPosition(center);
    closeX.setPosition({card.getPosition().x + card.getSize().x / 2.f - 28.f,
                        card.getPosition().y - card.getSize().y / 2.f + 8.f});

    if (isForgotMode) {
        forgotCard.setPosition(center);
        if (mousePressed) {
            forgotEmailActive = forgotEmailBox.getGlobalBounds().contains(mouse);
            forgotPassActive  = forgotNewPassBox.getGlobalBounds().contains(mouse);

            if (forgotCloseBtn.getGlobalBounds().contains(mouse)) {
                isForgotMode = false;
                forgotMsg.setString(L"");
            }
            else if (forgotBtn.getGlobalBounds().contains(mouse)) {
                string fEmail(forgotEmailInput.begin(), forgotEmailInput.end());
                string fPass(forgotPassInput.begin(), forgotPassInput.end());
                if (fEmail.empty() || fPass.empty()) {
                    forgotMsg.setFillColor(Color(200, 60, 60));
                    forgotMsg.setString(L"Vui lòng nhập đầy đủ thông tin.");
                } else {
                    User* user = auth.getUser(fEmail);
                    if (user) {
                        user->setPasswordHash(PasswordHasher::hashPassword(fPass));
                        if (auth.getRepository()) auth.getRepository()->saveToFile();
                        forgotMsg.setFillColor(Color(40, 160, 60));
                        forgotMsg.setString(L"Đặt lại mật khẩu thành công! Vui lòng đăng nhập.");
                    } else {
                        forgotMsg.setFillColor(Color(200, 60, 60));
                        forgotMsg.setString(L"Email không tồn tại trong hệ thống.");
                    }
                }
            }
        }
        return false;
    }

    if (mousePressed) {
        if (eyeToggleArea.getGlobalBounds().contains(mouse)) showPassword = !showPassword;
        emailActive = emailBox.getGlobalBounds().contains(mouse);
        passActive  = passBox.getGlobalBounds().contains(mouse);

        if (btn.getGlobalBounds().contains(mouse)) {
            string u(emailInput.begin(), emailInput.end());
            string p(passInput.begin(), passInput.end());
            if (auth.login(u, p)) {
                User* uObj = auth.getUser(u);
                if (uObj && uObj->getRole() == AppRole::Admin) {
                    msg.setFillColor(Color(40, 160, 60));
                    msg.setString(L"Đăng nhập Quản trị viên thành công!");
                    loginSuccess = true;
                    loggedUser = u;
                    loginClock.restart();
                } else {
                    msg.setFillColor(Color(200, 60, 60));
                    msg.setString(L"Tài khoản không có quyền Quản trị viên.");
                }
            } else {
                msg.setFillColor(Color(200, 60, 60));
                msg.setString(L"Sai thông tin email hoặc mật khẩu.");
            }
        }

        if (linkCreate.getGlobalBounds().contains(mouse)) {
            state = AppState::REGISTER;
        }

        if (linkForgot.getGlobalBounds().contains(mouse)) {
            isForgotMode = true;
            forgotEmailInput = emailInput;
            forgotPassInput = L"";
            forgotMsg.setString(L"");
        }
    }

    if (caretClock.getElapsedTime().asSeconds() >= 0.5f) {
        caretVisible = !caretVisible;
        caretClock.restart();
    }

    if (loginSuccess && loginClock.getElapsedTime().asSeconds() >= 0.4f) {
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
    auto size = window.getSize();
    overlay.setSize({(float)size.x, (float)size.y});
    Vector2f center({size.x * 0.5f, size.y * 0.5f});
    card.setPosition(center);

    // Title centered
    FloatRect tb = title.getLocalBounds();
    title.setPosition({center.x - tb.size.x / 2.f, center.y - 170.f});

    float startY = center.y - 110.f;
    emailBox.setPosition({center.x - 190.f, startY});
    passBox.setPosition({center.x - 190.f, startY + 60.f});

    eyeToggleArea.setPosition({emailBox.getPosition().x + emailBox.getSize().x - 40.f, passBox.getPosition().y});
    if (eyeSprite) {
        eyeSprite->setTexture(showPassword ? eyeOpenTexture : eyeClosedTexture);
        eyeSprite->setPosition({eyeToggleArea.getPosition().x + 8.f, eyeToggleArea.getPosition().y + 10.f});
    }

    btn.setPosition({center.x - 190.f, startY + 125.f});
    FloatRect btnTb = btnText.getLocalBounds();
    btnText.setPosition({center.x - btnTb.size.x / 2.f, btn.getPosition().y + 10.f});

    // Links
    FloatRect lfb = linkForgot.getLocalBounds();
    linkForgot.setPosition({center.x + 190.f - lfb.size.x, startY + 180.f});

    FloatRect lcb = linkCreate.getLocalBounds();
    linkCreate.setPosition({center.x - lcb.size.x / 2.f, startY + 215.f});

    // Message
    FloatRect mb = msg.getLocalBounds();
    msg.setPosition({center.x - mb.size.x / 2.f, startY - 30.f});

    // Texts inside inputs
    emailDisplay.setString(emailInput);
    emailDisplay.setPosition({emailBox.getPosition().x + 12.f, emailBox.getPosition().y + 10.f});
    emailPH.setPosition({emailBox.getPosition().x + 12.f, emailBox.getPosition().y + 10.f});

    wstring pStr = showPassword ? passInput : bullets(passInput.size());
    passDisplay.setString(pStr);
    passDisplay.setPosition({passBox.getPosition().x + 12.f, passBox.getPosition().y + 10.f});
    passPH.setPosition({passBox.getPosition().x + 12.f, passBox.getPosition().y + 10.f});

    window.draw(overlay);
    window.draw(card);
    window.draw(title);
    window.draw(emailBox);
    window.draw(passBox);
    window.draw(btn);
    window.draw(btnText);
    window.draw(linkForgot);
    window.draw(linkCreate);
    window.draw(msg);

    if (emailInput.empty() && !emailActive) window.draw(emailPH);
    else window.draw(emailDisplay);

    if (passInput.empty() && !passActive) window.draw(passPH);
    else window.draw(passDisplay);

    if (eyeSprite) window.draw(*eyeSprite);

    // Draw Caret
    if (caretVisible) {
        if (emailActive) {
            caret.setPosition({emailDisplay.getPosition().x + emailDisplay.getGlobalBounds().size.x + 2.f,
                               emailBox.getPosition().y + 11.f});
            window.draw(caret);
        } else if (passActive) {
            caret.setPosition({passDisplay.getPosition().x + passDisplay.getGlobalBounds().size.x + 2.f,
                               passBox.getPosition().y + 11.f});
            window.draw(caret);
        }
    }

    // Draw Forgot Password Modal if active
    if (isForgotMode) {
        forgotCard.setPosition(center);
        window.draw(forgotCard);

        FloatRect ftb = forgotTitle.getLocalBounds();
        forgotTitle.setPosition({center.x - ftb.size.x / 2.f, center.y - 150.f});
        window.draw(forgotTitle);

        float fStartY = center.y - 90.f;
        forgotEmailBox.setPosition({center.x - 190.f, fStartY});
        forgotNewPassBox.setPosition({center.x - 190.f, fStartY + 60.f});
        window.draw(forgotEmailBox);
        window.draw(forgotNewPassBox);

        forgotEmailDisplay.setString(forgotEmailInput);
        forgotEmailDisplay.setPosition({forgotEmailBox.getPosition().x + 12.f, forgotEmailBox.getPosition().y + 10.f});
        forgotEmailPH.setPosition({forgotEmailBox.getPosition().x + 12.f, forgotEmailBox.getPosition().y + 10.f});

        forgotPassDisplay.setString(forgotPassInput);
        forgotPassDisplay.setPosition({forgotNewPassBox.getPosition().x + 12.f, forgotNewPassBox.getPosition().y + 10.f});
        forgotNewPassPH.setPosition({forgotNewPassBox.getPosition().x + 12.f, forgotNewPassBox.getPosition().y + 10.f});

        if (forgotEmailInput.empty() && !forgotEmailActive) window.draw(forgotEmailPH);
        else window.draw(forgotEmailDisplay);

        if (forgotPassInput.empty() && !forgotPassActive) window.draw(forgotNewPassPH);
        else window.draw(forgotPassDisplay);

        forgotBtn.setPosition({center.x - 190.f, fStartY + 130.f});
        forgotCloseBtn.setPosition({center.x + 10.f, fStartY + 130.f});
        window.draw(forgotBtn);
        window.draw(forgotCloseBtn);

        FloatRect fbb = forgotBtnText.getLocalBounds();
        forgotBtnText.setPosition({forgotBtn.getPosition().x + (forgotBtn.getSize().x - fbb.size.x) / 2.f, forgotBtn.getPosition().y + 10.f});
        window.draw(forgotBtnText);

        FloatRect fcb = forgotCloseBtnText.getLocalBounds();
        forgotCloseBtnText.setPosition({forgotCloseBtn.getPosition().x + (forgotCloseBtn.getSize().x - fcb.size.x) / 2.f, forgotCloseBtn.getPosition().y + 10.f});
        window.draw(forgotCloseBtnText);

        FloatRect fmb = forgotMsg.getLocalBounds();
        forgotMsg.setPosition({center.x - fmb.size.x / 2.f, fStartY - 25.f});
        window.draw(forgotMsg);
    }
}