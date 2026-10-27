#include "core/App.h"
#include <SFML/Graphics.hpp>

using namespace sf;

App::App()
    : window(VideoMode({1728, 972}), L"PBL2 - HỆ THỐNG QUẢN LÝ RẠP CHIẾU PHIM & THỐNG KÊ DOANH THU", Style::Titlebar | Style::Close),
      font("../assets/fonts/segoeui.ttf"),
      detail_font("../assets/fonts/quicksand_medium.ttf"),
      title_font("../assets/fonts/BEBAS_NEUE_ZSMALL.ttf"),
      state(AppState::LOGIN),
      previousState(AppState::LOGIN)
{
    window.setFramerateLimit(60);

    ShowtimeCleanupService::maintainShowtimes("../data/showtimes.txt", 6);

    authService = make_unique<AuthService>("../data/users.txt");

    loginScreen = make_unique<LoginScreen>(font, *authService);
    registerScreen = make_unique<RegisterScreen>(font, *authService);
}

bool App::isAdminState(AppState s) {
    return (s >= AppState::ADMIN_DASHBOARD && s <= AppState::ADMIN_CHANGE_PASSWORD);
}

void App::handleStateChange() {
    if (state == previousState) return;

    if (isAdminState(state)) {
        if (!adminScreen) {
            adminScreen = make_unique<AdminScreen>(font, window, state, authService.get());
        }
    }
    previousState = state;
}

void App::resetAfterLogout() {
    authService->logout();
    adminScreen.reset();

    loginScreen = make_unique<LoginScreen>(font, *authService);
    registerScreen = make_unique<RegisterScreen>(font, *authService);

    currentUser = "";
    currentUserEmail = "";
    state = AppState::LOGIN;
    previousState = AppState::LOGIN;

    BaseScreen::clearLogoutFlag();
}

void App::render() {
    window.clear(Color(13, 27, 42));

    if (isAdminState(state)) {
        if (adminScreen) adminScreen->render();
    }
    else if (state == AppState::LOGIN) {
        if (loginScreen) loginScreen->draw(window);
    }
    else if (state == AppState::REGISTER) {
        if (registerScreen) registerScreen->draw(window);
    }

    window.display();
}

void App::run() {
    while (window.isOpen()) {
        bool mousePressed = false;
        Vector2f mousePos = window.mapPixelToCoords(Mouse::getPosition(window));

        // 1. Event Polling
        while (auto event = window.pollEvent()) {
            if (event->is<Event::Closed>()) window.close();
            if (event->is<Event::MouseButtonPressed>()) mousePressed = true;

            if (isAdminState(state)) {
                if (!adminScreen) adminScreen = make_unique<AdminScreen>(font, window, state, authService.get());
                adminScreen->handleEvent(*event);
            }
            else if (state == AppState::LOGIN) {
                if (loginScreen) loginScreen->handleEvent(*event, state, currentUser, currentUserEmail, loginSuccess);
            }
            else if (state == AppState::REGISTER) {
                if (registerScreen) registerScreen->handleEvent(*event, state);
            }
        }

        // 2. Logic Updates
        if (isAdminState(state)) {
            if (adminScreen) adminScreen->update(mousePos, mousePressed, state);
        }
        else if (state == AppState::LOGIN) {
            loginSuccess = false;
            bool loginCompleted = loginScreen->update(mousePos, mousePressed, currentUser, currentUserEmail, state);
            if (loginCompleted && state != AppState::LOGIN) {
                BaseScreen::setLoggedInUser(currentUser, currentUserEmail);
                if (!adminScreen) {
                    adminScreen = make_unique<AdminScreen>(font, window, state, authService.get());
                }
            }
        }
        else if (state == AppState::REGISTER) {
            registerScreen->update(mousePos, mousePressed, state);
        }

        // 3. State Management
        handleStateChange();

        if (BaseScreen::isLogoutRequested()) {
            resetAfterLogout();
            continue;
        }

        // 4. Render
        render();
    }
}