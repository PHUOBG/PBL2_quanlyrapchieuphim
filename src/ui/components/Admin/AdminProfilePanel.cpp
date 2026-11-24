#include "UI/components/Admin/AdminProfilePanel.h"
#include "UI/components/Admin/FontManager.h"
#include "UI/components/Admin/RoundedRectRenderer.h"
#include "UI/components/Admin/UITheme.h"
#include "utils/PasswordHasher.h"
#include "utils/Validator.h"
#include "utils/FileUtils.h"

#include <algorithm>
#include <array>
#include <fstream>
#include <iomanip>
#include <sstream>

using namespace sf;
using namespace std;

static wstring s2ws(const string& s) {
    if (s.empty()) return L"";
    auto sfStr = String::fromUtf8(s.begin(), s.end());
    return sfStr.toWideString();
}

AdminProfilePanel::AdminProfilePanel(Font& fontRef, float w, float h, AuthService* auth)
    : font(fontRef),
      boldFont(FontManager::instance().bold()),
      width(w),
      height(h),
      position(280.f, 0.f),
      authService(auth)
{
    loadUserData();
}

void AdminProfilePanel::setPosition(const Vector2f& pos) {
    position = pos;
    width = 1728.f - pos.x;
}

void AdminProfilePanel::reload() {
    loadUserData();
}

void AdminProfilePanel::loadUserData() {
    if (authService) {
        currentUser = authService->getCurrentUser();
        if (currentUser) {
            fullName = currentUser->getFullName().empty() ? "ADMIN" : currentUser->getFullName();
            birthDate = currentUser->getBirthDate();
            phone = currentUser->getPhone();
            email = currentUser->getEmail();
        }
    }
}

void AdminProfilePanel::saveUserData() {
    if (!Validator::isValidFullName(fullName)) {
        toastActive = true;
        toastTimer = 2.5f;
        toastMessage = L"Họ và tên không hợp lệ (không để trống hoặc chứa chữ số)!";
        return;
    }
    if (!Validator::isValidDate(birthDate)) {
        toastActive = true;
        toastTimer = 2.5f;
        toastMessage = L"Ngày sinh không hợp lệ (định dạng DD/MM/YYYY)!";
        return;
    }
    if (!Validator::isValidPhone(phone)) {
        toastActive = true;
        toastTimer = 2.5f;
        toastMessage = L"Số điện thoại không hợp lệ (10 chữ số nhà mạng VN)!";
        return;
    }
    if (authService && currentUser) {
        currentUser->setFullName(fullName);
        currentUser->setBirthDate(birthDate);
        currentUser->setPhone(phone);
        auto* repo = authService->getRepository();
        if (!repo || !repo->updateUser(*currentUser)) {
            toastActive = true;
            toastTimer = 3.f;
            toastMessage = L"Không thể lưu thông tin (lỗi ghi dữ liệu)!";
            return;
        }
    }
    toastActive = true;
    toastTimer = 3.f;
    toastMessage = L"Đã lưu và đồng bộ thông tin quản trị viên thành công!";
}

void AdminProfilePanel::changePassword() {
    if (oldPass.empty()) {
        toastActive = true;
        toastTimer = 2.5f;
        toastMessage = L"Vui lòng nhập mật khẩu hiện tại!";
        return;
    }
    if (!Validator::isStrongPassword(newPass)) {
        toastActive = true;
        toastTimer = 2.5f;
        toastMessage = L"Mật khẩu mới cần tối thiểu 8 ký tự gồm chữ hoa, chữ thường và số!";
        return;
    }
    if (newPass != confirmPass) {
        toastActive = true;
        toastTimer = 2.5f;
        toastMessage = L"Xác nhận mật khẩu mới không khớp!";
        return;
    }

    if (authService && currentUser) {
        if (!PasswordHasher::verifyPassword(oldPass, currentUser->getPasswordHash())) {
            toastActive = true;
            toastTimer = 2.5f;
            toastMessage = L"Mật khẩu cũ không chính xác!";
            return;
        }
        string oldHash = currentUser->getPasswordHash();
        currentUser->setPasswordHash(PasswordHasher::hashPassword(newPass));
        auto* repo = authService->getRepository();
        if (!repo || !repo->updateUser(*currentUser)) {
            currentUser->setPasswordHash(oldHash);
            toastActive = true;
            toastTimer = 3.f;
            toastMessage = L"Không thể lưu mật khẩu mới (lỗi ghi dữ liệu)!";
            return;
        }
    }

    oldPass = "";
    newPass = "";
    confirmPass = "";
    toastActive = true;
    toastTimer = 3.f;
    toastMessage = L"Đã cập nhật mật khẩu mới thành công!";
}

