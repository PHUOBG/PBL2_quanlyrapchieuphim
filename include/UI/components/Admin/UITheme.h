#pragma once

#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <cmath>
#include <algorithm>
#include "UI/components/Admin/RoundedRectRenderer.h"
#include "UI/components/Admin/FontManager.h"

namespace UITheme {
    using namespace sf;
    using namespace std;

    // ==========================================
    // 1. TYPOGRAPHY TOKENS (Font Sizes & Weights)
    // ==========================================
    inline constexpr unsigned int PageTitleSize    = 24;
    inline constexpr unsigned int SectionTitleSize = 16;
    inline constexpr unsigned int CardTitleSize    = 14;
    inline constexpr unsigned int BodySize         = 16;
    inline constexpr unsigned int TableHeaderSize  = 16;
    inline constexpr unsigned int TableBodySize    = 16;
    inline constexpr unsigned int FormLabelSize    = 14;
    inline constexpr unsigned int ButtonTextSize   = 16;
    inline constexpr unsigned int BreadcrumbSize   = 14;
    inline constexpr unsigned int HelperTextSize   = 14;
    inline constexpr unsigned int SmallBadgeSize   = 14;
    inline constexpr unsigned int KpiValueSize     = 24;

    // ==========================================
    // 2. SPACING SCALE (Multiples of 4px)
    // ==========================================
    inline constexpr float Space4  = 4.f;
    inline constexpr float Space8  = 8.f;
    inline constexpr float Space12 = 12.f;
    inline constexpr float Space16 = 16.f;
    inline constexpr float Space20 = 20.f;
    inline constexpr float Space24 = 24.f;
    inline constexpr float Space32 = 32.f;

    // ==========================================
    // 3. COLOR PALETTE TOKENS
    // ==========================================
    // Primary Brand Blue (CinExiNe / CineCore)
    inline const Color PrimaryNavy     = Color(18, 59, 93);     // #123B5D
    inline const Color Primary         = Color(8, 125, 181);    // #087DB5
    inline const Color PrimaryHover    = Color(6, 103, 150);    // #066796
    inline const Color PrimaryActive   = Color(18, 59, 93);     // #123B5D
    inline const Color PrimaryLight    = Color(239, 246, 255);  // #EFF6FF
    inline const Color PrimaryBorder   = Color(186, 230, 253);  // #BAE6FD

    // Neutrals & Surfaces
    inline const Color PageBg          = Color(244, 246, 249);  // #F4F6F9 Canvas
    inline const Color Surface         = Color(255, 255, 255);  // #FFFFFF Pure White
    inline const Color SurfaceHover    = Color(248, 250, 252);  // #F8FAFC Row Hover
    inline const Color SurfaceActive   = Color(224, 242, 254);  // #E0F2FE Selected Row
    inline const Color SurfaceSubtle   = Color(241, 245, 249);  // #F1F5F9 Badge / Input bg

    // Typography Colors
    inline const Color TextPrimary     = Color(23, 32, 51);     // #172033
    inline const Color TextSecondary   = Color(100, 116, 139);  // #64748B
    inline const Color TextTertiary    = Color(148, 163, 184);  // #94A3B8
    inline const Color TextWhite       = Color(255, 255, 255);  // #FFFFFF

    // Borders & Dividers
    inline const Color BorderLight     = Color(226, 232, 240);  // #E2E8F0
    inline const Color BorderMedium    = Color(203, 213, 225);  // #CBD5E1

    // Semantic Colors
    inline const Color Success         = Color(21, 128, 61);    // #15803D
    inline const Color SuccessDark     = Color(21, 128, 61);    // #15803D
    inline const Color SuccessLight    = Color(220, 252, 231);  // #DCFCE7

    inline const Color Warning         = Color(217, 119, 6);    // #D97706
    inline const Color WarningDark     = Color(180, 83, 9);     // #B45309
    inline const Color WarningLight    = Color(254, 243, 199);  // #FEF3C7

    inline const Color Danger          = Color(220, 38, 38);    // #DC2626
    inline const Color DangerDark      = Color(185, 28, 28);    // #B91C1C
    inline const Color DangerLight     = Color(254, 226, 226);  // #FEE2E2

    inline const Color Info            = Color(8, 125, 181);    // #087DB5
    inline const Color InfoDark        = Color(18, 59, 93);     // #123B5D
    inline const Color InfoLight       = Color(224, 242, 254);  // #E0F2FE

    // ==========================================
    // 4. SHARED COMPONENT RENDERERS
    // ==========================================

