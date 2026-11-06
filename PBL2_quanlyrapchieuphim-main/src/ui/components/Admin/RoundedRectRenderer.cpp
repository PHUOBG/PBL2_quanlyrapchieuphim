#include "UI/components/Admin/RoundedRectRenderer.h"
#include <algorithm>
#include <cmath>

void RoundedRectRenderer::draw(
    sf::RenderTarget& target,
    const sf::Vector2f& position,
    const sf::Vector2f& size,
    float radius,
    const sf::Color& fillColor,
    float outlineThickness,
    const sf::Color& outlineColor,
    std::size_t cornerPointCount) {
    if (size.x <= 0.f || size.y <= 0.f) {
        return;
    }

    const float r = std::max(0.f, std::min(radius, std::min(size.x, size.y) * 0.5f));
    if (r <= 0.f) {
        sf::RectangleShape rect(size);
        rect.setPosition(position);
        rect.setFillColor(fillColor);
        if (outlineThickness > 0.f && outlineColor.a > 0) {
            rect.setOutlineThickness(outlineThickness);
            rect.setOutlineColor(outlineColor);
        }
        target.draw(rect);
        return;
    }

    const std::size_t pointsPerCorner = std::max<std::size_t>(cornerPointCount, 6u);
    const std::size_t totalPoints = pointsPerCorner * 4;

    sf::ConvexShape shape(totalPoints);
    shape.setFillColor(fillColor);
    if (outlineThickness > 0.f && outlineColor.a > 0) {
        shape.setOutlineThickness(outlineThickness);
        shape.setOutlineColor(outlineColor);
    }
    shape.setPosition(position);

    const float pi = 3.14159265358979323846f;
    std::size_t index = 0;

    auto addArc = [&](float cx, float cy, float startAngle, float endAngle) {
        for (std::size_t i = 0; i < pointsPerCorner; ++i) {
            float t = static_cast<float>(i) / static_cast<float>(pointsPerCorner - 1);
            float angle = startAngle + t * (endAngle - startAngle);
            float rad = angle * pi / 180.f;
            shape.setPoint(index++, {cx + r * std::cos(rad), cy + r * std::sin(rad)});
        }
    };

    // 1. Top-Right Corner (270 -> 360 deg)
    addArc(size.x - r, r, 270.f, 360.f);

    // 2. Bottom-Right Corner (0 -> 90 deg)
    addArc(size.x - r, size.y - r, 0.f, 90.f);

    // 3. Bottom-Left Corner (90 -> 180 deg)
    addArc(r, size.y - r, 90.f, 180.f);

    // 4. Top-Left Corner (180 -> 270 deg)
    addArc(r, r, 180.f, 270.f);

    target.draw(shape);
}