void AdminProfilePanel::handleEvent(const Event& event, const RenderWindow& window) {
    if (showDeleteConfirm) {
        if (auto mousePress = event.getIf<Event::MouseButtonPressed>()) {
            if (mousePress->button == Mouse::Button::Left) {
                Vector2f mPos = window.mapPixelToCoords(Mouse::getPosition(window));
                float dW = 460.f;
                float dH = 220.f;
                Vector2f dPos(position.x + (width - dW) / 2.f, position.y + (height - dH) / 2.f);

                // Cancel button (khớp drawDeleteModal: dPos.x + dW - 220.f, dPos.y + dH - 52.f)
                FloatRect cancelRect({dPos.x + dW - 220.f, dPos.y + dH - 52.f}, {95.f, 36.f});
                if (cancelRect.contains(mPos)) {
                    showDeleteConfirm = false;
                    return;
                }

                // Confirm delete button (khớp drawDeleteModal: dPos.x + dW - 110.f, dPos.y + dH - 52.f)
                FloatRect confirmRect({dPos.x + dW - 110.f, dPos.y + dH - 52.f}, {95.f, 36.f});
                if (confirmRect.contains(mPos)) {
                    showDeleteConfirm = false;
                    toastActive = true;
                    toastTimer = 3.f;
                    if (authService && currentUser && authService->deleteAccount(currentUser->getEmail())) {
                        currentUser = nullptr;  // pointer was invalidated by the delete
                        if (onAccountDeleted) onAccountDeleted();  // returns to login screen (session cleared)
                    } else {
                        toastMessage = L"Không thể xóa tài khoản (lỗi ghi dữ liệu)!";
                    }
                    return;
                }
            }
        } else if (const auto* keyEv = event.getIf<Event::KeyPressed>()) {
            if (keyEv->code == Keyboard::Key::Escape) {
                showDeleteConfirm = false;
            }
        }
        return;
    }

    if (auto mousePress = event.getIf<Event::MouseButtonPressed>()) {
        if (mousePress->button == Mouse::Button::Left) {
            Vector2f mPos = window.mapPixelToCoords(Mouse::getPosition(window));

            const float colGap = 18.f;
            const float colW = (width - 40.f - colGap) / 2.f;

            // 1. Delete Account button (khớp drawHeaderBanner: cardX + cardW - 254.f, cardY + 40.f)
            FloatRect delRect({position.x + width - 274.f, position.y + 60.f}, {230.f, 42.f});
            if (delRect.contains(mPos)) {
                showDeleteConfirm = true;
                return;
            }

            // 2. Left Card Form Inputs (khớp drawPersonalInfoCard: cardY = position.y + 158.f)
            float leftCardX = position.x + 20.f;
            float leftCardY = position.y + 158.f;
            float inW = (colW - 48.f - 18.f) / 2.f;
            float inH = 42.f;

            // Row 1: Họ tên (Field 1) & Ngày sinh (Field 2)
            FloatRect f1Rect({leftCardX + 24.f, leftCardY + 96.f}, {inW, inH});
            if (f1Rect.contains(mPos)) { activeField = 1; return; }

            FloatRect f2Rect({leftCardX + 24.f + inW + 18.f, leftCardY + 96.f}, {inW, inH});
            if (f2Rect.contains(mPos)) { activeField = 2; return; }

            // Row 2: Số điện thoại (Field 3)
            FloatRect f3Rect({leftCardX + 24.f, leftCardY + 174.f}, {inW, inH});
            if (f3Rect.contains(mPos)) { activeField = 3; return; }

            // Row 3: Chức vụ (Field 4) & Cơ sở (Field 5)
            FloatRect f4Rect({leftCardX + 24.f, leftCardY + 252.f}, {inW, inH});
            if (f4Rect.contains(mPos)) { activeField = 4; return; }

            FloatRect f5Rect({leftCardX + 24.f + inW + 18.f, leftCardY + 252.f}, {inW, inH});
            if (f5Rect.contains(mPos)) { activeField = 5; return; }

            // Save Info Button
            FloatRect saveRect({leftCardX + 24.f, leftCardY + 386.f}, {185.f, 44.f});
            if (saveRect.contains(mPos)) {
                saveUserData();
                return;
            }

            // 3. Right Card Password Inputs (khớp drawPasswordCard: cardY = position.y + 158.f)
            float rightCardX = leftCardX + colW + colGap;
            float rightCardY = position.y + 158.f;
            float passW = colW - 48.f;

            // Field 6: Mật khẩu cũ (y = cardY + 74, box at cardY + 96)
            FloatRect f6Rect({rightCardX + 24.f, rightCardY + 96.f}, {passW, inH});
            if (f6Rect.contains(mPos)) {
                if (mPos.x > rightCardX + 24.f + passW - 40.f) {
                    showOldPass = !showOldPass;
                } else {
                    activeField = 6;
                }
                return;
            }

            // Field 7: Mật khẩu mới (y = cardY + 150, box at cardY + 172)
            FloatRect f7Rect({rightCardX + 24.f, rightCardY + 172.f}, {passW, inH});
            if (f7Rect.contains(mPos)) {
                if (mPos.x > rightCardX + 24.f + passW - 40.f) {
                    showNewPass = !showNewPass;
                } else {
                    activeField = 7;
                }
                return;
            }

            // Field 8: Xác thực mật khẩu (y = cardY + 268, box at cardY + 290)
            FloatRect f8Rect({rightCardX + 24.f, rightCardY + 290.f}, {passW, inH});
            if (f8Rect.contains(mPos)) {
                if (mPos.x > rightCardX + 24.f + passW - 40.f) {
                    showConfirmPass = !showConfirmPass;
                } else {
                    activeField = 8;
                }
                return;
            }

            // Change Password Button
            FloatRect chgRect({rightCardX + 24.f, rightCardY + 434.f}, {185.f, 44.f});
            if (chgRect.contains(mPos)) {
                changePassword();
                return;
            }

            activeField = 0;
        }
    }

    if (const auto* keyEv = event.getIf<Event::KeyPressed>()) {
        if (keyEv->code == Keyboard::Key::Backspace) {
            auto popUtf8 = [](string& s) {
                while (!s.empty() && (static_cast<unsigned char>(s.back()) & 0xC0) == 0x80) s.pop_back();
                if (!s.empty()) s.pop_back();
            };
            if (activeField == 1) popUtf8(fullName);
            else if (activeField == 2) popUtf8(birthDate);
            else if (activeField == 3) popUtf8(phone);
            else if (activeField == 4) popUtf8(department);
            else if (activeField == 5) popUtf8(location);
            else if (activeField == 6 && !oldPass.empty()) oldPass.pop_back();
            else if (activeField == 7 && !newPass.empty()) newPass.pop_back();
            else if (activeField == 8 && !confirmPass.empty()) confirmPass.pop_back();
        } else if (keyEv->code == Keyboard::Key::Tab) {
            if (activeField >= 1 && activeField <= 8) activeField = (activeField % 8) + 1;
        } else if (keyEv->code == Keyboard::Key::Escape) {
            activeField = 0;
        } else if (keyEv->code == Keyboard::Key::Enter) {
            if (activeField >= 1 && activeField <= 5) saveUserData();
            else if (activeField >= 6 && activeField <= 8) changePassword();
        }
    }

    if (auto textEvt = event.getIf<Event::TextEntered>()) {
        if (textEvt->unicode >= 32 && textEvt->unicode != 127) {
            string u8 = UITheme::wstrToUtf8(wstring(1, static_cast<wchar_t>(textEvt->unicode)));
            if (activeField == 1 && fullName.size() < 60) fullName += u8;
            else if (activeField == 2 && birthDate.size() < 15) birthDate += u8;
            else if (activeField == 3 && phone.size() < 18) phone += u8;
            else if (activeField == 4 && department.size() < 60) department += u8;
            else if (activeField == 5 && location.size() < 60) location += u8;
            else if (textEvt->unicode < 127) {
                char c = static_cast<char>(textEvt->unicode);
                if (activeField == 6 && oldPass.size() < 30) oldPass.push_back(c);
                else if (activeField == 7 && newPass.size() < 30) newPass.push_back(c);
                else if (activeField == 8 && confirmPass.size() < 30) confirmPass.push_back(c);
            }
        }
    }
}