    // Status Badge Variants
    enum class BadgeVariant {
        Success, // Green (Đang chiếu, Hoạt động, Thành công)
        Warning, // Amber (Sắp chiếu, Chờ duyệt, Tạm dừng)
        Danger,  // Red (Hết vé, Hủy, Quá hạn)
        Info,    // Blue (VIP, Khuyến mãi, Định dạng)
        Neutral  // Slate / Gray (Mặc định)
    };

    // Render a standardized status badge with optional dot indicator
    inline void drawBadge(RenderTarget& target, Vector2f pos, const wstring& label,
                          BadgeVariant variant, const Font& font, bool withDot = true, float height = 26.f) {
        Color bg, text, dot;
        switch (variant) {
            case BadgeVariant::Success:
                bg = SuccessLight; text = SuccessDark; dot = SuccessDark; break;
            case BadgeVariant::Warning:
                bg = WarningLight; text = WarningDark; dot = WarningDark; break;
            case BadgeVariant::Danger:
                bg = DangerLight; text = DangerDark; dot = DangerDark; break;
            case BadgeVariant::Info:
                bg = InfoLight; text = InfoDark; dot = InfoDark; break;
            case BadgeVariant::Neutral:
            default:
                bg = SurfaceSubtle; text = TextSecondary; dot = TextSecondary; break;
        }

        Text txt(font, label, SmallBadgeSize);
        txt.setFillColor(text);
        FloatRect b = txt.getLocalBounds();

        float paddingH = 12.f;
        float dotOffset = withDot ? 12.f : 0.f;
        float totalW = b.size.x + paddingH * 2.f + dotOffset;
        float radius = height / 2.f;

        RoundedRectRenderer::draw(target, pos, {totalW, height}, radius, bg);

        if (withDot) {
            CircleShape cDot(3.f, 10);
            cDot.setFillColor(dot);
            cDot.setPosition({pos.x + 10.f, pos.y + (height - 6.f) / 2.f});
            target.draw(cDot);
        }

        float textX = std::round(pos.x + (withDot ? 20.f : paddingH) - b.position.x);
        float textY = std::round(pos.y + (height - b.size.y) / 2.f - b.position.y - 1.f);
        txt.setPosition({textX, textY});
        target.draw(txt);
    }

    // Standard Top Bar Navigation & Admin Profile Header
    inline void drawTopBar(RenderTarget& target, float startX, float panelY, float panelWidth,
                           const wstring& sectionName, const Font& font, const Font& boldFont) {
        (void)sectionName;
        (void)font;

        // 1. "CỔNG QUẢN TRỊ" Pill Badge (Tự co giãn chuẩn xác theo nội dung, căn giữa hoàn hảo)
        float bcY = panelY + 18.f;
        Text bc1(boldFont, L"CỔNG QUẢN TRỊ", BreadcrumbSize);
        bc1.setFillColor(Color(2, 132, 199));
        FloatRect bcb = bc1.getLocalBounds();

        float pillH = 30.f;
        float pillW = bcb.size.x + 36.f; // 10px lề trái + 7px chấm tròn + 6px khoảng cách + chữ + 13px lề phải
        RoundedRectRenderer::draw(target, {startX, bcY}, {pillW, pillH}, pillH / 2.f, Color(224, 242, 254), 1.f, Color(186, 230, 253));

        CircleShape pin(3.5f, 10);
        pin.setFillColor(Color(2, 132, 199));
        pin.setPosition({startX + 10.f, bcY + (pillH - 7.f) / 2.f});
        target.draw(pin);

        bc1.setPosition({
            startX + 22.f - bcb.position.x,
            bcY + (pillH - bcb.size.y) / 2.f - bcb.position.y - 1.f
        });
        target.draw(bc1);

        // (Đã loại bỏ chuỗi điều hướng tĩnh '/ HỆ THỐNG / QUẢN LÝ PHIM' theo yêu cầu)
        // (Đã loại bỏ chuông thông báo không có chức năng theo yêu cầu)

        // 2. Admin Profile Pill (Cân đối hoàn hảo ảnh đại diện và chữ)
        float userW = 264.f;
        float userH = 48.f;
        float userY = panelY + 10.f;
        float userX = startX + panelWidth - userW;

        RoundedRectRenderer::draw(target, {userX, userY}, {userW, userH}, userH / 2.f, Color::White, 1.2f, Color(203, 213, 225));

        // Avatar tròn đường kính 36px
        float avRadius = 18.f;
        float avX = userX + 7.f;
        float avY = userY + (userH - avRadius * 2.f) / 2.f;
        CircleShape av(avRadius, 24);
        av.setFillColor(Color(0, 119, 182));
        av.setPosition({avX, avY});
        target.draw(av);

        // Ký tự "AD" căn giữa tuyệt đối trong vòng tròn
        Text avTxt(boldFont, L"AD", SmallBadgeSize);
        avTxt.setFillColor(Color::White);
        FloatRect atb = avTxt.getLocalBounds();
        avTxt.setPosition({
            avX + avRadius - atb.size.x / 2.f - atb.position.x,
            avY + avRadius - atb.size.y / 2.f - atb.position.y - 1.f
        });
        target.draw(avTxt);

        // Khối văn bản Admin CineCore & Toàn quyền
        float textStartX = avX + avRadius * 2.f + 10.f;
        Text uName(boldFont, L"Admin CineCore", BodySize);
        uName.setFillColor(Color(15, 23, 42));
        FloatRect unb = uName.getLocalBounds();
        uName.setPosition({
            textStartX - unb.position.x,
            userY + 6.f - unb.position.y
        });
        target.draw(uName);

        Text uRole(font, L"Toàn quyền • SuperAdmin", HelperTextSize);
        uRole.setFillColor(Color(100, 116, 139));
        FloatRect urb = uRole.getLocalBounds();
        uRole.setPosition({
            textStartX - urb.position.x,
            userY + 26.f - urb.position.y
        });
        target.draw(uRole);
    }

