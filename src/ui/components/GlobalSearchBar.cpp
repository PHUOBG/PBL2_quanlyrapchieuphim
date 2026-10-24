#include "UI/components/GlobalSearchBar.h"
#include <iostream>

GlobalSearchBar::GlobalSearchBar(Font& f, Vector2f position, Vector2f size) 
        : inputText(f),
            placeholderText(f),
            isActive(false),
            showSuggestions(false),
            selectedSuggestion(-1),
            hoveredSuggestion(-1),
            font(f),
            searchManager(nullptr),
            showCursor(false),
            originalPosition(position),
            originalSize(size) {
    
    // Input box styling - TRANSPARENT (chỉ thấy ảnh search_bar.png)
    inputBox.setSize(size);
    inputBox.setPosition(position);
    inputBox.setFillColor(Color::Transparent); // Trong suốt hoàn toàn
    inputBox.setOutlineThickness(0.f); // Không có viền
    
    // Input text - căn chỉnh vào giữa search bar với font size lớn hơn
    inputText.setCharacterSize(18);
    inputText.setFillColor(Color::Black); // Chữ màu đen để thấy trên nền trắng
    inputText.setPosition(Vector2f(position.x + 18.f, position.y + 3.f)); // Dịch vào trong, căn giữa dọc
    
    // Placeholder text
    placeholderText.setCharacterSize(18);
    placeholderText.setFillColor(Color(120, 120, 120)); // Màu xám đậm hơn
    placeholderText.setString("Search...");
    placeholderText.setPosition(Vector2f(position.x + 18.f, position.y + 3.f));
    
    // Suggestion box - HIỂN THỊ PHÍA DƯỚI
    suggestionBox.setFillColor(Color(255, 255, 255, 250)); // Nền trắng
    suggestionBox.setOutlineColor(Color(200, 200, 200));
    suggestionBox.setOutlineThickness(1.f);
    // Position sẽ được set động trong update() khi có suggestions
}

void GlobalSearchBar::setSearchManager(MovieSearchManager* manager) {
    searchManager = manager;
}

void GlobalSearchBar::handleEvent(const Event& event) {
    if (!isActive) return;
    
    if (event.is<Event::TextEntered>()) {
        const auto& textEvent = *event.getIf<Event::TextEntered>();
        uint32_t unicode = textEvent.unicode;
        
        // Handle backspace
        if (unicode == 8) {
            if (!inputString.empty()) {
                // Use SFML String to handle UTF-8 properly
                String temp = String::fromUtf8(inputString.begin(), inputString.end());
                if (!temp.isEmpty()) {
                    temp.erase(temp.getSize() - 1);
                    auto utf8 = temp.toUtf8();
                    inputString = std::string(reinterpret_cast<const char*>(utf8.c_str()));
                }
            }
        }
        // Handle printable characters (including Vietnamese)
        else if (unicode >= 32 && unicode != 127) {
            // Convert unicode to UTF-8 string
            String temp;
            temp += static_cast<char32_t>(unicode);
            auto utf8 = temp.toUtf8();
            inputString += std::string(reinterpret_cast<const char*>(utf8.c_str()));
        }
        
        // Update text display
        inputText.setString(String::fromUtf8(inputString.begin(), inputString.end()));
        
        // Update suggestions
        if (searchManager && !inputString.empty()) {
            suggestions = searchManager->searchMovies(inputString, 5);
            showSuggestions = !suggestions.empty();
            selectedSuggestion = -1;
            hoveredSuggestion = -1;
        } else {
            showSuggestions = false;
            suggestions.clear();
        }
    }
    
    if (event.is<Event::KeyPressed>()) {
        const auto& keyEvent = *event.getIf<Event::KeyPressed>();
        
        // Handle arrow keys for suggestion navigation
        if (keyEvent.code == Keyboard::Key::Down && showSuggestions) {
            selectedSuggestion = (selectedSuggestion + 1) % suggestions.size();
            hoveredSuggestion = selectedSuggestion;
        } else if (keyEvent.code == Keyboard::Key::Up && showSuggestions) {
            selectedSuggestion--;
            if (selectedSuggestion < 0) selectedSuggestion = suggestions.size() - 1;
            hoveredSuggestion = selectedSuggestion;
        } else if (keyEvent.code == Keyboard::Key::Enter && selectedSuggestion >= 0) {
            // Selection made via Enter key (handled in hasSelectedMovie)
        } else if (keyEvent.code == Keyboard::Key::Escape) {
            isActive = false;
            showSuggestions = false;
        }
    }
}

