#pragma once

#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include "repositories/UserRepository.h"
#include "repositories/booking/TicketRepository.h"

enum class CustomerTier {
    STANDARD,
    SILVER,
    GOLD
};

struct CustomerItem {
    std::string email;
    std::string fullName;
    std::wstring displayFullName;
    std::string birthDate;
    std::string phone;
    std::string registeredDate;
    time_t registeredAt = 0;
    std::string uid;
    std::wstring initials;
    sf::Color avatarBg;
    CustomerTier tier = CustomerTier::STANDARD;
    int ticketsBought = 0;
    long long totalSpent = 0;
    bool selected = false;
};

class UserPanel {
private:
    sf::Font& font;
    sf::Font boldFont;
    float width;
    float height;
    sf::Vector2f position;

    std::unique_ptr<UserRepository> repository;
    std::vector<CustomerItem> allCustomers;
    std::vector<CustomerItem*> filteredCustomers;

    // KPI Counters
    int totalCount = 15;
    int activeCount = 12;
    int goldCount = 6;
    int silverCount = 7;
    int standardCount = 2;
    long long avgSpending = 789500;
    float avgTicketsPerUser = 6.4f;

    // Filters and search
    std::string searchText;
    int tierFilterIndex = 0; // 0: Tất cả, 1: Hạng Vàng, 2: Hạng Bạc, 3: Tiêu chuẩn
    bool searchActive = false;

    // Table state
    int selectedIndex = -1;
    int hoveredRow = -1;
    int page = 0;
    int rowsPerPage = 7;

    // Buttons & Dialogs
    bool btnViewHovered = false;
    bool btnEditHovered = false;
    bool btnDeleteHovered = false;
    bool btnResetPwdHovered = false;
    bool btnAddHovered = false;
    bool btnExportHovered = false;
    bool btnReloadHovered = false;

    // Dialog state
    enum class DialogType { NONE, VIEW, EDIT, DELETE_CONFIRM, ADD_NEW };
    DialogType activeDialog = DialogType::NONE;
    int dialogUserIndex = -1;

    // Dialog inputs
    std::string inputEmail;
    std::string inputName;
    std::string inputPhone;
    std::string inputBirth;
    int activeInputField = 0;

    // Notification toast
    bool toastActive = false;
    float toastTimer = 0.f;
    std::wstring toastMessage;

    void loadData();
    void applyFilter();
    void calculateCustomerStats();

    // Render methods
    void drawHeader(sf::RenderTarget& target) const;
    void drawKpiCards(sf::RenderTarget& target) const;
    void drawActionToolbar(sf::RenderTarget& target) const;
    void drawCustomerTable(sf::RenderTarget& target) const;
    void drawDialog(sf::RenderTarget& target) const;
    void drawToast(sf::RenderTarget& target) const;
    void drawBottomBar(sf::RenderTarget& target) const;

public:
    UserPanel(sf::Font& fontRef, float w, float h);

    void setPosition(const sf::Vector2f& pos);
    void handleEvent(const sf::Event& event, const sf::RenderWindow& window);
    void update(const sf::Vector2f& mousePos, bool mouseDown);
    void render(sf::RenderTarget& target) const;
    void refreshData();
};
