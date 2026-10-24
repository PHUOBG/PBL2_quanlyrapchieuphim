#pragma once

#include "UI/screens/BaseScreen.h"
#include "UI/components/Admin/AdminSidebar.h"
#include "UI/components/Admin/MoviePanel.h"
#include "UI/components/Admin/UserPanel.h"
#include "UI/components/Admin/ComboPanel.h"
#include "UI/components/Admin/RoomPanel.h"
#include "UI/components/Admin/TicketPanel.h"
#include "UI/components/Admin/RevenuePanel.h"
#include "UI/components/Admin/ShowtimePanel.h"
#include "UI/components/Admin/AdminProfilePanel.h"
#include "UI/components/Admin/VoucherPanel.h"
#include "services/AuthService.h"
#include <memory>

class AdminScreen : public BaseScreen {
private:
    RenderWindow& window;
    AppState& appState;
    AuthService* authService;
    unique_ptr<AdminSidebar> sidebar;
    
    // Current panel
    AppState currentPanelState;
    unique_ptr<MoviePanel> moviePanel;
    unique_ptr<UserPanel> userPanel;
    unique_ptr<ComboPanel> comboPanel;
    unique_ptr<RoomPanel> roomPanel;
    unique_ptr<TicketPanel> ticketPanel;
    unique_ptr<RevenuePanel> revenuePanel;
    unique_ptr<ShowtimePanel> showtimePanel;
    unique_ptr<AdminProfilePanel> profilePanel;
    unique_ptr<VoucherPanel> voucherPanel;
    
    // Background
    RectangleShape mainPanelBg;
    
    void switchPanel(AppState newState);
    void handleAdminLogout();
    
public:
    AdminScreen(Font& font, RenderWindow& win, AppState& state, AuthService* auth);
    
    void handleEvent(const Event& event) override;
    void update(Vector2f mousePos, bool mousePressed, AppState& state) override;
    void render();
};