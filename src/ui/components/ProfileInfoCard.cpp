#include "UI/components/ProfileInfoCard.h"
#include "UI/screens/BaseScreen.h"
#include "utils/PasswordHasher.h"
#include "utils/Validator.h"

#ifdef _WIN32
#include <windows.h>
#endif

using namespace sf;
using namespace std;

namespace {
static wstring utf8_to_wstring(const string& str) {
    if (str.empty()) return L"";
#ifdef _WIN32
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.length(), NULL, 0);
    wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.length(), &wstrTo[0], size_needed);
    return wstrTo;
#else
    wstring_convert<codecvt_utf8<wchar_t>> converter;
    return converter.from_bytes(str);
#endif
}

static string wstring_to_utf8(const wstring& wstr) {
    if (wstr.empty()) return "";
#ifdef _WIN32
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.length(), NULL, 0, NULL, NULL);
    string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.length(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
#else
    wstring_convert<codecvt_utf8<wchar_t>> converter;
    return converter.to_bytes(wstr);
#endif
}
}

ProfileInfoCard::ProfileInfoCard(const Font& font, AuthService& auth, const Config& config)
    : font_(font),
      authService_(&auth),
      config_(config),
      position_(0.f, 0.f),
      // Section 1
      personalInfoTitle_(font, L"THÔNG TIN CÁ NHÂN", 22),
      fullNameLabel_(font, L"Họ và tên", 16),
      birthDateLabel_(font, L"Ngày sinh", 16),
      phoneLabel_(font, L"Số điện thoại", 16),
      emailLabel_(font, L"Email", 16),
      fullNameText_(font, "", 16),
      birthDateText_(font, "", 16),
      phoneText_(font, "", 16),
      emailText_(font, "", 16),
      saveInfoBtn_(font, L"LƯU THÔNG TIN", 260.f, 50.f, 18),
      infoMessage_(font, "", 15),
      // Section 2
      passwordTitle_(font, L"ĐỔI MẬT KHẨU", 22),
      oldPwdLabel_(font, L"Mật khẩu cũ *", 16),
      newPwdLabel_(font, L"Mật khẩu mới *", 16),
      confirmPwdLabel_(font, L"Xác thực mật khẩu *", 16),
      oldPwdField_(font, config.passwordFieldWidth, config.fieldHeight),
      newPwdField_(font, config.passwordFieldWidth, config.fieldHeight),
      confirmPwdField_(font, config.passwordFieldWidth, config.fieldHeight),
      changePwdBtn_(font, L"ĐỔI MẬT KHẨU", 260.f, 50.f, 18),
      pwdMessage_(font, "", 15)
{
    setupStyles();
}