    // Standard Page Title & Optional Subtitle
    inline void drawPageHeader(RenderTarget& target, float startX, float posY,
                               const wstring& title, const wstring& subtitle,
                               const Font& boldFont, const Font& font) {
        FontManager::drawText(target, boldFont, title, PageTitleSize, TextPrimary, {startX, posY});
        if (!subtitle.empty()) {
            FontManager::drawText(target, font, subtitle, BodySize, TextSecondary, {startX, posY + 34.f});
        }
    }

    // Unified Section Header Card (matching RoomPanel card style)
    inline void drawSectionHeaderCard(RenderTarget& target, float startX, float posY, float cardW,
                                      const wstring& title, const wstring& badgeText,
                                      Color badgeBg, Color badgeFg, const wstring& subtitle,
                                      const Font& boldFont, const Font& font, bool withDot = false) {
        RoundedRectRenderer::draw(target, {startX, posY}, {cardW, 84.f}, 12.f, Surface, 1.f, BorderLight);

        // Soft blue icon container + CineCore logo
        RoundedRectRenderer::draw(target, {startX + 18.f, posY + 16.f}, {52.f, 52.f}, 12.f, Color(224, 242, 254));
        static Texture s_logoTex;
        static bool s_logoLoaded = false;
        static bool s_logoTried = false;
        if (!s_logoTried) {
            s_logoTried = true;
            s_logoLoaded = s_logoTex.loadFromFile("assets/elements/cinexine_icon.png") ||
                           s_logoTex.loadFromFile("../assets/elements/cinexine_icon.png") ||
                           s_logoTex.loadFromFile("assets/elements/icon.png") ||
                           s_logoTex.loadFromFile("../assets/elements/icon.png");
            if (s_logoLoaded) s_logoTex.setSmooth(true);
        }
        if (s_logoLoaded) {
            Sprite logoSpr(s_logoTex);
            float targetS = 36.f;
            logoSpr.setScale({targetS / (float)s_logoTex.getSize().x, targetS / (float)s_logoTex.getSize().y});
            logoSpr.setPosition({startX + 18.f + (52.f - targetS) / 2.f, posY + 16.f + (52.f - targetS) / 2.f});
            target.draw(logoSpr);
        }

        float textX = std::round(startX + 85.f);
        float titleY = std::round(subtitle.empty() ? (posY + 28.f) : (posY + 13.f));
        Text titleTxt(boldFont, title, PageTitleSize);
        titleTxt.setFillColor(TextPrimary);
        titleTxt.setPosition({textX, titleY});
        target.draw(titleTxt);

        if (!badgeText.empty()) {
            FloatRect tb = titleTxt.getGlobalBounds();
            Text bTxt(boldFont, badgeText, SmallBadgeSize);
            bTxt.setFillColor(badgeFg);
            FloatRect btb = bTxt.getLocalBounds();
            float dotR = 3.5f;
            float dotGap = 6.f;
            float padH = 12.f;
            float innerW = (withDot ? (dotR * 2.f + dotGap) : 0.f) + btb.size.x;
            float badgeW = std::round(padH * 2.f + innerW);
            float badgeH = 24.f;
            float badgeX = std::round(tb.position.x + tb.size.x + 14.f);
            float badgeY = std::round(tb.position.y + (tb.size.y - badgeH) / 2.f);
            RoundedRectRenderer::draw(target, {badgeX, badgeY}, {badgeW, badgeH}, 12.f, badgeBg);

            float cStartX = badgeX + (badgeW - innerW) / 2.f;
            if (withDot) {
                CircleShape dot(dotR);
                dot.setFillColor(badgeFg);
                dot.setPosition({std::round(cStartX), std::round(badgeY + (badgeH - dotR * 2.f) / 2.f)});
                target.draw(dot);
                cStartX += dotR * 2.f + dotGap;
            }
            bTxt.setPosition({
                std::round(cStartX - btb.position.x),
                std::round(badgeY + (badgeH - btb.size.y) / 2.f - btb.position.y)
            });
            target.draw(bTxt);
        }

        if (!subtitle.empty()) {
            Text subTxt(font, subtitle, 17);
            subTxt.setFillColor(TextSecondary);
            subTxt.setPosition({textX, std::round(posY + 45.f)});
            target.draw(subTxt);
        }
    }