void AdminProfilePanel::update(const Vector2f& mousePos, bool /*mouseDown*/) {
    const float colGap = 18.f;
    const float colW = (width - 40.f - colGap) / 2.f;

    // Delete button hover
    deleteHovered = FloatRect({position.x + width - 274.f, position.y + 60.f}, {230.f, 42.f}).contains(mousePos);

    // Save button hover
    saveInfoHovered = FloatRect({position.x + 44.f, position.y + 158.f + 386.f}, {185.f, 44.f}).contains(mousePos);

    // Change pass button hover
    changePassHovered = FloatRect({position.x + 20.f + colW + colGap + 24.f, position.y + 158.f + 434.f}, {185.f, 44.f}).contains(mousePos);

    if (toastActive) {
        toastTimer -= 0.016f;
        if (toastTimer <= 0.f) toastActive = false;
    }
}

void AdminProfilePanel::render(RenderTarget& target) const {
    // 1. Off-white Canvas Background
    RectangleShape bg(Vector2f(width, height));
    bg.setPosition(position);
    bg.setFillColor(Color(241, 245, 249));
    target.draw(bg);

    // 2. Top Profile Banner Card
    drawHeaderBanner(target);

    // 3. Two-Column Layout Cards
    drawPersonalInfoCard(target);
    drawPasswordCard(target);

    // 4. Two-Factor Authentication Card
    drawTwoFactorCard(target);

    // 5. Bottom Engine Status Bar
    drawBottomBar(target);

    // 6. Delete Confirmation Modal
    if (showDeleteConfirm) {
        drawDeleteModal(target);
    }

    // 7. Toast Notification
    if (toastActive) {
        drawToast(target);
    }
}

