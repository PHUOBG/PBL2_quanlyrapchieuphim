#include "UI/components/Admin/EditableTable.h"
#include <algorithm>
#include <cmath>

EditableTable::Cell::Cell(const string& val, bool editable)
    : value(val), isEditable(editable) {}

EditableTable::EditableTable(Font& font, float x, float y, float width, float height)
    : font(font), x(x), y(y), width(width), height(height) {
    
    // Setup background với CineXine theme
    background.setSize(Vector2f(width, height));
    background.setPosition(Vector2f(x, y));
    background.setFillColor(bgColor);
    background.setOutlineColor(borderColor);
    background.setOutlineThickness(1.f);
    
    // Setup scroll bar background
    scrollBar.setSize(Vector2f(scrollBarWidth, height - padding * 2));
    scrollBar.setPosition(Vector2f(x + width - scrollBarWidth - padding/2, y + padding));
    scrollBar.setFillColor(Color(30, 40, 55));
    
    // Setup scroll thumb
    scrollThumb.setSize(Vector2f(scrollBarWidth - 2, 50.f));
    scrollThumb.setPosition(Vector2f(x + width - scrollBarWidth - padding/2 + 1, y + padding + headerHeight));
    scrollThumb.setFillColor(Color(60, 80, 100));
}

void EditableTable::setHeaders(const vector<string>& headers) {
    this->headers = headers;
    calculateColumnWidths();
}

void EditableTable::setColumnWidths(const vector<int>& widths) {
    this->columnWidths = widths;
}

void EditableTable::setOnCellChange(function<bool(int, int, const string&)> callback) {
    onCellChange = std::move(callback);
}

void EditableTable::setOnRowSelect(function<void(int)> callback) {
    onRowSelect = std::move(callback);
}

int EditableTable::getSelectedRow() const {
    return selectedRow;
}

int EditableTable::getRowCount() const {
    return static_cast<int>(rows.size());
}

void EditableTable::calculateColumnWidths() {
    if (headers.empty()) return;
    
    // If no custom widths, distribute equally
    if (columnWidths.empty()) {
        float availableWidth = width - scrollBarWidth - padding * 3;
        int colWidth = static_cast<int>(availableWidth / static_cast<float>(headers.size()));
        columnWidths.clear();
        for (size_t i = 0; i < headers.size(); i++) {
            columnWidths.push_back(colWidth);
        }
    }
}

void EditableTable::addRow(const vector<string>& rowData, bool editable) {
    vector<Cell> newRow;
    for (const auto& data : rowData) {
        newRow.push_back(Cell(data, editable));
    }
    rows.push_back(newRow);
    updateScrollBar();
}

void EditableTable::updateRow(int row, const vector<string>& newData) {
    if (row < 0 || row >= static_cast<int>(rows.size())) return;
    
    for (size_t i = 0; i < newData.size() && i < rows[row].size(); i++) {
        rows[row][i].value = newData[i];
    }
}

void EditableTable::deleteRow(int row) {
    if (row < 0 || row >= static_cast<int>(rows.size())) return;
    
    rows.erase(rows.begin() + row);
    if (selectedRow == row) {
        selectedRow = -1;
        selectedCol = -1;
        isEditing = false;
    } else if (selectedRow > row) {
        selectedRow--;
    }
    updateScrollBar();
}

void EditableTable::clearRows() {
    rows.clear();
    selectedRow = -1;
    selectedCol = -1;
    isEditing = false;
    scrollOffset = 0.f;
    hoveredRow = -1;
    updateScrollBar();
}