void ProfileInfoCard::setupStyles() {
    // Titles
    personalInfoTitle_.setFillColor(config_.titleColor);
    personalInfoTitle_.setStyle(Text::Bold);
    passwordTitle_.setFillColor(config_.titleColor);
    passwordTitle_.setStyle(Text::Bold);
    
    // Labels
    fullNameLabel_.setFillColor(config_.labelColor);
    birthDateLabel_.setFillColor(config_.labelColor);
    phoneLabel_.setFillColor(config_.labelColor);
    emailLabel_.setFillColor(config_.labelColor);
    oldPwdLabel_.setFillColor(config_.labelColor);
    newPwdLabel_.setFillColor(config_.labelColor);
    confirmPwdLabel_.setFillColor(config_.labelColor);
    
    // Info input boxes
    auto setupBox = [&](RectangleShape& box, bool readOnly = false) {
        box.setSize({config_.fieldWidth, config_.fieldHeight});
        box.setFillColor(readOnly ? config_.readOnlyBgColor : config_.inputBgColor);
        box.setOutlineThickness(1.f);
        box.setOutlineColor(config_.outlineColor);
    };
    setupBox(fullNameBox_);
    setupBox(birthDateBox_);
    setupBox(phoneBox_);
    setupBox(emailBox_, true);
    
    // Info text
    fullNameText_.setFillColor(config_.inputTextColor);
    birthDateText_.setFillColor(config_.inputTextColor);
    phoneText_.setFillColor(config_.inputTextColor);
    emailText_.setFillColor(config_.readOnlyTextColor);
    
    // Password fields
    auto setupPwdField = [&](PasswordField& field) {
        field.setFillColor(config_.inputBgColor);
        field.setOutlineColor(config_.outlineColor);
        field.setActiveOutlineColor(config_.accentColor);
        field.setTextColor(config_.inputTextColor);
    };
    setupPwdField(oldPwdField_);
    setupPwdField(newPwdField_);
    setupPwdField(confirmPwdField_);
    
    oldPwdField_.setPlaceholder(L"Nhập mật khẩu cũ");
    newPwdField_.setPlaceholder(L"Nhập mật khẩu mới");
    confirmPwdField_.setPlaceholder(L"Xác nhận mật khẩu mới");
    
    // Buttons
    saveInfoBtn_.setFillColor(config_.accentColor);
    saveInfoBtn_.setTextColor(Color::White);
    changePwdBtn_.setFillColor(config_.accentColor);
    changePwdBtn_.setTextColor(Color::White);
    
    // Cursor
    cursor_.setSize({2.f, 20.f});
    cursor_.setFillColor(config_.accentColor);
    
    // Messages
    infoMessage_.setFillColor(config_.successColor);
    pwdMessage_.setFillColor(config_.successColor);
}

void ProfileInfoCard::setUser(const string& email) {
    if (userDataLoaded_ && currentEmail_ == email) return;
    
    currentEmail_ = email;
    currentUser_ = authService_->getUser(email);
    
    if (currentUser_) {
        fullNameInput_ = utf8_to_wstring(currentUser_->getFullName());
        birthDateInput_ = utf8_to_wstring(currentUser_->getBirthDate());
        phoneInput_ = utf8_to_wstring(currentUser_->getPhone());
        
        fullNameText_.setString(fullNameInput_);
        birthDateText_.setString(birthDateInput_);
        phoneText_.setString(phoneInput_);
        emailText_.setString(utf8_to_wstring(currentUser_->getEmail()));
        
        userDataLoaded_ = true;
    } else {
        fullNameText_.setString("");
        birthDateText_.setString("");
        phoneText_.setString("");
        emailText_.setString("");
        fullNameInput_.clear();
        birthDateInput_.clear();
        phoneInput_.clear();
        userDataLoaded_ = false;
    }
}

void ProfileInfoCard::setPosition(Vector2f pos) {
    position_ = pos;
    layoutElements();
}

void ProfileInfoCard::layoutElements() {
    const float padding = config_.cardPadding;
    const float labelHeight = 20.f;
    const float labelInputGap = config_.labelInputGap;
    const float rowGap = config_.rowGap;
    const float col1X = position_.x + padding;
    const float col2X = col1X + config_.fieldWidth + 24.f + 12.f;
    float currentY = position_.y + padding;
    
    // Section 1: Personal Info Title (aligned with other tabs)
    personalInfoTitle_.setPosition({col1X, currentY});
    currentY += 45.f;
    
    // Row 1: FullName + BirthDate
    fullNameLabel_.setPosition({col1X, currentY});
    birthDateLabel_.setPosition({col2X, currentY});
    currentY += labelHeight + labelInputGap;
    
    fullNameBox_.setPosition({col1X, currentY});
    fullNameText_.setPosition({col1X + 10.f, currentY + 11.f});
    birthDateBox_.setPosition({col2X, currentY});
    birthDateText_.setPosition({col2X + 10.f, currentY + 11.f});
    currentY += config_.fieldHeight + rowGap;
    
    // Row 2: Phone + Email
    phoneLabel_.setPosition({col1X, currentY});
    emailLabel_.setPosition({col2X, currentY});
    currentY += labelHeight + labelInputGap;
    
    phoneBox_.setPosition({col1X, currentY});
    phoneText_.setPosition({col1X + 10.f, currentY + 11.f});
    emailBox_.setPosition({col2X, currentY});
    emailText_.setPosition({col2X + 10.f, currentY + 11.f});
    currentY += config_.fieldHeight + 24.f;
    
    // Save Info Button
    saveInfoBtn_.setPosition({col1X, currentY});
    infoMessage_.setPosition({col1X, currentY + 60.f});
    currentY += 70.f + config_.sectionGap;
    
    // Section 2: Password Change
    if (config_.showPasswordSection) {
        passwordTitle_.setPosition({col1X, currentY});
        currentY += 45.f;
        
        oldPwdLabel_.setPosition({col1X, currentY});
        currentY += labelHeight + labelInputGap;
        oldPwdField_.setPosition({col1X, currentY});
        currentY += config_.fieldHeight + rowGap;
        
        newPwdLabel_.setPosition({col1X, currentY});
        currentY += labelHeight + labelInputGap;
        newPwdField_.setPosition({col1X, currentY});
        currentY += config_.fieldHeight + rowGap;
        
        confirmPwdLabel_.setPosition({col1X, currentY});
        currentY += labelHeight + labelInputGap;
        confirmPwdField_.setPosition({col1X, currentY});
        currentY += config_.fieldHeight + 24.f;
        
        changePwdBtn_.setPosition({col1X, currentY});
        pwdMessage_.setPosition({col1X, currentY + 60.f});
    }
}

