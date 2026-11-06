#include "UI/components/PasswordField.h"
#include <algorithm>

PasswordField::PasswordField(const sf::Font& font, float width, float height)
    : font_(font),
      displayText_(font, "", 16),
      placeholderText_(font, L"", 16)
{
    // Setup box
    box_.setSize({width, height});
    box_.setFillColor(sf::Color::White);
    box_.setOutlineThickness(1.f);
    box_.setOutlineColor(normalOutline_);
    
    // Setup text
    displayText_.setFillColor(textColor_);
    placeholderText_.setFillColor(sf::Color(150, 150, 150));
    
    // Setup cursor
    cursor_.setSize({2.f, height - 12.f});
    cursor_.setFillColor(sf::Color(20, 118, 172));
    
    // Setup eye toggle area
    eyeToggleArea_.setFillColor(sf::Color::Transparent);
    
    // Load eye icon textures
    loadTextures();
}

void PasswordField::loadTextures() {
    if (eyeOpenTexture_.loadFromFile("../assets/elements/eye-open.png") &&
        eyeClosedTexture_.loadFromFile("../assets/elements/eye-close.png")) {
        eyeOpenTexture_.setSmooth(true);
        eyeClosedTexture_.setSmooth(true);
        eyeSprite_ = std::make_unique<sf::Sprite>(eyeClosedTexture_);
        texturesLoaded_ = true;
    }
}

void PasswordField::setPosition(sf::Vector2f pos) {
    box_.setPosition(pos);
    updateDisplay();
    updateEyeIcon();
}

void PasswordField::setSize(sf::Vector2f size) {
    box_.setSize(size);
    cursor_.setSize({2.f, size.y - 12.f});
    updateDisplay();
    updateEyeIcon();
}

bool PasswordField::isActive() const {
    return active_;
}

const std::string& PasswordField::getValue() const {
    return input_;
}

sf::Vector2f PasswordField::getPosition() const {
    return box_.getPosition();
}

sf::Vector2f PasswordField::getSize() const {
    return box_.getSize();
}

void PasswordField::handleEvent(const sf::Event& event, sf::Vector2f mousePos, bool mousePressed) {
    // Edge detection: only trigger on mouse down (not hold)
    bool mouseJustPressed = mousePressed && !wasMousePressed_;
    wasMousePressed_ = mousePressed;
    
    // Handle click on eye toggle
    if (mouseJustPressed && texturesLoaded_) {
        if (eyeToggleArea_.getGlobalBounds().contains(mousePos)) {
            showPassword_ = !showPassword_;
            updateDisplay();
            updateEyeIcon();
            return;
        }
    }
    
    // Handle click on box to focus
    if (mouseJustPressed) {
        bool wasActive = active_;
        active_ = box_.getGlobalBounds().contains(mousePos);
        if (active_ && !wasActive) {
            cursorVisible_ = true;
            cursorClock_.restart();
        }
        box_.setOutlineColor(active_ ? activeOutline_ : normalOutline_);
        box_.setOutlineThickness(active_ ? 2.f : 1.f);
    }
    
    // Handle text input when active
    if (active_) {
        if (const auto* textEvent = event.getIf<sf::Event::TextEntered>()) {
            char32_t unicode = textEvent->unicode;
            
            // Backspace
            if (unicode == 8) {
                if (!input_.empty()) {
                    input_.pop_back();
                    updateDisplay();
                }
            }
            // Regular character (printable ASCII for password)
            else if (unicode >= 32 && unicode < 127) {
                input_ += static_cast<char>(unicode);
                updateDisplay();
            }
            
            cursorVisible_ = true;
            cursorClock_.restart();
        }
        
        if (const auto* keyEvent = event.getIf<sf::Event::KeyPressed>()) {
            if (keyEvent->code == sf::Keyboard::Key::Escape) {
                active_ = false;
                box_.setOutlineColor(normalOutline_);
                box_.setOutlineThickness(1.f);
            }
        }
    }
}

void PasswordField::update(sf::Vector2f mousePos) {
    (void)mousePos;
    // Cursor blinking
    if (cursorClock_.getElapsedTime().asSeconds() >= 0.5f) {
        cursorVisible_ = !cursorVisible_;
        cursorClock_.restart();
    }
}

void PasswordField::draw(sf::RenderWindow& window) {
    window.draw(box_);

    // Clip text + cursor to the box (prevents overflow)
    sf::View oldView = window.getView();
    const auto ws = window.getSize();
    sf::FloatRect box = box_.getGlobalBounds();
    sf::View clipView;
    clipView.setCenter({box.position.x + box.size.x / 2.f, box.position.y + box.size.y / 2.f});
    clipView.setSize({box.size.x, box.size.y});
    clipView.setViewport(sf::FloatRect(
        {box.position.x / static_cast<float>(ws.x), box.position.y / static_cast<float>(ws.y)},
        {box.size.x / static_cast<float>(ws.x), box.size.y / static_cast<float>(ws.y)}
    ));
    window.setView(clipView);

    // Draw placeholder or actual text
    if (input_.empty()) {
        window.draw(placeholderText_);
    } else {
        window.draw(displayText_);
    }

    // Draw cursor when active
    if (active_ && cursorVisible_) {
        window.draw(cursor_);
    }

    window.setView(oldView);
    
    // Draw eye icon
    if (texturesLoaded_ && eyeSprite_) {
        window.draw(eyeToggleArea_);
        window.draw(*eyeSprite_);
    }
}

