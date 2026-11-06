#pragma once
#include <SFML/Graphics.hpp>
#include <memory>
#include <string>

#include "UI/screens/LoginScreen.h"
#include "UI/screens/RegisterScreen.h"
#include "UI/screens/AdminScreen.h"

#include "services/AuthService.h"
#include "services/ShowtimeCleanupService.h"
#include "core/AppState.h"

using namespace std;
using namespace sf;

class App {
private:
    RenderWindow window;
    Font font, detail_font, title_font;

    unique_ptr<LoginScreen> loginScreen;
    unique_ptr<RegisterScreen> registerScreen;
    unique_ptr<AdminScreen> adminScreen;

    unique_ptr<AuthService> authService;
    string currentUser;
    string currentUserEmail;
    bool loginSuccess = false;

    AppState state;
    AppState previousState;

    void handleStateChange();
    void resetAfterLogout();
    bool isAdminState(AppState s);
    void render();

public:
    App();
    void run();
};