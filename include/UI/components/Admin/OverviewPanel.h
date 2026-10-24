#pragma once

#include <SFML/Graphics.hpp>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "data-structures/BTree.h"
#include "data-structures/DLL.h"
#include "repositories/admin/AdminTicketRepository.h"
#include "UI/components/Admin/StatCard.h"

struct MonthlyRevenuePoint {
    int month;
    int year;
    long long revenue;
};

struct MovieTicketCount {
    std::string title;
    int tickets;
    long long revenue;  // Thêm doanh thu
};

class OverviewPanel {
private:
    sf::Font& font;
    float width;
    float height;
    sf::Vector2f position;

    // Duong dan du lieu
    std::string ticketsFilePath;
    std::string usersFilePath;
    std::string showtimesFilePath;
    std::string combosFilePath;

    // Hang muc ho tro tinh doanh thu
    std::unordered_map<std::string, int> showtimeSeatPrices;
    std::unordered_map<std::string, int> comboPrices;

    // Bo dem du lieu
    std::unordered_map<std::string, long long> registrationTimes;
    DLL<Ticket> ticketsCache;
    bool dataLoaded = false;  // Đánh dấu đã load dữ liệu chưa

    long long dailyRevenue = 0;
    int newCustomersThisMonth = 0;
    int totalTicketsThisMonth = 0;
    long long totalRevenueThisMonth = 0;

    DLL<MovieTicketCount> topMovies;
    DLL<MonthlyRevenuePoint> monthlyTrend;

    // Thanh phan giao dien
    sf::RectangleShape background;
    StatCard revenueTodayCard;
    StatCard newCustomersCard;
    StatCard ticketsCard;
    StatCard totalRevenueCard;
    StatCard movieRevenueCard;
    StatCard comboRevenueCard;

    sf::RectangleShape ticketChartCard;
    sf::RectangleShape revenueChartCard;
    sf::RectangleShape movieTableCard;    // Bảng phim bên phải biểu đồ cột
    sf::RectangleShape revenueTableCard;  // Bảng doanh thu bên phải biểu đồ đường

    sf::Color cardOutlineCyan = sf::Color(13, 148, 136);
    sf::Color cardOutlineGreen = sf::Color(34, 197, 94);
    sf::Color cardOutlineYellow = sf::Color(221, 180, 67);
    sf::Color cardOutlineRed = sf::Color(239, 68, 68);

    // Cac ham xu ly bo tro
    std::string resolveDataPath(const std::string& relative) const;
    void initializeUI();
    void loadData();
    void loadTickets();
    void loadRegistrations();
    void loadShowtimePrices();
    void loadComboPrices();

    void calculateDailyRevenue();
    void calculateMonthlyStats();
    void calculateTopMovies();
    void calculateMonthlyTrend();

    int countSeats(const std::string& seatList) const;
    std::string trim(const std::string& text) const;
    long long computeComboRevenue(const std::string& comboList) const;
    long long computeTicketRevenue(const Ticket& ticket, int seatCount, long long comboRevenue) const;
    std::string toCurrency(long long amount) const;
    long long roundUpNice(long long value) const;
    sf::String toSfString(const std::string& text) const;
    std::string ellipsize(const std::string& text, std::size_t maxChars) const;
    void sortTopMovies();
    void sortMonthlyTrend();

    void drawTicketChart(sf::RenderTarget& target) const;
    void drawRevenueChart(sf::RenderTarget& target) const;
    void drawMovieTable(sf::RenderTarget& target) const;
    void drawRevenueTable(sf::RenderTarget& target) const;

public:
    OverviewPanel(sf::Font& font, float width, float height);

    void setPosition(const sf::Vector2f& pos);
    void render(sf::RenderTarget& target) const;
    void refresh();
};
