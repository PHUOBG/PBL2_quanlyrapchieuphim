#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include "services/MovieSearchManager.h"

using namespace sf;
using namespace std;

class GlobalSearchBar {
private:
    RectangleShape inputBox;
    RectangleShape suggestionBox;
    Text inputText;
    Text placeholderText;
    vector<Text> suggestionTexts;
    
    string inputString;
    bool isActive;
    bool showSuggestions;
    int selectedSuggestion;
    int hoveredSuggestion;  // Theo doi item dang duoc hover
    vector<SearchResult> suggestions;
    
    Font& font;
    MovieSearchManager* searchManager;
    
    Clock blinkClock;
    bool showCursor;
    
    // Luu lai vi tri & kich thuoc goc de dat lai de dang
    Vector2f originalPosition;
    Vector2f originalSize;
    
public:
    GlobalSearchBar(Font& f, Vector2f position, Vector2f size);
    void setSearchManager(MovieSearchManager* manager);
    void handleEvent(const Event& event);
    void update(Vector2f mousePos, bool mousePressed);
    void draw(RenderWindow& window);
    bool hasSelectedMovie(int& movieIndex);
    void clear();
    bool isInputActive() const;
    string getInput() const;
    void setPosition(Vector2f position);
};
