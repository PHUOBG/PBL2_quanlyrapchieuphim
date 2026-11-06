#pragma once

#include <SFML/Graphics.hpp>
#include <memory>
#include <vector>
#include <string>
#include <functional>
#include <set>
#include "UI/components/Admin/UIInputBox.h"
#include "UI/components/Admin/UICheckboxItem.h"
#include "UI/components/Admin/UIScrollableList.h"
#include "UI/components/Button.h"

struct UserInfo {
    std::string email;
    std::string name;
    
    UserInfo() = default;
    UserInfo(const std::string& e, const std::string& n);
};

class UserSelectPopup {
public:
    using ConfirmCallback = std::function<void(const std::vector<UserInfo>& selectedUsers)>;
    using CancelCallback = std::function<void()>;

    explicit UserSelectPopup(sf::Font& font);
    
    // Du lieu nguoi dung
    void setUsers(const std::vector<UserInfo>& users);
    void setTitle(const std::string& title);
    void setExtraFields(const std::vector<std::pair<std::string, std::string>>& fields); // Label, default value pairs
    
    // Lay gia tri cac truong bo sung
    std::string getExtraFieldValue(size_t index) const;
    
    // Trang thai popup
    void open();
    void close();
    bool isOpen() const;
    
    // Lay danh sach nguoi dung da chon
    std::vector<UserInfo> getSelectedUsers() const;
    size_t getSelectedCount() const;
    
    // Ham goi lai
    void setOnConfirm(ConfirmCallback callback);
    void setOnCancel(CancelCallback callback);
    
    // Xu ly su kien
    void handleEvent(const sf::Event& event, const sf::RenderWindow& window);
    void update(sf::Vector2f mousePos, bool mousePressed = false);
    void render(sf::RenderWindow& window);

private:
    sf::Font& font_;
    bool isOpen_ = false;
    
    std::string title_ = "Chọn người dùng";
    
    // Danh sach nguoi dung
    std::vector<UserInfo> allUsers_;
    std::vector<size_t> filteredIndices_;
    std::set<size_t> selectedIndices_;
    std::string searchFilter_;
    
    // Thong tin cuon
    float scrollOffset_ = 0.f;
    int hoveredIndex_ = -1;
    
    // Cac truong nhap bo sung (vi du so ngay het han)
    std::vector<std::pair<std::string, std::unique_ptr<UIInputBox>>> extraFields_;
    
    // Thanh phan giao dien
    std::unique_ptr<UIInputBox> searchBox_;
    std::unique_ptr<Button> btnSelectAll_;
    std::unique_ptr<Button> btnConfirm_;
    std::unique_ptr<Button> btnCancel_;
    
    // Thanh phan dung de ve
    sf::RectangleShape overlay_;
    sf::RectangleShape background_;
    std::unique_ptr<sf::Text> titleText_;
    std::unique_ptr<sf::Text> selectedCountText_;
    sf::RectangleShape listBackground_;
    
    // Hang so bo cuc
    static constexpr float POPUP_WIDTH = 600.f;
    static constexpr float POPUP_HEIGHT = 580.f;
    static constexpr float PADDING = 24.f;
    static constexpr float ITEM_HEIGHT = 38.f;
    static constexpr float LIST_HEIGHT = 280.f;
    static constexpr int MAX_VISIBLE_ITEMS = 6;
    
    // Bang mau su dung
    sf::Color bgColor_{255, 255, 255};
    sf::Color borderColor_{210, 216, 224};
    sf::Color headerColor_{27, 38, 59};
    sf::Color accentColor_{20, 118, 172};
    sf::Color textColor_{33, 37, 41};
    sf::Color rowColor_{255, 255, 255};
    sf::Color rowAltColor_{248, 250, 252};
    sf::Color hoverColor_{235, 244, 252};
    sf::Color selectedColor_{214, 232, 248};
    sf::Color successColor_{40, 167, 69};
    sf::Color grayColor_{108, 117, 125};
    
    // Ham goi lai
    ConfirmCallback onConfirm_;
    CancelCallback onCancel_;
    
    // Cac ham ho tro
    void filterUsers();
    void toggleSelection(size_t index);
    void selectAll();
    void deselectAll();
    void renderUserList(sf::RenderWindow& window);
    void setupUI();
    
    static sf::String toUtf8(const std::string& text);
};
