#pragma once

#include <SFML/Graphics.hpp>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "data-structures/BTree.h"
#include "data-structures/DLL.h"
#include "repositories/admin/AdminTicketRepository.h"
#include "UI/components/Admin/DropdownBox.h"
#include "UI/components/Admin/RoundRectButton.h"
#include "UI/components/Admin/StatCard.h"
#include "repositories/booking/ShowtimeRepository.h"

struct MovieRevenueEntry {
    string title;
    long long ticketRevenue = 0;
    long long comboRevenue = 0;
    int ticketCount = 0;

    long long totalRevenue() const {
        return ticketRevenue + comboRevenue;
    }
};

enum class ChartMode {
    Tickets,
    Revenue,
    MovieRevenue,
    ComboRevenue
};

struct ModeToggle {
    RectangleShape shape;
    Text label;
    bool active = false;

    explicit ModeToggle(Font& font, unsigned int characterSize = 13)
        : label(font, String(), characterSize) {}

    ModeToggle(const ModeToggle&) = default;
    ModeToggle& operator=(const ModeToggle&) = default;
};

class MovieRevenuePanel {
private:
    Font& font;
    float width;
    float height;
    Vector2f position;

    // Du lieu thong ke
    BTree<long long, Ticket> ticketTree;
    unordered_map<string, int> showtimeSeatPrices;
    unordered_map<string, int> comboPrices;
    unordered_set<string> knownMovieTitles;

    DLL<Ticket> filteredTickets;
    vector<MovieRevenueEntry> movieStats;
    struct ComboRevenueEntry {
        string label;
        int quantity = 0;
        long long revenue = 0;
    };
    vector<ComboRevenueEntry> comboStats;
    vector<long long> availableDateKeys;

    long long selectedStartKey = 0;
    long long selectedEndKey = 0;

    long long totalRevenueValue = 0;
    long long movieRevenueValue = 0;
    long long comboRevenueValue = 0;

    bool suppressClicksUntilMouseRelease = false;

    // Duong dan file
    string ticketsFilePath;
    string moviesFilePath;
    string showtimesFilePath;
    string combosFilePath;

    // Thanh phan giao dien
    RectangleShape background;
    Text titleText;
    Text fromLabel;
    Text toLabel;

    StatCard totalRevenueCard;
    StatCard movieRevenueCard;
    StatCard comboRevenueCard;

    unique_ptr<DropdownBox> fromYearDropdown;
    unique_ptr<DropdownBox> fromMonthDropdown;
    unique_ptr<DropdownBox> fromDayDropdown;

    unique_ptr<DropdownBox> toYearDropdown;
    unique_ptr<DropdownBox> toMonthDropdown;
    unique_ptr<DropdownBox> toDayDropdown;

    RoundRectButton loadButton;

    ModeToggle ticketsToggle;
    ModeToggle revenueToggle;
    ModeToggle movieOnlyToggle;
    ModeToggle comboToggle;
    ChartMode currentChartMode = ChartMode::Tickets;

    RectangleShape chartCard;

    // Ham ho tro xu ly
    string resolveDataPath(const string& relative) const;
    void initializeUI();
    void layoutComponents();

    void loadData();
    void loadMovies();
    void loadTickets();
    void loadShowtimePrices();
    void loadComboPrices();

    void populateDropdowns();
    void populateYears(DropdownBox& dropdown) const;
    void populateMonths(DropdownBox& dropdown, const DropdownBox& yearDropdown) const;
    void populateDays(DropdownBox& dropdown, const DropdownBox& yearDropdown, const DropdownBox& monthDropdown) const;
    void updateDropdownEnabling();

    bool isLeapYear(int year) const;
    int daysInMonth(int month, int year) const;

    long long dateToKey(const string& displayDate) const; // dinh dang dd/mm/yyyy
    string keyToDisplay(long long key) const;

    void applySelection();
    void updateMovieStats();
    void updateComboStats();
    void updateSummaryCards();
    string currentRangeLabel() const;
    bool isAnyDropdownOpen() const;

    int countSeats(const string& seatList) const;
    string trim(const string& text) const;
    string ellipsize(const string& text, size_t maxChars) const;
    long long computeComboRevenue(const string& comboList) const;
    long long computeTicketRevenue(const Ticket& ticket, int seatCount, long long comboRevenue) const;

    void setChartMode(ChartMode mode);
    void sortStatsForCurrentMode();
    void sortComboStats();
    void drawChart(RenderTarget& target) const;

public:
    MovieRevenuePanel(Font& font, float width, float height);

    void setPosition(const Vector2f& pos);
    void handleEvent(const Event& event, const RenderWindow& window);
    void update(const Vector2f& mousePos, bool mouseDown);
    void render(RenderTarget& target) const;
    void refresh();
};
