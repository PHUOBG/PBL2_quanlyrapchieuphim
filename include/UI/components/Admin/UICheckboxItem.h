#pragma once

#include <SFML/Graphics.hpp>
#include <string>
#include <functional>
#include <memory>

class UICheckboxItem {
public:
    struct Config {
        float width;
        float height;
        float checkboxSize;
        float padding;
        unsigned int fontSize;
        
        sf::Color bgColor;
        sf::Color bgHoverColor;
        sf::Color bgSelectedColor;
        sf::Color checkboxColor;
        sf::Color checkboxCheckedColor;
        sf::Color borderColor;
        sf::Color textColor;
        sf::Color secondaryTextColor;
        
        Config();
    };

    explicit UICheckboxItem(sf::Font& font, const Config& config = Config{});
    
    // Thiet lap du lieu hien thi
    void setPrimaryText(const std::string& text);
    void setSecondaryText(const std::string& text);
    void setData(const std::string& primary, const std::string& secondary = "");
    
    // Quan ly trang thai
    void setChecked(bool checked);
    bool isChecked() const;
    void toggle();
    
    // Vi tri va bo cuc
    void setPosition(sf::Vector2f pos);
    sf::Vector2f getPosition() const;
    sf::FloatRect getBounds() const;
    float getHeight() const;
    
    // Tuong tac
    bool handleClick(sf::Vector2f mousePos);  // Returns true if state changed
    void updateHover(sf::Vector2f mousePos);
    bool isHovered() const;
    
    // Ham goi lai
    using ToggleCallback = std::function<void(bool checked)>;
    void setOnToggle(ToggleCallback callback);
    
    // Ve len man hinh
    void render(sf::RenderTarget& target);
    
    // Dieu chinh nen phu hop voi ngoai vi (vi du hang xen ke)
    void setAlternateBackground(bool alternate);

private:
    sf::Font& font_;
    Config config_;
    
    sf::Vector2f position_;
    bool checked_ = false;
    bool hovered_ = false;
    bool alternateBackground_ = false;
    
    std::string primaryText_;
    std::string secondaryText_;
    
    // Thanh phan da duoc tao san de ve nhanh
    sf::RectangleShape background_;
    sf::RectangleShape checkbox_;
    std::unique_ptr<sf::Text> primaryTextObj_;
    std::unique_ptr<sf::Text> secondaryTextObj_;
    std::unique_ptr<sf::Text> checkmark_;
    
    ToggleCallback onToggle_;
    
    void updateVisuals();
    static sf::String toUtf8(const std::string& text);
};
