#pragma once

#include <SFML/Graphics.hpp>
#include <cmath>
#include <string>

class FontManager {
private:
    sf::Font regularFont;
    sf::Font boldFont;
    bool initialized = false;

    static bool loadFromCandidates(sf::Font& target, const char* const* paths, size_t count) {
        for (size_t i = 0; i < count; ++i) {
            if (target.openFromFile(paths[i])) {
                target.setSmooth(true);
                return true;
            }
        }
        return false;
    }

    FontManager() = default;

public:
    static FontManager& instance() {
        static FontManager mgr;
        if (!mgr.initialized) {
            mgr.init();
        }
        return mgr;
    }

    void init() {
        if (initialized) return;
        const char* regularPaths[] = {
            "assets/fonts/segoeui.ttf",
            "../assets/fonts/segoeui.ttf",
            "C:/Windows/Fonts/segoeui.ttf"
        };
        const char* boldPaths[] = {
            "assets/fonts/segoeuib.ttf",
            "../assets/fonts/segoeuib.ttf",
            "assets/fonts/Montserrat_SemiBold.ttf",
            "../assets/fonts/Montserrat_SemiBold.ttf",
            "C:/Windows/Fonts/segoeuib.ttf"
        };
        loadFromCandidates(regularFont, regularPaths, 3);
        if (!loadFromCandidates(boldFont, boldPaths, 5)) {
            boldFont = regularFont;
        }
        initialized = true;
    }

    const sf::Font& regular() const { return regularFont; }
    const sf::Font& bold() const { return boldFont; }

    static sf::Vector2f snap(sf::Vector2f pos) {
        return {std::round(pos.x), std::round(pos.y)};
    }

    static sf::Text createText(const sf::Font& font, const std::wstring& str, unsigned int size,
                               sf::Color color, sf::Vector2f pos = {0.f, 0.f}) {
        sf::Text txt(font, str, size);
        txt.setFillColor(color);
        txt.setPosition(snap(pos));
        return txt;
    }

    static void drawText(sf::RenderTarget& target, const sf::Font& font, const std::wstring& str,
                         unsigned int size, sf::Color color, sf::Vector2f pos) {
        sf::Text txt = createText(font, str, size, color, pos);
        target.draw(txt);
    }

    static void drawCenteredText(sf::RenderTarget& target, const sf::Font& font, const std::wstring& str,
                                 unsigned int size, sf::Color color, sf::FloatRect bounds) {
        sf::Text txt(font, str, size);
        txt.setFillColor(color);
        sf::FloatRect lb = txt.getLocalBounds();
        float x = bounds.position.x + (bounds.size.x - lb.size.x) / 2.f - lb.position.x;
        float y = bounds.position.y + (bounds.size.y - lb.size.y) / 2.f - lb.position.y;
        txt.setPosition(snap({x, y}));
        target.draw(txt);
    }
};

