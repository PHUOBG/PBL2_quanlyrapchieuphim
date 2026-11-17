#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <optional>
#include <array>
#include "services/AuthService.h"
#include "core/AppState.h"

using namespace sf;
using namespace std;

class RegisterScreen {
private:
    AuthService& auth;
    const Font& font;
    Font boldFont;

    Clock cursorClock, messageTimer;
    bool showCursor = true, showSuccessMessage = false;

    int activeField = -1;
    wstring inputs[6];
    bool showPassword = false;
    bool agreeTerms = false;

    // Hover states
    bool closeHovered = false;
    bool eyeHovered = false;
    bool checkHovered = false;
    bool backHovered = false;
    bool registerHovered = false;
    bool loginLinkHovered = false;

    wstring statusMessage;
    Color statusColor;

    Texture logoTexture;
    optional<Sprite> logoSprite;

    // Cached layout rects for hit-testing in update()
    array<FloatRect, 6> fieldRects;
    FloatRect closeRect;
    FloatRect eyeRect;
    FloatRect checkRect;
    FloatRect backRect;
    FloatRect registerRect;
    FloatRect loginLinkRect;

    wstring bullets(size_t n) const;
    bool validateInputs();
    void submitRegistration();
    void computeLayout(float W, float H);

    // Vector icon helpers
    void drawDotGrid(RenderTarget& target, float W, float H) const;
    void drawVectorUser(RenderTarget& target, Vector2f pos, Color color) const;
    void drawVectorCalendar(RenderTarget& target, Vector2f pos, Color color) const;
    void drawVectorPhone(RenderTarget& target, Vector2f pos, Color color) const;
    void drawVectorMail(RenderTarget& target, Vector2f pos, Color color) const;
    void drawVectorLock(RenderTarget& target, Vector2f pos, Color color) const;
    void drawVectorShield(RenderTarget& target, Vector2f pos, Color color) const;
    void drawVectorEye(RenderTarget& target, Vector2f pos, bool open, bool hovered) const;
    void drawVectorArrow(RenderTarget& target, Vector2f pos, bool right, Color color) const;
    void drawVectorCheck(RenderTarget& target, Vector2f pos, Color color) const;
    void drawVectorPortalLogo(RenderTarget& target, Vector2f pos, float size) const;

public:
    RegisterScreen(const Font&, AuthService&);

    bool update(Vector2f, bool, AppState&);
    void handleEvent(const Event&, AppState&);
    void draw(RenderWindow&);
    void reset();
};