void ProfileInfoCard::deactivateAllPasswordFields() {
    oldPwdField_.setActive(false);
    newPwdField_.setActive(false);
    confirmPwdField_.setActive(false);
}

void ProfileInfoCard::handleInfoInput(const Event* event) {
    if (!event || activeInfoField_ < 0) return;
    
    if (auto* textEvent = event->getIf<Event::TextEntered>()) {
        showInfoMsg_ = false;
        wchar_t c = static_cast<wchar_t>(textEvent->unicode);
        
        if (c == '\b') {
            if (activeInfoField_ == 0 && !fullNameInput_.empty()) fullNameInput_.pop_back();
            else if (activeInfoField_ == 1 && !birthDateInput_.empty()) birthDateInput_.pop_back();
            else if (activeInfoField_ == 2 && !phoneInput_.empty()) phoneInput_.pop_back();
        }
        else if (c >= 32 && c != 127) {
            if (activeInfoField_ == 0) fullNameInput_ += c;
            else if (activeInfoField_ == 1 && birthDateInput_.length() < 10) birthDateInput_ += c;
            else if (activeInfoField_ == 2 && phoneInput_.length() < 15) phoneInput_ += c;
        }
        
        if (activeInfoField_ == 0) fullNameText_.setString(fullNameInput_);
        else if (activeInfoField_ == 1) birthDateText_.setString(birthDateInput_);
        else if (activeInfoField_ == 2) phoneText_.setString(phoneInput_);
    }
}

void ProfileInfoCard::showInfoMessage(const wstring& msg, bool isError) {
    infoMessage_.setFillColor(isError ? config_.errorColor : config_.successColor);
    infoMessage_.setString(msg);
    showInfoMsg_ = true;
    infoMsgClock_.restart();
}

void ProfileInfoCard::showPwdMessage(const wstring& msg, bool isError) {
    pwdMessage_.setFillColor(isError ? config_.errorColor : config_.successColor);
    pwdMessage_.setString(msg);
    showPwdMsg_ = true;
    pwdMsgClock_.restart();
}

