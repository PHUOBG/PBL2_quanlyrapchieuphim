#include "UI/components/Admin/AdminProfilePanel.h"
#include "UI/screens/BaseScreen.h"
#include <algorithm>
#include <cwctype>

#ifdef _WIN32
#include <windows.h>
#endif

using namespace std;
using namespace sf;

namespace {
static wstring utf8_to_wstring(const string& str) {
    if (str.empty()) return L"";
#ifdef _WIN32
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), static_cast<int>(str.length()), nullptr, 0);
    wstring wstr(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), static_cast<int>(str.length()), &wstr[0], size_needed);
    return wstr;
#else
    return wstring(str.begin(), str.end());
#endif
}
}

static ProfileInfoCard::Config createAdminConfig() {
    ProfileInfoCard::Config cfg;
    cfg.darkMode = false;
    cfg.backgroundColor = Color(255, 255, 255);
    cfg.labelColor = Color(108, 122, 140);
    cfg.titleColor = Color(19, 33, 58);
    cfg.inputBgColor = Color(248, 250, 252);
    cfg.inputTextColor = Color(19, 33, 58);
    cfg.outlineColor = Color(214, 222, 230);
    cfg.accentColor = Color(20, 118, 172);
    cfg.readOnlyBgColor = Color(238, 241, 245);
    cfg.readOnlyTextColor = Color(120, 130, 145);
    cfg.successColor = Color(63, 151, 90);
    cfg.errorColor = Color(209, 67, 67);
    return cfg;
}

AdminProfilePanel::AdminProfilePanel(Font& f, float w, float h, AuthService* auth)
    : font(f),
      width(w),
      height(h),
      position(Vector2f(0.f, 0.f)),
      authService(auth),
      panelBackground(Vector2f(w, h)),
      contentCard(),
      heroBanner(),
      avatarCircle(),
      heroNameText(f, L"", 22),
      heroMetaText(f, L"", 15),
      avatarInitialsText(f, L"", 20),
      profileCard_(f, *auth, createAdminConfig()),
      deleteAccountBtn(Vector2f(200.f, 40.f)),
      deleteAccountText(f, L"XÓA VĨNH VIỄN TÀI KHOẢN", 13),
      confirmOverlay(Vector2f(1728.f, 972.f)),
      confirmCard(Vector2f(500.f, 260.f)),
      confirmTitle(f, L"XÁC NHẬN XÓA TÀI KHOẢN ADMIN", 20),
      confirmMsg(f, L"Bạn có chắc muốn xóa vĩnh viễn tài khoản quản trị này?\nHành động này không thể hoàn tác!", 16),
      confirmYesBtn(Vector2f(180.f, 44.f)),
      confirmNoBtn(Vector2f(180.f, 44.f)),
      confirmYesText(f, L"Xóa vĩnh viễn", 16),
      confirmNoText(f, L"Hủy bỏ", 16)
{
    Color panelColor(241, 245, 250);
    Color cardColor(255, 255, 255);
    Color outlineColor(214, 222, 230);
    Color mutedText(108, 122, 140);
    Color headingColor(19, 33, 58);
    Color heroBg(227, 233, 243);
    Color chipColor(20, 118, 172);

    panelBackground.setFillColor(panelColor);

    contentCard.setFillColor(cardColor);
    contentCard.setOutlineColor(outlineColor);
    contentCard.setOutlineThickness(1.2f);

    heroBanner.setFillColor(heroBg);
    heroBanner.setOutlineColor(outlineColor);
    heroBanner.setOutlineThickness(1.f);

    avatarCircle.setRadius(26.f);
    avatarCircle.setFillColor(chipColor);

    heroNameText.setFillColor(headingColor);
    heroNameText.setStyle(Text::Bold);
    heroMetaText.setFillColor(mutedText);
    avatarInitialsText.setFillColor(Color::White);
    avatarInitialsText.setStyle(Text::Bold);

    // Delete button
    deleteAccountBtn.setFillColor(Color(211, 47, 47));
    deleteAccountText.setFillColor(Color::White);
    deleteAccountText.setStyle(Text::Bold);

    // Confirm dialog setup
    confirmOverlay.setFillColor(Color(0, 0, 0, 190));
    confirmCard.setFillColor(Color::White);
    confirmCard.setOutlineColor(Color(211, 47, 47));
    confirmCard.setOutlineThickness(2.f);
    confirmCard.setOrigin(confirmCard.getSize() / 2.f);

    confirmTitle.setFillColor(Color(211, 47, 47));
    confirmTitle.setStyle(Text::Bold);
    confirmMsg.setFillColor(Color(33, 37, 41));

    confirmYesBtn.setFillColor(Color(211, 47, 47));
    confirmYesText.setFillColor(Color::White);
    confirmYesText.setStyle(Text::Bold);

    confirmNoBtn.setFillColor(Color(150, 155, 165));
    confirmNoText.setFillColor(Color::White);

    refreshUser();
    layoutElements();
}

