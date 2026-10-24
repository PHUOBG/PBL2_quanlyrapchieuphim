#include "UI/screens/AdminScreen.h"

AdminScreen::AdminScreen(Font& font, RenderWindow& win, AppState& state, AuthService* auth)
    : BaseScreen(font), window(win), appState(state), authService(auth), currentPanelState(AppState::ADMIN_DASHBOARD) {
    
    // Get current user
    User* currentUser = authService ? authService->getCurrentUser() : nullptr;
    string userName = currentUser ? currentUser->getFullName() : "Admin";
    
    // Create sidebar
    sidebar = make_unique<AdminSidebar>(260, window.getSize().y, userName);
    sidebar->setPosition(Vector2f(0, 0));
    
    // Set callbacks
    sidebar->setOnStateChange([this](AppState newState) {
        switchPanel(newState);
    });
    
    sidebar->setOnLogout([this]() {
        handleAdminLogout();
    });
    
    // Main panel background
    mainPanelBg.setSize(Vector2f(window.getSize().x - 260, window.getSize().y));
    mainPanelBg.setPosition(Vector2f(260, 0));
    mainPanelBg.setFillColor(Color(20, 30, 48)); // Slightly lighter than sidebar
    
    // Start with dashboard (or first panel)
    switchPanel(AppState::ADMIN_MOVIES);
}

void AdminScreen::switchPanel(AppState newState) {
    currentPanelState = newState;
    sidebar->setActiveState(newState);
    
    float panelWidth = window.getSize().x - 260 - 40;
    float panelHeight = window.getSize().y - 40;
    moviePanel.reset();
    userPanel.reset();
    comboPanel.reset();
    roomPanel.reset();
    ticketPanel.reset();
    revenuePanel.reset();
    showtimePanel.reset();
    profilePanel.reset();
    voucherPanel.reset();
    
    // Create appropriate panel based on state
    switch (newState) {
        case AppState::ADMIN_MOVIES:
            moviePanel = make_unique<MoviePanel>(font, panelWidth, panelHeight);
            moviePanel->setPosition(Vector2f(280, 20));
            break;

        case AppState::ADMIN_COMBOS:
            comboPanel = make_unique<ComboPanel>(font, panelWidth, panelHeight);
            comboPanel->setPosition(Vector2f(280, 20));
            break;
        case AppState::ADMIN_CUSTOMERS:
            userPanel = make_unique<UserPanel>(font, panelWidth, panelHeight);
            userPanel->setPosition(Vector2f(280, 20));
            break;
        case AppState::ADMIN_ROOMS:
            roomPanel = make_unique<RoomPanel>(font, panelWidth, panelHeight);
            roomPanel->setPosition(Vector2f(280, 20));
            break;
        case AppState::ADMIN_SHOWTIMES:
            showtimePanel = make_unique<ShowtimePanel>(font, panelWidth, panelHeight);
            showtimePanel->setPosition(Vector2f(280, 20));
            break;
        case AppState::ADMIN_TICKETS:
            ticketPanel = make_unique<TicketPanel>(font, panelWidth, panelHeight);
            ticketPanel->setPosition(Vector2f(280, 20));
            break;
        case AppState::ADMIN_REVENUE:
            revenuePanel = make_unique<RevenuePanel>(font, panelWidth, panelHeight, StatsMode::Revenue);
            revenuePanel->setPosition(Vector2f(280, 20));
            break;
        case AppState::ADMIN_SOLD_TICKETS:
            revenuePanel = make_unique<RevenuePanel>(font, panelWidth, panelHeight, StatsMode::Tickets);
            revenuePanel->setPosition(Vector2f(280, 20));
            break;
        case AppState::ADMIN_CHANGE_PASSWORD:
            profilePanel = make_unique<AdminProfilePanel>(font, panelWidth, panelHeight, authService);
            profilePanel->setPosition(Vector2f(280, 20));
            break;
        case AppState::ADMIN_VOUCHERS:
            voucherPanel = make_unique<VoucherPanel>(font, panelWidth, panelHeight);
            voucherPanel->setPosition(Vector2f(280, 20));
            break;
        default:
            break;
    }
}

void AdminScreen::handleAdminLogout() {
    BaseScreen::handleLogout();
}

void AdminScreen::handleEvent(const Event& event) {
    sidebar->handleEvent(event, window);
    
    // Pass events to current panel
    if (moviePanel) {
        moviePanel->handleEvent(event, window);
    }
    if (userPanel) {
        userPanel->handleEvent(event, window);
    }
    if (comboPanel) {
        comboPanel->handleEvent(event, window);
    }
    if (roomPanel) {
        roomPanel->handleEvent(event, window);
    }
    if (ticketPanel) {
        ticketPanel->handleEvent(event, window);
    }
    if (revenuePanel) {
        revenuePanel->handleEvent(event, window);
    }
    if (showtimePanel) {
        showtimePanel->handleEvent(event, window);
    }
    if (profilePanel) {
        profilePanel->handleEvent(event, window);
    }
    if (voucherPanel) {
        voucherPanel->handleEvent(event, window);
    }
}

void AdminScreen::update(Vector2f mousePos, bool mousePressed, AppState& state) {
    sidebar->update(window);
    appState = state; // Sync state changes
    
    // Update current panel
    if (moviePanel) {
        moviePanel->update(mousePos, mousePressed);
    }
    if (userPanel) {
        userPanel->update(mousePos, mousePressed);
    }
    if (comboPanel) {
        comboPanel->update(mousePos, mousePressed);
    }
    if (roomPanel) {
        roomPanel->update(mousePos, mousePressed);
    }
    if (ticketPanel) {
        ticketPanel->update(mousePos, mousePressed);
    }
    if (revenuePanel) {
        revenuePanel->update(mousePos, mousePressed);
    }
    if (showtimePanel) {
        showtimePanel->update(mousePos, mousePressed);
    }
    if (profilePanel) {
        profilePanel->update(mousePos, mousePressed);
    }
    if (voucherPanel) {
        voucherPanel->update(mousePos, mousePressed);
    }
}

void AdminScreen::render() {
    // Draw main panel background
    window.draw(mainPanelBg);
    
    if (moviePanel) moviePanel->render(window);
    if (userPanel) userPanel->render(window);
    if (comboPanel) comboPanel->render(window);
    if (roomPanel) roomPanel->render(window);
    if (ticketPanel) ticketPanel->render(window);
    if (revenuePanel) revenuePanel->render(window);
    if (showtimePanel) showtimePanel->render(window);
    if (profilePanel) profilePanel->render(window);
    if (voucherPanel) voucherPanel->render(window);

    sidebar->render(window);
}
