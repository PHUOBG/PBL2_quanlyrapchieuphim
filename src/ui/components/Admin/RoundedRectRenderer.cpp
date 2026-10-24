#include "UI/components/Admin/RoundedRectRenderer.h"

#include <algorithm>

namespace {
    void drawCore(sf::RenderTarget& target,
                  const sf::Vector2f& position,
                  const sf::Vector2f& size,
                  float radius,
                  const sf::Color& color,
                  std::size_t cornerPointCount) {
        if (size.x <= 0.f || size.y <= 0.f) {
            return;
        }

        const float clampedRadius = std::max(0.f, std::min(radius, std::min(size.x, size.y) * 0.5f));
        const float innerWidth = std::max(size.x - 2.f * clampedRadius, 0.f);
        const float innerHeight = std::max(size.y - 2.f * clampedRadius, 0.f);

        static sf::RectangleShape rect;
        rect.setFillColor(color);

        // Horizontal core
        rect.setSize({innerWidth, size.y});
        rect.setPosition({position.x + clampedRadius, position.y});
        target.draw(rect);

        // Vertical core
        rect.setSize({size.x, innerHeight});
        rect.setPosition({position.x, position.y + clampedRadius});
        target.draw(rect);

        static sf::CircleShape corner;
        corner.setFillColor(color);
        corner.setPointCount(std::max<std::size_t>(cornerPointCount, 8u));
        corner.setRadius(clampedRadius);

        if (clampedRadius == 0.f) {
            return;
        }

        corner.setPosition(position);
        target.draw(corner);

        corner.setPosition({position.x + size.x - clampedRadius * 2.f, position.y});
        target.draw(corner);

        corner.setPosition({position.x, position.y + size.y - clampedRadius * 2.f});
        target.draw(corner);

        corner.setPosition({position.x + size.x - clampedRadius * 2.f, position.y + size.y - clampedRadius * 2.f});
        target.draw(corner);
    }
}

void RoundedRectRenderer::draw(sf::RenderTarget& target,
                               const sf::Vector2f& position,
                               const sf::Vector2f& size,
                               float radius,
                               const sf::Color& fillColor,
                               float outlineThickness,
                               const sf::Color& outlineColor,
                               std::size_t cornerPointCount) {
    if (outlineThickness > 0.f && outlineColor.a > 0) {
        const sf::Vector2f outlinePos{
            position.x - outlineThickness,
            position.y - outlineThickness
        };
        const sf::Vector2f outlineSize{
            size.x + outlineThickness * 2.f,
            size.y + outlineThickness * 2.f
        };
        drawCore(target, outlinePos, outlineSize, radius + outlineThickness, outlineColor, cornerPointCount);
    }

    drawCore(target, position, size, radius, fillColor, cornerPointCount);
}
