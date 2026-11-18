#include "UI/screens/RegisterScreen.h"
#include "UI/components/Admin/RoundedRectRenderer.h"
#ifdef _WIN32
#include <windows.h>
#endif
#include <array>
#include <cmath>

static std::string wstring_to_utf8(const std::wstring& wstr) {
    if (wstr.empty()) return "";
#ifdef _WIN32
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.length(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.length(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
#else
    return std::string(wstr.begin(), wstr.end());
#endif
}

RegisterScreen::RegisterScreen(const Font& f, AuthService& authRef)
    : auth(authRef), font(f) {
    if (!boldFont.openFromFile("../assets/fonts/segoeuib.ttf")) {
        boldFont = f;
    }
    if (logoTexture.loadFromFile("../assets/elements/cinexine_logo.png") ||
        logoTexture.loadFromFile("assets/elements/cinexine_logo.png") ||
        logoTexture.loadFromFile("../assets/elements/icon.png") ||
        logoTexture.loadFromFile("assets/elements/icon.png")) {
        logoTexture.setSmooth(true);
        logoSprite.emplace(logoTexture);
    }
    computeLayout(1728.f, 972.f);
}

void RegisterScreen::computeLayout(float W, float H) {
    const float cardW = 680.f;
    const float cardH = 810.f;
    const float cardX = (W - cardW) / 2.f;
    const float cardY = (H - cardH) / 2.f;
    const float padX = 46.f;
    const float innerW = cardW - padX * 2.f;
    const float leftX = cardX + padX;
    const float halfGap = 16.f;
    const float halfW = (innerW - halfGap) / 2.f;
    const float fieldH = 44.f;

    closeRect = FloatRect({cardX + cardW - padX - 24.f, cardY + 114.f}, {28.f, 28.f});

    // Row 1: Họ và tên (index 0)
    float r1Y = cardY + 266.f;
    fieldRects[0] = FloatRect({leftX, r1Y}, {innerW, fieldH});

    // Row 2: Ngày sinh (index 1) + Số điện thoại (index 3)
    float r2Y = r1Y + 76.f;
    fieldRects[1] = FloatRect({leftX, r2Y}, {halfW, fieldH});
    fieldRects[3] = FloatRect({leftX + halfW + halfGap, r2Y}, {halfW, fieldH});

    // Row 3: Email doanh nghiệp (index 2)
    float r3Y = r2Y + 76.f;
    fieldRects[2] = FloatRect({leftX, r3Y}, {innerW, fieldH});

    // Row 4: Mật khẩu (index 4) + Xác nhận mật khẩu (index 5)
    float r4Y = r3Y + 76.f;
    fieldRects[4] = FloatRect({leftX, r4Y}, {halfW, fieldH});
    fieldRects[5] = FloatRect({leftX + halfW + halfGap, r4Y}, {halfW, fieldH});

    eyeRect = FloatRect({leftX + halfW - 36.f, r4Y + 8.f}, {28.f, 28.f});

    float chkY = r4Y + 62.f;
    checkRect = FloatRect({leftX - 2.f, chkY - 2.f}, {innerW, 24.f});

    float btnY = chkY + 46.f;
    float backW = 190.f;
    float regW = innerW - backW - 14.f;
    backRect = FloatRect({leftX, btnY}, {backW, 46.f});
    registerRect = FloatRect({leftX + backW + 14.f, btnY}, {regW, 46.f});

    loginLinkRect = FloatRect({cardX + cardW / 2.f + 50.f, cardY + cardH - 52.f}, {140.f, 24.f});
}

wstring RegisterScreen::bullets(size_t n) const {
    return wstring(n, L'•');
}

bool RegisterScreen::validateInputs() {
    for (int i = 0; i < 6; i++) {
        if (inputs[i].empty()) {
            statusColor = Color(220, 38, 38);
            statusMessage = L"Vui lòng điền đầy đủ thông tin bắt buộc (*).";
            return false;
        }
    }
    string email = wstring_to_utf8(inputs[2]);
    if (auth.emailExists(email)) {
        statusColor = Color(220, 38, 38);
        statusMessage = L"Email doanh nghiệp này đã được sử dụng.";
        return false;
    }
    if (inputs[4].size() < 8) {
        statusColor = Color(220, 38, 38);
        statusMessage = L"Mật khẩu phải có tối thiểu 8 ký tự.";
        return false;
    }
    if (inputs[4] != inputs[5]) {
        statusColor = Color(220, 38, 38);
        statusMessage = L"Mật khẩu xác nhận không khớp.";
        return false;
    }
    if (!agreeTerms) {
        statusColor = Color(220, 38, 38);
        statusMessage = L"Vui lòng đồng ý với Quy chế nội bộ trước khi đăng ký.";
        return false;
    }
    return true;
}

void RegisterScreen::submitRegistration() {
    if (!validateInputs()) return;

    string fullName = wstring_to_utf8(inputs[0]);
    string birthDate = wstring_to_utf8(inputs[1]);
    string email = wstring_to_utf8(inputs[2]);
    string phone = wstring_to_utf8(inputs[3]);
    string password = wstring_to_utf8(inputs[4]);

    if (auth.registerUser(email, password, fullName, birthDate, phone, AppRole::Admin)) {
        statusColor = Color(16, 185, 129);
        statusMessage = L"Đăng ký thành công! Đang chuyển sang đăng nhập...";
        showSuccessMessage = true;
        messageTimer.restart();
    } else {
        statusColor = Color(220, 38, 38);
        statusMessage = L"Đăng ký thất bại. Vui lòng thử lại.";
    }
}

void RegisterScreen::handleEvent(const Event& event, AppState& state) {
    if (const auto* keyEvent = event.getIf<Event::KeyPressed>()) {
        auto code = keyEvent->code;

        if (code == Keyboard::Key::Escape) {
            state = AppState::LOGIN;
            reset();
            return;
        }
        if (code == Keyboard::Key::Tab) {
            // Visual tab order: 0 -> 1 -> 3 -> 2 -> 4 -> 5
            static const int tabOrder[6] = {0, 1, 3, 2, 4, 5};
            int curPos = -1;
            for (int i = 0; i < 6; ++i) {
                if (tabOrder[i] == activeField) { curPos = i; break; }
            }
            activeField = tabOrder[(curPos + 1) % 6];
            showCursor = true;
            cursorClock.restart();
        } else if (code == Keyboard::Key::Backspace && activeField >= 0) {
            if (!inputs[activeField].empty()) {
                inputs[activeField].pop_back();
                showCursor = true;
                cursorClock.restart();
            }
        } else if (code == Keyboard::Key::Enter) {
            submitRegistration();
        }
    }

    if (event.is<Event::TextEntered>() && activeField >= 0) {
        char32_t unicode = event.getIf<Event::TextEntered>()->unicode;
        if (unicode >= 32 && unicode != 127 && inputs[activeField].size() < 55) {
            inputs[activeField].push_back((wchar_t)unicode);
            showCursor = true;
            cursorClock.restart();
        }
    }
}

bool RegisterScreen::update(Vector2f mouse, bool mousePressed, AppState& state) {
    closeHovered = closeRect.contains(mouse);
    eyeHovered = eyeRect.contains(mouse);
    checkHovered = checkRect.contains(mouse);
    backHovered = backRect.contains(mouse);
    registerHovered = registerRect.contains(mouse);
    loginLinkHovered = loginLinkRect.contains(mouse);

    if (cursorClock.getElapsedTime().asMilliseconds() > 530) {
        showCursor = !showCursor;
        cursorClock.restart();
    }

    if (showSuccessMessage && messageTimer.getElapsedTime().asSeconds() > 1.1f) {
        reset();
        state = AppState::LOGIN;
        return false;
    }

    if (mousePressed) {
        if (closeHovered || backHovered || loginLinkHovered) {
            reset();
            state = AppState::LOGIN;
            return false;
        }

        if (eyeHovered) {
            showPassword = !showPassword;
            return false;
        }

        if (checkHovered) {
            agreeTerms = !agreeTerms;
            return false;
        }

        if (registerHovered) {
            submitRegistration();
            return false;
        }

        activeField = -1;
        for (int i = 0; i < 6; i++) {
            if (fieldRects[i].contains(mouse)) {
                activeField = i;
                showCursor = true;
                cursorClock.restart();
                break;
            }
        }
    }
    return false;
}

void RegisterScreen::draw(RenderWindow& window) {
    Vector2u winSize = window.getSize();
    const float W = (float)winSize.x;
    const float H = (float)winSize.y;
    computeLayout(W, H);

    // 1. Off-white canvas (#F4F6F9) + dot grid
    RectangleShape bgCanvas({W, H});
    bgCanvas.setFillColor(Color(244, 246, 249));
    window.draw(bgCanvas);
    drawDotGrid(window, W, H);

    // 2. Top Header Bar
    if (logoSprite) {
        float hScale = 22.f / (float)logoTexture.getSize().y;
        logoSprite->setScale({hScale, hScale});
        logoSprite->setPosition({24.f, 16.f});
        window.draw(*logoSprite);
    } else {
        drawVectorPortalLogo(window, {24.f, 16.f}, 24.f);
    }

    Text headerTitle(boldFont, L"PBL2 \u2013 H\u1ec6 TH\u1ed0NG \u0110\u1eb6T V\u00c9 XEM PHIM K\u1ebeT H\u1ee2P TH\u1ed0NG K\u00ca PH\u00c2N T\u00cdCH DOANH THU", 13);
    headerTitle.setFillColor(Color(30, 41, 59));
    headerTitle.setPosition({56.f, 19.f});
    window.draw(headerTitle);

    RoundedRectRenderer::draw(window, {W - 220.f, 15.f}, {145.f, 26.f}, 13.f, Color(226, 232, 240), 1.f, Color(203, 213, 225));
    Text verText(font, L"Phi\u00ean b\u1ea3n Desktop v2.4", 13);
    verText.setFillColor(Color(100, 116, 139));
    verText.setPosition({W - 212.f, 20.f});
    window.draw(verText);

    CircleShape dotRed(4.5f, 12); dotRed.setFillColor(Color(239, 68, 68)); dotRed.setPosition({W - 55.f, 24.f}); window.draw(dotRed);
    CircleShape dotYellow(4.5f, 12); dotYellow.setFillColor(Color(245, 158, 11)); dotYellow.setPosition({W - 40.f, 24.f}); window.draw(dotYellow);
    CircleShape dotGreen(4.5f, 12); dotGreen.setFillColor(Color(16, 185, 129)); dotGreen.setPosition({W - 25.f, 24.f}); window.draw(dotGreen);

    // 3. Central White Card
    const float cardW = 680.f;
    const float cardH = 810.f;
    const float cardX = (W - cardW) / 2.f;
    const float cardY = (H - cardH) / 2.f;
    const float padX = 46.f;
    const float leftX = cardX + padX;
    const float innerW = cardW - padX * 2.f;

    RoundedRectRenderer::draw(window, {cardX - 5.f, cardY + 6.f}, {cardW + 10.f, cardH + 8.f}, 22.f, Color(15, 23, 42, 12));
    RoundedRectRenderer::draw(window, {cardX - 2.f, cardY + 2.f}, {cardW + 4.f, cardH + 4.f}, 20.f, Color(15, 23, 42, 18));
    RoundedRectRenderer::draw(window, {cardX, cardY}, {cardW, cardH}, 20.f, Color::White, 1.f, Color(226, 232, 240));

    // Brand Header inside Card
    float brandTextX = leftX + 58.f;
    if (logoSprite) {
        float targetH = 44.f;
        float scale = targetH / (float)logoTexture.getSize().y;
        logoSprite->setScale({scale, scale});
        logoSprite->setPosition({leftX, cardY + 32.f});
        window.draw(*logoSprite);
        brandTextX = leftX + (float)logoTexture.getSize().x * scale + 14.f;
    } else {
        drawVectorPortalLogo(window, {leftX, cardY + 32.f}, 44.f);
    }
    Text brandTop(boldFont, L"CINECORE ENTERPRISE", 13);
    brandTop.setFillColor(Color(100, 116, 139));
    brandTop.setPosition({brandTextX, cardY + 35.f});
    window.draw(brandTop);

    Text brandMain(boldFont, L"CineCore Portal", 17.f);
    brandMain.setFillColor(Color(15, 23, 42));
    brandMain.setPosition({brandTextX, cardY + 51.f});
    window.draw(brandMain);

    RectangleShape topDiv({cardW, 1.f});
    topDiv.setPosition({cardX, cardY + 96.f});
    topDiv.setFillColor(Color(241, 245, 249));
    window.draw(topDiv);

    // Portal Pill + Close X
    Vector2f pillPos(leftX, cardY + 114.f);
    Vector2f pillSize(188.f, 28.f);
    RoundedRectRenderer::draw(window, pillPos, pillSize, 14.f, Color(240, 249, 255), 1.f, Color(186, 230, 253));
    drawVectorCheck(window, {pillPos.x + 12.f, pillPos.y + 9.f}, Color(2, 132, 199));
    Text pillTxt(boldFont, L"C\u1ed4NG QU\u1ea2N TR\u1eca N\u1ed8I B\u1ed8", 11.f);
    pillTxt.setFillColor(Color(2, 132, 199));
    pillTxt.setPosition({pillPos.x + 30.f, pillPos.y + 7.f});
    window.draw(pillTxt);

    // Close X
    Color xColor = closeHovered ? Color(15, 23, 42) : Color(148, 163, 184);
    Vertex x1[2] = {Vertex({closeRect.position.x + 7.f, closeRect.position.y + 7.f}, xColor),
                    Vertex({closeRect.position.x + 19.f, closeRect.position.y + 19.f}, xColor)};
    Vertex x2[2] = {Vertex({closeRect.position.x + 19.f, closeRect.position.y + 7.f}, xColor),
                    Vertex({closeRect.position.x + 7.f, closeRect.position.y + 19.f}, xColor)};
    window.draw(x1, 2, PrimitiveType::Lines);
    window.draw(x2, 2, PrimitiveType::Lines);

    // Title & Subtitle
    Text titleTxt(boldFont, L"\u0110\u0102NG K\u00dd T\u00c0I KHO\u1ea2N M\u1edaI", 25.f);
    titleTxt.setFillColor(Color(15, 23, 42));
    titleTxt.setPosition({leftX, cardY + 158.f});
    window.draw(titleTxt);

    Text subTxt(font, L"\u0110i\u1ec1n \u0111\u1ea7y \u0111\u1ee7 th\u00f4ng tin \u0111\u1ec3 c\u1ea5p quy\u1ec1n truy c\u1eadp h\u1ec7 th\u1ed1ng qu\u1ea3n tr\u1ecb r\u1ea1p phim CineCore.", 13);
    subTxt.setFillColor(Color(100, 116, 139));
    subTxt.setPosition({leftX, cardY + 196.f});
    window.draw(subTxt);

    // 4. Form Fields
    struct FieldMeta {
        int idx;
        wstring label;
        wstring placeholder;
        bool isPass;
    };
    const array<FieldMeta, 6> fields = {{
        {0, L"H\u1ecc V\u00c0 T\u00caN", L"Nh\u1eadp h\u1ecd v\u00e0 t\u00ean \u0111\u1ea7y \u0111\u1ee7", false},
        {1, L"NG\u00c0Y SINH (DD/MM/YYYY)", L"dd/mm/yyyy", false},
        {3, L"S\u1ed0 \u0110I\u1ec6N THO\u1ea0I", L"0123456789", false},
        {2, L"EMAIL DOANH NGHI\u1ec6P", L"example@cinecore.com", false},
        {4, L"M\u1eacT KH\u1ea8U", L"T\u1ed1i thi\u1ec3u 8 k\u00fd t\u1ef1", true},
        {5, L"X\u00c1C NH\u1eacN M\u1eacT KH\u1ea8U", L"Nh\u1eadp l\u1ea1i m\u1eadt kh\u1ea9u", true}
    }};

    for (const auto& fm : fields) {
        const FloatRect& r = fieldRects[fm.idx];
        bool isActive = (activeField == fm.idx);

        Text lbl(boldFont, fm.label, 11.f);
        lbl.setFillColor(Color(51, 65, 85));
        lbl.setPosition({r.position.x, r.position.y - 21.f});
        window.draw(lbl);

        FloatRect lb = lbl.getLocalBounds();
        Text star(boldFont, L"*", 13);
        star.setFillColor(Color(239, 68, 68));
        star.setPosition({r.position.x + lb.size.x + 4.f, r.position.y - 21.f});
        window.draw(star);

        Color boxFill = isActive ? Color::White : Color(248, 250, 252);
        Color boxBorder = isActive ? Color(2, 132, 199) : Color(226, 232, 240);
        float borderThick = isActive ? 1.6f : 1.f;
        RoundedRectRenderer::draw(window, r.position, r.size, 10.f, boxFill, borderThick, boxBorder);

        Color icoColor = isActive ? Color(2, 132, 199) : Color(148, 163, 184);
        Vector2f icoPos(r.position.x + 14.f, r.position.y + 15.f);
        if (fm.idx == 0) drawVectorUser(window, icoPos, icoColor);
        else if (fm.idx == 1) drawVectorCalendar(window, icoPos, icoColor);
        else if (fm.idx == 3) drawVectorPhone(window, icoPos, icoColor);
        else if (fm.idx == 2) drawVectorMail(window, icoPos, icoColor);
        else if (fm.idx == 4) drawVectorLock(window, icoPos, icoColor);
        else drawVectorShield(window, icoPos, icoColor);

        float textX = r.position.x + 38.f;
        float textY = r.position.y + 12.f;

        if (inputs[fm.idx].empty()) {
            Text ph(font, fm.placeholder, 13.f);
            ph.setFillColor(Color(148, 163, 184));
            ph.setPosition({textX, textY});
            window.draw(ph);

            if (isActive && showCursor) {
                RectangleShape cur({1.8f, 20.f});
                cur.setFillColor(Color(2, 132, 199));
                cur.setPosition({textX, r.position.y + 12.f});
                window.draw(cur);
            }
        } else {
            wstring shown = (fm.isPass && !showPassword) ? bullets(inputs[fm.idx].size()) : inputs[fm.idx];
            Text val(font, shown, 13);
            val.setFillColor(Color(15, 23, 42));
            val.setPosition({textX, textY});
            window.draw(val);

            if (isActive && showCursor) {
                FloatRect vb = val.getLocalBounds();
                RectangleShape cur({1.8f, 20.f});
                cur.setFillColor(Color(2, 132, 199));
                cur.setPosition({textX + vb.size.x + 2.f, r.position.y + 12.f});
                window.draw(cur);
            }
        }

        if (fm.idx == 4) {
            drawVectorEye(window, {eyeRect.position.x + 4.f, eyeRect.position.y + 6.f}, showPassword, eyeHovered);
        }
    }

    // 5. Terms Checkbox
    Vector2f chkBoxPos(leftX, checkRect.position.y + 3.f);
    Color chkBg = agreeTerms ? Color(2, 132, 199) : Color::White;
    Color chkBorder = agreeTerms ? Color(2, 132, 199) : (checkHovered ? Color(100, 116, 139) : Color(203, 213, 225));
    RoundedRectRenderer::draw(window, chkBoxPos, {16.f, 16.f}, 4.f, chkBg, 1.3f, chkBorder);
    if (agreeTerms) {
        drawVectorCheck(window, {chkBoxPos.x + 3.f, chkBoxPos.y + 4.f}, Color::White);
    }

    Text chk1(font, L"T\u00f4i \u0111\u1ed3ng \u00fd v\u1edbi ", 12.f);
    chk1.setFillColor(Color(100, 116, 139));
    chk1.setPosition({leftX + 26.f, chkBoxPos.y});
    window.draw(chk1);
    float chk1W = chk1.getLocalBounds().size.x;

    Text chkLink(boldFont, L"Quy ch\u1ebf n\u1ed9i b\u1ed9", 12.f);
    chkLink.setFillColor(Color(2, 132, 199));
    chkLink.setPosition({leftX + 26.f + chk1W + 2.f, chkBoxPos.y});
    window.draw(chkLink);
    float chkLinkW = chkLink.getLocalBounds().size.x;

    Text chk2(font, L" v\u00e0 cam k\u1ebft b\u1ea3o m\u1eadt d\u1eef li\u1ec7u doanh thu c\u1ee7a c\u1ee5m r\u1ea1p.", 12.f);
    chk2.setFillColor(Color(100, 116, 139));
    chk2.setPosition({leftX + 26.f + chk1W + chkLinkW + 3.f, chkBoxPos.y});
    window.draw(chk2);

    // Status message (if any)
    if (!statusMessage.empty()) {
        Text stTxt(boldFont, statusMessage, 12.f);
        stTxt.setFillColor(statusColor);
        stTxt.setPosition({leftX, checkRect.position.y + 25.f});
        window.draw(stTxt);
    }

    // 6. Action Buttons Row
    Color backBg = backHovered ? Color(248, 250, 252) : Color::White;
    RoundedRectRenderer::draw(window, backRect.position, backRect.size, 11.f, backBg, 1.2f, Color(203, 213, 225));
    Text backTxt(boldFont, L"Quay l\u1ea1i", 13);
    backTxt.setFillColor(Color(51, 65, 85));
    FloatRect bb = backTxt.getLocalBounds();
    float backTotalW = 16.f + bb.size.x;
    float backStartX = backRect.position.x + (backRect.size.x - backTotalW) / 2.f;
    drawVectorArrow(window, {backStartX, backRect.position.y + 23.f}, false, Color(51, 65, 85));
    backTxt.setPosition({backStartX + 18.f, backRect.position.y + (backRect.size.y - bb.size.y) / 2.f - bb.position.y});
    window.draw(backTxt);

    Color regBg = registerHovered ? Color(3, 105, 161) : Color(0, 119, 182);
    RoundedRectRenderer::draw(window, registerRect.position, registerRect.size, 11.f, regBg);
    Text regTxt(boldFont, L"\u0110\u0102NG K\u00dd T\u00c0I KHO\u1ea2N", 13);
    regTxt.setFillColor(Color::White);
    FloatRect rb = regTxt.getLocalBounds();
    float regTotalW = rb.size.x + 20.f;
    float regStartX = registerRect.position.x + (registerRect.size.x - regTotalW) / 2.f;
    regTxt.setPosition({regStartX, registerRect.position.y + (registerRect.size.y - rb.size.y) / 2.f - rb.position.y});
    window.draw(regTxt);
    drawVectorArrow(window, {regStartX + rb.size.x + 10.f, registerRect.position.y + 23.f}, true, Color::White);

    // 7. Bottom Footer Link inside Card
    RectangleShape botDiv({innerW, 1.f});
    botDiv.setPosition({leftX, cardY + cardH - 70.f});
    botDiv.setFillColor(Color(241, 245, 249));
    window.draw(botDiv);

    Text footQ(font, L"\u0110\u00e3 c\u00f3 t\u00e0i kho\u1ea3n nh\u00e2n vi\u00ean ho\u1eb7c qu\u1ea3n tr\u1ecb? ", 12.f);
    footQ.setFillColor(Color(100, 116, 139));
    Text footL(boldFont, L"\u0110\u0103ng nh\u1eadp ngay \u2192", 12.f);
    footL.setFillColor(loginLinkHovered ? Color(3, 105, 161) : Color(2, 132, 199));

    float totalLinkW = footQ.getLocalBounds().size.x + footL.getLocalBounds().size.x;
    float linkStartX = cardX + (cardW - totalLinkW) / 2.f;
    float linkY = cardY + cardH - 48.f;
    footQ.setPosition({linkStartX, linkY});
    footL.setPosition({linkStartX + footQ.getLocalBounds().size.x + 2.f, linkY});
    loginLinkRect = FloatRect({footL.getPosition().x - 4.f, linkY - 4.f}, {footL.getLocalBounds().size.x + 8.f, 24.f});
    window.draw(footQ);
    window.draw(footL);

    // 8. Global Bottom Footer Bar
    Text footLeft(font, L"\u00a9 2025 PBL2 \u2013 B\u1ed9 m\u00f4n \u0110\u1ed3 \u00e1n L\u1eadp tr\u00ecnh t\u00ednh to\u00e1n", 13);
    footLeft.setFillColor(Color(100, 116, 139));
    footLeft.setPosition({24.f, H - 30.f});
    window.draw(footLeft);

    Text footRight(font, L"\u0110i\u1ec1u kho\u1ea3n d\u1ecbch v\u1ee5   \u2022   Tr\u1ee3 gi\u00fap k\u1ef9 thu\u1eadt   \u2022   CineCore Enterprise Edition", 13);
    footRight.setFillColor(Color(100, 116, 139));
    FloatRect frb = footRight.getLocalBounds();
    footRight.setPosition({W - 24.f - frb.size.x, H - 30.f});
    window.draw(footRight);
}

void RegisterScreen::reset() {
    for (int i = 0; i < 6; i++) inputs[i].clear();
    activeField = -1;
    showPassword = false;
    agreeTerms = false;
    showSuccessMessage = false;
    statusMessage.clear();
}

// =========================================================================
// VECTOR ICON HELPERS
// =========================================================================

void RegisterScreen::drawDotGrid(RenderTarget& target, float W, float H) const {
    CircleShape dot(1.1f, 6);
    dot.setFillColor(Color(218, 224, 232));
    for (float x = 28.f; x < W; x += 28.f) {
        for (float y = 28.f; y < H; y += 28.f) {
            dot.setPosition({x, y});
            target.draw(dot);
        }
    }
}

void RegisterScreen::drawVectorPortalLogo(RenderTarget& target, Vector2f pos, float size) const {
    RoundedRectRenderer::draw(target, pos, {size, size}, size * 0.27f, Color(0, 119, 182));
    float cx = pos.x + size / 2.f;
    float cy = pos.y + size / 2.f;
    float r = size * 0.28f;
    CircleShape ring(r, 20);
    ring.setFillColor(Color::Transparent);
    ring.setOutlineThickness(1.8f);
    ring.setOutlineColor(Color::White);
    ring.setPosition({cx - r, cy - r});
    target.draw(ring);

    ConvexShape tri(3);
    tri.setPoint(0, {cx - r * 0.30f, cy - r * 0.42f});
    tri.setPoint(1, {cx + r * 0.50f, cy});
    tri.setPoint(2, {cx - r * 0.30f, cy + r * 0.42f});
    tri.setFillColor(Color::White);
    target.draw(tri);
}

void RegisterScreen::drawVectorUser(RenderTarget& target, Vector2f pos, Color color) const {
    CircleShape head(3.2f, 12);
    head.setFillColor(Color::Transparent);
    head.setOutlineThickness(1.4f);
    head.setOutlineColor(color);
    head.setPosition({pos.x + 4.f, pos.y});
    target.draw(head);

    RoundedRectRenderer::draw(target, {pos.x + 1.f, pos.y + 8.f}, {12.f, 6.f}, 3.f, Color::Transparent, 1.4f, color);
}

void RegisterScreen::drawVectorCalendar(RenderTarget& target, Vector2f pos, Color color) const {
    RoundedRectRenderer::draw(target, {pos.x + 1.f, pos.y + 2.f}, {13.f, 12.f}, 2.5f, Color::Transparent, 1.4f, color);
    RectangleShape topBar({13.f, 1.3f});
    topBar.setPosition({pos.x + 1.f, pos.y + 6.f});
    topBar.setFillColor(color);
    target.draw(topBar);
    RectangleShape pin1({1.4f, 3.f}); pin1.setPosition({pos.x + 4.f, pos.y}); pin1.setFillColor(color); target.draw(pin1);
    RectangleShape pin2({1.4f, 3.f}); pin2.setPosition({pos.x + 10.f, pos.y}); pin2.setFillColor(color); target.draw(pin2);
}

void RegisterScreen::drawVectorPhone(RenderTarget& target, Vector2f pos, Color color) const {
    RoundedRectRenderer::draw(target, {pos.x + 3.f, pos.y}, {9.f, 14.f}, 2.f, Color::Transparent, 1.4f, color);
    CircleShape dot(1.f, 8);
    dot.setFillColor(color);
    dot.setPosition({pos.x + 6.5f, pos.y + 10.5f});
    target.draw(dot);
}

void RegisterScreen::drawVectorMail(RenderTarget& target, Vector2f pos, Color color) const {
    RoundedRectRenderer::draw(target, {pos.x + 1.f, pos.y + 2.f}, {14.f, 10.f}, 2.f, Color::Transparent, 1.4f, color);
    Vertex v[3] = {
        Vertex({pos.x + 2.f, pos.y + 3.f}, color),
        Vertex({pos.x + 8.f, pos.y + 8.f}, color),
        Vertex({pos.x + 14.f, pos.y + 3.f}, color)
    };
    target.draw(v, 3, PrimitiveType::LineStrip);
}

void RegisterScreen::drawVectorLock(RenderTarget& target, Vector2f pos, Color color) const {
    RoundedRectRenderer::draw(target, {pos.x + 3.f, pos.y}, {8.f, 7.f}, 3.5f, Color::Transparent, 1.4f, color);
    RoundedRectRenderer::draw(target, {pos.x + 1.f, pos.y + 5.f}, {12.f, 9.f}, 2.f, Color::Transparent, 1.4f, color);
}

void RegisterScreen::drawVectorShield(RenderTarget& target, Vector2f pos, Color color) const {
    RoundedRectRenderer::draw(target, {pos.x + 1.5f, pos.y + 1.f}, {12.f, 12.f}, 4.f, Color::Transparent, 1.4f, color);
    drawVectorCheck(target, {pos.x + 3.5f, pos.y + 4.f}, color);
}

void RegisterScreen::drawVectorEye(RenderTarget& target, Vector2f pos, bool open, bool hovered) const {
    Color c = hovered ? Color(2, 132, 199) : Color(148, 163, 184);
    RoundedRectRenderer::draw(target, {pos.x, pos.y + 3.f}, {16.f, 10.f}, 5.f, Color::Transparent, 1.4f, c);
    CircleShape pupil(2.5f, 12);
    pupil.setFillColor(c);
    pupil.setPosition({pos.x + 5.5f, pos.y + 5.5f});
    target.draw(pupil);
    if (!open) {
        Vertex slash[2] = {
            Vertex({pos.x + 2.f, pos.y + 13.f}, c),
            Vertex({pos.x + 14.f, pos.y + 2.f}, c)
        };
        target.draw(slash, 2, PrimitiveType::Lines);
    }
}

void RegisterScreen::drawVectorArrow(RenderTarget& target, Vector2f pos, bool right, Color color) const {
    float dir = right ? 1.f : -1.f;
    float x0 = right ? pos.x : pos.x + 10.f;
    float x1 = x0 + 10.f * dir;
    Vertex shaft[2] = {Vertex({x0, pos.y}, color), Vertex({x1, pos.y}, color)};
    Vertex head[3] = {
        Vertex({x1 - 4.f * dir, pos.y - 4.f}, color),
        Vertex({x1, pos.y}, color),
        Vertex({x1 - 4.f * dir, pos.y + 4.f}, color)
    };
    target.draw(shaft, 2, PrimitiveType::Lines);
    target.draw(head, 3, PrimitiveType::LineStrip);
}

void RegisterScreen::drawVectorCheck(RenderTarget& target, Vector2f pos, Color color) const {
    Vertex chk[3] = {
        Vertex({pos.x, pos.y + 4.f}, color),
        Vertex({pos.x + 3.5f, pos.y + 7.5f}, color),
        Vertex({pos.x + 9.5f, pos.y + 1.f}, color)
    };
    target.draw(chk, 3, PrimitiveType::LineStrip);
}