void AdminProfilePanel::drawHeaderBanner(RenderTarget& target) const {
    const float cardX = position.x + 20.f;
    const float cardY = position.y + 20.f;
    const float cardW = width - 40.f;
    const float cardH = 122.f;

    // White Card Background
    RoundedRectRenderer::draw(target, {cardX, cardY}, {cardW, cardH}, 12.f, Color::White, 1.f, Color(226, 232, 240));

    // Large Square Avatar
    Vector2f avPos(cardX + 24.f, cardY + 21.f);
    RoundedRectRenderer::draw(target, avPos, {80.f, 80.f}, 16.f, Color(8, 125, 181));

    // Letter 'A' in Avatar
    Text avText(boldFont);
    avText.setString(L"A");
    avText.setCharacterSize(38);
    avText.setFillColor(Color::White);
    FloatRect ab = avText.getLocalBounds();
    avText.setPosition({avPos.x + 40.f - ab.size.x / 2.f - ab.position.x, avPos.y + 40.f - ab.size.y / 2.f - ab.position.y});
    target.draw(avText);

    // Green Online Badge on Avatar corner
    Vector2f chkPos(avPos.x + 80.f - 18.f, avPos.y + 80.f - 18.f);
    CircleShape chkCircle(10.5f);
    chkCircle.setPosition(chkPos);
    chkCircle.setFillColor(Color(21, 128, 61));
    chkCircle.setOutlineThickness(2.f);
    chkCircle.setOutlineColor(Color::White);
    target.draw(chkCircle);
    UITheme::drawCheckmark(target, {chkPos.x + 10.5f, chkPos.y + 10.5f}, 0.78f, Color::White, 2.f);

    // Line 1: Title + Role Badge + Active Pill
    float textX = avPos.x + 100.f;
    Text title(boldFont);
    title.setString(L"HỒ SƠ CỦA ADMIN");
    title.setCharacterSize(24);
    title.setFillColor(Color(23, 32, 51));
    title.setPosition({textX, cardY + 26.f});
    target.draw(title);

    float tW = title.getGlobalBounds().size.x;
    Vector2f bPos(textX + tW + 14.f, cardY + 26.f);
    Text bTxt(boldFont, L"Quản trị viên cấp cao", 15.f);
    bTxt.setFillColor(Color(8, 125, 181));
    float bPillW = bTxt.getGlobalBounds().size.x + 40.f;
    RoundedRectRenderer::draw(target, bPos, {bPillW, 28.f}, 14.f, Color(224, 242, 254), 1.f, Color(186, 230, 253));
    UITheme::drawShieldIcon(target, {bPos.x + 15.f, bPos.y + 14.f}, 6.f, Color(8, 125, 181));
    FloatRect btb = bTxt.getLocalBounds();
    bTxt.setPosition({bPos.x + 28.f, std::round(bPos.y + (28.f - btb.size.y) / 2.f - btb.position.y)});
    target.draw(bTxt);

    Vector2f stPos(bPos.x + bPillW + 12.f, cardY + 26.f);
    Text stTxt(boldFont, L"Đang hoạt động (Active)", 15.f);
    stTxt.setFillColor(Color(21, 128, 61));
    float stPillW = stTxt.getGlobalBounds().size.x + 44.f;
    RoundedRectRenderer::draw(target, stPos, {stPillW, 28.f}, 14.f, Color(220, 252, 231), 1.f, Color(167, 243, 208));
    CircleShape stDot(3.5f);
    stDot.setFillColor(Color(21, 128, 61));
    stDot.setPosition({stPos.x + 14.f, stPos.y + 10.5f});
    target.draw(stDot);
    FloatRect stb = stTxt.getLocalBounds();
    stTxt.setPosition({stPos.x + 26.f, std::round(stPos.y + (28.f - stb.size.y) / 2.f - stb.position.y)});
    target.draw(stTxt);

    // Line 2: Email • Vai trò • Đăng nhập gần nhất • IP pill (tăng 30% cỡ chữ lên 17px)
    Text meta(font);
    meta.setString(L"Email: " + s2ws(email) + L"   •   Vai trò: Quản trị viên hệ thống   •   Đăng nhập gần nhất: 10 phút trước   •   ");
    meta.setCharacterSize(17.f);
    meta.setFillColor(Color(100, 116, 139));
    meta.setPosition({textX, cardY + 66.f});
    target.draw(meta);

    float metaW = meta.getGlobalBounds().size.x;
    Text ipTxt(font, L"IP: 118.69.182.45 (Đà Nẵng)", 16.f);
    ipTxt.setFillColor(Color(71, 85, 105));
    float ipPillW = ipTxt.getGlobalBounds().size.x + 20.f;
    Vector2f ipPos(textX + metaW + 4.f, cardY + 63.f);
    RoundedRectRenderer::draw(target, ipPos, {ipPillW, 26.f}, 6.f, Color(241, 245, 249));
    FloatRect ipB = ipTxt.getLocalBounds();
    ipTxt.setPosition({std::round(ipPos.x + (ipPillW - ipB.size.x) / 2.f - ipB.position.x), std::round(ipPos.y + (26.f - ipB.size.y) / 2.f - ipB.position.y)});
    target.draw(ipTxt);

    // Danger Button: XÓA VĨNH VIỄN TÀI KHOẢN
    Vector2f delPos(cardX + cardW - 254.f, cardY + 40.f);
    Color delBg = deleteHovered ? Color(185, 28, 28) : Color(220, 38, 38);
    RoundedRectRenderer::draw(target, delPos, {230.f, 42.f}, 8.f, delBg);
    // Vector trash icon
    RectangleShape trHandle({4.f, 1.5f}); trHandle.setFillColor(Color::White); trHandle.setPosition({delPos.x + 16.f, delPos.y + 14.5f}); target.draw(trHandle);
    RectangleShape trLid({10.f, 1.5f}); trLid.setFillColor(Color::White); trLid.setPosition({delPos.x + 13.f, delPos.y + 16.5f}); target.draw(trLid);
    RoundedRectRenderer::draw(target, {delPos.x + 14.f, delPos.y + 19.f}, {8.f, 9.f}, 1.5f, Color::Transparent, 1.4f, Color::White);
    Text delTxt(boldFont, L"XÓA VĨNH VIỄN TÀI KHOẢN", 14.5f);
    delTxt.setFillColor(Color::White);
    FloatRect db = delTxt.getLocalBounds();
    delTxt.setPosition({delPos.x + 32.f, delPos.y + (42.f - db.size.y) / 2.f - db.position.y});
    target.draw(delTxt);
}

