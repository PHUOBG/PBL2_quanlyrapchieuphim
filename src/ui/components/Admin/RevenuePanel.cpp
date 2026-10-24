#include "UI/components/Admin/RevenuePanel.h"

using sf::Color;
using sf::Event;
using sf::RenderWindow;
using sf::Vector2f;

namespace {
    constexpr Color kBackgroundColor(241, 245, 249);
}

RevenuePanel::RevenuePanel(sf::Font& fontRef, float w, float h, StatsMode initialMode)
    : font(fontRef),
      width(w),
      height(h),
      position(0.f, 0.f),
      background(Vector2f(w, h)),
            overviewPanel(fontRef, w, h),
            movieRevenuePanel(fontRef, w, h),
      mode(initialMode) {
    background.setFillColor(kBackgroundColor);
    layoutComponents();
    refreshData();
}

void RevenuePanel::layoutComponents() {
    background.setSize(Vector2f(width, height));
    background.setPosition(position);

    const Vector2f contentPos(position.x, position.y);
    overviewPanel.setPosition(contentPos);
    movieRevenuePanel.setPosition(contentPos);
}

void RevenuePanel::setPosition(const Vector2f& pos) {
    position = pos;
    layoutComponents();
}

void RevenuePanel::handleEvent(const Event& event, const RenderWindow& window) {
    if (mode == StatsMode::Tickets) {
        movieRevenuePanel.handleEvent(event, window);
    }
}

void RevenuePanel::update(const Vector2f& mousePos, bool mouseDown) {
    if (mode == StatsMode::Tickets) {
        movieRevenuePanel.update(mousePos, mouseDown);
    }
}

void RevenuePanel::render(RenderWindow& window) {
    window.draw(background);

    if (mode == StatsMode::Revenue) {
        overviewPanel.render(window);
    } else {
        movieRevenuePanel.render(window);
    }
}

void RevenuePanel::refreshData() {
    overviewPanel.refresh();
    movieRevenuePanel.refresh();
}
