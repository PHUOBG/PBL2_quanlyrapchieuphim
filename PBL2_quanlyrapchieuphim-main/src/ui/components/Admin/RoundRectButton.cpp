#include "UI/components/Admin/RoundRectButton.h"

RoundRectButton::RoundRectButton(sf::Font& fontRef, const std::string& text, const sf::Vector2f& initialSize, float initialRadius)
        : font(fontRef),
            size(initialSize),
            position(0.f, 0.f),
            radius(initialRadius),
            baseColor(59, 130, 246),
            fillColor(59, 130, 246),
            outlineColor(sf::Color::Transparent),
            outlineThickness(0.f),
            label(fontRef, sf::String::fromUtf8(text.begin(), text.end()), 16) {
    label.setFillColor(sf::Color::White);
    label.setStyle(sf::Text::Bold);
    setPosition(position);
}

void RoundRectButton::setPosition(const sf::Vector2f& pos) {
    position = pos;
    const sf::FloatRect bounds = label.getLocalBounds();
    label.setPosition(sf::Vector2f(
        position.x + (size.x - bounds.size.x) / 2.f - bounds.position.x,
        position.y + (size.y - bounds.size.y) / 2.f - bounds.position.y
    ));
}

void RoundRectButton::setSize(const sf::Vector2f& newSize) {
    size = newSize;
    setPosition(position);
}

void RoundRectButton::setRadius(float newRadius) {
    radius = newRadius;
}

void RoundRectButton::setFillColor(const sf::Color& color) {
    baseColor = color;
    fillColor = color;
}

void RoundRectButton::setOutline(float thickness, const sf::Color& color) {
    outlineThickness = thickness;
    outlineColor = color;
}

void RoundRectButton::setTextColor(const sf::Color& color) {
    label.setFillColor(color);
}

void RoundRectButton::setLabel(const std::string& text) {
    label.setString(sf::String::fromUtf8(text.begin(), text.end()));
    setPosition(position);
}

sf::FloatRect RoundRectButton::getGlobalBounds() const {
    return sf::FloatRect(position, size);
}

void RoundRectButton::update(const sf::Vector2f& mousePos, bool mouseDown, const sf::Color& hoverColor, const sf::Color& activeColor) {
    const bool contains = getGlobalBounds().contains(mousePos);
    if (contains && mouseDown) {
        pressed = true;
        fillColor = activeColor;
    } else if (contains) {
        fillColor = hoverColor;
    } else {
        if (!mouseDown) {
            pressed = false;
        }
        fillColor = baseColor;
    }
    hovered = contains;
}

bool RoundRectButton::isClicked(const sf::Vector2f& mousePos, bool mouseDown) {
    const bool contains = getGlobalBounds().contains(mousePos);
    if (!mouseDown && pressed && contains) {
        pressed = false;
        fillColor = baseColor;
        return true;
    }
    if (!mouseDown) {
        pressed = false;
        if (!hovered) {
            fillColor = baseColor;
        }
    }
    return false;
}

void RoundRectButton::draw(sf::RenderTarget& target) const {
    RoundedRectRenderer::draw(target, position, size, radius, fillColor, outlineThickness, outlineColor);
    target.draw(label);
}