void AdminProfilePanel::drawPersonalInfoCard(RenderTarget& target) const {
    const float colGap = 18.f;
    const float cardW = (width - 40.f - colGap) / 2.f;
    const float cardX = position.x + 20.f;
    const float cardY = position.y + 158.f;
    const float cardH = 448.f;

    // Card Background
    RoundedRectRenderer::draw(target, {cardX, cardY}, {cardW, cardH}, 12.f, Color::White, 1.f, Color(226, 232, 240));

    // Header Icon (vector ID Card icon)
    Vector2f icPos(cardX + 24.f, cardY + 20.f);
    RoundedRectRenderer::draw(target, icPos, {34.f, 34.f}, 8.f, Color(224, 242, 254));
    UITheme::drawIdCardIcon(target, {icPos.x + 17.f, icPos.y + 17.f}, Color(8, 125, 181));

    // Title
    Text title(boldFont, L"THÔNG TIN CÁ NHÂN", 16.f);
    title.setFillColor(Color(23, 32, 51));
    title.setPosition({cardX + 68.f, cardY + 16.f});
    target.draw(title);

    // Subtitle
    Text sub(font, L"Cập nhật hồ sơ và các thông tin liên lạc hành chính", 14.5f);
    sub.setFillColor(Color(100, 116, 139));
    sub.setPosition({cardX + 68.f, cardY + 38.f});
    target.draw(sub);

    // ID Pill on the right
    Vector2f idPillSize(148.f, 28.f);
    Vector2f idPillPos(cardX + cardW - 24.f - idPillSize.x, cardY + 20.f);
    RoundedRectRenderer::draw(target, idPillPos, idPillSize, 6.f, Color(248, 250, 252), 1.f, Color(226, 232, 240));
    Text idTxt(boldFont, L"ID: ADM-2024-001", 15.f);
    idTxt.setFillColor(Color(100, 116, 139));
    FloatRect idb = idTxt.getLocalBounds();
    idTxt.setPosition({idPillPos.x + (idPillSize.x - idb.size.x) / 2.f - idb.position.x, idPillPos.y + (idPillSize.y - idb.size.y) / 2.f - idb.position.y});
    target.draw(idTxt);

    // Form Inputs (2-column layout)
    float inW = (cardW - 48.f - 18.f) / 2.f;
    float inH = 42.f;

    auto drawInputBox = [&](float x, float y, const wstring& label, bool required, const string& val, bool isAct, bool hasVerifiedBadge) {
        Text lbl(boldFont, label, 15.5f);
        lbl.setFillColor(Color(51, 65, 85));
        lbl.setPosition({x, y});
        target.draw(lbl);

        if (required) {
            FloatRect lb = lbl.getLocalBounds();
            Text star(boldFont, L"*", 15.5f);
            star.setFillColor(Color(220, 38, 38));
            star.setPosition({x + lb.size.x + 3.f, y});
            target.draw(star);
        }

        if (hasVerifiedBadge) {
            float badgeStartX = x + inW - 106.f;
            UITheme::drawCheckmark(target, {badgeStartX + 4.f, y + 8.f}, 0.76f, Color(21, 128, 61), 2.f);
            Text vBadge(boldFont, L"Đã xác minh", 14.5f);
            vBadge.setFillColor(Color(21, 128, 61));
            vBadge.setPosition({badgeStartX + 15.f, y});
            target.draw(vBadge);
        }

        Vector2f bPos(x, y + 22.f);
        Color borderCol = isAct ? Color(8, 125, 181) : Color(226, 232, 240);
        RoundedRectRenderer::draw(target, bPos, {inW, inH}, 6.f, Color::White, 1.f, borderCol);

        Text vTxt(font, s2ws(val) + (isAct ? L"|" : L""), 15.5f);
        vTxt.setFillColor(Color(30, 41, 59));
        FloatRect vb = vTxt.getLocalBounds();
        vTxt.setPosition({bPos.x + 12.f, std::round(bPos.y + (inH - vb.size.y) / 2.f - vb.position.y)});
        target.draw(vTxt);
    };

    // Row 1: Họ tên (Field 1) & Ngày sinh (Field 2)
    drawInputBox(cardX + 24.f, cardY + 74.f, L"Họ và tên", true, fullName, activeField == 1, false);
    drawInputBox(cardX + 24.f + inW + 18.f, cardY + 74.f, L"Ngày sinh", true, birthDate, activeField == 2, false);

    // Row 2: Số điện thoại (Field 3) & Email hệ thống
    drawInputBox(cardX + 24.f, cardY + 152.f, L"Số điện thoại", true, phone, activeField == 3, false);
    drawInputBox(cardX + 24.f + inW + 18.f, cardY + 152.f, L"Email hệ thống", true, email, false, true);

    // Row 3: Chức vụ (Field 4) & Cơ sở trực thuộc (Field 5)
    drawInputBox(cardX + 24.f, cardY + 230.f, L"Chức vụ / Bộ phận", false, department, activeField == 4, false);
    drawInputBox(cardX + 24.f + inW + 18.f, cardY + 230.f, L"Cơ sở trực thuộc", false, location, activeField == 5, false);

    // Audit Log Notice Box
    Vector2f notPos(cardX + 24.f, cardY + 310.f);
    RoundedRectRenderer::draw(target, notPos, {cardW - 48.f, 58.f}, 8.f, Color(248, 250, 252), 1.f, Color(226, 232, 240));
    CircleShape infoBg(8.5f, 16);
    infoBg.setFillColor(Color(8, 125, 181));
    infoBg.setPosition({notPos.x + 14.f, notPos.y + 20.f});
    target.draw(infoBg);
    Text notIc(boldFont, L"i", 14);
    notIc.setFillColor(Color::White);
    notIc.setPosition({notPos.x + 20.8f, notPos.y + 21.f});
    target.draw(notIc);

    Text notTxt(font, L"Mọi thay đổi thông tin cá nhân sẽ được lưu vào Nhật ký kiểm toán hệ thống (Audit Log)\ntheo tiêu chuẩn ISO 27001.", 14.5f);
    notTxt.setFillColor(Color(71, 85, 105));
    notTxt.setPosition({notPos.x + 40.f, notPos.y + 10.f});
    target.draw(notTxt);

    // Save Button: LƯU THÔNG TIN
    Vector2f savePos(cardX + 24.f, cardY + 386.f);
    Color saveBg = saveInfoHovered ? Color(18, 59, 93) : Color(8, 125, 181);
    RoundedRectRenderer::draw(target, savePos, {185.f, 44.f}, 8.f, saveBg);
    Text saveTxt(boldFont, L"LƯU THÔNG TIN", 15.5f);
    saveTxt.setFillColor(Color::White);
    FloatRect sb = saveTxt.getLocalBounds();
    saveTxt.setPosition({std::round(savePos.x + (185.f - sb.size.x) / 2.f - sb.position.x), std::round(savePos.y + (44.f - sb.size.y) / 2.f - sb.position.y)});
    target.draw(saveTxt);
}

