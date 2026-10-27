#pragma once

#include <SFML/Graphics.hpp>
#include <string>
#include <memory>
#include "services/AuthService.h"

class AdminProfilePanel {
private:
    sf::Font& font;
    sf::Font boldFont;
    float width;
    float height;
    sf::Vector2f position;

    AuthService* authService;
    User* currentUser = nullptr;

    // Field values
    std::string fullName = "ADMIN";
    std::string birthDate = "12/02/2006";
    std::string phone = "0898234406";
    std::string email = "admin@cinexine.vn";
    std::string department = "Phòng Quản lý vận hành rạp";
    std::string location = "CineXine Đà Nẵng Center";

    // Password values
    std::string oldPass = "";
    std::string newPass = "";
    std::string confirmPass = "";
    bool showOldPass = false;
    bool showNewPass = false;
    bool showConfirmPass = false;

    // Active input field
    // 1: fullName, 2: birthDate, 3: phone, 4: dept, 5: loc, 6: oldPass, 7: newPass, 8: confirmPass
    int activeField = 0;

    // Button hover states
    bool saveInfoHovered = false;
    bool changePassHovered = false;
    bool deleteHovered = false;
    bool showDeleteConfirm = false;

    // Toast notification
    bool toastActive = false;
    float toastTimer = 0.f;
    std::wstring toastMessage;

    void loadUserData();
    void saveUserData();
    void changePassword();

    // Render helpers
    void drawHeaderBanner(sf::RenderTarget& target) const;
    void drawPersonalInfoCard(sf::RenderTarget& target) const;
    void drawPasswordCard(sf::RenderTarget& target) const;
    void drawTwoFactorCard(sf::RenderTarget& target) const;
    void drawDeleteModal(sf::RenderTarget& target) const;
    void drawToast(sf::RenderTarget& target) const;
    void drawBottomBar(sf::RenderTarget& target) const;

public:
    AdminProfilePanel(sf::Font& fontRef, float w, float h, AuthService* auth);

    void setPosition(const sf::Vector2f& pos);
    void handleEvent(const sf::Event& event, const sf::RenderWindow& window);
    void update(const sf::Vector2f& mousePos, bool mouseDown);
    void render(sf::RenderTarget& target) const;
    void reload();
};