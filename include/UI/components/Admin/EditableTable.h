#pragma once

#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <functional>
#include <memory>

using namespace sf;
using namespace std;

class EditableTable {
public:
    struct Cell {
        string value;
        bool isEditable;
        
        Cell(const string& val = "", bool editable = true);
    };
    
private:
    Font& font;  // Dung tham chieu de tranh load nhieu lan
    vector<string> headers;
    vector<int> columnWidths;
    vector<vector<Cell>> rows;
    
    float x, y, width, height;
    float rowHeight = 48.f;
    float headerHeight = 52.f;
    float scrollOffset = 0.f;
    float maxScroll = 0.f;
    float padding = 24.f;
    
    int selectedRow = -1;
    int selectedCol = -1;
    bool isEditing = false;
    string editBuffer;
    Clock cursorBlinkClock;
    int hoveredRow = -1;
    
    // Bang mau chu de CineXine
    Color bgColor = Color(17, 28, 42);           // #111C2A - dam hon sidebar
    Color headerColor = Color(20, 118, 172);     // #1476AC - xanh CineXine
    Color rowColor = Color(24, 44, 64);          // #182C40 - nen dong
    Color borderColor = Color(34, 59, 84);       // #223B54 - vien nhe
    Color hoverColor = Color(30, 58, 92);        // #1E3A5C - mau hover
    Color selectedColor = Color(40, 80, 120);    // Xanh nhat hon khi chon
    Color editingColor = Color(50, 90, 130);     // Sang hon khi edit
    Color textColor = Color(240, 240, 240);      // #F0F0F0 - chu sang
    Color headerTextColor = Color(255, 255, 255); // Trang cho header
    
    RectangleShape background;
    RectangleShape scrollBar;
    RectangleShape scrollThumb;
    bool isDraggingScrollBar = false;
    float scrollBarWidth = 10.f;
    
    // Ham goi lai
    function<bool(int row, int col, const string& newValue)> onCellChange;
    function<void(int row)> onRowSelect;
    
    void calculateColumnWidths();
    void updateScrollBar();
    FloatRect getCellBounds(int row, int col) const;
    pair<int, int> getCellAtPosition(Vector2f mousePos) const;
    
public:
    EditableTable(Font& font, float x, float y, float width, float height);
    
    void setHeaders(const vector<string>& headers);
    void addRow(const vector<string>& rowData, bool editable = true);
    void clearRows();
    void setColumnWidths(const vector<int>& widths);
    
    void handleEvent(const Event& event, const RenderWindow& window);
    void update(Vector2f mousePos, bool mousePressed);
    void render(RenderWindow& window);
    
    // Ham goi lai
    void setOnCellChange(function<bool(int, int, const string&)> callback);
    void setOnRowSelect(function<void(int)> callback);
    
    int getSelectedRow() const;
    vector<string> getRowData(int row) const;
    int getRowCount() const;
    
    // Cap nhat hoac xoa dong
    void updateRow(int row, const vector<string>& newData);
    void deleteRow(int row);
};