void AdminProfilePanel::drawPasswordCard(RenderTarget& target) const {
    const float colGap = 18.f;
    const float cardW = (width - 40.f - colGap) / 2.f;
    const float cardX = position.x + 20.f + cardW + colGap;
    const float cardY = position.y + 158.f;
    const float cardH = 496.f;

    // Card Background
    RoundedRectRenderer::draw(target, {cardX, cardY}, {cardW, cardH}, 12.f, Color::White, 1.f, Color(226, 232, 240));

    // Header Icon (vector Key icon)
    Vector2f icPos(cardX + 24.f, cardY + 20.f);
    RoundedRectRenderer::draw(target, icPos, {34.f, 34.f}, 8.f, Color(254, 243, 199));
    UITheme::drawKeyIcon(target, {icPos.x + 17.f, icPos.y + 17.f}, Color(217, 119, 6));

    // Title
    Text title(boldFont, L"ĐỔI MẬT KHẨU", 16.f);
    title.setFillColor(Color(23, 32, 51));
    title.setPosition({cardX + 68.f, cardY + 16.f});
    target.draw(title);

    // Subtitle
    Text sub(font, L"Nên đổi mật khẩu định kỳ 90 ngày để đảm bảo an toàn", 14.5f);
    sub.setFillColor(Color(100, 116, 139));
    sub.setPosition({cardX + 68.f, cardY + 38.f});
    target.draw(sub);

    // Security Level on Right
    Text secTxt(boldFont, L"Bảo mật cấp 3", 15.f);
    secTxt.setFillColor(Color(100, 116, 139));
    secTxt.setPosition({cardX + cardW - 24.f - secTxt.getGlobalBounds().size.x, cardY + 22.f});
    target.draw(secTxt);

    // Password Fields
    float passW = cardW - 48.f;
    float passH = 42.f;

    auto drawPassInput = [&](float y, const wstring& lbl, const string& val, const wstring& placeholder, bool isAct, bool showEye) {
        Text l(boldFont, lbl, 15.5f);
        l.setFillColor(Color(51, 65, 85));
        l.setPosition({cardX + 24.f, y});
        target.draw(l);

        FloatRect lb = l.getLocalBounds();
        Text star(boldFont, L"*", 15.5f);
        star.setFillColor(Color(220, 38, 38));
        star.setPosition({cardX + 24.f + lb.size.x + 3.f, y});
        target.draw(star);

        Vector2f bPos(cardX + 24.f, y + 22.f);
        Color borderCol = isAct ? Color(8, 125, 181) : Color(226, 232, 240);
        RoundedRectRenderer::draw(target, bPos, {passW, passH}, 6.f, Color::White, 1.f, borderCol);

        UITheme::drawLockIcon(target, {bPos.x + 18.f, bPos.y + 21.f}, isAct ? Color(8, 125, 181) : Color(148, 163, 184));

        wstring displayStr;
        if (val.empty()) {
            displayStr = isAct ? L"|" : placeholder;
        } else if (showEye) {
            displayStr = s2ws(val) + (isAct ? L"|" : L"");
        } else {
            displayStr = wstring(val.size(), L'●') + (isAct ? L"|" : L"");
        }

        Text vTxt((val.empty() && !isAct) ? font : boldFont, displayStr, 15.5f);
        vTxt.setFillColor((val.empty() && !isAct) ? Color(148, 163, 184) : Color(23, 32, 51));
        FloatRect vb = vTxt.getLocalBounds();
        vTxt.setPosition({bPos.x + 36.f, std::round(bPos.y + (passH - vb.size.y) / 2.f - vb.position.y)});
        target.draw(vTxt);

        // Clean light-gray vector eye icon
        Color eyeCol = showEye ? Color(8, 125, 181) : Color(148, 163, 184);
        Vector2f ePos(bPos.x + passW - 30.f, bPos.y + 16.f);
        RoundedRectRenderer::draw(target, ePos, {15.f, 10.f}, 5.f, Color::Transparent, 1.3f, eyeCol);
        CircleShape pupil(2.2f, 12);
        pupil.setFillColor(eyeCol);
        pupil.setPosition({ePos.x + 5.3f, ePos.y + 2.8f});
        target.draw(pupil);
    };

    // 1. Mật khẩu cũ (Field 6)
    drawPassInput(cardY + 74.f, L"Mật khẩu cũ", oldPass, L"Nhập mật khẩu cũ", activeField == 6, showOldPass);

    // 2. Mật khẩu mới (Field 7)
    drawPassInput(cardY + 150.f, L"Mật khẩu mới", newPass, L"Nhập mật khẩu mới", activeField == 7, showNewPass);

    // Password Strength Meter
    float meterY = cardY + 226.f;
    Text sLbl(font, L"Độ mạnh mật khẩu:", 15.f);
    sLbl.setFillColor(Color(100, 116, 139));
    sLbl.setPosition({cardX + 24.f, meterY});
    target.draw(sLbl);

    Text sVal(boldFont, L"Rất mạnh", 15.f);
    sVal.setFillColor(Color(21, 128, 61));
    sVal.setPosition({cardX + cardW - 24.f - sVal.getGlobalBounds().size.x, meterY});
    target.draw(sVal);

    float barGap = 8.f;
    float segW = (passW - 3 * barGap) / 4.f;
    for (int i = 0; i < 4; ++i) {
        Vector2f segPos(cardX + 24.f + i * (segW + barGap), meterY + 22.f);
        RoundedRectRenderer::draw(target, segPos, {segW, 8.f}, 4.f, Color(21, 128, 61));
    }

    // 3. Xác thực mật khẩu (Field 8)
    drawPassInput(cardY + 268.f, L"Xác thực mật khẩu", confirmPass, L"Xác nhận mật khẩu mới", activeField == 8, showConfirmPass);

    // Password Rules Checklist (2x2 grid)
    float ruleY = cardY + 346.f;
    Text rTitle(boldFont, L"Quy định mật khẩu CineCore:", 15.f);
    rTitle.setFillColor(Color(71, 85, 105));
    rTitle.setPosition({cardX + 24.f, ruleY});
    target.draw(rTitle);

    auto drawRuleItem = [&](float x, float y, const wstring& rTxt, bool checked) {
        CircleShape dot(3.5f);
        dot.setPosition({x + 2.f, y + 6.f});
        if (checked) {
            dot.setFillColor(Color(21, 128, 61));
        } else {
            dot.setFillColor(Color::Transparent);
            dot.setOutlineThickness(1.3f);
            dot.setOutlineColor(Color(148, 163, 184));
        }
        target.draw(dot);

        Text t(font, rTxt, 14.5f);
        t.setFillColor(Color(100, 116, 139));
        t.setPosition({x + 16.f, y});
        target.draw(t);
    };

    drawRuleItem(cardX + 24.f, ruleY + 26.f, L"Tối thiểu 8 ký tự", true);
    drawRuleItem(cardX + 24.f + passW / 2.f, ruleY + 26.f, L"Ký tự chữ hoa & số", true);
    drawRuleItem(cardX + 24.f, ruleY + 50.f, L"Ít nhất 1 ký tự đặc biệt", true);
    drawRuleItem(cardX + 24.f + passW / 2.f, ruleY + 50.f, L"Khác 3 mật khẩu cũ", false);

    // Change Password Button: ĐỔI MẬT KHẨU
    Vector2f chgPos(cardX + 24.f, cardY + 434.f);
    Color chgBg = changePassHovered ? Color(18, 59, 93) : Color(8, 125, 181);
    RoundedRectRenderer::draw(target, chgPos, {185.f, 44.f}, 8.f, chgBg);
    Text chgTxt(boldFont, L"ĐỔI MẬT KHẨU", 15.5f);
    chgTxt.setFillColor(Color::White);
    FloatRect cb = chgTxt.getLocalBounds();
    chgTxt.setPosition({std::round(chgPos.x + (185.f - cb.size.x) / 2.f - cb.position.x), std::round(chgPos.y + (44.f - cb.size.y) / 2.f - cb.position.y)});
    target.draw(chgTxt);
}

