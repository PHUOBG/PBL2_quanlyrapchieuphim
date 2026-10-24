#pragma once

#include <SFML/Graphics.hpp>
#include <cstddef>

class RoundedRectRenderer {
public:
    static void draw(
        sf::RenderTarget& target,
        const sf::Vector2f& position,
        const sf::Vector2f& size,
        float radius,
        const sf::Color& fillColor,
        float outlineThickness = 0.f,
        const sf::Color& outlineColor = sf::Color::Transparent,
        std::size_t cornerPointCount = 24u);
};
