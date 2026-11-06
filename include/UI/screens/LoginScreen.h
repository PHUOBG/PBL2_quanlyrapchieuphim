#pragma once
#include <SFML/Graphics.hpp> 
#include <string>
#include <memory>
#include <optional>

#include "services/AuthService.h"
#include "core/AppState.h"

using namespace sf;
using namespace std;

class LoginScreen {
public:
    LoginScreen(const Font& font, AuthService& auth);
    bool update(Vector2f mousePos, bool mousePressed, string& currentUser, string& currentUserEmail, AppState& state);
    void handleEvent(const Event& event, AppState& state, string& user, string& email, bool& successFlag);
    void draw(RenderWindow& window);

private:
    AuthService& auth;
    const Font& font;
    Font boldFont;

    // User inputs
    wstring emailInput;
    wstring passInput;
    bool emailActive = false;
    bool passActive = false;
    bool showPassword = false;
    bool rememberMe = true;
    bool caretVisible = true;
    bool loginSuccess = false;
    Clock caretClock;
    Clock loginClock;
    string loggedUser;

    // Interactive Hover States
    bool btnHovered = false;
    bool linkCreateHovered = false;
    bool linkForgotHovered = false;
    bool posBtnHovered = false;
    bool eyeHovered = false;
    bool checkHovered = false;

    // Status Message
    wstring statusMessage;
    Color statusColor;

    // Forgot Password Modal
    bool isForgotMode = false;
    wstring forgotEmailInput;
    wstring forgotPassInput;
    bool forgotEmailActive = false;
    bool forgotPassActive = false;
    wstring forgotMsg;
    Color forgotMsgColor;
    bool forgotBtnHovered = false;
    bool forgotCloseHovered = false;

    // Brand logo
    Texture logoTexture;
    optional<Sprite> logoSprite;

    // Helper functions
    wstring bullets(size_t n);
    void drawVectorEye(RenderTarget& target, Vector2f pos, bool open, bool hovered);
    void drawVectorLock(RenderTarget& target, Vector2f pos, Color color);
    void drawVectorUser(RenderTarget& target, Vector2f pos, Color color);
    void drawVectorClapper(RenderTarget& target, Vector2f pos, Color color);
    void drawVectorFilm(RenderTarget& target, Vector2f pos, Color color);
    void drawVectorCalendar(RenderTarget& target, Vector2f pos, Color color);
    void drawVectorTicket(RenderTarget& target, Vector2f pos, Color color);
    void drawVectorCup(RenderTarget& target, Vector2f pos, Color color);
    void drawVectorChart(RenderTarget& target, Vector2f pos, Color color);
    void drawVectorPos(RenderTarget& target, Vector2f pos, Color color);
    void drawVectorHeaderScreen(RenderTarget& target, Vector2f pos, Color color);
    void drawVectorAdminUser(RenderTarget& target, Vector2f pos, Color color);
    void drawVectorArrow(RenderTarget& target, Vector2f pos, Color color);
    void drawDotGrid(RenderTarget& target, float W, float H);
};