    // Standard KPI Summary Stat Card
    inline void drawKpiCard(RenderTarget& target, Vector2f pos, Vector2f size,
                            const wstring& title, const wstring& value, const wstring& subtext,
                            Color iconBg, Color iconColor, const Font& boldFont) {
        RoundedRectRenderer::draw(target, pos, size, 8.f, Surface, 1.f, BorderLight);
        (void)iconBg;

        FontManager::drawCenteredText(target, boldFont, title, FormLabelSize, TextSecondary,
                                      {pos, {size.x, 32.f}});
        FontManager::drawCenteredText(target, boldFont, value, KpiValueSize, TextPrimary,
                                      {{pos.x, pos.y + 30.f}, {size.x, 36.f}});
        FontManager::drawCenteredText(target, boldFont, subtext, HelperTextSize, iconColor,
                                      {{pos.x, pos.y + size.y - 32.f}, {size.x, 24.f}});
    }

    inline void drawPrimaryButton(RenderTarget& target, FloatRect rect, const wstring& label,
                                  const Font& boldFont, bool hovered = false, float radius = 6.f) {
        RoundedRectRenderer::draw(target, rect.position, rect.size, radius,
                                  hovered ? PrimaryHover : Primary);
        FontManager::drawCenteredText(target, boldFont, label, ButtonTextSize, TextWhite, rect);
    }

    inline void drawSecondaryButton(RenderTarget& target, FloatRect rect, const wstring& label,
                                    const Font& boldFont, bool hovered = false, float radius = 6.f) {
        RoundedRectRenderer::draw(target, rect.position, rect.size, radius,
                                  hovered ? SurfaceHover : Surface, 1.f, BorderMedium);
        FontManager::drawCenteredText(target, boldFont, label, ButtonTextSize, TextPrimary, rect);
    }

    inline void drawDangerButton(RenderTarget& target, FloatRect rect, const wstring& label,
                                 const Font& boldFont, bool hovered = false, float radius = 6.f) {
        RoundedRectRenderer::draw(target, rect.position, rect.size, radius,
                                  hovered ? DangerDark : Danger);
        FontManager::drawCenteredText(target, boldFont, label, ButtonTextSize, TextWhite, rect);
    }

    inline void drawStatusBadge(RenderTarget& target, FloatRect rect, const wstring& label,
                                Color bg, Color fg, const Font& boldFont, float radius = 12.f) {
        RoundedRectRenderer::draw(target, rect.position, rect.size, radius, bg);
        FontManager::drawCenteredText(target, boldFont, label, SmallBadgeSize, fg, rect);
    }

    inline void drawEmptyState(RenderTarget& target, FloatRect area, const wstring& message,
                               const Font& font) {
        FontManager::drawCenteredText(target, font, message, BodySize, TextSecondary, area);
    }

    inline void drawToastNotification(RenderTarget& target, Vector2f panelPos, float panelW, float panelH,
                                      const wstring& message, const Font& boldFont) {
        Text measure(boldFont, message, ButtonTextSize);
        float tw = measure.getLocalBounds().size.x + 40.f;
        float th = 40.f;
        Vector2f tPos(std::round(panelPos.x + (panelW - tw) / 2.f), std::round(panelPos.y + panelH - 76.f));
        RoundedRectRenderer::draw(target, tPos, {tw, th}, 8.f, PrimaryNavy);
        FontManager::drawCenteredText(target, boldFont, message, ButtonTextSize, TextWhite, {tPos, {tw, th}});
    }

    // ==========================================
    // 4B. REUSABLE VECTOR & TEXTURE ICON RENDERERS
    // ==========================================

    // Vẽ dấu tích chuẩn (Checkmark vector) thay thế ký tự Unicode thiếu trong font
    inline void drawCheckmark(RenderTarget& target, Vector2f center, float scale, Color color, float thickness = 2.f) {
        RectangleShape seg1({5.5f * scale, thickness * scale});
        seg1.setFillColor(color);
        seg1.setOrigin({0.f, thickness * scale / 2.f});
        seg1.setPosition({center.x - 4.5f * scale, center.y + 0.5f * scale});
        seg1.setRotation(degrees(42.f));
        target.draw(seg1);

        RectangleShape seg2({9.5f * scale, thickness * scale});
        seg2.setFillColor(color);
        seg2.setOrigin({0.f, thickness * scale / 2.f});
        seg2.setPosition({center.x - 1.2f * scale, center.y + 3.8f * scale});
        seg2.setRotation(degrees(-48.f));
        target.draw(seg2);
    }

