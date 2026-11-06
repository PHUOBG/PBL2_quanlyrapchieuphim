#pragma once
#include <SFML/Graphics.hpp>
using namespace sf;
using namespace std;

class TextButton {
    private:
        Text text;
        bool hovered = false;
    public:
        TextButton(const Font&, const String&, unsigned int, Vector2f);
        void update(const Vector2f&);
        bool isClicked(const Vector2f&, bool) const;
        void draw(RenderWindow&) const;
        void setOutlineColor(const Color&);
        void setOutlineThickness(float);
        void setString(const String&);
        void setPosition(Vector2f);
        FloatRect getGlobalBounds() const;
        Vector2f getPosition() const;
};