void PasswordField::setActive(bool active) {
    active_ = active;
    box_.setOutlineColor(active_ ? activeOutline_ : normalOutline_);
    box_.setOutlineThickness(active_ ? 2.f : 1.f);
    if (active_) {
        cursorVisible_ = true;
        cursorClock_.restart();
    }
}

bool PasswordField::containsPoint(sf::Vector2f point) const {
    return box_.getGlobalBounds().contains(point);
}

void PasswordField::setValue(const std::string& value) {
    input_ = value;
    updateDisplay();
}

void PasswordField::clear() {
    input_.clear();
    showPassword_ = false;
    updateDisplay();
    updateEyeIcon();
}

void PasswordField::setFillColor(sf::Color color) {
    box_.setFillColor(color);
}

void PasswordField::setOutlineColor(sf::Color color) {
    normalOutline_ = color;
    if (!active_) {
        box_.setOutlineColor(color);
    }
}

void PasswordField::setOutlineThickness(float thickness) {
    if (!active_) {
        box_.setOutlineThickness(thickness);
    }
}

void PasswordField::setActiveOutlineColor(sf::Color color) {
    activeOutline_ = color;
    if (active_) {
        box_.setOutlineColor(color);
    }
}

void PasswordField::setTextColor(sf::Color color) {
    textColor_ = color;
    displayText_.setFillColor(color);
}

void PasswordField::setPlaceholder(const std::wstring& text) {
    placeholderText_.setString(text);
}

void PasswordField::setCharacterSize(unsigned int size) {
    displayText_.setCharacterSize(size);
    placeholderText_.setCharacterSize(size);
}

sf::FloatRect PasswordField::getGlobalBounds() const {
    return box_.getGlobalBounds();
}

void PasswordField::updateDisplay() {
    sf::Vector2f pos = box_.getPosition();
    sf::Vector2f size = box_.getSize();
    
    // Update display text (masked or plain)
    if (showPassword_) {
        displayText_.setString(input_);
    } else {
        displayText_.setString(std::string(input_.length(), '*'));
    }
    
    // Position text (Google-like horizontal scrolling when overflow)
    float textY = pos.y + (size.y - displayText_.getCharacterSize()) / 2.f - 2.f;

    float textX = pos.x + TEXT_PADDING;
    float reservedRight = (texturesLoaded_ ? (EYE_ICON_SIZE + EYE_PADDING * 2.f) : 0.f);
    float available = std::max(0.f, size.x - TEXT_PADDING * 2.f - reservedRight);
    sf::FloatRect textBounds = displayText_.getLocalBounds();
    if (textBounds.size.x > available) {
        textX += (available - textBounds.size.x);
    }

    displayText_.setPosition({textX, textY});
    placeholderText_.setPosition({pos.x + TEXT_PADDING, textY});
    
    // Position cursor after text
    float cursorX = displayText_.getPosition().x + textBounds.size.x + 2.f;
    float cursorY = pos.y + (size.y - cursor_.getSize().y) / 2.f;
    cursor_.setPosition({cursorX, cursorY});
}

void PasswordField::updateEyeIcon() {
    if (!texturesLoaded_ || !eyeSprite_) return;
    
    sf::Vector2f pos = box_.getPosition();
    sf::Vector2f size = box_.getSize();
    
    // Update texture based on state
    eyeSprite_->setTexture(showPassword_ ? eyeOpenTexture_ : eyeClosedTexture_);
    
    // Scale icon
    auto texSize = eyeSprite_->getTexture().getSize();
    float scale = EYE_ICON_SIZE / static_cast<float>(std::max(texSize.x, texSize.y));
    eyeSprite_->setScale({scale, scale});
    
    // Position icon
    float scaledW = texSize.x * scale;
    float scaledH = texSize.y * scale;
    float iconX = pos.x + size.x - scaledW - EYE_PADDING;
    float iconY = pos.y + (size.y - scaledH) / 2.f;
    eyeSprite_->setPosition({iconX, iconY});
    
    // Position toggle area
    eyeToggleArea_.setSize({scaledW + EYE_PADDING * 2.f, size.y});
    eyeToggleArea_.setPosition({pos.x + size.x - eyeToggleArea_.getSize().x, pos.y});
}

std::wstring PasswordField::getMaskedText() const {
    return std::wstring(input_.length(), L'*');
}