    // Vẽ nút đóng / xóa (Close X vector)
    inline void drawCloseIcon(RenderTarget& target, Vector2f center, float size, Color color, float thickness = 1.8f) {
        for (float angle : {45.f, -45.f}) {
            RectangleShape bar({size, thickness});
            bar.setOrigin({size / 2.f, thickness / 2.f});
            bar.setPosition(center);
            bar.setFillColor(color);
            bar.setRotation(degrees(angle));
            target.draw(bar);
        }
    }

    // Vẽ mũi tên tam giác chỉ xuống (Dropdown Chevron)
    inline void drawChevronDown(RenderTarget& target, Vector2f center, float halfW, Color color) {
        ConvexShape chev(3);
        chev.setPoint(0, {center.x - halfW, center.y - halfW * 0.55f});
        chev.setPoint(1, {center.x + halfW, center.y - halfW * 0.55f});
        chev.setPoint(2, {center.x, center.y + halfW * 0.65f});
        chev.setFillColor(color);
        target.draw(chev);
    }

    // Vẽ biểu tượng Làm mới (Reload) vector sắc nét, đồng bộ toàn dự án
    inline void drawReloadIcon(RenderTarget& target, Vector2f center, float size, Color color) {
        float outerR = max(7.0f, min(9.0f, size * 0.45f));
        float innerR = outerR - 2.1f;
        float midR = (outerR + innerR) * 0.5f;
        constexpr float PI = 3.14159265f;
        float startRad = 25.f * PI / 180.f;
        float endRad = 305.f * PI / 180.f;
        const int steps = 28;

        VertexArray arc(PrimitiveType::TriangleStrip, (steps + 1) * 2);
        for (int i = 0; i <= steps; ++i) {
            float t = static_cast<float>(i) / static_cast<float>(steps);
            float a = startRad + (endRad - startRad) * t;
            float ca = std::cos(a);
            float sa = std::sin(a);
            arc[i * 2].position = {center.x + ca * innerR, center.y + sa * innerR};
            arc[i * 2].color = color;
            arc[i * 2 + 1].position = {center.x + ca * outerR, center.y + sa * outerR};
            arc[i * 2 + 1].color = color;
        }
        target.draw(arc);

        float nx = std::cos(endRad);
        float ny = std::sin(endRad);
        float tx = -ny;
        float ty = nx;
        Vector2f tipBase(center.x + nx * midR, center.y + ny * midR);

        ConvexShape arrow(3);
        arrow.setPoint(0, {tipBase.x + tx * 4.8f, tipBase.y + ty * 4.8f});
        arrow.setPoint(1, {tipBase.x - tx * 1.0f + nx * 3.8f, tipBase.y - ty * 1.0f + ny * 3.8f});
        arrow.setPoint(2, {tipBase.x - tx * 1.0f - nx * 3.8f, tipBase.y - ty * 1.0f - ny * 3.8f});
        arrow.setFillColor(color);
        target.draw(arrow);
    }

    // Vẽ biểu tượng Mắt ẩn/hiện mật khẩu (tái sử dụng eye-open.png / eye-close.png của PasswordField)
    inline void drawEyeIcon(RenderTarget& target, Vector2f center, bool isOpen, Color tint = Color(100, 116, 139)) {
        static Texture openTex, closeTex;
        static bool attempted = false;
        static bool loaded = false;
        if (!attempted) {
            attempted = true;
            bool ok1 = openTex.loadFromFile("../assets/elements/eye-open.png") ||
                       openTex.loadFromFile("assets/elements/eye-open.png");
            bool ok2 = closeTex.loadFromFile("../assets/elements/eye-close.png") ||
                       closeTex.loadFromFile("assets/elements/eye-close.png");
            loaded = ok1 && ok2;
            if (loaded) {
                openTex.setSmooth(true);
                closeTex.setSmooth(true);
            }
        }
        if (loaded) {
            const Texture& tex = isOpen ? openTex : closeTex;
            Sprite sp(tex);
            float targetS = 16.f;
            float scX = targetS / static_cast<float>(tex.getSize().x);
            float scY = targetS / static_cast<float>(tex.getSize().y);
            sp.setScale({scX, scY});
            sp.setColor(tint);
            sp.setPosition({center.x - targetS / 2.f, center.y - targetS / 2.f});
            target.draw(sp);
        } else {
            CircleShape outer(5.5f, 16);
            outer.setScale({1.35f, 0.85f});
            outer.setFillColor(Color::Transparent);
            outer.setOutlineThickness(1.4f);
            outer.setOutlineColor(tint);
            outer.setPosition({center.x - 7.4f, center.y - 4.6f});
            target.draw(outer);

            CircleShape pupil(2.2f, 12);
            pupil.setFillColor(tint);
            pupil.setPosition({center.x - 2.2f, center.y - 2.2f});
            target.draw(pupil);

            if (!isOpen) {
                RectangleShape slash({14.f, 1.6f});
                slash.setOrigin({7.f, 0.8f});
                slash.setPosition(center);
                slash.setFillColor(tint);
                slash.setRotation(degrees(-35.f));
                target.draw(slash);
            }
        }
    }