void GlobalSearchBar::update(Vector2f mousePos, bool mousePressed) {
    // Update cursor blink
    if (blinkClock.getElapsedTime().asSeconds() > 0.5f) {
        showCursor = !showCursor;
        blinkClock.restart();
    }
    
    // Update hover state for suggestions
    hoveredSuggestion = -1;
    if (showSuggestions && !suggestions.empty()) {
        FloatRect suggestionBounds = suggestionBox.getGlobalBounds();
        if (suggestionBounds.contains(mousePos)) {
            float relativeY = mousePos.y - suggestionBounds.position.y;
            int hoveredIndex = static_cast<int>(relativeY / 40.f);
            if (hoveredIndex >= 0 && hoveredIndex < (int)suggestions.size()) {
                hoveredSuggestion = hoveredIndex;
            }
        }
    }
    
    // Check if input box is clicked
    if (mousePressed) {
        if (inputBox.getGlobalBounds().contains(mousePos)) {
            isActive = true;
            inputBox.setOutlineColor(Color(20, 118, 172)); // Accent color when active
        } else if (showSuggestions) {
            // Check if a suggestion was clicked
            FloatRect suggestionBounds = suggestionBox.getGlobalBounds();
            if (suggestionBounds.contains(mousePos)) {
                float relativeY = mousePos.y - suggestionBounds.position.y;
                int clickedIndex = static_cast<int>(relativeY / 40.f);
                if (clickedIndex >= 0 && clickedIndex < (int)suggestions.size()) {
                    selectedSuggestion = clickedIndex;
                }
            } else {
                isActive = false;
                showSuggestions = false;
                inputBox.setOutlineColor(Color(100, 100, 100));
            }
        } else {
            isActive = false;
            inputBox.setOutlineColor(Color(100, 100, 100));
        }
    }
    
    // Update suggestion box size and position - HIỂN THỊ PHÍA DƯỚI
    if (showSuggestions && !suggestions.empty()) {
        float boxHeight = suggestions.size() * 40.f;
        suggestionBox.setSize(Vector2f(inputBox.getSize().x, boxHeight));
        
        // Đặt suggestion box PHÍA DƯỚI search bar
        Vector2f inputPos = inputBox.getPosition();
        Vector2f inputSize = inputBox.getSize();
        suggestionBox.setPosition(Vector2f(inputPos.x, inputPos.y + inputSize.y + 2.f));
    }
}

void GlobalSearchBar::draw(RenderWindow& window) {
    // Draw input box
    window.draw(inputBox);
    
    // Draw input text or placeholder
    if (inputString.empty() && !isActive) {
        window.draw(placeholderText);
    } else {
        window.draw(inputText);
        
        // Draw cursor if active
        if (isActive && showCursor) {
            FloatRect textBounds = inputText.getGlobalBounds();
            RectangleShape cursor(Vector2f(2.f, 18.f));
            cursor.setPosition(Vector2f(textBounds.position.x + textBounds.size.x + 3.f, inputText.getPosition().y + 2.f));
            cursor.setFillColor(Color::Black); // Cursor màu đen
            window.draw(cursor);
        }
    }
    
    // Draw suggestions
    if (showSuggestions && !suggestions.empty()) {
        window.draw(suggestionBox);
        
        suggestionTexts.clear();
        float maxTextWidth = suggestionBox.getSize().x - 30.f; // Trừ padding 2 bên
        
        for (size_t i = 0; i < suggestions.size(); i++) {
            Text suggestionText(font);
            suggestionText.setCharacterSize(16);
            
            // Convert string to SFML String with UTF-8 support
            String originalText = String::fromUtf8(suggestions[i].title.begin(), suggestions[i].title.end());
            suggestionText.setString(originalText);
            
            // Kiểm tra độ dài text và cắt nếu quá dài
            FloatRect textBounds = suggestionText.getLocalBounds();
            if (textBounds.size.x > maxTextWidth) {
                // Cắt text và thêm "..."
                String truncatedText = originalText;
                while (textBounds.size.x > maxTextWidth && truncatedText.getSize() > 0) {
                    truncatedText.erase(truncatedText.getSize() - 1);
                    suggestionText.setString(truncatedText + "...");
                    textBounds = suggestionText.getLocalBounds();
                }
            }
            
            Vector2f pos = suggestionBox.getPosition();
            suggestionText.setPosition(Vector2f(pos.x + 15.f, pos.y + 10.f + i * 40.f));
            
            // Highlight hovered or selected suggestion with different colors
            if ((int)i == hoveredSuggestion || (int)i == selectedSuggestion) {
                RectangleShape highlight(Vector2f(suggestionBox.getSize().x, 40.f));
                highlight.setPosition(Vector2f(pos.x, pos.y + i * 40.f));
                
                // Use different color for hover vs keyboard selection
                if ((int)i == hoveredSuggestion) {
                    highlight.setFillColor(Color(240, 240, 240)); // Lighter highlight for hover
                } else {
                    highlight.setFillColor(Color(230, 230, 230)); // Slightly darker for keyboard selection
                }
                
                window.draw(highlight);
                suggestionText.setFillColor(Color::Black); // Chữ đen khi highlight
            } else {
                suggestionText.setFillColor(Color(50, 50, 50)); // Chữ đen nhạt
            }
            
            window.draw(suggestionText);
        }
    }
}

bool GlobalSearchBar::hasSelectedMovie(int& movieIndex) {
    if (selectedSuggestion >= 0 && selectedSuggestion < (int)suggestions.size()) {
        movieIndex = suggestions[selectedSuggestion].movieIndex;
        // Clear after selection
        inputString.clear();
        inputText.setString("");
        showSuggestions = false;
        selectedSuggestion = -1;
        hoveredSuggestion = -1;
        isActive = false;
        return true;
    }
    return false;
}

void GlobalSearchBar::clear() {
    inputString.clear();
    inputText.setString("");
    showSuggestions = false;
    selectedSuggestion = -1;
    hoveredSuggestion = -1;
    isActive = false;
    suggestions.clear();
}

bool GlobalSearchBar::isInputActive() const {
    return isActive;
}

string GlobalSearchBar::getInput() const {
    return inputString;
}

void GlobalSearchBar::setPosition(Vector2f position) {
    originalPosition = position;
    inputBox.setPosition(position);
    inputText.setPosition(Vector2f(position.x + 18.f, position.y + 3.f));
    placeholderText.setPosition(Vector2f(position.x + 18.f, position.y + 3.f));
}