void AdminProfilePanel::drawTwoFactorCard(RenderTarget& target) const {
    const float colGap = 18.f;
    const float cardW = (width - 40.f - colGap) / 2.f;
    const float cardX = position.x + 20.f + cardW + colGap;
    const float cardY = position.y + 668.f;
    const float cardH = 68.f;

    // White Card Background
    RoundedRectRenderer::draw(target, {cardX, cardY}, {cardW, cardH}, 10.f, Color::White, 1.f, Color(226, 232, 240));

    // Shield Icon (vector)
    Vector2f icPos(cardX + 18.f, cardY + 17.f);
    CircleShape ic(17.f);
    ic.setPosition(icPos);
    ic.setFillColor(Color(220, 252, 231));
    target.draw(ic);
    UITheme::drawShieldIcon(target, {icPos.x + 17.f, icPos.y + 17.f}, 8.f, Color(21, 128, 61));
    UITheme::drawCheckmark(target, {icPos.x + 17.f, icPos.y + 17.f}, 0.58f, Color::White, 2.f);

    // Title & Subtitle
    Text title(boldFont, L"Xác thực 2 yếu tố (2FA Google Authenticator)", 16.f);
    title.setFillColor(Color(23, 32, 51));
    title.setPosition({cardX + 64.f, cardY + 13.f});
    target.draw(title);

    Text sub(font, L"Bảo vệ tài khoản qua mã OTP ứng dụng điện thoại", 14.5f);
    sub.setFillColor(Color(100, 116, 139));
    sub.setPosition({cardX + 64.f, cardY + 36.f});
    target.draw(sub);

    // Badge: ● ĐÃ BẬT
    Vector2f bPos(cardX + cardW - 120.f, cardY + 20.f);
    RoundedRectRenderer::draw(target, bPos, {100.f, 28.f}, 14.f, Color(220, 252, 231), 1.f, Color(167, 243, 208));
    CircleShape bDot(3.5f);
    bDot.setFillColor(Color(21, 128, 61));
    bDot.setPosition({bPos.x + 12.f, bPos.y + 10.5f});
    target.draw(bDot);
    Text bTxt(boldFont, L"ĐÃ BẬT", 14.5f);
    bTxt.setFillColor(Color(21, 128, 61));
    bTxt.setPosition({bPos.x + 24.f, bPos.y + 4.f});
    target.draw(bTxt);
}

