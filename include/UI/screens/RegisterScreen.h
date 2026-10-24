#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include "services/AuthService.h"
#include "core/AppState.h"

using namespace sf;
using namespace std;

class RegisterScreen {
private:
    AuthService& auth;

    RectangleShape overlay, card, registerBtn, backBtn, cursor;
    Text title, registerBtnText, backBtnText, closeX, msg;
    vector<RectangleShape> inputBoxes;
    vector<Text> labels;
    vector<Text> placeholders;
    vector<Text> displays; 

    Clock cursorClock, messageTimer;
    bool showCursor = true, showSuccessMessage = false;

    int activeField = -1;
    wstring inputs[6];

    wstring bullets(size_t n);
    bool validateInputs();
public:
    RegisterScreen(const Font&, AuthService&);

    bool update(Vector2f, bool, AppState&);
    void handleEvent(const Event&, AppState&);
    void draw(RenderWindow&);
    void reset();
};
