#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <memory>
#include <algorithm>

using namespace std;
using namespace sf;

class DropdownBox {
private:
    Font& font;
    unique_ptr<Text> labelText;
    unique_ptr<Text> selectedText;
    vector<unique_ptr<Text>> optionTexts;
    
    RectangleShape background;
    RectangleShape dropdownPanel;
    vector<RectangleShape> optionBackgrounds;
    
    RectangleShape arrow; // Mui ten chi trang thai dong/mo
    
    vector<string> options;
    int selectedIndex;
    bool isOpen;
    bool enabled;
    
    float x, y, width, height;
    string label;
    
    Color bgColor;
    Color hoverColor;
    Color selectedColor;
    Color borderColor;
    Color textColor;

    int maxVisibleOptions;
    int firstVisibleIndex;
    float optionHeight;

    void clampFirstVisible();
    void updateDropdownPanel();
    int getVisibleCount() const;
    
public:
    DropdownBox(Font& font, const string& label, float x, float y, float width, float height);
    
    void setEnabled(bool enabled);
    bool isEnabled() const;

    void setOptions(const vector<string>& opts);
    void setMaxVisibleOptions(int count);
    void setSelectedIndex(int index);
    void setSelectedValue(const string& value);
    string getSelectedValue() const;
    int getSelectedIndex() const;
    
    void setPosition(Vector2f pos);

    void handleEvent(const Event& event, const Vector2f& mousePos);
    void update();
    void draw(RenderWindow& window);
    
    bool isMouseOver(const Vector2f& mousePos) const;
    bool isDropdownOpen() const;
    Vector2f getPosition() const;
};