void EditableTable::updateScrollBar() {
    float contentHeight = rows.size() * rowHeight;
    float viewHeight = height - headerHeight - padding * 2;
    
    if (contentHeight > viewHeight) {
        maxScroll = contentHeight - viewHeight;
        
        // Update thumb size based on content ratio
        float thumbRatio = viewHeight / contentHeight;
        float thumbHeight = max(30.f, viewHeight * thumbRatio);
        scrollThumb.setSize(Vector2f(scrollBarWidth - 2, thumbHeight));
        
        // Update thumb position based on scroll offset
        float scrollRatio = scrollOffset / maxScroll;
        float thumbY = y + padding + headerHeight + 1 + scrollRatio * (viewHeight - thumbHeight - 2);
        scrollThumb.setPosition(Vector2f(x + width - scrollBarWidth - padding/2 + 1, thumbY));
    } else {
        maxScroll = 0.f;
        scrollOffset = 0.f;
    }
}

FloatRect EditableTable::getCellBounds(int row, int col) const {
    if (col < 0 || col >= static_cast<int>(columnWidths.size())) return FloatRect();
    
    float cellX = x + padding;
    for (int i = 0; i < col; i++) {
        cellX += columnWidths[i];
    }
    
    float cellY = y + padding + headerHeight + row * rowHeight - scrollOffset;
    
    return FloatRect(Vector2f(cellX, cellY), Vector2f(columnWidths[col], rowHeight));
}

pair<int, int> EditableTable::getCellAtPosition(Vector2f mousePos) const {
    // Check if in header
    if (mousePos.y < y + padding + headerHeight) return {-1, -1};
    
    // Check if beyond table
    float tableRight = x + width - scrollBarWidth - padding;
    float tableBottom = y + height - padding;
    
    if (mousePos.x < x + padding || mousePos.x > tableRight ||
        mousePos.y < y + padding + headerHeight || mousePos.y > tableBottom) {
        return {-1, -1};
    }
    
    // Calculate row
    int row = (mousePos.y - (y + padding + headerHeight) + scrollOffset) / rowHeight;
    if (row < 0 || row >= static_cast<int>(rows.size())) return {-1, -1};
    
    // Calculate column
    float cellX = x + padding;
    const int colCount = static_cast<int>(columnWidths.size());
    for (int col = 0; col < colCount; col++) {
        if (mousePos.x >= cellX && mousePos.x < cellX + columnWidths[col]) {
            return {row, col};
        }
        cellX += columnWidths[col];
    }
    
    return {-1, -1};
}

