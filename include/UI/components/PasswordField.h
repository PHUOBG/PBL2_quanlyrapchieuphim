#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <memory>

class PasswordField {
public:
    PasswordField(const sf::Font& font, float width = 280.f, float height = 42.f);
    
    // Nhom ham chinh
    void setPosition(sf::Vector2f pos);
    void setSize(sf::Vector2f size);
    void handleEvent(const sf::Event& event, sf::Vector2f mousePos, bool mousePressed);
    void update(sf::Vector2f mousePos);
    void draw(sf::RenderWindow& window);
    
    // Trang thai
    void setActive(bool active);
    bool isActive() const;
    bool containsPoint(sf::Vector2f point) const;
    
    // Gia tri
    const std::string& getValue() const;
    void setValue(const std::string& value);
    void clear();
    
    // Tuy chinh hien thi
    void setFillColor(sf::Color color);
    void setOutlineColor(sf::Color color);
    void setOutlineThickness(float thickness);
    void setActiveOutlineColor(sf::Color color);
    void setTextColor(sf::Color color);
    void setPlaceholder(const std::wstring& text);
    void setCharacterSize(unsigned int size);
    
    // Thong tin khung bao
    sf::FloatRect getGlobalBounds() const;
    sf::Vector2f getPosition() const;
    sf::Vector2f getSize() const;

private:
    void updateDisplay();
    void updateEyeIcon();
    void loadTextures();
    std::wstring getMaskedText() const;
    
    const sf::Font& font_;
    
    // Thanh phan hien thi
    sf::RectangleShape box_;
    sf::Text displayText_;
    sf::Text placeholderText_;
    sf::RectangleShape cursor_;
    
    // Nut hien/che mat khau
    sf::Texture eyeOpenTexture_;
    sf::Texture eyeClosedTexture_;
    std::unique_ptr<sf::Sprite> eyeSprite_;
    sf::RectangleShape eyeToggleArea_;
    
    // Cac bien trang thai
    std::string input_;
    bool active_ = false;
    bool showPassword_ = false;
    bool texturesLoaded_ = false;
    
    // Nhan chop tat cho con tro
    sf::Clock cursorClock_;
    bool cursorVisible_ = true;
    
    // Ghi nhan trang thai chuot cho viec bat su kien
    bool wasMousePressed_ = false;
    
    // Mau sac su dung
    sf::Color normalOutline_{201, 214, 226};
    sf::Color activeOutline_{20, 118, 172};
    sf::Color textColor_{0, 0, 0};
    
    // Hang so bo cuc
    static constexpr float EYE_ICON_SIZE = 18.f;
    static constexpr float EYE_PADDING = 10.f;
    static constexpr float TEXT_PADDING = 10.f;
};