void ProfileInfoCard::saveInfoChange() {
    if (!currentUser_) return;
    
    const string oldFullName = currentUser_->getFullName();
    string newFullName = wstring_to_utf8(fullNameInput_);
    string newBirthDate = wstring_to_utf8(birthDateInput_);
    string newPhone = wstring_to_utf8(phoneInput_);
    
    // Reset outlines
    fullNameBox_.setOutlineColor(config_.outlineColor);
    fullNameBox_.setOutlineThickness(1.f);
    birthDateBox_.setOutlineColor(config_.outlineColor);
    birthDateBox_.setOutlineThickness(1.f);
    phoneBox_.setOutlineColor(config_.outlineColor);
    phoneBox_.setOutlineThickness(1.f);
    
    // Validation
    if (newFullName.empty()) {
        showInfoMessage(L"Họ tên không được để trống!", true);
        fullNameBox_.setOutlineColor(config_.errorColor);
        fullNameBox_.setOutlineThickness(2.f);
        return;
    }
    
    if (!Validator::isValidFullName(newFullName)) {
        showInfoMessage(L"Họ tên không được chứa số!", true);
        fullNameBox_.setOutlineColor(config_.errorColor);
        fullNameBox_.setOutlineThickness(2.f);
        return;
    }
    
    if (!newBirthDate.empty() && !Validator::isValidDate(newBirthDate)) {
        showInfoMessage(L"Ngày sinh không hợp lệ (dd/mm/yyyy)!", true);
        birthDateBox_.setOutlineColor(config_.errorColor);
        birthDateBox_.setOutlineThickness(2.f);
        return;
    }
    
    if (!newPhone.empty() && !Validator::isValidPhone(newPhone)) {
        showInfoMessage(L"Số điện thoại phải có 10 chữ số!", true);
        phoneBox_.setOutlineColor(config_.errorColor);
        phoneBox_.setOutlineThickness(2.f);
        return;
    }
    
    // Save
    currentUser_->setFullName(newFullName);
    currentUser_->setBirthDate(newBirthDate);
    currentUser_->setPhone(newPhone);

    // Persist immediately to users.txt
    if (authService_ && authService_->getRepository()) {
        authService_->getRepository()->saveToFile();
    }

    // If this is the logged-in user, update the greeting immediately
    if (newFullName != oldFullName && BaseScreen::getLoggedInUserEmail() == currentEmail_) {
        BaseScreen::setLoggedInUser(newFullName, currentEmail_);
    }
    
    // Persist changes to file
    authService_->getRepository()->saveToFile();
    
    userDataLoaded_ = false;
    setUser(currentEmail_);
    
    showInfoMessage(L"Cập nhật thông tin thành công!", false);
}

void ProfileInfoCard::savePasswordChange() {
    if (!currentUser_) return;
    
    string oldPwd = oldPwdField_.getValue();
    string newPwd = newPwdField_.getValue();
    string confirmPwd = confirmPwdField_.getValue();
    
    if (oldPwd.empty() || newPwd.empty() || confirmPwd.empty()) {
        showPwdMessage(L"Vui lòng điền đầy đủ thông tin!", true);
        return;
    }
    
    if (!PasswordHasher::verifyPassword(oldPwd, currentUser_->getPasswordHash())) {
        showPwdMessage(L"Mật khẩu cũ không đúng!", true);
        return;
    }
    
    if (!Validator::isStrongPassword(newPwd)) {
        showPwdMessage(L"Mật khẩu mới phải có ít nhất 8 ký tự, 1 chữ hoa, 1 chữ thường, 1 số!", true);
        return;
    }
    
    if (newPwd != confirmPwd) {
        showPwdMessage(L"Mật khẩu mới không khớp!", true);
        return;
    }
    
    currentUser_->setPasswordHash(PasswordHasher::hashPassword(newPwd));

    // Persist immediately to users.txt
    if (authService_ && authService_->getRepository()) {
        authService_->getRepository()->saveToFile();
    }
    
    // Persist changes to file
    authService_->getRepository()->saveToFile();
    
    showPwdMessage(L"Đổi mật khẩu thành công!", false);
    
    // Clear password fields
    oldPwdField_.clear();
    newPwdField_.clear();
    confirmPwdField_.clear();
    deactivateAllPasswordFields();
}