void EditableTable::handleEvent(const Event& event, const RenderWindow& window) {
    (void)window;
    // Handle keyboard input for editing
    if (isEditing) {
        if (const auto* textEvent = event.getIf<Event::TextEntered>()) {
            if (textEvent->unicode == 8) { // Backspace
                if (!editBuffer.empty()) {
                    // Handle UTF-8 properly - remove last character
                    size_t len = editBuffer.length();
                    while (len > 0 && (editBuffer[len-1] & 0xC0) == 0x80) {
                        len--;
                    }
                    if (len > 0) len--;
                    editBuffer = editBuffer.substr(0, len);
                }
            } else if (textEvent->unicode == 13) { // Enter
                // Save changes
                if (onCellChange && selectedRow >= 0 && selectedCol >= 0) {
                    if (onCellChange(selectedRow, selectedCol, editBuffer)) {
                        rows[selectedRow][selectedCol].value = editBuffer;
                    }
                } else {
                    rows[selectedRow][selectedCol].value = editBuffer;
                }
                isEditing = false;
            } else if (textEvent->unicode == 27) { // Escape
                isEditing = false;
            } else if (textEvent->unicode >= 32) {
                // Add character (including Vietnamese)
                char32_t unicode = textEvent->unicode;
                if (unicode < 0x80) {
                    editBuffer += static_cast<char>(unicode);
                } else if (unicode < 0x800) {
                    editBuffer += static_cast<char>(0xC0 | (unicode >> 6));
                    editBuffer += static_cast<char>(0x80 | (unicode & 0x3F));
                } else if (unicode < 0x10000) {
                    editBuffer += static_cast<char>(0xE0 | (unicode >> 12));
                    editBuffer += static_cast<char>(0x80 | ((unicode >> 6) & 0x3F));
                    editBuffer += static_cast<char>(0x80 | (unicode & 0x3F));
                } else {
                    editBuffer += static_cast<char>(0xF0 | (unicode >> 18));
                    editBuffer += static_cast<char>(0x80 | ((unicode >> 12) & 0x3F));
                    editBuffer += static_cast<char>(0x80 | ((unicode >> 6) & 0x3F));
                    editBuffer += static_cast<char>(0x80 | (unicode & 0x3F));
                }
            }
        }
    }
    
    // Handle mouse wheel for scrolling
    if (const auto* wheelEvent = event.getIf<Event::MouseWheelScrolled>()) {
        if (wheelEvent->wheel == Mouse::Wheel::Vertical) {
            scrollOffset -= wheelEvent->delta * 40.f;
            scrollOffset = max(0.f, min(scrollOffset, maxScroll));
            updateScrollBar();
        }
    }
    
    // Handle mouse button press
    if (const auto* mouseButtonEvent = event.getIf<Event::MouseButtonPressed>()) {
        if (mouseButtonEvent->button == Mouse::Button::Left) {
            Vector2f mousePos(mouseButtonEvent->position.x, mouseButtonEvent->position.y);
            
            // Check scroll bar drag
            if (scrollThumb.getGlobalBounds().contains(mousePos)) {
                isDraggingScrollBar = true;
                return;
            }
            
            // Check cell click
            auto [row, col] = getCellAtPosition(mousePos);
            if (row >= 0 && col >= 0) {
                // Select cell
                selectedRow = row;
                selectedCol = col;
                
                if (onRowSelect) {
                    onRowSelect(row);
                }
                
                // Start editing if editable
                if (rows[row][col].isEditable) {
                    isEditing = true;
                    editBuffer = rows[row][col].value;
                    cursorBlinkClock.restart();
                }
            } else {
                // Click outside - deselect
                if (!isDraggingScrollBar) {
                    selectedRow = -1;
                    selectedCol = -1;
                    isEditing = false;
                }
            }
        }
    }
    
    // Handle mouse button release
    if (const auto* mouseButtonEvent = event.getIf<Event::MouseButtonReleased>()) {
        if (mouseButtonEvent->button == Mouse::Button::Left) {
            isDraggingScrollBar = false;
        }
    }
}

void EditableTable::update(Vector2f mousePos, bool mousePressed) {
    // Handle scroll bar dragging
    if (isDraggingScrollBar && mousePressed) {
        float viewHeight = height - headerHeight - padding * 2;
        float thumbHeight = scrollThumb.getSize().y;
        
        // Calculate new scroll position
        float relativeY = mousePos.y - (y + padding + headerHeight + thumbHeight / 2);
        float scrollRatio = relativeY / (viewHeight - thumbHeight);
        scrollRatio = max(0.f, min(1.f, scrollRatio));
        
        scrollOffset = scrollRatio * maxScroll;
        updateScrollBar();
    }
    
    // Update hovered row
    auto [row, col] = getCellAtPosition(mousePos);
    hoveredRow = row;
}

