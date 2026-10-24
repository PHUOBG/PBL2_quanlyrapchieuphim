#pragma once

#include <SFML/Graphics.hpp>

#include "UI/components/Admin/MovieRevenuePanel.h"
#include "UI/components/Admin/OverviewPanel.h"

enum class StatsMode {
    Revenue,
    Tickets
};

class RevenuePanel {
private:
    sf::Font& font;
    float width;
    float height;
    sf::Vector2f position;

    sf::RectangleShape background;
    OverviewPanel overviewPanel;
    MovieRevenuePanel movieRevenuePanel;
    StatsMode mode;

    static constexpr float contentPadding = 24.f;

    void layoutComponents();

public:
    RevenuePanel(sf::Font& font, float width, float height, StatsMode initialMode = StatsMode::Revenue);

    void setPosition(const sf::Vector2f& pos);
    void handleEvent(const sf::Event& event, const sf::RenderWindow& window);
    void update(const sf::Vector2f& mousePos, bool mouseDown);
    void render(sf::RenderWindow& window);
    void refreshData();
};
