#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <memory>
#include "repositories/booking/TicketRepository.h"
#include "repositories/admin/AdminTicketRepository.h"

using namespace sf;
using namespace std;

class TicketPanel {
private:
    Font& font;
    Font boldFont;
    float width;
    float height;
    Vector2f position;

    AdminTicketRepository repository;
    vector<Ticket> allTickets;
    vector<Ticket> filteredTickets;

    // Filter & Search
    string searchQuery;
    bool searchActive = false;

    // Table selection & scrolling
    int selectedRow = -1;
    int hoveredRow = -1;
    int scrollOffset = 0;
    const int maxVisibleRows = 16;

    // Buttons hover states
    bool btnViewHover = false;
    bool btnDeleteHover = false;
    bool btnReloadHover = false;

    // Detail popup
    bool detailVisible = false;
    Ticket currentDetailTicket;

    // Toast notification
    string notificationMessage;
    Clock notificationClock;
    bool notificationVisible = false;

    Texture reloadTex;
    bool hasReloadTex = false;

    void loadTickets();
    void applySearch();
    void deleteSelectedTicket();

    void drawRoundedCard(RenderWindow& win, Vector2f pos, Vector2f sz, float r, Color fill, float outline = 0.f, Color outCol = Color::Transparent);

public:
    TicketPanel(Font& fontRef, float w, float h);

    void setPosition(Vector2f pos);
    void handleEvent(const Event& event, const RenderWindow& window);
    void update(Vector2f mousePos, bool mousePressed);
    void render(RenderWindow& window);
};
