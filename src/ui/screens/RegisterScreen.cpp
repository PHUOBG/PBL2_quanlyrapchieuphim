#include "UI/screens/RegisterScreen.h"
#ifdef _WIN32
#include <windows.h>
#endif
#include <codecvt> // ThÃªm thÆ° viá»‡n nÃ y
#include <locale>  // ThÃªm thÆ° viá»‡n nÃ y

// Helper function: Convert wstring to UTF-8 string
static std::string wstring_to_utf8(const std::wstring& wstr) {
    if (wstr.empty()) return "";
    
#ifdef _WIN32
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.length(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.length(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
#else
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return converter.to_bytes(wstr);
#endif
}

RegisterScreen::RegisterScreen(const Font& font, AuthService& authRef)
    : auth(authRef),
      overlay({0.f, 0.f}),
      card({620.f, 650.f}),
      registerBtn({252.f, 44.f}),
            backBtn({200.f, 44.f}),
    cursor({2.f, 36.f}),
    title(font, L"Đăng ký tài khoản", 30),
      registerBtnText(font, L"Đăng ký", 20),
      backBtnText(font, L"Quay lại", 20),
      closeX(font, L"X", 24),
    msg(font, L"", 16)
{
    // ... (Pháº§n Constructor giá»¯ nguyÃªn nhÆ° cÅ© cá»§a báº¡n) ...
    // Äá»ƒ tiáº¿t kiá»‡m chá»— hiá»ƒn thá»‹, tÃ´i khÃ´ng copy láº¡i pháº§n khá»Ÿi táº¡o UI dÃ i dÃ²ng
    // Báº¡n giá»¯ nguyÃªn code Constructor cÅ© nhÃ©.
    
    // START Constructor code copy placeholder
    overlay.setFillColor(Color(0, 0, 0, 170));
    card.setFillColor(Color(255, 255, 255, 240));
    card.setOrigin(card.getSize() / 2.f);
    title.setFillColor(Color::Black);

    inputBoxes.reserve(6);
    labels.reserve(6);
    placeholders.reserve(6);
    displays.reserve(6);

    wstring labelTexts[] = { L"Họ và tên*", L"Ngày sinh (dd/mm/yyyy)*", L"Email*", L"Số điện thoại*", L"Mật khẩu*", L"Xác nhận mật khẩu*" };
    wstring placeholderTexts[] = { L"Nhập họ và tên", L"dd/mm/yyyy", L"example@email.com", L"0123456789", L"Nhập mật khẩu", L"Nhập lại mật khẩu" };

    for (int i = 0; i < 6; i++) {
        inputBoxes.emplace_back(Vector2f{480.f, 40.f});
        inputBoxes.back().setFillColor(Color(238, 238, 238));
        inputBoxes.back().setOutlineThickness(2.f);
        inputBoxes.back().setOutlineColor(Color(200, 200, 200));
        labels.emplace_back(font, labelTexts[i], 16);
        labels.back().setFillColor(Color(60, 60, 60));
        placeholders.emplace_back(font, placeholderTexts[i], 18);
        placeholders.back().setFillColor(Color(120, 120, 120));
        displays.emplace_back(font, L"", 18);
        displays.back().setFillColor(Color::Black);
    }
    registerBtn.setFillColor(Color(100, 149, 237));
    registerBtnText.setFillColor(Color::White);
    backBtn.setFillColor(Color(90, 90, 90));
    backBtnText.setFillColor(Color::White);
    closeX.setFillColor(Color(90, 90, 90));
    
    FloatRect regBounds = registerBtnText.getLocalBounds();
    registerBtnText.setOrigin({regBounds.size.x / 2.f, regBounds.size.y / 2.f});
    FloatRect backBounds = backBtnText.getLocalBounds();
    backBtnText.setOrigin({backBounds.size.x / 2.f, backBounds.size.y / 2.f});
    cursor.setFillColor(Color::Black);
    msg.setFillColor(Color(200, 60, 60));
    // END Constructor code
}

wstring RegisterScreen::bullets(size_t n) {
    return wstring(n, L'*'); // Má»™t sá»‘ font khÃ´ng há»— trá»£ bullet, dÃ¹ng * cho an toÃ n
}

bool RegisterScreen::validateInputs() {
    // ... (Giá»¯ nguyÃªn logic validateInputs cÅ© cá»§a báº¡n) ...
    for (int i = 0; i < 6; i++) {
        if (inputs[i].empty()) {
            msg.setFillColor(Color(200, 60, 60));
            msg.setString(L"Vui lòng điền đầy đủ thông tin.");
            return false;
        }
    }
    string email(inputs[2].begin(), inputs[2].end());
    if (auth.emailExists(email)) {
        msg.setFillColor(Color(200, 60, 60));
        msg.setString(L"Email đã được sử dụng.");
        return false;
    }
    if (inputs[4] != inputs[5]) {
        msg.setFillColor(Color(200, 60, 60));
        msg.setString(L"Mật khẩu xác nhận không khớp.");
        return false;
    }
    return true; 
}

// [Má»šI] HÃ m xá»­ lÃ½ sá»± kiá»‡n bÃ n phÃ­m/nháº­p liá»‡u
void RegisterScreen::handleEvent(const Event& event, AppState& state) {
    if (const auto* keyEvent = event.getIf<Event::KeyPressed>()) {
        auto code = keyEvent->code;

        if (code == Keyboard::Key::Escape) {
            state = AppState::LOGIN;
            reset();
            showSuccessMessage = false;
            return;
        }
        if (code == Keyboard::Key::Tab) {
            activeField = (activeField + 1) % 6;
            showCursor = true;
            cursorClock.restart();
        } else if (code == Keyboard::Key::Backspace && activeField >= 0) {
            if (!inputs[activeField].empty()) {
                inputs[activeField].pop_back();
                showCursor = true;
                cursorClock.restart();
            }
        } else if (code == Keyboard::Key::Enter) {
            if (validateInputs()) {
                string fullName = wstring_to_utf8(inputs[0]);
                string birthDate = wstring_to_utf8(inputs[1]);
                string email = wstring_to_utf8(inputs[2]);
                string phone = wstring_to_utf8(inputs[3]);
                string password = wstring_to_utf8(inputs[4]);

                if (auth.registerUser(email, password, fullName, birthDate, phone, AppRole::Admin)) {
                    msg.setFillColor(Color(60, 160, 90));
                    msg.setString(L"Đăng ký thành công! Đang chuyển sang đăng nhập...");
                    showSuccessMessage = true;
                    messageTimer.restart();
                } else {
                    msg.setFillColor(Color(200, 60, 60));
                    msg.setString(L"Đăng ký thất bại.");
                }
            }
        }
    }

    if (event.is<Event::TextEntered>() && activeField >= 0) {
        char32_t unicode = event.getIf<Event::TextEntered>()->unicode;
        if (unicode >= 32 && unicode != 127 && inputs[activeField].size() < 50) {
            inputs[activeField].push_back((wchar_t)unicode);
            showCursor = true;
            cursorClock.restart();
        }
    }
}

// [Sá»¬A] HÃ m update chá»‰ xá»­ lÃ½ chuá»™t vÃ  animation
bool RegisterScreen::update(Vector2f mouse, bool mousePressed, AppState& state) {
    const Vector2f center(864.f, 486.f);
    card.setPosition(center);
    closeX.setPosition({card.getPosition().x + card.getSize().x / 2.f - 30.f,
                        card.getPosition().y - card.getSize().y / 2.f + 10.f});

    float fieldWidth = inputBoxes.empty() ? 0.f : inputBoxes[0].getSize().x;
    float inputLeft = center.x - fieldWidth / 2.f;
    float inputRight = inputLeft + fieldWidth;

    for (int i = 0; i < 6; i++) {
        float y = center.y - 215.f + i * 80.f;
        inputBoxes[i].setPosition({inputLeft, y});
    }

    float buttonY = center.y + 240.f;
    registerBtn.setPosition({inputLeft, buttonY});
    backBtn.setPosition({inputRight - backBtn.getSize().x, buttonY});

    if (cursorClock.getElapsedTime().asMilliseconds() > 530) {
        showCursor = !showCursor;
        cursorClock.restart();
    }

    if (showSuccessMessage && messageTimer.getElapsedTime().asSeconds() > 1.0f) {
        showSuccessMessage = false;
        reset();
        state = AppState::LOGIN;
        return false;
    }

    if (mousePressed && closeX.getGlobalBounds().contains(mouse)) {
        state = AppState::LOGIN;
        reset();
        showSuccessMessage = false;
        return false;
    }

    if (mousePressed) {
        activeField = -1;
        for (int i = 0; i < 6; i++) {
            if (inputBoxes[i].getGlobalBounds().contains(mouse)) {
                activeField = i;
                showCursor = true;
                cursorClock.restart();
                break;
            }
        }

        if (registerBtn.getGlobalBounds().contains(mouse)) {
            if (validateInputs()) {
                string fullName = wstring_to_utf8(inputs[0]);
                string birthDate = wstring_to_utf8(inputs[1]);
                string email = wstring_to_utf8(inputs[2]);
                string phone = wstring_to_utf8(inputs[3]);
                string password = wstring_to_utf8(inputs[4]);

                if (auth.registerUser(email, password, fullName, birthDate, phone, AppRole::Admin)) {
                    msg.setFillColor(Color(60, 160, 90));
                    msg.setString(L"Đăng ký thành công! Đang chuyển sang đăng nhập...");
                    showSuccessMessage = true;
                    messageTimer.restart();
                    return false;
                } else {
                    msg.setFillColor(Color(200, 60, 60));
                    msg.setString(L"Đăng ký thất bại. Vui lòng thử lại.");
                }
            }
        } else if (backBtn.getGlobalBounds().contains(mouse)) {
            state = AppState::LOGIN;
            reset();
            showSuccessMessage = false;
            return false;
        }
    }
    return false;
}

void RegisterScreen::draw(RenderWindow& window) {
    // ... (Giá»¯ nguyÃªn code draw cÅ© cá»§a báº¡n) ...
    // START Draw code copy placeholder
    auto size = window.getSize();
    overlay.setSize({(float)size.x, (float)size.y});
    Vector2f center({size.x * 0.5f, size.y * 0.5f});
    card.setPosition(center);
    closeX.setPosition({card.getPosition().x + card.getSize().x/2.f - 30.f, card.getPosition().y - card.getSize().y/2.f + 10.f});
    float fieldWidth = inputBoxes.empty() ? 0.f : inputBoxes[0].getSize().x;
    float inputLeft = center.x - fieldWidth / 2.f;
    float inputRight = inputLeft + fieldWidth;
    FloatRect titleBounds = title.getLocalBounds();
    title.setPosition({center.x - titleBounds.size.x / 2.f - titleBounds.position.x, center.y - card.getSize().y / 2.f + 40.f});
    for (int i = 0; i < 6; i++) {
        float y = center.y - 215.f + i * 80.f;
        labels[i].setPosition({inputLeft, y - 24.f});
        inputBoxes[i].setPosition({inputLeft, y});
        Vector2f textPos = {inputLeft + 10.f, y + 10.f};
        placeholders[i].setPosition(textPos);
        displays[i].setPosition(textPos);
    }
    float buttonY = center.y + 240.f;
    registerBtn.setPosition({inputLeft, buttonY});
    registerBtnText.setPosition({registerBtn.getPosition().x + registerBtn.getSize().x / 2.f, registerBtn.getPosition().y + registerBtn.getSize().y / 2.f - 3});
    backBtn.setPosition({inputRight - backBtn.getSize().x, buttonY});
    backBtnText.setPosition({backBtn.getPosition().x + backBtn.getSize().x / 2.f, backBtn.getPosition().y + backBtn.getSize().y / 2.f - 3});
    msg.setPosition({inputLeft, center.y + 285.f});
    window.draw(overlay);
    window.draw(card);
    window.draw(title);
    window.draw(closeX);
    for (int i = 0; i < 6; i++) {
        window.draw(labels[i]);
        window.draw(inputBoxes[i]);
        if (inputs[i].empty()) {
            window.draw(placeholders[i]);
        } else {
            if (i == 4 || i == 5) displays[i].setString(bullets(inputs[i].size()));
            else displays[i].setString(inputs[i]);
            window.draw(displays[i]);
        }
        if (activeField == i && showCursor) {
            Vector2f boxPos = inputBoxes[i].getPosition();
            Vector2f boxSize = inputBoxes[i].getSize();
            Vector2f textPos = displays[i].getPosition();
            FloatRect bounds = displays[i].getLocalBounds();
            cursor.setSize({2.f, boxSize.y - 8.f});
            float caretX = textPos.x + bounds.size.x + 2.f;
            float caretY = boxPos.y + (boxSize.y - cursor.getSize().y) / 2.f;
            cursor.setPosition({caretX, caretY});
            window.draw(cursor);
        }
    }
    window.draw(registerBtn);
    window.draw(registerBtnText);
    window.draw(backBtn);
    window.draw(backBtnText);
    window.draw(msg);
    // END Draw code
}

void RegisterScreen::reset() {
    for (int i = 0; i < 6; i++) inputs[i].clear();
    activeField = -1;
    msg.setString(L"");
    showSuccessMessage = false;
}