void ProfileInfoCard::handleEvent(const Event& event, Vector2f mousePos, bool mousePressed) {
    // Handle keyboard
    if (auto* keyEvent = event.getIf<Event::KeyPressed>()) {
        if (keyEvent->code == Keyboard::Key::Tab) {
            if (activeInfoField_ >= 0) {
                activeInfoField_ = (activeInfoField_ + 1) % 3;
                deactivateAllPasswordFields();
            }
            showCursor_ = true;
            cursorClock_.restart();
        }
        else if (keyEvent->code == Keyboard::Key::Escape) {
            activeInfoField_ = -1;
            deactivateAllPasswordFields();
        }
    }
    
    // Handle password field events
    if (config_.showPasswordSection) {
        oldPwdField_.handleEvent(event, mousePos, mousePressed);
        newPwdField_.handleEvent(event, mousePos, mousePressed);
        confirmPwdField_.handleEvent(event, mousePos, mousePressed);
    }
    
    // Handle info field input
    handleInfoInput(&event);
}

void ProfileInfoCard::update(Vector2f mousePos, bool mousePressed) {
    // Cursor blinking
    if (cursorClock_.getElapsedTime().asSeconds() >= 0.5f) {
        showCursor_ = !showCursor_;
        cursorClock_.restart();
    }
    
    // Hide messages after 2 seconds
    if (showInfoMsg_ && infoMsgClock_.getElapsedTime().asSeconds() >= 2.0f) {
        showInfoMsg_ = false;
    }
    if (showPwdMsg_ && pwdMsgClock_.getElapsedTime().asSeconds() >= 2.0f) {
        showPwdMsg_ = false;
    }
    
    // Update password fields
    if (config_.showPasswordSection) {
        oldPwdField_.update(mousePos);
        newPwdField_.update(mousePos);
        confirmPwdField_.update(mousePos);
    }
    
    // Handle field clicks
    bool mouseJustPressed = mousePressed && !wasMousePressed_;
    
    if (mouseJustPressed) {
        // Info fields
        if (fullNameBox_.getGlobalBounds().contains(mousePos)) {
            activeInfoField_ = 0;
            deactivateAllPasswordFields();
            showCursor_ = true;
            cursorClock_.restart();
        }
        else if (birthDateBox_.getGlobalBounds().contains(mousePos)) {
            activeInfoField_ = 1;
            deactivateAllPasswordFields();
            showCursor_ = true;
            cursorClock_.restart();
        }
        else if (phoneBox_.getGlobalBounds().contains(mousePos)) {
            activeInfoField_ = 2;
            deactivateAllPasswordFields();
            showCursor_ = true;
            cursorClock_.restart();
        }
        // Password fields - clicking them deactivates info fields
        else if (config_.showPasswordSection && 
                (oldPwdField_.containsPoint(mousePos) ||
                 newPwdField_.containsPoint(mousePos) ||
                 confirmPwdField_.containsPoint(mousePos))) {
            activeInfoField_ = -1;
        }
        // Click outside
        else if (!saveInfoBtn_.getGlobalBounds().contains(mousePos) && 
                 !changePwdBtn_.getGlobalBounds().contains(mousePos)) {
            activeInfoField_ = -1;
        }
        
        // Button clicks
        if (saveInfoBtn_.isClicked(mousePos, true)) {
            saveInfoChange();
        }
        else if (config_.showPasswordSection && changePwdBtn_.isClicked(mousePos, true)) {
            savePasswordChange();
        }
    }
    
    wasMousePressed_ = mousePressed;
    
    // Update info box outlines
    fullNameBox_.setOutlineColor(activeInfoField_ == 0 ? config_.accentColor : config_.outlineColor);
    birthDateBox_.setOutlineColor(activeInfoField_ == 1 ? config_.accentColor : config_.outlineColor);
    phoneBox_.setOutlineColor(activeInfoField_ == 2 ? config_.accentColor : config_.outlineColor);
    
    fullNameBox_.setOutlineThickness(activeInfoField_ == 0 ? 2.f : 1.f);
    birthDateBox_.setOutlineThickness(activeInfoField_ == 1 ? 2.f : 1.f);
    phoneBox_.setOutlineThickness(activeInfoField_ == 2 ? 2.f : 1.f);
    
    // Update buttons
    Color btnHover(0, 191, 255);
    saveInfoBtn_.update(mousePos, mousePressed, btnHover, config_.accentColor);
    changePwdBtn_.update(mousePos, mousePressed, btnHover, config_.accentColor);
}