    // Vẽ biểu tượng Khiên bảo mật (Shield icon)
    inline void drawShieldIcon(RenderTarget& target, Vector2f center, float r, Color color) {
        ConvexShape shield(6);
        shield.setPoint(0, {center.x, center.y - r});
        shield.setPoint(1, {center.x + r * 0.85f, center.y - r * 0.55f});
        shield.setPoint(2, {center.x + r * 0.75f, center.y + r * 0.25f});
        shield.setPoint(3, {center.x, center.y + r});
        shield.setPoint(4, {center.x - r * 0.75f, center.y + r * 0.25f});
        shield.setPoint(5, {center.x - r * 0.85f, center.y - r * 0.55f});
        shield.setFillColor(color);
        target.draw(shield);
    }

    // Vẽ biểu tượng Thẻ hồ sơ (ID Card icon) thay cho emoji 💳 bị lỗi ô vuông
    inline void drawIdCardIcon(RenderTarget& target, Vector2f center, Color color) {
        RoundedRectRenderer::draw(target, {center.x - 9.f, center.y - 6.5f}, {18.f, 13.f}, 2.5f, Color::Transparent, 1.6f, color);
        CircleShape head(2.2f, 12);
        head.setFillColor(color);
        head.setPosition({center.x - 6.5f, center.y - 3.f});
        target.draw(head);

        RectangleShape line1({6.5f, 1.6f});
        line1.setFillColor(color);
        line1.setPosition({center.x - 0.5f, center.y - 2.8f});
        target.draw(line1);

        RectangleShape line2({5.f, 1.6f});
        line2.setFillColor(color);
        line2.setPosition({center.x - 0.5f, center.y + 1.f});
        target.draw(line2);
    }

    // Vẽ biểu tượng Chìa khóa (Key icon) thay cho emoji 🔑
    inline void drawKeyIcon(RenderTarget& target, Vector2f center, Color color) {
        CircleShape head(4.2f, 16);
        head.setFillColor(Color::Transparent);
        head.setOutlineThickness(1.8f);
        head.setOutlineColor(color);
        head.setPosition({center.x - 7.5f, center.y - 4.2f});
        target.draw(head);

        RectangleShape shaft({8.5f, 2.f});
        shaft.setFillColor(color);
        shaft.setPosition({center.x + 0.5f, center.y - 1.f});
        target.draw(shaft);

        RectangleShape tooth1({1.8f, 3.2f});
        tooth1.setFillColor(color);
        tooth1.setPosition({center.x + 4.2f, center.y + 0.8f});
        target.draw(tooth1);

        RectangleShape tooth2({1.8f, 3.2f});
        tooth2.setFillColor(color);
        tooth2.setPosition({center.x + 7.2f, center.y + 0.8f});
        target.draw(tooth2);
    }

    // Vẽ biểu tượng Ổ khóa (Lock icon) thay cho emoji 🔒
    inline void drawLockIcon(RenderTarget& target, Vector2f center, Color color) {
        CircleShape shackle(3.5f, 14);
        shackle.setFillColor(Color::Transparent);
        shackle.setOutlineThickness(1.6f);
        shackle.setOutlineColor(color);
        shackle.setPosition({center.x - 3.5f, center.y - 6.8f});
        target.draw(shackle);

        RoundedRectRenderer::draw(target, {center.x - 5.5f, center.y - 1.5f}, {11.f, 8.f}, 2.f, color);
        CircleShape keyhole(1.2f, 8);
        keyhole.setFillColor(Color::White);
        keyhole.setPosition({center.x - 1.2f, center.y + 1.2f});
        target.draw(keyhole);
    }

