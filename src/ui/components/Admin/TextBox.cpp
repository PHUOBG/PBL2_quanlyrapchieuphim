#include "UI/components/Admin/TextBox.h"
#include <algorithm>

TextBox::TextBox(Font& font, const string& label, float x, float y, float width, float height)
    : font(font), x(x), y(y), width(width), height(height), isFocused(false), isActive(true), editable(true) {
    
    // Background
    background.setSize({width, height});
    background.setPosition({x, y});
    background.setFillColor(bgColor);
    
    // Border
    border.setSize({width, height});
    border.setPosition({x, y});
    border.setFillColor(Color::Transparent);
    border.setOutlineThickness(1.f);
    border.setOutlineColor(borderColor);
    
    // Label text
    labelText = make_unique<Text>(font);
    labelText->setString(String::fromUtf8(label.begin(), label.end()));
    labelText->setCharacterSize(14);
    labelText->setFillColor(Color(80, 80, 80));
    labelText->setPosition({x, y - 22});
    
    // Display text
    displayText = make_unique<Text>(font);
    displayText->setCharacterSize(15);
    displayText->setFillColor(textColor);
    displayText->setPosition({x + horizontalPadding, y});

    // Cursor setup (drawn manually to avoid placeholder flicker)
    cursor.setSize({1.6f, height - 16.f});
    cursor.setFillColor(focusColor);
    cursor.setPosition({x + horizontalPadding, y + (height - cursor.getSize().y) / 2});
}

TextBox::TextBox(Font& font, float width, float height)
    : font(font), x(0), y(0), width(width), height(height), isFocused(false), isActive(true), editable(true) {
    
    // Background
    background.setSize({width, height});
    background.setPosition({0, 0});
    background.setFillColor(bgColor);
    
    // Border
    border.setSize({width, height});
    border.setPosition({0, 0});
    border.setFillColor(Color::Transparent);
    border.setOutlineThickness(1.f);
    border.setOutlineColor(borderColor);
    
    // No label for this constructor
    labelText = nullptr;
    
    // Display text
    displayText = make_unique<Text>(font);
    displayText->setCharacterSize(15);
    displayText->setFillColor(textColor);
    displayText->setPosition({horizontalPadding, 0});

    // Cursor setup
    cursor.setSize({1.6f, height - 16.f});
    cursor.setFillColor(focusColor);
    cursor.setPosition({horizontalPadding, (height - cursor.getSize().y) / 2});
}

void TextBox::setPlaceholder(const string& text) {
    placeholder = text;
}

void TextBox::setValue(const string& text) {
    value = text;
}

void TextBox::setText(const string& text) {
    setValue(text);
}

string TextBox::getValue() const {
    return value;
}

string TextBox::getText() const {
    return value;
}

void TextBox::setPosition(Vector2f pos) {
    x = pos.x;
    y = pos.y;
    background.setPosition({x, y});
    border.setPosition({x, y});
    if (labelText) labelText->setPosition({x, y - 22});
    // displayText position is updated in render()
}

void TextBox::setFocus(bool focus) {
    if (!editable) {
        isFocused = false;
        return;
    }
    
    isFocused = focus;
    if (isFocused) {
        border.setOutlineColor(focusColor);
        border.setOutlineThickness(2.f);
        cursorClock.restart();
    } else {
        border.setOutlineColor(borderColor);
        border.setOutlineThickness(1.f);
        cursorClock.restart();
    }
}

void TextBox::setEditable(bool edit) {
    editable = edit;
    if (!editable) {
        background.setFillColor(disabledBgColor);
        setFocus(false);
    } else {
        background.setFillColor(bgColor);
    }
}

bool TextBox::isEditable() const {
    return editable;
}

void TextBox::handleEvent(const Event& event, const RenderWindow& window) {
    (void)window;
    handleEvent(event);
}

