#include "UI/components/Admin/UICheckboxItem.h"
#include <cstdint>

UICheckboxItem::Config::Config()
    : width(400.f)
    , height(36.f)
    , checkboxSize(18.f)
    , padding(10.f)
    , fontSize(13)
    , bgColor(sf::Color::White)
    , bgHoverColor(235, 244, 252)
    , bgSelectedColor(214, 232, 248)
    , checkboxColor(sf::Color::White)
    , checkboxCheckedColor(20, 118, 172)
    , borderColor(210, 216, 224)
    , textColor(33, 37, 41)
    , secondaryTextColor(100, 100, 100)
{}

UICheckboxItem::UICheckboxItem(sf::Font& font, const Config& config)
    : font_(font)
    , config_(config)
    , primaryTextObj_(std::make_unique<sf::Text>(font))
    , secondaryTextObj_(std::make_unique<sf::Text>(font))
    , checkmark_(std::make_unique<sf::Text>(font))
{
    // Setup background
    background_.setSize({config_.width, config_.height});
    background_.setFillColor(config_.bgColor);
    
    // Setup checkbox
    checkbox_.setSize({config_.checkboxSize, config_.checkboxSize});
    checkbox_.setFillColor(config_.checkboxColor);
    checkbox_.setOutlineThickness(1.f);
    checkbox_.setOutlineColor(config_.borderColor);
    
    // Setup primary text
    primaryTextObj_->setCharacterSize(config_.fontSize);
    primaryTextObj_->setFillColor(config_.textColor);
    
    // Setup secondary text
    secondaryTextObj_->setCharacterSize(config_.fontSize - 2);
    secondaryTextObj_->setFillColor(config_.secondaryTextColor);
    
    // Setup checkmark
    checkmark_->setString(L"✓");
    checkmark_->setCharacterSize(config_.fontSize);
    checkmark_->setFillColor(sf::Color::White);
    
    updateVisuals();
}

void UICheckboxItem::setPrimaryText(const std::string& text) {
    primaryText_ = text;
    primaryTextObj_->setString(toUtf8(text));
    updateVisuals();
}

void UICheckboxItem::setSecondaryText(const std::string& text) {
    secondaryText_ = text;
    secondaryTextObj_->setString(toUtf8(text));
    updateVisuals();
}

void UICheckboxItem::setData(const std::string& primary, const std::string& secondary) {
    primaryText_ = primary;
    secondaryText_ = secondary;
    primaryTextObj_->setString(toUtf8(primary));
    secondaryTextObj_->setString(toUtf8(secondary));
    updateVisuals();
}

void UICheckboxItem::setChecked(bool checked) {
    if (checked_ != checked) {
        checked_ = checked;
        updateVisuals();
    }
}

bool UICheckboxItem::isChecked() const {
    return checked_;
}

void UICheckboxItem::toggle() {
    checked_ = !checked_;
    updateVisuals();
    if (onToggle_) {
        onToggle_(checked_);
    }
}

void UICheckboxItem::setPosition(sf::Vector2f pos) {
    position_ = pos;
    updateVisuals();
}

sf::Vector2f UICheckboxItem::getPosition() const {
    return position_;
}

float UICheckboxItem::getHeight() const {
    return config_.height;
}

sf::FloatRect UICheckboxItem::getBounds() const {
    return sf::FloatRect(position_, {config_.width, config_.height});
}

bool UICheckboxItem::handleClick(sf::Vector2f mousePos) {
    if (getBounds().contains(mousePos)) {
        toggle();
        return true;
    }
    return false;
}

void UICheckboxItem::updateHover(sf::Vector2f mousePos) {
    bool wasHovered = hovered_;
    hovered_ = getBounds().contains(mousePos);
    if (wasHovered != hovered_) {
        updateVisuals();
    }
}

bool UICheckboxItem::isHovered() const {
    return hovered_;
}

void UICheckboxItem::setOnToggle(ToggleCallback callback) {
    onToggle_ = std::move(callback);
}

void UICheckboxItem::setAlternateBackground(bool alternate) {
    alternateBackground_ = alternate;
}

void UICheckboxItem::render(sf::RenderTarget& target) {
    target.draw(background_);
    target.draw(checkbox_);
    target.draw(*primaryTextObj_);
    
    if (!secondaryText_.empty()) {
        target.draw(*secondaryTextObj_);
    }
    
    if (checked_) {
        target.draw(*checkmark_);
    }
}

void UICheckboxItem::updateVisuals() {
    // Background position and color
    background_.setPosition(position_);
    
    sf::Color bgColor = config_.bgColor;
    if (alternateBackground_) {
        bgColor = sf::Color(248, 249, 250);
    }
    if (checked_) {
        bgColor = config_.bgSelectedColor;
    }
    if (hovered_) {
        bgColor = config_.bgHoverColor;
    }
    background_.setFillColor(bgColor);
    
    // Checkbox position and color
    float checkboxY = position_.y + (config_.height - config_.checkboxSize) / 2.f;
    checkbox_.setPosition({position_.x + config_.padding, checkboxY});
    
    if (checked_) {
        checkbox_.setFillColor(config_.checkboxCheckedColor);
        checkbox_.setOutlineColor(config_.checkboxCheckedColor);
    } else {
        checkbox_.setFillColor(config_.checkboxColor);
        checkbox_.setOutlineColor(config_.borderColor);
    }
    
    // Text positions
    float textX = position_.x + config_.padding + config_.checkboxSize + config_.padding;
    float textY = position_.y + (config_.height - static_cast<float>(config_.fontSize)) / 2.f - 3.f;
    
    // If has secondary text, arrange vertically
    if (!secondaryText_.empty()) {
        float totalTextHeight = static_cast<float>(config_.fontSize) + 2.f + static_cast<float>(config_.fontSize - 2);
        float startY = position_.y + (config_.height - totalTextHeight) / 2.f;
        primaryTextObj_->setPosition({textX, startY - 2.f});
        secondaryTextObj_->setPosition({textX, startY + static_cast<float>(config_.fontSize) + 2.f});
    } else {
        primaryTextObj_->setPosition({textX, textY});
    }
    
    // Checkmark position (centered in checkbox)
    sf::FloatRect checkmarkBounds = checkmark_->getLocalBounds();
    float cmX = checkbox_.getPosition().x + (config_.checkboxSize - checkmarkBounds.size.x) / 2.f - checkmarkBounds.position.x;
    float cmY = checkbox_.getPosition().y + (config_.checkboxSize - checkmarkBounds.size.y) / 2.f - checkmarkBounds.position.y;
    checkmark_->setPosition({cmX, cmY});
}

sf::String UICheckboxItem::toUtf8(const std::string& text) {
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