void AdminProfilePanel::setPosition(Vector2f pos) {
    position = pos;
    layoutElements();
}

void AdminProfilePanel::layoutElements() {
    panelBackground.setSize(Vector2f(width, height));
    panelBackground.setPosition(position);

    float outerPadding = 28.f;
    float heroHeight = 72.f;
    float contentWidth = width - outerPadding * 2.f;

    // Hero banner
    Vector2f heroPos(position.x + outerPadding, position.y + outerPadding);
    heroBanner.setPosition(heroPos);
    heroBanner.setSize(Vector2f(contentWidth, heroHeight));

    // Avatar
    float avatarRadius = avatarCircle.getRadius();
    float avatarY = heroPos.y + (heroHeight - avatarRadius * 2.f) / 2.f;
    avatarCircle.setPosition(Vector2f(heroPos.x + 20.f, avatarY));
    
    // Center initials in avatar
    FloatRect initialsBounds = avatarInitialsText.getLocalBounds();
    float avatarCenterX = avatarCircle.getPosition().x + avatarRadius;
    float avatarCenterY = avatarCircle.getPosition().y + avatarRadius;
    avatarInitialsText.setPosition(Vector2f(
        avatarCenterX - (initialsBounds.size.x / 2.f) - initialsBounds.position.x,
        avatarCenterY - (initialsBounds.size.y / 2.f) - initialsBounds.position.y
    ));

    // Hero text
    float textStartX = avatarCircle.getPosition().x + avatarRadius * 2.f + 16.f;
    float textCenterY = heroPos.y + heroHeight / 2.f;
    heroNameText.setPosition(Vector2f(textStartX, textCenterY - 20.f));
    heroMetaText.setPosition(Vector2f(textStartX, textCenterY + 6.f));

    // Delete account button on top right of hero banner
    deleteAccountBtn.setPosition(Vector2f(heroPos.x + contentWidth - 220.f, heroPos.y + 16.f));
    FloatRect datBounds = deleteAccountText.getLocalBounds();
    deleteAccountText.setPosition(Vector2f(
        deleteAccountBtn.getPosition().x + (deleteAccountBtn.getSize().x - datBounds.size.x) / 2.f,
        deleteAccountBtn.getPosition().y + 12.f
    ));

    // Content card
    float cardStartY = heroPos.y + heroHeight + 16.f;
    contentCard.setPosition({heroPos.x, cardStartY});
    contentCard.setSize({contentWidth, height - cardStartY - outerPadding + position.y});

    // Profile card inside content card
    profileCard_.setPosition({heroPos.x + 28.f, cardStartY + 28.f});
}

void AdminProfilePanel::refreshUser() {
    if (!authService) return;
    User* user = authService->getCurrentUser();
    if (!user) return;

    if (userDataLoaded && currentEmail == user->getEmail()) {
        return;
    }

    currentUser = user;
    currentEmail = user->getEmail();
    
    // Update profile card
    profileCard_.setUser(currentEmail);
    
    updateHeroContents();
    userDataLoaded = true;
}

void AdminProfilePanel::handleEvent(const Event& event, const RenderWindow& window) {
    Vector2f mousePos = window.mapPixelToCoords(Mouse::getPosition(window));
    bool mousePressed = Mouse::isButtonPressed(Mouse::Button::Left);
    
    if (showDeleteConfirm) {
        if (const auto* keyEvent = event.getIf<Event::KeyPressed>()) {
            if (keyEvent->code == Keyboard::Key::Escape) {
                showDeleteConfirm = false;
            }
        }
        return;
    }

    profileCard_.handleEvent(event, mousePos, mousePressed);
}

void AdminProfilePanel::update(Vector2f mousePos, bool mousePressed) {
    refreshUser();
    layoutElements();
    
    if (showDeleteConfirm) {
        if (mousePressed) {
            if (confirmNoBtn.getGlobalBounds().contains(mousePos)) {
                showDeleteConfirm = false;
            }
            else if (confirmYesBtn.getGlobalBounds().contains(mousePos)) {
                // Delete user from repository and file
                if (authService && authService->getRepository() && currentUser) {
                    authService->getRepository()->deleteUser(currentUser->getEmail());
                    authService->logout();
                    BaseScreen::handleLogout();
                }
                showDeleteConfirm = false;
            }
        }
        return;
    }

    if (mousePressed && deleteAccountBtn.getGlobalBounds().contains(mousePos)) {
        showDeleteConfirm = true;
    }

    profileCard_.update(mousePos, mousePressed);
}

