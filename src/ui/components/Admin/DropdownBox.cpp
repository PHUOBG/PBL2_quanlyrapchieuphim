#include "UI/components/Admin/DropdownBox.h"
#include <algorithm>
#include <iostream>

DropdownBox::DropdownBox(Font& font, const string& label, float x, float y, float width, float height)
    : font(font),
      selectedIndex(0),
      isOpen(false),
      enabled(true),
      x(x), y(y), width(width), height(height),
      label(label),
      bgColor(255, 255, 255), hoverColor(230, 230, 230),
      selectedColor(100, 149, 237), borderColor(200, 200, 200),
      textColor(50, 50, 50),
      maxVisibleOptions(0), firstVisibleIndex(0), optionHeight(40.f) {
    
    // Label text
    labelText = make_unique<Text>(font, String::fromUtf8(label.begin(), label.end()), 14);
    labelText->setFillColor(Color(80, 80, 80));
    labelText->setPosition(Vector2f(x, y - 22));
    
    // Main background
    background.setSize(Vector2f(width, height));
    background.setPosition(Vector2f(x, y));
    background.setFillColor(bgColor);
    background.setOutlineThickness(1.f);
    background.setOutlineColor(borderColor);
    
    // Selected text (shown in main box)
    selectedText = make_unique<Text>(font, "", 16);
    selectedText->setFillColor(textColor);
    selectedText->setPosition(Vector2f(x + 10, y + (height - 20) / 2));
    
    // Arrow (small triangle)
    arrow.setSize(Vector2f(8, 8));
    arrow.setFillColor(Color(100, 100, 100));
    arrow.setPosition(Vector2f(x + width - 20, y + height / 2 - 4));
}

void DropdownBox::setEnabled(bool e) {
    enabled = e;
    if (!enabled) {
        isOpen = false;
        background.setFillColor(Color(240, 240, 240)); // Gray out
        if (selectedText) selectedText->setFillColor(Color(150, 150, 150));
    } else {
        background.setFillColor(bgColor);
        if (selectedText) selectedText->setFillColor(textColor);
    }
}

bool DropdownBox::isEnabled() const {
    return enabled;
}

bool DropdownBox::isDropdownOpen() const {
    return isOpen;
}

Vector2f DropdownBox::getPosition() const {
    return Vector2f(x, y);
}

void DropdownBox::setOptions(const vector<string>& opts) {
    options = opts;
    optionTexts.clear();
    optionBackgrounds.clear();
    
    // Create dropdown panel
    dropdownPanel.setPosition(Vector2f(x, y + height));
    dropdownPanel.setFillColor(Color::White);
    dropdownPanel.setOutlineThickness(1.f);
    dropdownPanel.setOutlineColor(borderColor);
    
    // Create option items
    for (size_t i = 0; i < opts.size(); i++) {
        // Background
        RectangleShape optBg;
        optBg.setSize(Vector2f(width, optionHeight));
        optBg.setFillColor(bgColor);
        optionBackgrounds.push_back(optBg);
        
        // Text
        auto text = make_unique<Text>(font, String::fromUtf8(opts[i].begin(), opts[i].end()), 16);
        text->setFillColor(textColor);
        optionTexts.push_back(move(text));
    }
    
    // Set initial selected text
    if (!options.empty()) {
        selectedText->setString(String::fromUtf8(options[0].begin(), options[0].end()));
    }

    firstVisibleIndex = 0;
    updateDropdownPanel();
}

void DropdownBox::setSelectedIndex(int index) {
    if (index >= 0 && index < (int)options.size()) {
        selectedIndex = index;
        selectedText->setString(String::fromUtf8(options[index].begin(), options[index].end()));
        if (maxVisibleOptions > 0) {
            int visible = getVisibleCount();
            if (selectedIndex < firstVisibleIndex) {
                firstVisibleIndex = selectedIndex;
            } else if (selectedIndex >= firstVisibleIndex + visible) {
                firstVisibleIndex = selectedIndex - visible + 1;
            }
            clampFirstVisible();
        }
        updateDropdownPanel();
    }
}

void DropdownBox::setSelectedValue(const string& value) {
    for (size_t i = 0; i < options.size(); i++) {
        if (options[i] == value) {
            setSelectedIndex(i);
            break;
        }
    }
}

string DropdownBox::getSelectedValue() const {
    if (selectedIndex >= 0 && selectedIndex < (int)options.size()) {
        return options[selectedIndex];
    }
    return "";
}

int DropdownBox::getSelectedIndex() const {
    return selectedIndex;
}

void DropdownBox::setPosition(Vector2f pos) {
    x = pos.x;
    y = pos.y;
    
    if (labelText) labelText->setPosition(Vector2f(x, y - 22));
    background.setPosition(Vector2f(x, y));
    if (selectedText) selectedText->setPosition(Vector2f(x + 10, y + (height - 20) / 2));
    arrow.setPosition(Vector2f(x + width - 20, y + height / 2 - 4));
    
    updateDropdownPanel();
}

