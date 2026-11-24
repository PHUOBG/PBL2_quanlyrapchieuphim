#pragma once

#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include "repositories/UserRepository.h"
#include "repositories/booking/TicketRepository.h"
#include "exporters/CustomerReportExporter.h"
#include "UI/components/Admin/DeleteConfirmModal.h"

#ifndef CUSTOMER_TIER_DEFINED
#define CUSTOMER_TIER_DEFINED
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
#endif

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
    int totalCount = 0;
    int activeCount = 0;
    int goldCount = 0;
    int silverCount = 0;
    int standardCount = 0;
    long long avgSpending = 0;
    float avgTicketsPerUser = 0.f;

    // Filters and search
    std::string searchText;
    int tierFilterIndex = 0; // 0: Tất cả, 1: Hạng Vàng, 2: Hạng Bạc, 3: Tiêu chuẩn
    int timeFilterIndex = 0; // 0: Cả năm 2025, 1: Tháng 12/2025, 2: Tháng 11/2025, 3: Tháng 10/2025, 4: Tháng 09/2025
    bool searchActive = false;

    // Table state
    int selectedIndex = -1;
    int hoveredRow = -1;
    int page = 0;
    int rowsPerPage = 10;

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
    DeleteConfirmModal deleteModal;
    void performDeleteCustomer(const std::string& email);

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