void EditableTable::render(RenderWindow& window) {
    // Draw background
    window.draw(background);
    
    // Draw header
    float tableWidth = width - scrollBarWidth - padding * 2;
    RectangleShape headerBg(Vector2f(tableWidth, headerHeight));
    headerBg.setPosition(Vector2f(x + padding, y + padding));
    headerBg.setFillColor(headerColor);
    window.draw(headerBg);
    
    // Draw header text
    float cellX = x + padding;
    for (size_t i = 0; i < headers.size() && i < columnWidths.size(); i++) {
        Text headerText(font);
        headerText.setString(String::fromUtf8(headers[i].begin(), headers[i].end()));
        headerText.setCharacterSize(15);
        headerText.setFillColor(headerTextColor);
        headerText.setStyle(Text::Bold);
        
        FloatRect textBounds = headerText.getLocalBounds();
        headerText.setPosition(Vector2f(
            cellX + (columnWidths[i] - textBounds.size.x) / 2,
            y + padding + (headerHeight - textBounds.size.y) / 2 - 6
        ));
        
        window.draw(headerText);
        
        // Draw vertical separator in header
        if (i < headers.size() - 1) {
            RectangleShape separator(Vector2f(1, headerHeight));
            separator.setPosition(Vector2f(cellX + columnWidths[i], y + padding));
            separator.setFillColor(Color(30, 90, 140));
            window.draw(separator);
        }
        
        cellX += columnWidths[i];
    }
    
    // Draw rows
    float viewTop = y + padding + headerHeight;
    float viewBottom = y + height - padding;
    
    const int rowCount = static_cast<int>(rows.size());
    for (int row = 0; row < rowCount; row++) {
        float rowY = y + padding + headerHeight + row * rowHeight - scrollOffset;
        
        // Skip if row is outside visible area
        if (rowY + rowHeight < viewTop || rowY > viewBottom) continue;
        
        // Determine row background color
        Color currentRowColor = rowColor;
        if (isEditing && row == selectedRow) {
            currentRowColor = editingColor;
        } else if (row == selectedRow) {
            currentRowColor = selectedColor;
        } else if (row == hoveredRow) {
            currentRowColor = hoverColor;
        }
        
        // Draw row background
        RectangleShape rowBg(Vector2f(tableWidth, rowHeight));
        rowBg.setPosition(Vector2f(x + padding, rowY));
        rowBg.setFillColor(currentRowColor);
        window.draw(rowBg);
        
        // Draw horizontal separator
        RectangleShape hSeparator(Vector2f(tableWidth, 1));
        hSeparator.setPosition(Vector2f(x + padding, rowY + rowHeight));
        hSeparator.setFillColor(borderColor);
        window.draw(hSeparator);
        
        // Draw cell text
        cellX = x + padding;
        const int rowSize = static_cast<int>(rows[row].size());
        const int widthCount = static_cast<int>(columnWidths.size());
        for (int col = 0; col < rowSize && col < widthCount; col++) {
            string displayText = rows[row][col].value;
            
            // If editing this cell, show edit buffer with cursor
            if (isEditing && row == selectedRow && col == selectedCol) {
                displayText = editBuffer;
                
                // Add blinking cursor
                float elapsed = cursorBlinkClock.getElapsedTime().asSeconds();
                if (fmod(elapsed, 1.0f) < 0.5f) {
                    displayText += "|";
                }
            }
            
            Text cellText(font);
            cellText.setString(String::fromUtf8(displayText.begin(), displayText.end()));
            cellText.setCharacterSize(14);
            cellText.setFillColor(textColor);
            
            FloatRect textBounds = cellText.getLocalBounds();
            cellText.setPosition(Vector2f(
                cellX + 8,
                rowY + (rowHeight - textBounds.size.y) / 2 - 4
            ));
            
            window.draw(cellText);
            
            // Draw vertical separator
            if (col < rowSize - 1) {
                RectangleShape vSeparator(Vector2f(1, rowHeight));
                vSeparator.setPosition(Vector2f(cellX + columnWidths[col], rowY));
                vSeparator.setFillColor(borderColor);
                window.draw(vSeparator);
            }
            
            cellX += columnWidths[col];
        }
    }
    
    // Draw scroll bar
    if (maxScroll > 0) {
        window.draw(scrollBar);
        window.draw(scrollThumb);
    }
}

vector<string> EditableTable::getRowData(int row) const {
    if (row < 0 || row >= static_cast<int>(rows.size())) return {};
    
    vector<string> data;
    for (const auto& cell : rows[row]) {
        data.push_back(cell.value);
    }
    return data;
}
