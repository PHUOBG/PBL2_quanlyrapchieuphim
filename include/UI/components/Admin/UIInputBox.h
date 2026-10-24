#pragma once

#include <SFML/Graphics.hpp>
#include <string>
#include <functional>
#include <memory>
#include <cstdint>

class UIInputBox {
public:
    struct Config {
        float width;
        float height;
        float padding;
        unsigned int fontSize;
        bool showSearchIcon;
        float borderRadius;
        
        sf::Color backgroundColor;
        sf::Color borderColor;
        sf::Color borderFocusColor;
        sf::Color textColor;
        sf::Color placeholderColor;
        sf::Color disabledBackgroundColor;
        
        Config();
    };

    using TextChangeCallback = std::function<void(const std::string& text)>;

    UIInputBox(sf::Font& font, const Config& config = Config{});
    
    // Quan ly chuoi hien thi
    void setText(const std::string& text);
    const std::string& getText() const;
    void clear();
    
    void setPlaceholder(const std::string& placeholder);
    const std::string& getPlaceholder() const;
    
    // Trang thai
    void setEnabled(bool enabled);
    bool isEnabled() const;
    
    void setFocus(bool focus);
    bool isFocused() const;
    
    // Bo cuc/vi tri
    void setPosition(sf::Vector2f pos);
    sf::Vector2f getPosition() const;
    sf::FloatRect getBounds() const;
    void setWidth(float width);
    float getWidth() const;
    float getHeight() const;
    
    // Xu ly su kien
    void handleEvent(const sf::Event& event);
    void update(sf::Vector2f mousePos, bool mousePressed = false);
    
    // Ham goi lai
    void setOnTextChange(TextChangeCallback callback);
    
    // Ve giao dien
    void render(sf::RenderTarget& target);

private:
    sf::Font& font_;
    Config config_;
    
    sf::Vector2f position_;
    std::string text_;
    std::string placeholder_;
    bool focused_ = false;
    bool enabled_ = true;
    
    // Dieu khien chop tat con tro
    sf::Clock cursorClock_;
    bool cursorVisible_ = true;
    
    // Thanh phan ve da cache
    sf::RectangleShape background_;
    sf::RectangleShape border_;
    std::unique_ptr<sf::Text> textObj_;
    std::unique_ptr<sf::Text> placeholderObj_;
    sf::RectangleShape cursor_;
    std::unique_ptr<sf::Text> searchIcon_;
    
    TextChangeCallback onTextChange_;
    
    void updateVisuals();
    void processTextInput(uint32_t unicode);
    static sf::String toUtf8(const std::string& text);
};
