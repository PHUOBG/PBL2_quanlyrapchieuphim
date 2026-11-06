#ifndef TEXTBOX_H
#define TEXTBOX_H

#include <SFML/Graphics.hpp>
#include <string>
#include <functional>
#include <memory>

using namespace sf;
using namespace std;

class TextBox {
private:
    Font& font;
    float x, y, width, height;
    
    RectangleShape background;
    RectangleShape border;
    unique_ptr<Text> displayText;
    unique_ptr<Text> labelText;
    RectangleShape cursor;
    
    string value;
    string placeholder;
    bool isFocused;
    bool isActive;
    bool editable = true;
    
    Clock cursorClock;
    
    Color bgColor = Color(255, 255, 255);
    Color borderColor = Color(201, 206, 214);  // #C9CED6
    Color focusColor = Color(20, 118, 172);    // #1476AC
    Color textColor = Color(34, 34, 34);
    Color placeholderColor = Color(160, 160, 160);
    Color disabledBgColor = Color(240, 240, 240);
    float horizontalPadding = 10.f;
    
public:
    // Khoi tao kem nhan hien thi
    TextBox(Font& font, const string& label, float x, float y, float width, float height);
    
    // Khoi tao khong co nhan (dung trong popup)
    TextBox(Font& font, float width, float height);
    
    void setPlaceholder(const string& text);
    void setValue(const string& text);
    void setText(const string& text);
    string getValue() const;
    string getText() const;
    
    void setPosition(Vector2f pos);
    void setEditable(bool edit);
    bool isEditable() const;
    
    void handleEvent(const Event& event, const RenderWindow& window);
    void handleEvent(const Event& event);
    void update(Vector2f mousePos, bool mousePressed);
    void update(Vector2f mousePos);
    void render(RenderWindow& window);
    void draw(RenderWindow& window);
    
    void setFocus(bool focus);
    bool getFocus() const;
};

#endif
