#include "UI/components/TextButton.h"

TextButton::TextButton(const Font& font, const String& str, unsigned int size, Vector2f pos) : text(font, str, size) {
    text.setPosition(pos);
    text.setFillColor(Color(170, 182, 197));
}

void TextButton::update(const Vector2f& mousePos) {
    if (text.getGlobalBounds().contains(mousePos)) {
        hovered = true;
        text.setFillColor(Color::White);
    }
    else {
        hovered = false;
        text.setFillColor(Color(214, 215, 230));
    }
}

bool TextButton::isClicked(const Vector2f& mousePos, bool mousePressed) const {
    (void)mousePos;
    return hovered && mousePressed;
}

void TextButton::draw(RenderWindow& window) const {
    window.draw(text);
}

void TextButton::setOutlineColor(const sf::Color& color) {
    text.setOutlineColor(color);
}

void TextButton::setOutlineThickness(float thickness) {
    text.setOutlineThickness(thickness);
}

void TextButton::setString(const sf::String& str) {
    text.setString(str);
}

void TextButton::setPosition(Vector2f pos) {
    text.setPosition(pos);
}

FloatRect TextButton::getGlobalBounds() const{
    return text.getGlobalBounds();
}

Vector2f TextButton::getPosition() const {
    return text.getPosition();
}