#pragma once

#include <SFML/Graphics.hpp>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include "data-structures/DLL.h"
#include "repositories/booking/TicketRepository.h"

enum class RevenueTab {
    TICKETS_BY_MOVIE = 0,
    TOTAL_REVENUE = 1,
    REVENUE_BY_MOVIE = 2,
    REVENUE_BY_COMBO = 3
};

struct MovieRevItem {
    std::string id;
    std::wstring title;
    std::wstring shortTitle;
    std::wstring genre;
    int showtimes = 0;
    int tickets = 0;
    long long revenue = 0;
    float share = 0.f;
    bool isHot = false;
};

struct MonthRevItem {
    std::string month;
    std::wstring displayMonth;
    long long ticketRev = 0;
    long long comboRev = 0;
    long long totalRev = 0;
    std::wstring growth;
    float comboShare = 0.f;
    bool isRecord = false;
};

struct ComboRevItem {
    std::string code;
    std::wstring name;
    int unitPrice = 0;
    int quantity = 0;
    long long revenue = 0;
    float share = 0.f;
    bool isTop1 = false;
};

class MovieRevenuePanel {
private:
    sf::Font& font;
    sf::Font boldFont;
    float width;
    float height;
    sf::Vector2f position;

    RevenueTab activeTab = RevenueTab::TICKETS_BY_MOVIE;

    // Duong dan du lieu
    std::string ticketsFilePath;
    std::string showtimesFilePath;
    std::string combosFilePath;

    // Cache gia va ve
    std::unordered_map<std::string, int> showtimePrices;
    std::unordered_map<std::string, int> comboPrices;
    DLL<Ticket> ticketsCache;

    // Tong quan KPI
    long long totalRevenue = 11842500;
    long long movieRevenue = 7000000;
    long long comboRevenue = 4842500;
    int totalTickets = 101;
    int totalCombos = 89;

    // Danh sach du lieu
    std::vector<MovieRevItem> moviesList;
    std::vector<MonthRevItem> monthsList;
    std::vector<ComboRevItem> combosList;

    // UI state
    int hoveredTab = -1;
    int hoveredRow = -1;
    bool exportHovered = false;
    bool reloadHovered = false;
    bool toastActive = false;
    float toastTimer = 0.f;
    std::wstring toastMessage;

    std::string resolveDataPath(const std::string& relative) const;
    void loadData();
    void loadTickets();
    void loadShowtimePrices();
    void loadComboPrices();
    void initDefaultStats();

    int countSeats(const std::string& seatList) const;
    std::string trim(const std::string& text) const;
    long long computeComboRevenue(const std::string& comboList) const;
    std::string toCurrency(long long amount) const;
    sf::String toSfString(const std::string& text) const;

    // Render cac phan giao dien
    void drawHeader(sf::RenderTarget& target) const;
    void drawKpiCards(sf::RenderTarget& target) const;
    void drawTabBar(sf::RenderTarget& target) const;
    
    // 4 View chi tiet tuong ung voi 4 tab
    void drawTab1TicketsByMovie(sf::RenderTarget& target) const;
    void drawTab2TotalRevenue(sf::RenderTarget& target) const;
    void drawTab3RevenueByMovie(sf::RenderTarget& target) const;
    void drawTab4RevenueByCombo(sf::RenderTarget& target) const;

    void drawBottomBar(sf::RenderTarget& target) const;
    void drawToast(sf::RenderTarget& target) const;

public:
    MovieRevenuePanel(sf::Font& fontRef, float w, float h);

    void setPosition(const sf::Vector2f& pos);
    void handleEvent(const sf::Event& event, const sf::RenderWindow& window);
    void update(const sf::Vector2f& mousePos, bool mouseDown);
    void render(sf::RenderTarget& target) const;
    void refresh();
};
