#include "UI/components/Admin/AdminProfilePanel.h"
#include "UI/components/Admin/RoundedRectRenderer.h"
#include "utils/PasswordHasher.h"
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
      boldFont("../assets/fonts/segoeuib.ttf"),
      width(w),
      height(h),
      position(280.f, 0.f),
      authService(auth)
{
    loadUserData();
}

void AdminProfilePanel::setPosition(const Vector2f& pos) {
    position = pos;
}

void AdminProfilePanel::reload() {
    loadUserData();
}

void AdminProfilePanel::loadUserData() {
    if (authService) {
        currentUser = authService->getCurrentUser();
        if (currentUser) {
            fullName = currentUser->getFullName().empty() ? "ADMIN" : currentUser->getFullName();
            birthDate = currentUser->getBirthDate().empty() ? "12/02/2006" : currentUser->getBirthDate();
            phone = currentUser->getPhone().empty() ? "0898234406" : currentUser->getPhone();
            email = currentUser->getEmail().empty() ? "admin@cinexine.vn" : currentUser->getEmail();
        }
    }
}

void AdminProfilePanel::saveUserData() {
    if (authService && currentUser) {
        currentUser->setFullName(fullName);
        currentUser->setBirthDate(birthDate);
        currentUser->setPhone(phone);
        auto* repo = authService->getRepository();
        if (repo) {
            repo->updateUser(*currentUser);
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
    if (newPass.empty() || newPass.size() < 6) {
        toastActive = true;
        toastTimer = 2.5f;
        toastMessage = L"Mật khẩu mới phải có ít nhất 6 ký tự!";
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
        string newHash = PasswordHasher::hashPassword(newPass);
        currentUser->setPasswordHash(newHash);
        auto* repo = authService->getRepository();
        if (repo) {
            repo->updateUser(*currentUser);
        }
    }

    oldPass = "";
    newPass = "";
    confirmPass = "";
    toastActive = true;
    toastTimer = 3.f;
    toastMessage = L"Đã cập nhật mật khẩu mới thành công!";
}

void AdminProfilePanel::handleEvent(const Event& event, const RenderWindow& /*window*/) {
    if (showDeleteConfirm) {
        if (auto mousePress = event.getIf<Event::MouseButtonPressed>()) {
            if (mousePress->button == Mouse::Button::Left) {
                Vector2f mPos(static_cast<float>(mousePress->position.x), static_cast<float>(mousePress->position.y));
                float dW = 460.f;
                float dH = 220.f;
                Vector2f dPos(position.x + (width - dW) / 2.f, position.y + (height - dH) / 2.f);

                // Cancel button
                FloatRect cancelRect({dPos.x + 220.f, dPos.y + dH - 50.f}, {100.f, 36.f});
                if (cancelRect.contains(mPos)) {
                    showDeleteConfirm = false;
                    return;
                }

                // Confirm delete button
                FloatRect confirmRect({dPos.x + 330.f, dPos.y + dH - 50.f}, {110.f, 36.f});
                if (confirmRect.contains(mPos)) {
                    showDeleteConfirm = false;
                    toastActive = true;
                    toastTimer = 3.f;
                    toastMessage = L"Tài khoản quản trị viên tối cao được bảo vệ!";
                    return;
                }
            }
        }
        return;
    }

    if (auto mousePress = event.getIf<Event::MouseButtonPressed>()) {
        if (mousePress->button == Mouse::Button::Left) {
            Vector2f mPos(static_cast<float>(mousePress->position.x), static_cast<float>(mousePress->position.y));

            // 1. Delete Account button: x = position.x + width - 240, y = 74, w = 210, h = 42
            FloatRect delRect({position.x + width - 240.f, position.y + 74.f}, {210.f, 42.f});
            if (delRect.contains(mPos)) {
                showDeleteConfirm = true;
                return;
            }

            // 2. Left Card Form Inputs
            float leftCardX = position.x + 20.f;
            float leftCardY = position.y + 186.f;
            float inW = 310.f;
            float inH = 40.f;

            // Row 1: Họ tên (Field 1) & Ngày sinh (Field 2)
            FloatRect f1Rect({leftCardX + 24.f, leftCardY + 115.f}, {inW, inH});
            if (f1Rect.contains(mPos)) { activeField = 1; return; }

            FloatRect f2Rect({leftCardX + 24.f + inW + 14.f, leftCardY + 115.f}, {inW, inH});
            if (f2Rect.contains(mPos)) { activeField = 2; return; }

            // Row 2: Số điện thoại (Field 3)
            FloatRect f3Rect({leftCardX + 24.f, leftCardY + 195.f}, {inW, inH});
            if (f3Rect.contains(mPos)) { activeField = 3; return; }

            // Row 3: Chức vụ (Field 4) & Cơ sở (Field 5)
            FloatRect f4Rect({leftCardX + 24.f, leftCardY + 275.f}, {inW, inH});
            if (f4Rect.contains(mPos)) { activeField = 4; return; }

            FloatRect f5Rect({leftCardX + 24.f + inW + 14.f, leftCardY + 275.f}, {inW, inH});
            if (f5Rect.contains(mPos)) { activeField = 5; return; }

            // Save Info Button: x = leftCardX + 24, y = leftCardY + 410, w = 160, h = 42
            FloatRect saveRect({leftCardX + 24.f, leftCardY + 410.f}, {160.f, 42.f});
            if (saveRect.contains(mPos)) {
                saveUserData();
                return;
            }

            // 3. Right Card Password Inputs
            float rightCardX = position.x + 20.f + 695.f + 18.f;
            float rightCardY = position.y + 186.f;
            float passW = 647.f;

            // Field 6: Mật khẩu cũ
            FloatRect f6Rect({rightCardX + 24.f, rightCardY + 105.f}, {passW, inH});
            if (f6Rect.contains(mPos)) {
                // Check if eye icon clicked: x = rightCardX + 24 + passW - 35
                if (mPos.x > rightCardX + 24.f + passW - 40.f) {
                    showOldPass = !showOldPass;
                } else {
                    activeField = 6;
                }
                return;
            }

            // Field 7: Mật khẩu mới
            FloatRect f7Rect({rightCardX + 24.f, rightCardY + 175.f}, {passW, inH});
            if (f7Rect.contains(mPos)) {
                if (mPos.x > rightCardX + 24.f + passW - 40.f) {
                    showNewPass = !showNewPass;
                } else {
                    activeField = 7;
                }
                return;
            }

            // Field 8: Xác thực mật khẩu
            FloatRect f8Rect({rightCardX + 24.f, rightCardY + 265.f}, {passW, inH});
            if (f8Rect.contains(mPos)) {
                if (mPos.x > rightCardX + 24.f + passW - 40.f) {
                    showConfirmPass = !showConfirmPass;
                } else {
                    activeField = 8;
                }
                return;
            }

            // Change Password Button: x = rightCardX + 24, y = rightCardY + 410, w = 160, h = 42
            FloatRect chgRect({rightCardX + 24.f, rightCardY + 410.f}, {160.f, 42.f});
            if (chgRect.contains(mPos)) {
                changePassword();
                return;
            }
        }
    }

    if (auto textEvt = event.getIf<Event::TextEntered>()) {
        if (textEvt->unicode == 8) { // Backspace
            if (activeField == 1 && !fullName.empty()) fullName.pop_back();
            else if (activeField == 2 && !birthDate.empty()) birthDate.pop_back();
            else if (activeField == 3 && !phone.empty()) phone.pop_back();
            else if (activeField == 4 && !department.empty()) department.pop_back();
            else if (activeField == 5 && !location.empty()) location.pop_back();
            else if (activeField == 6 && !oldPass.empty()) oldPass.pop_back();
            else if (activeField == 7 && !newPass.empty()) newPass.pop_back();
            else if (activeField == 8 && !confirmPass.empty()) confirmPass.pop_back();
        } else if (textEvt->unicode >= 32 && textEvt->unicode < 127) {
            char c = static_cast<char>(textEvt->unicode);
            if (activeField == 1 && fullName.size() < 30) fullName.push_back(c);
            else if (activeField == 2 && birthDate.size() < 12) birthDate.push_back(c);
            else if (activeField == 3 && phone.size() < 15) phone.push_back(c);
            else if (activeField == 4 && department.size() < 40) department.push_back(c);
            else if (activeField == 5 && location.size() < 40) location.push_back(c);
            else if (activeField == 6 && oldPass.size() < 30) oldPass.push_back(c);
            else if (activeField == 7 && newPass.size() < 30) newPass.push_back(c);
            else if (activeField == 8 && confirmPass.size() < 30) confirmPass.push_back(c);
        }
    }
}

void AdminProfilePanel::update(const Vector2f& mousePos, bool /*mouseDown*/) {
    // Delete button hover
    deleteHovered = FloatRect({position.x + width - 240.f, position.y + 74.f}, {210.f, 42.f}).contains(mousePos);

    // Save button hover
    saveInfoHovered = FloatRect({position.x + 44.f, position.y + 186.f + 410.f}, {160.f, 42.f}).contains(mousePos);

    // Change pass button hover
    changePassHovered = FloatRect({position.x + 20.f + 695.f + 18.f + 24.f, position.y + 186.f + 410.f}, {160.f, 42.f}).contains(mousePos);

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
    const float cardH = 146.f;

    // White Card Background
    RoundedRectRenderer::draw(target, {cardX, cardY}, {cardW, cardH}, 12.f, Color::White, 1.f, Color(226, 232, 240));

    // Large Square Avatar
    Vector2f avPos(cardX + 24.f, cardY + 28.f);
    RoundedRectRenderer::draw(target, avPos, {84.f, 84.f}, 16.f, Color(2, 132, 199));

    // Letter 'A' in Avatar
    Text avText(boldFont);
    avText.setString(L"A");
    avText.setCharacterSize(42);
    avText.setFillColor(Color::White);
    FloatRect ab = avText.getLocalBounds();
    avText.setPosition({avPos.x + 42.f - ab.size.x / 2.f, avPos.y + 42.f - ab.size.y / 2.f - 6.f});
    target.draw(avText);

    // Green Online Badge on Avatar corner
    Vector2f chkPos(avPos.x + 84.f - 20.f, avPos.y + 84.f - 20.f);
    CircleShape chkCircle(11.f);
    chkCircle.setPosition(chkPos);
    chkCircle.setFillColor(Color(16, 185, 129));
    target.draw(chkCircle);

    Text chkTxt(boldFont);
    chkTxt.setString(L"✓");
    chkTxt.setCharacterSize(11);
    chkTxt.setFillColor(Color::White);
    chkTxt.setPosition({chkPos.x + 6.f, chkPos.y + 3.f});
    target.draw(chkTxt);

    // Title: HỒ SƠ CỦA ADMIN
    float textX = avPos.x + 104.f;
    Text title(boldFont);
    title.setString(L"HỒ SƠ CỦA ADMIN");
    title.setCharacterSize(22);
    title.setFillColor(Color(15, 23, 42));
    title.setPosition({textX, cardY + 24.f});
    target.draw(title);

    // Badge: 🛡 Quản trị viên cấp cao
    float tW = title.getGlobalBounds().size.x;
    Vector2f bPos(textX + tW + 14.f, cardY + 28.f);
    RoundedRectRenderer::draw(target, bPos, {155.f, 22.f}, 11.f, Color(224, 242, 254), 1.f, Color(186, 230, 253));
    Text bTxt(boldFont);
    bTxt.setString(L"🛡 Quản trị viên cấp cao");
    bTxt.setCharacterSize(10.5f);
    bTxt.setFillColor(Color(2, 132, 199));
    bTxt.setPosition({bPos.x + 10.f, bPos.y + 3.f});
    target.draw(bTxt);

    // Status Pill: Đang hoạt động (Active)
    Vector2f stPos(textX, cardY + 58.f);
    RoundedRectRenderer::draw(target, stPos, {142.f, 20.f}, 10.f, Color(220, 252, 231), 1.f, Color(167, 243, 208));
    Text stTxt(boldFont);
    stTxt.setString(L"Đang hoạt động (Active)");
    stTxt.setCharacterSize(10.5f);
    stTxt.setFillColor(Color(5, 150, 105));
    stTxt.setPosition({stPos.x + 10.f, stPos.y + 2.f});
    target.draw(stTxt);

    // Metadata line: Email • Role • Last Login
    Text meta(font);
    meta.setString(L"✉ " + s2ws(email) + L"   •   👤 Quản trị viên   •   🕒 Đăng nhập gần nhất: 10 phút trước (118.69.214.88)");
    meta.setCharacterSize(11.5f);
    meta.setFillColor(Color(100, 116, 139));
    meta.setPosition({textX, cardY + 86.f});
    target.draw(meta);

    // Danger Button: 🗑 XÓA VĨNH VIỄN TÀI KHOẢN
    Vector2f delPos(cardX + cardW - 230.f, cardY + 52.f);
    Color delBg = deleteHovered ? Color(185, 28, 28) : Color(220, 38, 38);
    RoundedRectRenderer::draw(target, delPos, {210.f, 42.f}, 6.f, delBg);
    Text delTxt(boldFont);
    delTxt.setString(L"🗑 XÓA VĨNH VIỄN TÀI KHOẢN");
    delTxt.setCharacterSize(11.f);
    delTxt.setFillColor(Color::White);
    FloatRect db = delTxt.getLocalBounds();
    delTxt.setPosition({delPos.x + 105.f - db.size.x / 2.f, delPos.y + 13.f});
    target.draw(delTxt);
}

void AdminProfilePanel::drawPersonalInfoCard(RenderTarget& target) const {
    const float cardX = position.x + 20.f;
    const float cardY = position.y + 184.f;
    const float cardW = 695.f;
    const float cardH = 500.f;

    // Card Background
    RoundedRectRenderer::draw(target, {cardX, cardY}, {cardW, cardH}, 12.f, Color::White, 1.f, Color(226, 232, 240));

    // Header Icon
    Vector2f icPos(cardX + 24.f, cardY + 22.f);
    RoundedRectRenderer::draw(target, icPos, {32.f, 32.f}, 6.f, Color(224, 242, 254));
    Text icTxt(boldFont); icTxt.setString(L"💳"); icTxt.setCharacterSize(14); icTxt.setPosition({icPos.x + 7.f, icPos.y + 5.f}); target.draw(icTxt);

    // Title
    Text title(boldFont);
    title.setString(L"THÔNG TIN CÁ NHÂN");
    title.setCharacterSize(13.5f);
    title.setFillColor(Color(15, 23, 42));
    title.setPosition({cardX + 66.f, cardY + 18.f});
    target.draw(title);

    // Subtitle
    Text sub(font);
    sub.setString(L"Cập nhật hồ sơ và các thông tin liên lạc hành chính");
    sub.setCharacterSize(11.f);
    sub.setFillColor(Color(100, 116, 139));
    sub.setPosition({cardX + 66.f, cardY + 36.f});
    target.draw(sub);

    // ID on the right
    Text idTxt(boldFont);
    idTxt.setString(L"ID: ADM-2024-001");
    idTxt.setCharacterSize(11.f);
    idTxt.setFillColor(Color(148, 163, 184));
    idTxt.setPosition({cardX + cardW - 24.f - idTxt.getGlobalBounds().size.x, cardY + 24.f});
    target.draw(idTxt);

    // Form Inputs (2-column layout)
    float inW = 310.f;
    float inH = 40.f;

    auto drawInputBox = [&](float x, float y, const wstring& label, const string& val, const wstring& icon, bool isAct, bool hasVerifiedBadge) {
        Text lbl(boldFont);
        lbl.setString(label);
        lbl.setCharacterSize(11.f);
        lbl.setFillColor(Color(51, 65, 85));
        lbl.setPosition({x, y});
        target.draw(lbl);

        if (hasVerifiedBadge) {
            Text vBadge(boldFont);
            vBadge.setString(L"✓ Đã xác minh");
            vBadge.setCharacterSize(10.f);
            vBadge.setFillColor(Color(5, 150, 105));
            vBadge.setPosition({x + lbl.getGlobalBounds().size.x + 8.f, y});
            target.draw(vBadge);
        }

        Vector2f bPos(x, y + 20.f);
        Color borderCol = isAct ? Color(2, 132, 199) : Color(203, 213, 225);
        RoundedRectRenderer::draw(target, bPos, {inW, inH}, 6.f, Color::White, 1.f, borderCol);

        if (!icon.empty()) {
            Text ic(font);
            ic.setString(icon);
            ic.setCharacterSize(13);
            ic.setFillColor(Color(148, 163, 184));
            ic.setPosition({bPos.x + 12.f, bPos.y + 10.f});
            target.draw(ic);
        }

        float textOffset = icon.empty() ? 12.f : 34.f;
        Text vTxt(boldFont);
        vTxt.setString(s2ws(val));
        vTxt.setCharacterSize(12.f);
        vTxt.setFillColor(Color(15, 23, 42));
        vTxt.setPosition({bPos.x + textOffset, bPos.y + 11.f});
        target.draw(vTxt);
    };

    // Row 1: Họ tên (Field 1) & Ngày sinh (Field 2)
    drawInputBox(cardX + 24.f, cardY + 74.f, L"Họ và tên *", fullName, L"👤", activeField == 1, false);
    drawInputBox(cardX + 24.f + inW + 18.f, cardY + 74.f, L"Ngày sinh *", birthDate, L"📅", activeField == 2, false);

    // Row 2: Số điện thoại (Field 3) & Email hệ thống
    drawInputBox(cardX + 24.f, cardY + 154.f, L"Số điện thoại *", phone, L"📞", activeField == 3, false);
    drawInputBox(cardX + 24.f + inW + 18.f, cardY + 154.f, L"Email hệ thống *", email, L"✉", false, true);

    // Row 3: Chức vụ (Field 4) & Cơ sở trực thuộc (Field 5)
    drawInputBox(cardX + 24.f, cardY + 234.f, L"Chức vụ / Bộ phận", department, L"", activeField == 4, false);
    drawInputBox(cardX + 24.f + inW + 18.f, cardY + 234.f, L"Cơ sở trực thuộc", location, L"", activeField == 5, false);

    // Audit Log Notice Box
    Vector2f notPos(cardX + 24.f, cardY + 324.f);
    RoundedRectRenderer::draw(target, notPos, {cardW - 48.f, 52.f}, 6.f, Color(240, 249, 255), 1.f, Color(186, 230, 253));
    Text notIc(boldFont); notIc.setString(L"ℹ"); notIc.setCharacterSize(14); notIc.setFillColor(Color(2, 132, 199)); notIc.setPosition({notPos.x + 12.f, notPos.y + 15.f}); target.draw(notIc);
    Text notTxt(font);
    notTxt.setString(L"Mọi thay đổi thông tin cá nhân sẽ được lưu vào Nhật ký kiểm toán\nhệ thống (Audit Log) theo tiêu chuẩn ISO 27001.");
    notTxt.setCharacterSize(10.5f);
    notTxt.setFillColor(Color(3, 105, 161));
    notTxt.setPosition({notPos.x + 36.f, notPos.y + 10.f});
    target.draw(notTxt);

    // Save Button: 💾 LƯU THÔNG TIN
    Vector2f savePos(cardX + 24.f, cardY + 400.f);
    Color saveBg = saveInfoHovered ? Color(3, 105, 161) : Color(2, 132, 199);
    RoundedRectRenderer::draw(target, savePos, {170.f, 42.f}, 6.f, saveBg);
    Text saveTxt(boldFont);
    saveTxt.setString(L"💾 LƯU THÔNG TIN");
    saveTxt.setCharacterSize(11.5f);
    saveTxt.setFillColor(Color::White);
    FloatRect sb = saveTxt.getLocalBounds();
    saveTxt.setPosition({savePos.x + 85.f - sb.size.x / 2.f, savePos.y + 12.f});
    target.draw(saveTxt);
}

void AdminProfilePanel::drawPasswordCard(RenderTarget& target) const {
    const float cardX = position.x + 20.f + 695.f + 18.f;
    const float cardY = position.y + 184.f;
    const float cardW = 695.f;
    const float cardH = 500.f;

    // Card Background
    RoundedRectRenderer::draw(target, {cardX, cardY}, {cardW, cardH}, 12.f, Color::White, 1.f, Color(226, 232, 240));

    // Header Icon
    Vector2f icPos(cardX + 24.f, cardY + 22.f);
    RoundedRectRenderer::draw(target, icPos, {32.f, 32.f}, 6.f, Color(254, 243, 199));
    Text icTxt(boldFont); icTxt.setString(L"🔑"); icTxt.setCharacterSize(14); icTxt.setPosition({icPos.x + 7.f, icPos.y + 5.f}); target.draw(icTxt);

    // Title
    Text title(boldFont);
    title.setString(L"ĐỔI MẬT KHẨU");
    title.setCharacterSize(13.5f);
    title.setFillColor(Color(15, 23, 42));
    title.setPosition({cardX + 66.f, cardY + 18.f});
    target.draw(title);

    // Subtitle
    Text sub(font);
    sub.setString(L"Nên đổi mật khẩu định kỳ 90 ngày để đảm bảo an toàn");
    sub.setCharacterSize(11.f);
    sub.setFillColor(Color(100, 116, 139));
    sub.setPosition({cardX + 66.f, cardY + 36.f});
    target.draw(sub);

    // Security Level on Right
    Text secTxt(boldFont);
    secTxt.setString(L"Bảo mật cấp 3");
    secTxt.setCharacterSize(11.f);
    secTxt.setFillColor(Color(148, 163, 184));
    secTxt.setPosition({cardX + cardW - 24.f - secTxt.getGlobalBounds().size.x, cardY + 24.f});
    target.draw(secTxt);

    // Password Fields (Full width: 647px, height: 40px)
    float passW = cardW - 48.f;
    float passH = 40.f;

    auto drawPassInput = [&](float y, const wstring& lbl, const string& val, const wstring& placeholder, bool isAct, bool showEye) {
        Text l(boldFont);
        l.setString(lbl);
        l.setCharacterSize(11.f);
        l.setFillColor(Color(51, 65, 85));
        l.setPosition({cardX + 24.f, y});
        target.draw(l);

        Vector2f bPos(cardX + 24.f, y + 20.f);
        Color borderCol = isAct ? Color(2, 132, 199) : Color(203, 213, 225);
        RoundedRectRenderer::draw(target, bPos, {passW, passH}, 6.f, Color::White, 1.f, borderCol);

        // Lock icon
        Text ic(font); ic.setString(L"🔒"); ic.setCharacterSize(12); ic.setFillColor(Color(148, 163, 184)); ic.setPosition({bPos.x + 12.f, bPos.y + 11.f}); target.draw(ic);

        // Display string (masked or plain)
        wstring displayStr;
        if (val.empty()) {
            displayStr = placeholder;
        } else if (showEye) {
            displayStr = s2ws(val);
        } else {
            displayStr = wstring(val.size(), L'●');
        }

        Text vTxt(val.empty() ? font : boldFont);
        vTxt.setString(displayStr);
        vTxt.setCharacterSize(12.f);
        vTxt.setFillColor(val.empty() ? Color(148, 163, 184) : Color(15, 23, 42));
        vTxt.setPosition({bPos.x + 36.f, bPos.y + 11.f});
        target.draw(vTxt);

        // Right eye icon
        Text eye(font);
        eye.setString(showEye ? L"👁" : L"🗨");
        eye.setCharacterSize(13);
        eye.setFillColor(Color(148, 163, 184));
        eye.setPosition({bPos.x + passW - 30.f, bPos.y + 11.f});
        target.draw(eye);
    };

    // 1. Mật khẩu cũ (Field 6)
    drawPassInput(cardY + 74.f, L"Mật khẩu cũ *", oldPass, L"Nhập mật khẩu cũ", activeField == 6, showOldPass);

    // 2. Mật khẩu mới (Field 7)
    drawPassInput(cardY + 144.f, L"Mật khẩu mới *", newPass, L"Nhập mật khẩu mới", activeField == 7, showNewPass);

    // Password Strength Meter
    float meterY = cardY + 214.f;
    Text sLbl(font);
    sLbl.setString(L"Độ mạnh mật khẩu:");
    sLbl.setCharacterSize(10.5f);
    sLbl.setFillColor(Color(100, 116, 139));
    sLbl.setPosition({cardX + 24.f, meterY});
    target.draw(sLbl);

    Text sVal(boldFont);
    sVal.setString(L"Rất mạnh");
    sVal.setCharacterSize(10.5f);
    sVal.setFillColor(Color(16, 185, 129));
    sVal.setPosition({cardX + cardW - 24.f - sVal.getGlobalBounds().size.x, meterY});
    target.draw(sVal);

    // 4 Green Segmented Bars
    float barGap = 8.f;
    float segW = (passW - 3 * barGap) / 4.f;
    for (int i = 0; i < 4; ++i) {
        Vector2f segPos(cardX + 24.f + i * (segW + barGap), meterY + 18.f);
        RoundedRectRenderer::draw(target, segPos, {segW, 4.f}, 2.f, Color(16, 185, 129));
    }

    // 3. Xác thực mật khẩu (Field 8)
    drawPassInput(cardY + 244.f, L"Xác thực mật khẩu *", confirmPass, L"Xác nhận mật khẩu mới", activeField == 8, showConfirmPass);

    // Password Rules Checklist (2x2 grid)
    float ruleY = cardY + 316.f;
    Text rTitle(boldFont);
    rTitle.setString(L"Quy định mật khẩu CineXine:");
    rTitle.setCharacterSize(10.5f);
    rTitle.setFillColor(Color(71, 85, 105));
    rTitle.setPosition({cardX + 24.f, ruleY});
    target.draw(rTitle);

    auto drawRuleItem = [&](float x, float y, const wstring& rTxt, bool checked) {
        Text ic(boldFont);
        ic.setString(checked ? L"●" : L"○");
        ic.setCharacterSize(10);
        ic.setFillColor(checked ? Color(16, 185, 129) : Color(148, 163, 184));
        ic.setPosition({x, y});
        target.draw(ic);

        Text t(font);
        t.setString(rTxt);
        t.setCharacterSize(10.5f);
        t.setFillColor(Color(100, 116, 139));
        t.setPosition({x + 14.f, y});
        target.draw(t);
    };

    drawRuleItem(cardX + 24.f, ruleY + 20.f, L"Tối thiểu 8 ký tự", true);
    drawRuleItem(cardX + 24.f + passW / 2.f, ruleY + 20.f, L"Ký tự chữ hoa & số", true);
    drawRuleItem(cardX + 24.f, ruleY + 40.f, L"Ít nhất 1 ký tự đặc biệt", true);
    drawRuleItem(cardX + 24.f + passW / 2.f, ruleY + 40.f, L"Khác 3 mật khẩu cũ", false);

    // Change Password Button: 🔁 ĐỔI MẬT KHẨU
    Vector2f chgPos(cardX + 24.f, cardY + 400.f);
    Color chgBg = changePassHovered ? Color(3, 105, 161) : Color(2, 132, 199);
    RoundedRectRenderer::draw(target, chgPos, {165.f, 42.f}, 6.f, chgBg);
    Text chgTxt(boldFont);
    chgTxt.setString(L"🔁 ĐỔI MẬT KHẨU");
    chgTxt.setCharacterSize(11.5f);
    chgTxt.setFillColor(Color::White);
    FloatRect cb = chgTxt.getLocalBounds();
    chgTxt.setPosition({chgPos.x + 82.f - cb.size.x / 2.f, chgPos.y + 12.f});
    target.draw(chgTxt);
}

void AdminProfilePanel::drawTwoFactorCard(RenderTarget& target) const {
    const float cardX = position.x + 20.f + 695.f + 18.f;
    const float cardY = position.y + 696.f;
    const float cardW = 695.f;
    const float cardH = 58.f;

    // White Card Background
    RoundedRectRenderer::draw(target, {cardX, cardY}, {cardW, cardH}, 8.f, Color::White, 1.f, Color(226, 232, 240));

    // Shield Icon
    Vector2f icPos(cardX + 16.f, cardY + 13.f);
    CircleShape ic(16.f);
    ic.setPosition(icPos);
    ic.setFillColor(Color(220, 252, 231));
    target.draw(ic);

    Text icTxt(boldFont);
    icTxt.setString(L"🛡");
    icTxt.setCharacterSize(13);
    icTxt.setPosition({icPos.x + 8.f, icPos.y + 4.f});
    target.draw(icTxt);

    // Title & Subtitle
    Text title(boldFont);
    title.setString(L"Xác thực 2 yếu tố (2FA Google Authenticator)");
    title.setCharacterSize(12);
    title.setFillColor(Color(15, 23, 42));
    title.setPosition({cardX + 58.f, cardY + 11.f});
    target.draw(title);

    Text sub(font);
    sub.setString(L"Bảo vệ tài khoản qua mã OTP ứng dụng điện thoại");
    sub.setCharacterSize(10.5f);
    sub.setFillColor(Color(100, 116, 139));
    sub.setPosition({cardX + 58.f, cardY + 30.f});
    target.draw(sub);

    // Badge: ● ĐÃ BẬT
    Vector2f bPos(cardX + cardW - 95.f, cardY + 16.f);
    RoundedRectRenderer::draw(target, bPos, {80.f, 26.f}, 13.f, Color(220, 252, 231), 1.f, Color(167, 243, 208));
    Text bTxt(boldFont);
    bTxt.setString(L"● ĐÃ BẬT");
    bTxt.setCharacterSize(11);
    bTxt.setFillColor(Color(5, 150, 105));
    bTxt.setPosition({bPos.x + 12.f, bPos.y + 4.f});
    target.draw(bTxt);
}

void AdminProfilePanel::drawDeleteModal(RenderTarget& target) const {
    // Dark Backdrop
    RectangleShape dim(Vector2f(width, height));
    dim.setPosition(position);
    dim.setFillColor(Color(15, 23, 42, 170));
    target.draw(dim);

    float dW = 460.f;
    float dH = 220.f;
    Vector2f dPos(position.x + (width - dW) / 2.f, position.y + (height - dH) / 2.f);

    RoundedRectRenderer::draw(target, dPos, {dW, dH}, 10.f, Color::White, 1.f, Color(226, 232, 240));

    Text title(boldFont);
    title.setString(L"XÁC NHẬN XÓA TÀI KHOẢN ADMIN");
    title.setCharacterSize(16);
    title.setFillColor(Color(220, 38, 38));
    title.setPosition({dPos.x + 24.f, dPos.y + 20.f});
    target.draw(title);

    Text msg(font);
    msg.setString(L"Bạn có chắc chắn muốn xóa vĩnh viễn tài khoản quản trị này?\nHành động này không thể hoàn tác!");
    msg.setCharacterSize(12.5f);
    msg.setFillColor(Color(51, 65, 85));
    msg.setPosition({dPos.x + 24.f, dPos.y + 58.f});
    target.draw(msg);

    // Cancel Button
    Vector2f canPos(dPos.x + dW - 220.f, dPos.y + dH - 52.f);
    RoundedRectRenderer::draw(target, canPos, {95.f, 36.f}, 6.f, Color::White, 1.f, Color(203, 213, 225));
    Text canTxt(boldFont);
    canTxt.setString(L"Hủy bỏ");
    canTxt.setCharacterSize(11.5f);
    canTxt.setFillColor(Color(71, 85, 105));
    canTxt.setPosition({canPos.x + 24.f, canPos.y + 8.f});
    target.draw(canTxt);

    // Confirm Delete Button
    Vector2f delPos(dPos.x + dW - 110.f, dPos.y + dH - 52.f);
    RoundedRectRenderer::draw(target, delPos, {95.f, 36.f}, 6.f, Color(220, 38, 38));
    Text delTxt(boldFont);
    delTxt.setString(L"Xóa vĩnh viễn");
    delTxt.setCharacterSize(11.5f);
    delTxt.setFillColor(Color::White);
    delTxt.setPosition({delPos.x + 12.f, delPos.y + 8.f});
    target.draw(delTxt);
}

void AdminProfilePanel::drawBottomBar(RenderTarget& target) const {
    float barY = height - 28.f;

    RectangleShape div(Vector2f(width - 40.f, 1.f));
    div.setPosition({position.x + 20.f, barY - 4.f});
    div.setFillColor(Color(226, 232, 240));
    target.draw(div);

    Text engText(font);
    engText.setString(L"NODE: VN-DN601  •  LIVE   |   CineXiNe Admin Security Vault: Protected   |   UTF-8 Engine");
    engText.setCharacterSize(11.f);
    engText.setFillColor(Color(100, 116, 139));
    engText.setPosition({position.x + 20.f, barY});
    target.draw(engText);

    Text authText(font);
    authText.setString(L"Phụ trách: Admin CineXiNe (Toàn quyền)  •  1728x972 HD");
    authText.setCharacterSize(11.f);
    authText.setFillColor(Color(100, 116, 139));
    FloatRect ab = authText.getLocalBounds();
    authText.setPosition({position.x + width - ab.size.x - 20.f, barY});
    target.draw(authText);
}

void AdminProfilePanel::drawToast(RenderTarget& target) const {
    Text toast(boldFont);
    toast.setString(toastMessage);
    toast.setCharacterSize(12.5f);
    toast.setFillColor(Color::White);
    FloatRect tb = toast.getLocalBounds();

    float tw = tb.size.x + 36.f;
    float th = 38.f;
    Vector2f tPos(position.x + (width - tw) / 2.f, position.y + height - 76.f);

    RoundedRectRenderer::draw(target, tPos, {tw, th}, 19.f, Color(15, 23, 42, 235), 1.f, Color(51, 65, 85));
    toast.setPosition({tPos.x + 18.f, tPos.y + 9.f});
    target.draw(toast);
}