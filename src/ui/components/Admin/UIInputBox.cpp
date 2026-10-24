#include "UI/components/Admin/UIInputBox.h"

UIInputBox::Config::Config()
    : width(300.f)
    , height(36.f)
    , padding(10.f)
    , fontSize(13)
    , showSearchIcon(false)
    , borderRadius(4.f)
    , backgroundColor(sf::Color::White)
    , borderColor(201, 206, 214)
    , borderFocusColor(20, 118, 172)
    , textColor(34, 34, 34)
    , placeholderColor(160, 160, 160)
    , disabledBackgroundColor(240, 240, 240)
{}

UIInputBox::UIInputBox(sf::Font& font, const Config& config)
    : font_(font)
    , config_(config)
    , textObj_(std::make_unique<sf::Text>(font))
    , placeholderObj_(std::make_unique<sf::Text>(font))
    , searchIcon_(std::make_unique<sf::Text>(font))
{
    // Background
    background_.setSize({config_.width, config_.height});
    background_.setFillColor(config_.backgroundColor);
    
    // Border
    border_.setSize({config_.width, config_.height});
    border_.setFillColor(sf::Color::Transparent);
    border_.setOutlineThickness(1.f);
    border_.setOutlineColor(config_.borderColor);
    
    // Text
    textObj_->setCharacterSize(config_.fontSize);
    textObj_->setFillColor(config_.textColor);
    
    // Placeholder
    placeholderObj_->setCharacterSize(config_.fontSize);
    placeholderObj_->setFillColor(config_.placeholderColor);
    
    // Cursor
    cursor_.setSize({1.5f, config_.height - 12.f});
    cursor_.setFillColor(config_.textColor);
    
    // Search icon
    searchIcon_->setString(L"");
    searchIcon_->setCharacterSize(config_.fontSize);
    searchIcon_->setFillColor(config_.placeholderColor);
    
    updateVisuals();
}

const std::string& UIInputBox::getText() const {
    return text_;
}

const std::string& UIInputBox::getPlaceholder() const {
    return placeholder_;
}

bool UIInputBox::isEnabled() const {
    return enabled_;
}

bool UIInputBox::isFocused() const {
    return focused_;
}

sf::Vector2f UIInputBox::getPosition() const {
    return position_;
}

float UIInputBox::getWidth() const {
    return config_.width;
}

float UIInputBox::getHeight() const {
    return config_.height;
}

void UIInputBox::setOnTextChange(TextChangeCallback callback) {
    onTextChange_ = std::move(callback);
}

void UIInputBox::setText(const std::string& text) {
    if (text_ != text) {
        text_ = text;
        textObj_->setString(toUtf8(text_));
        updateVisuals();
        if (onTextChange_) {
            onTextChange_(text_);
        }
    }
}

void UIInputBox::clear() {
    setText("");
}

void UIInputBox::setPlaceholder(const std::string& placeholder) {
    placeholder_ = placeholder;
    placeholderObj_->setString(toUtf8(placeholder));
    updateVisuals();
}

void UIInputBox::setEnabled(bool enabled) {
    enabled_ = enabled;
    updateVisuals();
}

void UIInputBox::setFocus(bool focus) {
    if (enabled_ && focused_ != focus) {
        focused_ = focus;
        cursorClock_.restart();
        cursorVisible_ = true;
        updateVisuals();
    }
}

void UIInputBox::setPosition(sf::Vector2f pos) {
    position_ = pos;
    updateVisuals();
}

sf::FloatRect UIInputBox::getBounds() const {
    return sf::FloatRect(position_, {config_.width, config_.height});
}

void UIInputBox::setWidth(float width) {
    config_.width = width;
    background_.setSize({config_.width, config_.height});
    border_.setSize({config_.width, config_.height});
    updateVisuals();
}

void UIInputBox::handleEvent(const sf::Event& event) {
    if (!enabled_) return;
    
    // Handle text entered
    if (focused_ && event.is<sf::Event::TextEntered>()) {
        const auto* textEvent = event.getIf<sf::Event::TextEntered>();
        if (textEvent) {
            processTextInput(textEvent->unicode);
        }
    }
    
    // Handle key pressed (backspace, etc.)
    if (focused_ && event.is<sf::Event::KeyPressed>()) {
        const auto* keyEvent = event.getIf<sf::Event::KeyPressed>();
        if (keyEvent) {
            if (keyEvent->code == sf::Keyboard::Key::Backspace && !text_.empty()) {
                // Handle UTF-8 backspace properly
                size_t len = text_.size();
                if (len > 0) {
                    // Find the start of the last UTF-8 character
                    size_t pos = len - 1;
                    while (pos > 0 && (static_cast<unsigned char>(text_[pos]) & 0xC0) == 0x80) {
                        --pos;
                    }
                    text_.erase(pos);
                    textObj_->setString(toUtf8(text_));
                    updateVisuals();
                    if (onTextChange_) {
                        onTextChange_(text_);
                    }
                }
            }
        }
    }
}

void UIInputBox::update(sf::Vector2f mousePos, bool mousePressed) {
    if (!enabled_) return;
    
    // Handle click focus
    if (mousePressed) {
        bool wasClicked = getBounds().contains(mousePos);
        setFocus(wasClicked);
    }
    
    // Update cursor blink
    if (focused_) {
        if (cursorClock_.getElapsedTime().asMilliseconds() > 500) {
            cursorVisible_ = !cursorVisible_;
            cursorClock_.restart();
        }
    }
}

