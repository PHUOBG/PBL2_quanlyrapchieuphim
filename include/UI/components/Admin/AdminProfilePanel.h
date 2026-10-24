#pragma once
#include <SFML/Graphics.hpp>
#include <cstdint>
#include <string>
#include <memory>
#include "services/AuthService.h"
#include "UI/components/ProfileInfoCard.h"

class AdminProfilePanel {
private:
    Font& font;
    float width;
    float height;
    Vector2f position;
    AuthService* authService;
    User* currentUser = nullptr;
    std::string currentEmail;
    bool userDataLoaded = false;

    // Cac khoi bo cuc
    RectangleShape panelBackground;
    RectangleShape contentCard;
    RectangleShape heroBanner;
    CircleShape avatarCircle;

    // Khu vuc thong tin chinh
    Text heroNameText;
    Text heroMetaText;
    Text avatarInitialsText;

    // The hien lai ProfileInfoCard
    ProfileInfoCard profileCard_;

    // Delete account feature
    RectangleShape deleteAccountBtn;
    Text deleteAccountText;
    bool showDeleteConfirm = false;
    RectangleShape confirmOverlay;
    RectangleShape confirmCard;
    Text confirmTitle;
    Text confirmMsg;
    RectangleShape confirmYesBtn;
    RectangleShape confirmNoBtn;
    Text confirmYesText;
    Text confirmNoText;

    void layoutElements();
    void refreshUser();
    void updateHeroContents();
    std::wstring getRoleLabel(AppRole role) const;
    std::wstring buildInitials(const std::string& source) const;

public:
    AdminProfilePanel(Font& font, float width, float height, AuthService* authService);

    void setPosition(Vector2f pos);
    void handleEvent(const Event& event, const RenderWindow& window);
    void update(Vector2f mousePos, bool mousePressed);
    void render(RenderWindow& window);
    void reload();
};