    // Vẽ biểu tượng Khách hàng (User silhouette icon)
    inline void drawUserIcon(RenderTarget& target, Vector2f center, Color color) {
        CircleShape head(3.2f, 14);
        head.setFillColor(color);
        head.setPosition({center.x - 3.2f, center.y - 6.5f});
        target.draw(head);

        RoundedRectRenderer::draw(target, {center.x - 6.f, center.y + 0.8f}, {12.f, 5.5f}, 2.8f, color);
    }

    // ==========================================
    // 4C. KPI & HEADER VECTOR ICONS (Cân đối quang học, sắc nét)
    // ==========================================

    // Vẽ biểu tượng Phát phim (Play triangle) - Cân đối trọng tâm quang học tuyệt đối
    inline void drawPlayIcon(RenderTarget& target, Vector2f center, float size, Color color) {
        ConvexShape tri(3);
        float h = size;
        float w = size * 0.9f;
        // Trọng tâm quang học tam giác hướng phải: đáy ở -0.42w, đỉnh phải ở +0.58w
        tri.setPoint(0, {center.x - w * 0.42f, center.y - h * 0.5f});
        tri.setPoint(1, {center.x + w * 0.58f, center.y});
        tri.setPoint(2, {center.x - w * 0.42f, center.y + h * 0.5f});
        tri.setFillColor(color);
        target.draw(tri);
    }

    // Vẽ biểu tượng Lịch chiếu (Calendar pad) - Có 2 chốt ghim trên, thanh header và các chấm ngày
    inline void drawCalendarIcon(RenderTarget& target, Vector2f center, float size, Color color) {
        float w = size;
        float h = size * 0.9f;
        float left = center.x - w / 2.f;
        float top = center.y - h / 2.f;

        // Khung nền lịch
        RoundedRectRenderer::draw(target, {left, top}, {w, h}, 2.5f, Color::Transparent, 1.5f, color);

        // Thanh ngang đầu lịch
        RectangleShape topBar({w - 1.f, 4.f});
        topBar.setFillColor(color);
        topBar.setPosition({left + 0.5f, top + 0.5f});
        target.draw(topBar);

        // 2 chốt ghim trên cùng
        for (float pegX : {left + w * 0.28f, left + w * 0.72f}) {
            RoundedRectRenderer::draw(target, {pegX - 1.f, top - 2.5f}, {2.f, 3.5f}, 1.f, color);
        }

        // 6 chấm ngày trong thân lịch (2 hàng x 3 cột)
        for (int r = 0; r < 2; ++r) {
            for (int c = 0; c < 3; ++c) {
                float dx = left + w * (0.26f + c * 0.24f);
                float dy = top + h * (0.50f + r * 0.24f);
                RectangleShape dot({2.f, 2.f});
                dot.setFillColor(color);
                dot.setPosition({dx - 1.f, dy - 1.f});
                target.draw(dot);
            }
        }
    }

    // Vẽ biểu tượng Đồng hồ (Clock icon) - Mặt tròn kim giờ & kim phút rõ nét
    inline void drawClockIcon(RenderTarget& target, Vector2f center, float radius, Color color) {
        CircleShape dial(radius, 24);
        dial.setFillColor(Color::Transparent);
        dial.setOutlineThickness(1.6f);
        dial.setOutlineColor(color);
        dial.setPosition({center.x - radius, center.y - radius});
        target.draw(dial);

        // Trục kim giữa
        CircleShape pin(1.5f, 10);
        pin.setFillColor(color);
        pin.setPosition({center.x - 1.5f, center.y - 1.5f});
        target.draw(pin);

        // Kim phút chỉ 12h
        RectangleShape minHand({1.4f, radius * 0.65f});
        minHand.setFillColor(color);
        minHand.setPosition({center.x - 0.7f, center.y - radius * 0.65f});
        target.draw(minHand);

        // Kim giờ chỉ 2h30 (nghiêng 40 độ)
        RectangleShape hrHand({radius * 0.45f, 1.4f});
        hrHand.setFillColor(color);
        hrHand.setPosition({center.x - 0.2f, center.y - 0.7f});
        hrHand.setRotation(degrees(40.f));
        target.draw(hrHand);
    }

    // Vẽ biểu tượng Cuộn phim / Clapperboard điện ảnh
    inline void drawFilmIcon(RenderTarget& target, Vector2f center, float size, Color color) {
        float w = size;
        float h = size * 0.85f;
        float left = center.x - w / 2.f;
        float top = center.y - h / 2.f;

        // Thân clapperboard
        RoundedRectRenderer::draw(target, {left, top}, {w, h}, 2.5f, Color::Transparent, 1.5f, color);

        // Thanh clapper trên cùng
        RectangleShape clapperBar({w, 3.5f});
        clapperBar.setFillColor(color);
        clapperBar.setPosition({left, top});
        target.draw(clapperBar);

        // 3 vạch sọc chéo đặc trưng
        for (int i = 0; i < 3; ++i) {
            RectangleShape stripe({1.4f, 3.5f});
            stripe.setFillColor(Color::White);
            stripe.setRotation(degrees(-25.f));
            stripe.setPosition({left + 3.5f + i * 4.5f, top});
            target.draw(stripe);
        }

        // Mini play icon ở giữa thân
        drawPlayIcon(target, {center.x, center.y + 2.f}, 5.5f, color);
    }

