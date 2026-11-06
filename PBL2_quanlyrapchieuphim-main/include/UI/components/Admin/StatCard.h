#pragma once

#include <SFML/Graphics.hpp>
#include <string>
using namespace sf;
using namespace std;

class StatCard {
private:
    Font& font;
    RectangleShape box;
    Text titleText;      // Tiêu đề + ngày/tháng trong ngoặc
    Text valueText;      // Giá trị số

    void positionValue();

public:
    StatCard(Font& font, const Vector2f& size, const Color& outlineColor);

    void setPosition(const Vector2f& position);
    void setSize(const Vector2f& size);
    void setOutlineThickness(float);
    void setOutlineColor(const Color&);

    void setTitleWithDate(const string& title, const string& dateStr);  // Tiêu đề (ngày/tháng)
    void setValue(const string& value);

    Vector2f getPosition() const;
    Vector2f getSize() const;

    void render(RenderTarget& target) const;
};