void ProfileInfoCard::draw(RenderWindow& window) {
    auto drawTextClippedToBox = [&](const RectangleShape& box, Text& textObj) {
        // Horizontal scrolling: keep the tail visible when text is too long
        FloatRect textBounds = textObj.getLocalBounds();
        FloatRect boxBounds = box.getGlobalBounds();
        float paddingX = 10.f;
        float available = std::max(0.f, boxBounds.size.x - 2.f * paddingX);
        float x = boxBounds.position.x + paddingX;
        if (textBounds.size.x > available) {
            x += (available - textBounds.size.x);
        }
        textObj.setPosition({x, textObj.getPosition().y});

        // Clip to the box rectangle (prevents overflow)
        View oldView = window.getView();
        const auto ws = window.getSize();
        View clipView;
        clipView.setCenter({boxBounds.position.x + boxBounds.size.x / 2.f, boxBounds.position.y + boxBounds.size.y / 2.f});
        clipView.setSize({boxBounds.size.x, boxBounds.size.y});
        clipView.setViewport(FloatRect(
            {boxBounds.position.x / static_cast<float>(ws.x), boxBounds.position.y / static_cast<float>(ws.y)},
            {boxBounds.size.x / static_cast<float>(ws.x), boxBounds.size.y / static_cast<float>(ws.y)}
        ));
        window.setView(clipView);
        window.draw(textObj);
        window.setView(oldView);
    };

    // Section 1: Personal Info
    window.draw(personalInfoTitle_);
    
    window.draw(fullNameLabel_);
    window.draw(fullNameBox_);
    drawTextClippedToBox(fullNameBox_, fullNameText_);
    
    window.draw(phoneLabel_);
    window.draw(phoneBox_);
    drawTextClippedToBox(phoneBox_, phoneText_);
    
    window.draw(birthDateLabel_);
    window.draw(birthDateBox_);
    drawTextClippedToBox(birthDateBox_, birthDateText_);
    
    window.draw(emailLabel_);
    window.draw(emailBox_);
    drawTextClippedToBox(emailBox_, emailText_);
    
    saveInfoBtn_.draw(window);
    
    // Section 2: Password Change
    if (config_.showPasswordSection) {
        window.draw(passwordTitle_);
        
        window.draw(oldPwdLabel_);
        oldPwdField_.draw(window);
        
        window.draw(newPwdLabel_);
        newPwdField_.draw(window);
        
        window.draw(confirmPwdLabel_);
        confirmPwdField_.draw(window);
        
        changePwdBtn_.draw(window);
    }
    
    // Draw cursor for info fields
    if (showCursor_ && activeInfoField_ >= 0) {
        Text* activeText = nullptr;
        if (activeInfoField_ == 0) activeText = &fullNameText_;
        else if (activeInfoField_ == 1) activeText = &birthDateText_;
        else if (activeInfoField_ == 2) activeText = &phoneText_;
        
        if (activeText) {
            FloatRect bounds = activeText->getLocalBounds();
            cursor_.setPosition({activeText->getPosition().x + bounds.size.x + 2.f, 
                               activeText->getPosition().y - 2.f});
            window.draw(cursor_);
        }
    }
    
    // Messages
    if (showInfoMsg_) window.draw(infoMessage_);
    if (showPwdMsg_) window.draw(pwdMessage_);
}

Vector2f ProfileInfoCard::getSize() const {
    return {config_.width, getTotalHeight()};
}

float ProfileInfoCard::getTotalHeight() const {
    float height = 45.f + (20.f + config_.labelInputGap + config_.fieldHeight + config_.rowGap) * 2 + 70.f;
    
    if (config_.showPasswordSection) {
        height += config_.sectionGap + 45.f + (20.f + config_.labelInputGap + config_.fieldHeight + config_.rowGap) * 3 + 70.f;
    }
    
    return height;
}