    // ==========================================
    // 5. VIETNAMESE UNICODE & DSA SEARCH HELPERS
    // ==========================================
    inline wstring utf8ToWstr(const string& str) {
        if (str.empty()) return L"";
        return sf::String::fromUtf8(str.begin(), str.end()).toWideString();
    }

    inline string wstrToUtf8(const wstring& wstr) {
        if (wstr.empty()) return "";
        auto u8 = sf::String(wstr).toUtf8();
        return string(u8.begin(), u8.end());
    }

    inline wchar_t stripViAccent(wchar_t c) {
        if (c >= L'A' && c <= L'Z') return c + (L'a' - L'A');
        switch (c) {
            case L'à': case L'á': case L'ả': case L'ã': case L'ạ':
            case L'ă': case L'ằ': case L'ắ': case L'ẳ': case L'ẵ': case L'ặ':
            case L'â': case L'ầ': case L'ấ': case L'ẩ': case L'ẫ': case L'ậ':
            case L'À': case L'Á': case L'Ả': case L'Ã': case L'Ạ':
            case L'Ă': case L'Ằ': case L'Ắ': case L'Ẳ': case L'Ẵ': case L'Ặ':
            case L'Â': case L'Ầ': case L'Ấ': case L'Ẩ': case L'Ẫ': case L'Ậ':
                return L'a';
            case L'đ': case L'Đ':
                return L'd';
            case L'è': case L'é': case L'ẻ': case L'ẽ': case L'ẹ':
            case L'ê': case L'ề': case L'ế': case L'ể': case L'ễ': case L'ệ':
            case L'È': case L'É': case L'Ẻ': case L'Ẽ': case L'Ẹ':
            case L'Ê': case L'Ề': case L'Ế': case L'Ể': case L'Ễ': case L'Ệ':
                return L'e';
            case L'ì': case L'í': case L'ỉ': case L'ĩ': case L'ị':
            case L'Ì': case L'Í': case L'Ỉ': case L'Ĩ': case L'Ị':
                return L'i';
            case L'ò': case L'ó': case L'ỏ': case L'õ': case L'ọ':
            case L'ô': case L'ồ': case L'ố': case L'ổ': case L'ỗ': case L'ộ':
            case L'ơ': case L'ờ': case L'ớ': case L'ở': case L'ỡ': case L'ợ':
            case L'Ò': case L'Ó': case L'Ỏ': case L'Õ': case L'Ọ':
            case L'Ô': case L'Ồ': case L'Ố': case L'Ổ': case L'Ỗ': case L'Ộ':
            case L'Ơ': case L'Ờ': case L'Ớ': case L'Ở': case L'Ỡ': case L'Ợ':
                return L'o';
            case L'ù': case L'ú': case L'ủ': case L'ũ': case L'ụ':
            case L'ư': case L'ừ': case L'ứ': case L'ử': case L'ữ': case L'ự':
            case L'Ù': case L'Ú': case L'Ủ': case L'Ũ': case L'Ụ':
            case L'Ư': case L'Ừ': case L'Ứ': case L'Ử': case L'Ữ': case L'Ự':
                return L'u';
            case L'ỳ': case L'ý': case L'ỷ': case L'ỹ': case L'ỵ':
            case L'Ỳ': case L'Ý': case L'Ỷ': case L'Ỹ': case L'Ỵ':
                return L'y';
            default:
                return c;
        }
    }

    inline wstring normalizeVi(const wstring& s) {
        wstring out;
        out.reserve(s.size());
        for (wchar_t c : s) {
            out.push_back(stripViAccent(c));
        }
        return out;
    }

    inline string normalizeViUtf8(const string& s) {
        wstring ws = utf8ToWstr(s);
        wstring norm = normalizeVi(ws);
        return wstrToUtf8(norm);
    }

    inline bool matchesViQuery(const wstring& text, const wstring& query) {
        if (query.empty()) return true;
        wstring normText = normalizeVi(text);
        wstring normQuery = normalizeVi(query);
        // Trim leading/trailing spaces in query
        size_t first = normQuery.find_first_not_of(L" \t");
        if (first == wstring::npos) return true;
        size_t last = normQuery.find_last_not_of(L" \t");
        normQuery = normQuery.substr(first, last - first + 1);
        return normText.find(normQuery) != wstring::npos;
    }
}