bool DropdownBox::isMouseOver(const Vector2f& mousePos) const {
    FloatRect bounds({x, y}, {width, height});
    return bounds.contains(mousePos);
}

void DropdownBox::setMaxVisibleOptions(int count) {
    maxVisibleOptions = count;
    clampFirstVisible();
    updateDropdownPanel();
}

void DropdownBox::clampFirstVisible() {
    int visible = getVisibleCount();
    if (maxVisibleOptions <= 0 || options.empty()) {
        firstVisibleIndex = 0;
        return;
    }
    int maxOffset = std::max(0, static_cast<int>(options.size()) - visible);
    if (firstVisibleIndex > maxOffset) firstVisibleIndex = maxOffset;
    if (firstVisibleIndex < 0) firstVisibleIndex = 0;
}

int DropdownBox::getVisibleCount() const {
    if (maxVisibleOptions <= 0 || maxVisibleOptions > static_cast<int>(options.size())) {
        return static_cast<int>(options.size());
    }
    return maxVisibleOptions;
}

void DropdownBox::updateDropdownPanel() {
    int visible = getVisibleCount();
    dropdownPanel.setSize(Vector2f(width, visible * optionHeight));
    dropdownPanel.setPosition(Vector2f(x, y + height));
}

void DropdownBox::handleEvent(const Event& event, const Vector2f& mousePos) {
    if (!enabled) return;

    if (const auto* wheelEvent = event.getIf<Event::MouseWheelScrolled>()) {
        FloatRect baseBounds(Vector2f(x, y), Vector2f(width, height));
        FloatRect dropBounds(Vector2f(x, y + height), dropdownPanel.getSize());
        bool withinBase = baseBounds.contains(mousePos);
        bool withinDrop = isOpen && dropBounds.contains(mousePos);

        if (withinBase || withinDrop) {
            if (maxVisibleOptions > 0 && static_cast<int>(options.size()) > maxVisibleOptions) {
                int direction = (wheelEvent->delta > 0) ? -1 : 1;
                firstVisibleIndex += direction;
                clampFirstVisible();
                updateDropdownPanel();
            } else if (!options.empty()) {
                int delta = (wheelEvent->delta > 0) ? -1 : 1;
                int newIndex = selectedIndex + delta;
                if (newIndex < 0) newIndex = 0;
                if (newIndex >= static_cast<int>(options.size())) newIndex = static_cast<int>(options.size()) - 1;
                setSelectedIndex(newIndex);
            }
        }
        return;
    }

    if (auto* mouseButtonPressed = event.getIf<Event::MouseButtonPressed>()) {
        if (mouseButtonPressed->button == Mouse::Button::Left) {
            // Click on main box - toggle dropdown
            if (isMouseOver(mousePos)) {
                isOpen = !isOpen;
                return;
            }
            
            // Click on option in dropdown
            if (isOpen) {
                int visible = getVisibleCount();
                for (int i = 0; i < visible; ++i) {
                    FloatRect optBounds(Vector2f(x, y + height + i * optionHeight), Vector2f(width, optionHeight));
                    if (optBounds.contains(mousePos)) {
                        int optionIndex = firstVisibleIndex + i;
                        setSelectedIndex(optionIndex);
                        isOpen = false;
                        return;
                    }
                }
                // Click outside - close
                isOpen = false;
            }
        }
    }
}

void DropdownBox::update() {
    // Update hover effects for dropdown options
    if (isOpen) {
    }
}

void DropdownBox::draw(RenderWindow& window) {
    // Draw label
    if (labelText) {
        window.draw(*labelText);
    }
    
    // Draw main box
    window.draw(background);
    
    // Draw selected text
    if (selectedText) {
        window.draw(*selectedText);
    }
    
    // Draw arrow
    window.draw(arrow);
    
    // Draw dropdown panel if open
    if (isOpen) {
        updateDropdownPanel();
        window.draw(dropdownPanel);
        
        int visible = getVisibleCount();
        Vector2i mousePixel = Mouse::getPosition(window);
        Vector2f mouseWorld = window.mapPixelToCoords(mousePixel);

        for (int i = 0; i < visible; ++i) {
            int optionIndex = firstVisibleIndex + i;
            if (optionIndex < 0 || optionIndex >= static_cast<int>(optionBackgrounds.size())) continue;

            RectangleShape& optBg = optionBackgrounds[optionIndex];
            optBg.setPosition(Vector2f(x, y + height + i * optionHeight));
            optBg.setSize(Vector2f(width, optionHeight));

            if (optBg.getGlobalBounds().contains(mouseWorld)) {
                optBg.setFillColor(hoverColor);
            } else if (optionIndex == selectedIndex) {
                optBg.setFillColor(Color(220, 235, 255));
            } else {
                optBg.setFillColor(bgColor);
            }

            window.draw(optBg);

            if (optionTexts[optionIndex]) {
                optionTexts[optionIndex]->setPosition(Vector2f(x + 10.f, y + height + i * optionHeight + optionHeight / 2.f - 10.f));
                window.draw(*optionTexts[optionIndex]);
            }
        }
    }
}
