#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <functional>
#include <memory>
#include "core/AppState.h"
#include "data-structures/DLL.h"
#include "UI/components/Admin/RoundedRectRenderer.h"

using namespace sf;
using namespace std;

class SidebarRoundRectButton {
private:
    Font& font;
    Vector2f size;
    Vector2f position;
    float radius;
    Color baseColor;
    Color hoverColor;
    bool hovered;
    unique_ptr<Text> label;

public:
    SidebarRoundRectButton(Font& font, const string& text, Vector2f size, float radius);

    void setPosition(Vector2f pos);
    void setColors(const Color& base, const Color& hover);
    void setTextSize(unsigned int size);
    void update(Vector2f mousePos);
    bool contains(Vector2f point) const;
    void draw(RenderWindow& window) const;
    Vector2f getSize() const;
};

class AdminSidebar {
private:
    struct MenuItem {
        string label;
        AppState targetState;
        bool isGroupHeader;
        bool isLogout;
        
        MenuItem(const string& l, AppState state, bool header = false, bool logout = false);
    };
    
    float width;
    float height;
    Vector2f position;
    
    // Cau hinh mau sac
    Color bgColor;          // #0D1B2A
    Color textColor;        // #E9EEF5
    Color hoverColor;       // #1B263B
    Color activeColor;      // #1476AC
    Color groupHeaderColor; // Sang hon mot chut
    
    // Danh sach muc
    DLL<MenuItem> menuItems;
    int hoveredIndex;
    int activeIndex;
    
    // Hinh chu nhat trang tri
    RectangleShape background;
    DLL<RectangleShape> itemBackgrounds;
    
    // Van ban
    Font font;
    Font montserratFont;
    unique_ptr<Text> headerText;
    unique_ptr<Text> userNameText;
    DLL<unique_ptr<Text>> itemTexts;
    
    // Ham goi lai khi doi trang thai
    function<void(AppState)> onStateChange;
    function<void()> onLogout;
    
    // Thong tin nguoi dung
    string userName;
    RectangleShape userInfoBg;
    unique_ptr<SidebarRoundRectButton> logoutButton;
    
    void initializeMenuItems();
    void setupGraphics();
    int getItemAtPosition(Vector2f mousePos);
    
public:
    AdminSidebar(float width, float height, const string& userName);
    
    void setPosition(Vector2f pos);
    void setOnStateChange(function<void(AppState)> callback);
    void setOnLogout(function<void()> callback);
    void setActiveState(AppState state);
    
    void handleEvent(const Event& event, const RenderWindow& window);
    void update(const RenderWindow& window);
    void render(RenderWindow& window);
    
    float getWidth() const;
};