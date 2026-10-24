#include "UI/components/Button.h"

Button::Button(const Font& font, const String& label, float width, float height, int text_size) 
    :   font(font), box({width, height})
{
    box.setOutlineThickness(2.f);
    box.setOutlineColor(Color(136, 136, 136));

    text = make_unique<Text>(font);
    text->setString(label);
    text->setFillColor(Color::Black);
    text->setCharacterSize(text_size);
}

Button::Button(const Font& font, const String& label, Vector2f size, int textSize)
    : font(font), box(size)
{
    box.setOutlineThickness(0.f);
    box.setOutlineColor(Color::Transparent);

    text = make_unique<Text>(font);
    text->setString(label);
    text->setFillColor(Color::White);
    text->setCharacterSize(textSize);
}

// Copy constructor - deep copy
Button::Button(const Button& other) 
    : font(other.font), box(other.box), hovered(other.hovered), pressed(other.pressed)
{
    if (other.text) {
        text = make_unique<Text>(font);
        text->setString(other.text->getString());
        text->setFillColor(other.text->getFillColor());
        text->setCharacterSize(other.text->getCharacterSize());
        text->setPosition(other.text->getPosition());
    }
}

// Copy assignment
Button& Button::operator=(const Button& other) {
    if (this != &other) {
        box = other.box;
        hovered = other.hovered;
        pressed = other.pressed;
        
        if (other.text) {
            text = make_unique<Text>(font);
            text->setString(other.text->getString());
            text->setFillColor(other.text->getFillColor());
            text->setCharacterSize(other.text->getCharacterSize());
            text->setPosition(other.text->getPosition());
        } else {
            text.reset();
        }
    }
    return *this;
}

void Button::setPosition(Vector2f pos) {
    box.setPosition(pos);
    if (text) {
        FloatRect bounds = text->getLocalBounds();
        text->setPosition({
            pos.x + (box.getSize().x - bounds.size.x) / 2.f - bounds.position.x,
            pos.y + (box.getSize().y - bounds.size.y) / 2.f - bounds.position.y,
        });
    }
}

void Button::setText(const String& label) {
    if (text) {
        text->setString(label);
        setPosition(box.getPosition());
    }
}

void Button::setFillColor(Color color) {
    box.setFillColor(color);
}

void Button::setTextColor(Color color) {
    if (text) text->setFillColor(color);
}

void Button::setOutlineThickness(float thickness) {
    box.setOutlineThickness(thickness);
}

void Button::setOutlineColor(Color color) {
    box.setOutlineColor(color);
}

void Button::setColors(Color normal, Color hover, Color textCol) {
    normalColor = normal;
    hoverColor = hover;
    box.setFillColor(normal);
    if (text) text->setFillColor(textCol);
}

void Button::setAlpha(std::uint8_t alpha) {
    {
        Color c = box.getFillColor();
        c.a = alpha;
        box.setFillColor(c);
    }
    {
        Color c = box.getOutlineColor();
        c.a = alpha;
        box.setOutlineColor(c);
    }
    if (text) {
        Color c = text->getFillColor();
        c.a = alpha;
        text->setFillColor(c);
    }
}

Vector2f Button::getPosition() const {
    return box.getPosition();
}

Vector2f Button::getSize() const {
    return box.getSize();
}

FloatRect Button::getGlobalBounds() const {
    return box.getGlobalBounds();
}

String Button::getString() const {
    return text ? text->getString() : "";
}

bool Button::isHovered() const {
    return hovered;
}

bool Button::isPressed() const {
    return pressed;
}

void Button::update(Vector2f mousePos, bool mousePressed, Color hover, Color normal) {
    (void)mousePressed;
    hovered = box.getGlobalBounds().contains(mousePos);

    if (hovered) {
        box.setFillColor(hover);
        // Keep existing text color, don't override it
    }
    else {
        box.setFillColor(normal);
        // Keep existing text color, don't override it
    }
}

void Button::update(Vector2f mousePos) {
    hovered = box.getGlobalBounds().contains(mousePos);
    box.setFillColor(hovered ? hoverColor : normalColor);
}

bool Button::isClicked(Vector2f mousePos, bool mousePressed) {
    return box.getGlobalBounds().contains(mousePos) && mousePressed;
}

bool Button::isClicked(Vector2f mousePos) {
    return box.getGlobalBounds().contains(mousePos);
}

void Button::draw(RenderWindow& window) {
    window.draw(box);
    if (text) window.draw(*text);
}
