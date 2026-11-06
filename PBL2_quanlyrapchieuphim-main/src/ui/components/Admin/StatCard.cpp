#include "UI/components/Admin/StatCard.h"

StatCard::StatCard(sf::Font& fontRef, const sf::Vector2f& size, const sf::Color& outlineColor)
    : font(fontRef),
      box(size),
      titleText(font, sf::String(), 12),
      valueText(font, sf::String(), 24) {
    box.setFillColor(sf::Color::White);
    box.setOutlineThickness(2.f);
    box.setOutlineColor(outlineColor);

    titleText.setFillColor(sf::Color(71, 85, 105));
    valueText.setFillColor(sf::Color(15, 23, 42));
    valueText.setStyle(sf::Text::Bold);
}

void StatCard::setPosition(const sf::Vector2f& position) {
    box.setPosition(position);
    titleText.setPosition(sf::Vector2f(position.x + 12.f, position.y + 10.f));
    positionValue();
}

void StatCard::setSize(const sf::Vector2f& size) {
    box.setSize(size);
    positionValue();
}

void StatCard::setOutlineThickness(float thickness) {
    box.setOutlineThickness(thickness);
}

void StatCard::setOutlineColor(const Color& color) {
    box.setOutlineColor(color);
}

void StatCard::setTitleWithDate(const std::string& title, const std::string& dateStr) {
    std::string combined = title + " (" + dateStr + ")";
    titleText.setString(sf::String::fromUtf8(combined.begin(), combined.end()));
}

void StatCard::setValue(const std::string& value) {
    valueText.setString(sf::String::fromUtf8(value.begin(), value.end()));
    positionValue();
}

void StatCard::positionValue() {
    sf::FloatRect bounds = valueText.getLocalBounds();
    sf::Vector2f pos = box.getPosition();
    sf::Vector2f size = box.getSize();

    // Căn giữa theo chiều ngang, lùi xuống một chút theo chiều dọc
    valueText.setPosition(sf::Vector2f(
        pos.x + (size.x - bounds.size.x) / 2.f - bounds.position.x,
        pos.y + size.y * 0.45f - bounds.position.y
    ));
}

void StatCard::render(sf::RenderTarget& target) const {
    target.draw(box);
    target.draw(titleText);
    target.draw(valueText);
}

sf::Vector2f StatCard::getPosition() const {
    return box.getPosition();
}

sf::Vector2f StatCard::getSize() const {
    return box.getSize();
}
