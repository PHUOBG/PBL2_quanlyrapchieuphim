#pragma once
#include <SFML/Graphics.hpp>
#include <cstdint>
#include <memory>
using namespace sf;
using namespace std;

class Button {
    private:
        const Font& font;
        RectangleShape box;
        unique_ptr<Text> text;
        bool hovered = false;
        bool pressed = false;
        
        Color normalColor = Color(200, 200, 200);
        Color hoverColor = Color(180, 180, 180);
        
    public:
        Button(const Font&, const String&, float, float, int);
        Button(const Font&, const String&, Vector2f size, int textSize);
        
        Button(const Button&);
        Button& operator=(const Button&);
        Button(Button&&) = default;
        Button& operator=(Button&&) = default;

        void setPosition(Vector2f);
        void setText(const String&);
        void setFillColor(Color);
        void setTextColor(Color);
        void setOutlineThickness(float);
        void setOutlineColor(Color);
        void setColors(Color normal, Color hover, Color textColor);
        void setAlpha(std::uint8_t alpha);

        Vector2f getPosition() const;
        Vector2f getSize() const;
        FloatRect getGlobalBounds() const;
        String getString() const;
        bool isHovered() const;
        bool isPressed() const;

        void update(Vector2f mousePos, bool mousePressed, Color hover, Color normal);
        void update(Vector2f mousePos);
        bool isClicked(Vector2f mousePos, bool mousePressed);
        bool isClicked(Vector2f mousePos);
        void draw(RenderWindow&);
};