void AdminProfilePanel::render(RenderWindow& window) {
    window.draw(panelBackground);
    window.draw(heroBanner);
    window.draw(avatarCircle);
    window.draw(avatarInitialsText);
    window.draw(heroNameText);
    window.draw(heroMetaText);
    window.draw(deleteAccountBtn);
    window.draw(deleteAccountText);
    window.draw(contentCard);
    
    profileCard_.draw(window);

    // Render confirm modal
    if (showDeleteConfirm) {
        confirmOverlay.setPosition(Vector2f(0.f, 0.f));
        window.draw(confirmOverlay);

        Vector2f winCenter(window.getSize().x / 2.f, window.getSize().y / 2.f);
        confirmCard.setPosition(winCenter);
        window.draw(confirmCard);

        FloatRect tb = confirmTitle.getLocalBounds();
        confirmTitle.setPosition(Vector2f(winCenter.x - tb.size.x / 2.f, winCenter.y - 100.f));
        window.draw(confirmTitle);

        FloatRect mb = confirmMsg.getLocalBounds();
        confirmMsg.setPosition(Vector2f(winCenter.x - mb.size.x / 2.f, winCenter.y - 40.f));
        window.draw(confirmMsg);

        confirmYesBtn.setPosition(Vector2f(winCenter.x - 190.f, winCenter.y + 40.f));
        confirmNoBtn.setPosition(Vector2f(winCenter.x + 10.f, winCenter.y + 40.f));
        window.draw(confirmYesBtn);
        window.draw(confirmNoBtn);

        FloatRect yb = confirmYesText.getLocalBounds();
        confirmYesText.setPosition(Vector2f(confirmYesBtn.getPosition().x + (confirmYesBtn.getSize().x - yb.size.x) / 2.f, confirmYesBtn.getPosition().y + 11.f));
        window.draw(confirmYesText);

        FloatRect nb = confirmNoText.getLocalBounds();
        confirmNoText.setPosition(Vector2f(confirmNoBtn.getPosition().x + (confirmNoBtn.getSize().x - nb.size.x) / 2.f, confirmNoBtn.getPosition().y + 11.f));
        window.draw(confirmNoText);
    }
}

void AdminProfilePanel::reload() {
    userDataLoaded = false;
    refreshUser();
}

void AdminProfilePanel::updateHeroContents() {
    if (!currentUser) return;

    wstring fullName = utf8_to_wstring(currentUser->getFullName());
    wstring displayName = fullName.empty() 
        ? utf8_to_wstring(currentUser->getEmail().substr(0, currentUser->getEmail().find('@')))
        : fullName;
    if (!displayName.empty()) {
        displayName[0] = towupper(displayName[0]);
    }
    heroNameText.setString(L"HỒ SƠ CỦA " + displayName);

    wstring roleDescription = getRoleLabel(currentUser->getRole());
    wstring emailW = utf8_to_wstring(currentUser->getEmail());
    heroMetaText.setString(emailW + L"  -  " + roleDescription);

    wstring initials = buildInitials(currentUser->getFullName());
    if (initials.empty()) {
        initials = buildInitials(currentUser->getEmail());
    }
    if (initials.length() > 1) initials = initials.substr(0, 1);
    avatarInitialsText.setString(initials);
}

wstring AdminProfilePanel::getRoleLabel(AppRole role) const {
    switch (role) {
        case AppRole::Admin: return L"Quản trị viên";
        case AppRole::Customer: return L"Khách hàng";
        default: return L"Khách";
    }
}

wstring AdminProfilePanel::buildInitials(const string& source) const {
    wstring wide = utf8_to_wstring(source);
    wstring result;
    bool takeNext = true;
    for (wchar_t ch : wide) {
        if (iswalpha(ch) || iswdigit(ch)) {
            if (takeNext) {
                result.push_back(towupper(ch));
                takeNext = false;
                if (result.size() == 2) break;
            }
        } else if (ch == L' ' || ch == L'-') {
            takeNext = true;
        }
    }
    if (result.empty() && !wide.empty()) {
        result.push_back(towupper(wide.front()));
    }
    return result;
}