void TextBox::handleEvent(const Event& event) {
    if (!isFocused || !editable) return;
    
    if (const auto* textEvent = event.getIf<Event::TextEntered>()) {
        if (textEvent->unicode == 8) { // Backspace
            if (!value.empty()) {
                // Handle UTF-8 properly
                size_t len = value.length();
                while (len > 0 && (value[len-1] & 0xC0) == 0x80) {
                    len--;
                }
                if (len > 0) len--;
                value = value.substr(0, len);
            }
            cursorClock.restart();
        } else if (textEvent->unicode == 13) { // Enter
            // Do nothing, handled by parent
        } else if (textEvent->unicode == 27) { // Escape
            setFocus(false);
        } else if (textEvent->unicode >= 32) {
            // Add character (including Vietnamese)
            char32_t unicode = textEvent->unicode;
            if (unicode < 0x80) {
                value += static_cast<char>(unicode);
            } else if (unicode < 0x800) {
                value += static_cast<char>(0xC0 | (unicode >> 6));
                value += static_cast<char>(0x80 | (unicode & 0x3F));
            } else if (unicode < 0x10000) {
                value += static_cast<char>(0xE0 | (unicode >> 12));
                value += static_cast<char>(0x80 | ((unicode >> 6) & 0x3F));
                value += static_cast<char>(0x80 | (unicode & 0x3F));
            } else {
                value += static_cast<char>(0xF0 | (unicode >> 18));
                value += static_cast<char>(0x80 | ((unicode >> 12) & 0x3F));
                value += static_cast<char>(0x80 | ((unicode >> 6) & 0x3F));
                value += static_cast<char>(0x80 | (unicode & 0x3F));
            }
            cursorClock.restart();
        }
    }
}

void TextBox::update(Vector2f mousePos, bool mousePressed) {
    if (!editable) return;
    
    if (mousePressed) {
        FloatRect bounds = background.getGlobalBounds();
        if (bounds.contains(mousePos)) {
            setFocus(true);
        } else {
            setFocus(false);
        }
    }
}

void TextBox::update(Vector2f mousePos) {
    // Just for hover effects, no action needed for now
    (void)mousePos;
}

void TextBox::render(RenderWindow& window) {
    window.draw(background);
    window.draw(border);
    if (labelText) window.draw(*labelText);
    
    // Display text or placeholder
    bool showPlaceholder = !isFocused && value.empty();
    string displayStr = showPlaceholder ? placeholder : value;
    Color displayColor = showPlaceholder ? placeholderColor : textColor;

    if (displayText) {
        displayText->setFillColor(displayColor);
        displayText->setString(String::fromUtf8(displayStr.begin(), displayStr.end()));

        // Re-center vertically inside the textbox each frame
        auto bounds = displayText->getLocalBounds();
        float baselineOffset = background.getPosition().y + (height - bounds.size.y) / 2.f - bounds.position.y;

        // Horizontal scrolling: keep the tail visible when text is too long
        float textX = background.getPosition().x + horizontalPadding;
        float available = width - 2.f * horizontalPadding;
        if (bounds.size.x > available) {
            textX += (available - bounds.size.x);
        }
        displayText->setPosition({textX, baselineOffset});

        // Clip text (and cursor) to the textbox rectangle
        View oldView = window.getView();
        const auto ws = window.getSize();
        FloatRect box = background.getGlobalBounds();
        View clipView;
        clipView.setCenter({box.position.x + box.size.x / 2.f, box.position.y + box.size.y / 2.f});
        clipView.setSize({box.size.x, box.size.y});
        clipView.setViewport(FloatRect(
            {box.position.x / static_cast<float>(ws.x), box.position.y / static_cast<float>(ws.y)},
            {box.size.x / static_cast<float>(ws.x), box.size.y / static_cast<float>(ws.y)}
        ));
        window.setView(clipView);
        window.draw(*displayText);

        // Blink cursor without altering text or placeholder
        if (isFocused) {
            float elapsed = cursorClock.getElapsedTime().asSeconds();
            if (elapsed >= 1.f) {
                cursorClock.restart();
                elapsed = cursorClock.getElapsedTime().asSeconds();
            }

            if (elapsed < 0.5f) {
                auto b2 = displayText->getLocalBounds();
                float cursorX = displayText->getPosition().x + b2.position.x + b2.size.x;
                float cursorY = background.getPosition().y + (height - cursor.getSize().y) / 2.f;
                cursor.setSize({cursor.getSize().x, height - 16.f});
                cursor.setPosition({cursorX + 2.f, cursorY});
                cursor.setFillColor(focusColor);
                window.draw(cursor);
            }
        }

        window.setView(oldView);
    }
}

void TextBox::draw(RenderWindow& window) {
    render(window);
}

bool TextBox::getFocus() const {
    return isFocused;
}
