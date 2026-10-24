#pragma once

#include <SFML/Graphics.hpp>
#include <cstddef>
#include <string>

#include "UI/components/Admin/RoundedRectRenderer.h"

class RoundRectButton {
private:
    sf::Font& font;
    sf::Vector2f size;
    sf::Vector2f position;
    float radius;
    sf::Color baseColor;
    sf::Color fillColor;
    sf::Color outlineColor;
    float outlineThickness;
    sf::Text label;
    bool hovered = false;
    bool pressed = false;

public:
    RoundRectButton(sf::Font& font, const std::string& text, const sf::Vector2f& size, float radius = 12.f);

    void setPosition(const sf::Vector2f& pos);
    void setSize(const sf::Vector2f& size);
    void setRadius(float radius);

    void setFillColor(const sf::Color& color);
    void setOutline(float thickness, const sf::Color& color);
    void setTextColor(const sf::Color& color);
    void setLabel(const std::string& text);

    sf::FloatRect getGlobalBounds() const;

    void update(const sf::Vector2f& mousePos, bool mouseDown, const sf::Color& hoverColor, const sf::Color& activeColor);
    bool isClicked(const sf::Vector2f& mousePos, bool mouseDown);
    void draw(sf::RenderTarget& target) const;
};
