#pragma once
#include <SFML/Graphics.hpp> 
#include <string>
#include <memory>

#include "services/AuthService.h"
#include "core/AppState.h"

using namespace sf;
using namespace std;

class LoginScreen {
public:
    LoginScreen(const Font&, AuthService&);
    bool update(Vector2f, bool, string&, string&, AppState&);
    void handleEvent(const Event&, AppState&, string&, string&, bool&);
    void draw(RenderWindow&);

private:
    AuthService& auth;

    RectangleShape overlay, card, emailBox, passBox, btn, eyeToggleArea, caret;
    Text title, emailPH, passPH, btnText, linkCreate, linkForgot, closeX, emailDisplay, passDisplay, msg;
    Texture eyeOpenTexture, eyeClosedTexture;
    unique_ptr<Sprite> eyeSprite;
    wstring emailInput, passInput;
    bool emailActive = false, passActive = false, showPassword = false, caretVisible = true, loginSuccess = false;
    wstring bullets(size_t n);
    Clock caretClock, loginClock;
    string loggedUser;

    // Forgot password modal
    bool isForgotMode = false;
    RectangleShape forgotCard, forgotEmailBox, forgotNewPassBox, forgotBtn, forgotCloseBtn;
    Text forgotTitle, forgotEmailPH, forgotNewPassPH, forgotBtnText, forgotCloseBtnText, forgotMsg;
    Text forgotEmailDisplay, forgotPassDisplay;
    wstring forgotEmailInput, forgotPassInput;
    bool forgotEmailActive = false, forgotPassActive = false;
};