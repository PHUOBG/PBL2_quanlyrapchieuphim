#pragma once

#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include "data-structures/DLL.h"
#include "repositories/booking/TicketRepository.h"

struct MonthlyRevenuePoint {
    int month;
    int year;
    long long revenue;
};

struct MovieTicketCount {
    std::string title;
    int tickets;
    long long revenue;
};

struct MovieStatItem {
    std::string title;
    std::wstring displayTitle;
    std::wstring shortTitle;
    int tickets;
    long long revenue;
};

struct MonthStatItem {
    std::string month;
    std::wstring displayMonth;
    long long revenue;
    std::wstring changeBadge;
    sf::Color badgeBg;
    sf::Color badgeText;
    bool isRecord = false;
};

class OverviewPanel {
private:
    sf::Font& font;
    sf::Font boldFont;
    float width;
    float height;
    sf::Vector2f position;

    // Duong dan du lieu
    std::string ticketsFilePath;
    std::string usersFilePath;
    std::string showtimesFilePath;
    std::string combosFilePath;

    // Cache gia ve va combo
    std::unordered_map<std::string, int> showtimeSeatPrices;
    std::unordered_map<std::string, int> comboPrices;
    std::unordered_map<std::string, long long> registrationTimes;
    DLL<Ticket> ticketsCache;
    bool dataLoaded = false;

    // Chi so KPI
    long long dailyRevenue = 1890000;
    int newCustomersThisMonth = 148;
    int totalTicketsThisMonth = 101;
    int standardTicketsCount = 68;
    int vipTicketsCount = 33;
    long long totalRevenueThisMonth = 12192500;
    long long movieRevenueThisMonth = 8535000;
    long long comboRevenueThisMonth = 3657500;
    long long totalAccumulatedRevenue = 11842500;

    // Danh sach thong ke
    std::vector<MovieStatItem> topMoviesList;
    std::vector<MonthStatItem> monthlyStatsList;

    // UI state
    int hoveredMovieRow = -1;
    int hoveredMonthRow = -1;
    bool exportHovered = false;
    bool reloadHovered = false;
    bool toastActive = false;
    float toastTimer = 0.f;
    std::wstring toastMessage;

    std::string resolveDataPath(const std::string& relative) const;
    void loadData();
    void loadTickets();
    void loadRegistrations();
    void loadShowtimePrices();
    void loadComboPrices();
    void calculateStats();

    int countSeats(const std::string& seatList) const;
    std::string trim(const std::string& text) const;
    long long computeComboRevenue(const std::string& comboList) const;
    long long computeTicketRevenue(const Ticket& ticket, int seatCount, long long comboRevenue) const;
    std::string toCurrency(long long amount) const;
    sf::String toSfString(const std::string& text) const;

    // Ve giao dien chi tiet theo thiet ke
    void drawHeader(sf::RenderTarget& target) const;
    void drawKpiCards(sf::RenderTarget& target) const;
    void drawTopLeftChart(sf::RenderTarget& target) const;
    void drawTopRightTable(sf::RenderTarget& target) const;
    void drawBottomLeftChart(sf::RenderTarget& target) const;
    void drawBottomRightTable(sf::RenderTarget& target) const;
    void drawBottomBar(sf::RenderTarget& target) const;
    void drawToast(sf::RenderTarget& target) const;

public:
    OverviewPanel(sf::Font& fontRef, float w, float h);

    void setPosition(const sf::Vector2f& pos);
    void handleEvent(const sf::Event& event, const sf::RenderWindow& window);
    void update(const sf::Vector2f& mousePos, bool mouseDown);
    void render(sf::RenderTarget& target) const;
    void refresh();
};