void UIInputBox::render(sf::RenderTarget& target) {
    target.draw(background_);
    target.draw(border_);
    
    float textX = position_.x + config_.padding;
    
    // Draw search icon
    if (config_.showSearchIcon) {
        target.draw(*searchIcon_);
        textX += 24.f; // Icon width + spacing
    }
    
    // Clip text/cursor to the input box (prevents overflow)
    if (auto* win = dynamic_cast<sf::RenderWindow*>(&target)) {
        sf::View oldView = win->getView();
        const auto ws = win->getSize();
        sf::FloatRect box(position_, {config_.width, config_.height});
        sf::View clipView;
        clipView.setCenter({box.position.x + box.size.x / 2.f, box.position.y + box.size.y / 2.f});
        clipView.setSize({box.size.x, box.size.y});
        clipView.setViewport(sf::FloatRect(
            {box.position.x / static_cast<float>(ws.x), box.position.y / static_cast<float>(ws.y)},
            {box.size.x / static_cast<float>(ws.x), box.size.y / static_cast<float>(ws.y)}
        ));
        win->setView(clipView);

        // Draw text or placeholder
        if (text_.empty() && !focused_) {
            win->draw(*placeholderObj_);
        } else {
            win->draw(*textObj_);
            if (focused_ && cursorVisible_) {
                win->draw(cursor_);
            }
        }

        win->setView(oldView);
    } else {
        // Fallback (no clipping) for non-window targets
        if (text_.empty() && !focused_) {
            target.draw(*placeholderObj_);
        } else {
            target.draw(*textObj_);
            if (focused_ && cursorVisible_) {
                target.draw(cursor_);
            }
        }
    }
}

void UIInputBox::updateVisuals() {
    // Background and border position
    background_.setPosition(position_);
    border_.setPosition(position_);
    
    // Background color based on state
    if (!enabled_) {
        background_.setFillColor(config_.disabledBackgroundColor);
    } else {
        background_.setFillColor(config_.backgroundColor);
    }
    
    // Border color based on focus
    if (focused_) {
        border_.setOutlineColor(config_.borderFocusColor);
    } else {
        border_.setOutlineColor(config_.borderColor);
    }
    
    // Calculate text X position
    float textX = position_.x + config_.padding;
    if (config_.showSearchIcon) {
        // Search icon position
        searchIcon_->setPosition({position_.x + config_.padding, position_.y + (config_.height - static_cast<float>(config_.fontSize)) / 2.f - 2.f});
        textX += 24.f;
    }
    
    // Text position (vertically centered)
    float textY = position_.y + (config_.height - static_cast<float>(config_.fontSize)) / 2.f - 3.f;

    // Horizontal scrolling: keep the tail visible when input is too long
    sf::FloatRect textBounds = textObj_->getLocalBounds();
    float available = config_.width - 2.f * config_.padding - (config_.showSearchIcon ? 24.f : 0.f);
    float scrolledX = textX;
    if (textBounds.size.x > available) {
        scrolledX += (available - textBounds.size.x);
    }

    textObj_->setPosition({scrolledX, textY});
    placeholderObj_->setPosition({textX, textY});
    
    // Cursor position (after text)
    float cursorX = scrolledX + textBounds.size.x + 2.f;
    float cursorY = position_.y + 6.f;
    cursor_.setPosition({cursorX, cursorY});
}

void UIInputBox::processTextInput(uint32_t unicode) {
    // Ignore control characters except printable
    if (unicode < 32 || unicode == 127) return;
    
    // Convert unicode to UTF-8 and append
    if (unicode < 0x80) {
        text_ += static_cast<char>(unicode);
    } else if (unicode < 0x800) {
        text_ += static_cast<char>(0xC0 | (unicode >> 6));
        text_ += static_cast<char>(0x80 | (unicode & 0x3F));
    } else if (unicode < 0x10000) {
        text_ += static_cast<char>(0xE0 | (unicode >> 12));
        text_ += static_cast<char>(0x80 | ((unicode >> 6) & 0x3F));
        text_ += static_cast<char>(0x80 | (unicode & 0x3F));
    } else {
        text_ += static_cast<char>(0xF0 | (unicode >> 18));
        text_ += static_cast<char>(0x80 | ((unicode >> 12) & 0x3F));
        text_ += static_cast<char>(0x80 | ((unicode >> 6) & 0x3F));
        text_ += static_cast<char>(0x80 | (unicode & 0x3F));
    }
    
    textObj_->setString(toUtf8(text_));
    updateVisuals();
    
    if (onTextChange_) {
        onTextChange_(text_);
    }
}

sf::String UIInputBox::toUtf8(const std::string& text) {
    sf::String result;
    size_t i = 0;
    while (i < text.size()) {
        uint32_t codepoint = 0;
        unsigned char c = text[i];
        
        if ((c & 0x80) == 0) {
            codepoint = c;
            i += 1;
        } else if ((c & 0xE0) == 0xC0) {
            codepoint = (c & 0x1F) << 6;
            if (i + 1 < text.size()) {
                codepoint |= (text[i + 1] & 0x3F);
            }
            i += 2;
        } else if ((c & 0xF0) == 0xE0) {
            codepoint = (c & 0x0F) << 12;
            if (i + 1 < text.size()) {
                codepoint |= (text[i + 1] & 0x3F) << 6;
            }
            if (i + 2 < text.size()) {
                codepoint |= (text[i + 2] & 0x3F);
            }
            i += 3;
        } else if ((c & 0xF8) == 0xF0) {
            codepoint = (c & 0x07) << 18;
            if (i + 1 < text.size()) {
                codepoint |= (text[i + 1] & 0x3F) << 12;
            }
            if (i + 2 < text.size()) {
                codepoint |= (text[i + 2] & 0x3F) << 6;
            }
            if (i + 3 < text.size()) {
                codepoint |= (text[i + 3] & 0x3F);
            }
            i += 4;
        } else {
            i += 1;
            continue;
        }
        
        result += static_cast<char32_t>(codepoint);
    }
    return result;
}
