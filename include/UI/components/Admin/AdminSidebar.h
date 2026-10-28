#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <functional>
#include <vector>
#include <optional>
#include "core/AppState.h"
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
    SidebarRoundRectButton(Font& font, const string& text, Vector2f size, float radius)
        : font(font), size(size), position(Vector2f()), radius(radius),
          baseColor(Color::White), hoverColor(Color::White), hovered(false) {
        label = make_unique<Text>(font);
        label->setCharacterSize(20);
        label->setFillColor(Color::White);
        label->setString(String::fromUtf8(text.begin(), text.end()));
    }

    void setPosition(Vector2f pos) {
        position = pos;
        if (label) {
            FloatRect bounds = label->getLocalBounds();
            label->setPosition(Vector2f(
                position.x + (size.x - bounds.size.x) / 2.f - bounds.position.x,
                position.y + (size.y - bounds.size.y) / 2.f - bounds.position.y
            ));
        }
    }

    void setColors(const Color& base, const Color& hover) {
        baseColor = base;
        hoverColor = hover;
    }

    void setTextSize(unsigned int sz) {
        if (label) {
            label->setCharacterSize(sz);
            setPosition(position);
        }
    }

    void update(Vector2f mousePos) {
        FloatRect bounds(position, size);
        hovered = bounds.contains(mousePos);
    }

    bool contains(Vector2f point) const {
        FloatRect bounds(position, size);
        return bounds.contains(point);
    }

    void draw(RenderWindow& window) const {
        const Color& color = hovered ? hoverColor : baseColor;
        RoundedRectRenderer::draw(window, position, size, radius, color);
        if (label) {
            window.draw(*label);
        }
    }

    Vector2f getSize() const {
        return size;
    }
};

enum class SidebarElementType {
    SECTION_HEADER,
    MENU_ITEM
};

struct SidebarElement {
    SidebarElementType type;
    wstring label;
    AppState targetState;
    FloatRect bounds;
};

class AdminSidebar {
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

private:
    float width;
    float height;
    Vector2f position;

    Font font;
    Font boldFont;
    string userName;

    AppState activeState = AppState::ADMIN_MOVIES;
    int hoveredIndex = -1;
    bool logoutHovered = false;

    vector<SidebarElement> elements;
    FloatRect logoutBounds;

    function<void(AppState)> onStateChange;
    function<void()> onLogout;

    Texture logoTexture;
    optional<Sprite> logoSprite;

    void recalculateLayout();
    void drawLogoutIcon(RenderTarget& target, Vector2f pos, Color col);
};