void AdminProfilePanel::drawDeleteModal(RenderTarget& target) const {
    // Dark Backdrop
    RectangleShape dim(Vector2f(width, height));
    dim.setPosition(position);
    dim.setFillColor(Color(15, 23, 42, 170));
    target.draw(dim);

    float dW = 480.f;
    float dH = 220.f;
    Vector2f dPos(position.x + (width - dW) / 2.f, position.y + (height - dH) / 2.f);

    RoundedRectRenderer::draw(target, dPos, {dW, dH}, 10.f, Color::White, 1.f, Color(226, 232, 240));

    Text title(boldFont);
    title.setString(L"XÁC NHẬN XÓA TÀI KHOẢN ADMIN");
    title.setCharacterSize(18);
    title.setFillColor(Color(220, 38, 38));
    title.setPosition({dPos.x + 24.f, dPos.y + 20.f});
    target.draw(title);

    Text msg(font);
    msg.setString(L"Bạn có chắc chắn muốn xóa vĩnh viễn tài khoản quản trị này?\nHành động này không thể hoàn tác!");
    msg.setCharacterSize(16);
    msg.setFillColor(Color(51, 65, 85));
    msg.setPosition({dPos.x + 24.f, dPos.y + 58.f});
    target.draw(msg);

    // Cancel Button
    Vector2f canPos(dPos.x + dW - 230.f, dPos.y + dH - 52.f);
    RoundedRectRenderer::draw(target, canPos, {95.f, 36.f}, 6.f, Color::White, 1.f, Color(203, 213, 225));
    Text canTxt(boldFont);
    canTxt.setString(L"Hủy bỏ");
    canTxt.setCharacterSize(16);
    canTxt.setFillColor(Color(71, 85, 105));
    FloatRect cb = canTxt.getLocalBounds();
    canTxt.setPosition({std::round(canPos.x + (95.f - cb.size.x) / 2.f - cb.position.x), std::round(canPos.y + (36.f - cb.size.y) / 2.f - cb.position.y)});
    target.draw(canTxt);

    // Confirm Delete Button
    Vector2f delPos(dPos.x + dW - 120.f, dPos.y + dH - 52.f);
    RoundedRectRenderer::draw(target, delPos, {105.f, 36.f}, 6.f, Color(220, 38, 38));
    Text delTxt(boldFont);
    delTxt.setString(L"Xác nhận");
    delTxt.setCharacterSize(16);
    delTxt.setFillColor(Color::White);
    FloatRect db = delTxt.getLocalBounds();
    delTxt.setPosition({std::round(delPos.x + (105.f - db.size.x) / 2.f - db.position.x), std::round(delPos.y + (36.f - db.size.y) / 2.f - db.position.y)});
    target.draw(delTxt);
}

void AdminProfilePanel::drawBottomBar(RenderTarget& target) const {
    float barY = height - 28.f;

    RectangleShape div(Vector2f(width - 40.f, 1.f));
    div.setPosition({position.x + 20.f, barY - 4.f});
    div.setFillColor(Color(226, 232, 240));
    target.draw(div);

    Text authText(font);
    authText.setString(L"Phụ trách: Admin CineCore (Toàn quyền)  •  1728x972 HD");
    authText.setCharacterSize(13.5f);
    authText.setFillColor(Color(100, 116, 139));
    FloatRect ab = authText.getLocalBounds();
    authText.setPosition({position.x + width - ab.size.x - 20.f, barY});
    target.draw(authText);
}

void AdminProfilePanel::drawToast(RenderTarget& target) const {
    Text toast(boldFont);
    toast.setString(toastMessage);
    toast.setCharacterSize(14);
    toast.setFillColor(Color::White);
    FloatRect tb = toast.getLocalBounds();

    float tw = max(380.f, min(width - 80.f, tb.size.x + 48.f));
    float th = 42.f;
    Vector2f tPos(position.x + (width - tw) / 2.f, position.y + height - 60.f);

    RoundedRectRenderer::draw(target, tPos, {tw, th}, 8.f, Color(22, 163, 74, 245), 1.5f, Color(21, 128, 61));
    toast.setPosition({std::round(tPos.x + (tw - tb.size.x) / 2.f - tb.position.x), std::round(tPos.y + (th - tb.size.y) / 2.f - tb.position.y)});
    target.draw(toast);
}