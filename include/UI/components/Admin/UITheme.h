#pragma once

#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <cmath>
#include <algorithm>
#include "UI/components/Admin/RoundedRectRenderer.h"

namespace UITheme {
    using namespace sf;
    using namespace std;

    // ==========================================
    // 1. TYPOGRAPHY TOKENS (Font Sizes & Weights)
    // ==========================================
    inline constexpr unsigned int PageTitleSize    = 22; // 22px, Weight 700 (boldFont)
    inline constexpr unsigned int SectionTitleSize = 16; // 16px, Weight 600 (boldFont)
    inline constexpr unsigned int CardTitleSize    = 14; // 14px, Weight 600 (boldFont)
    inline constexpr unsigned int BodySize         = 13; // 13px, Weight 400 (font)
    inline constexpr unsigned int TableHeaderSize  = 13; // 12.5-13px, Weight 600 (boldFont)
    inline constexpr unsigned int TableBodySize    = 13; // 13px, Weight 400-500
    inline constexpr unsigned int FormLabelSize    = 13; // 13px, Weight 500
    inline constexpr unsigned int ButtonTextSize   = 13; // 13px, Weight 600 (boldFont)
    inline constexpr unsigned int BreadcrumbSize   = 13; // 13px, Weight 500 (boldFont)
    inline constexpr unsigned int HelperTextSize   = 12; // 12px, Weight 400
    inline constexpr unsigned int SmallBadgeSize   = 12; // 12px, Weight 600
    inline constexpr unsigned int KpiValueSize     = 22; // 22-24px, Weight 700 (boldFont)

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
    inline const Color Primary         = Color(0, 119, 182);    // #0077B6
    inline const Color PrimaryHover    = Color(2, 132, 199);    // #0284C7
    inline const Color PrimaryActive   = Color(0, 95, 146);     // #005F92
    inline const Color PrimaryLight    = Color(239, 246, 255);  // #EFF6FF
    inline const Color PrimaryBorder   = Color(186, 230, 253);  // #BAE6FD

    // Neutrals & Surfaces
    inline const Color PageBg          = Color(244, 246, 249);  // #F4F6F9 Canvas
    inline const Color Surface         = Color(255, 255, 255);  // #FFFFFF Pure White
    inline const Color SurfaceHover    = Color(248, 250, 252);  // #F8FAFC Row Hover
    inline const Color SurfaceActive   = Color(224, 242, 254);  // #E0F2FE Selected Row
    inline const Color SurfaceSubtle   = Color(241, 245, 249);  // #F1F5F9 Badge / Input bg

    // Typography Colors
    inline const Color TextPrimary     = Color(15, 23, 42);     // #0F172A Deep Charcoal/Navy
    inline const Color TextSecondary   = Color(100, 116, 139);  // #64748B Muted Slate
    inline const Color TextTertiary    = Color(148, 163, 184);  // #94A3B8 Dim Slate
    inline const Color TextWhite       = Color(255, 255, 255);  // #FFFFFF

    // Borders & Dividers
    inline const Color BorderLight     = Color(226, 232, 240);  // #E2E8F0 Card borders
    inline const Color BorderMedium    = Color(203, 213, 225);  // #CBD5E1 Inputs & pills

    // Semantic Colors
    inline const Color Success         = Color(16, 185, 129);   // #10B981
    inline const Color SuccessDark     = Color(21, 128, 61);    // #15803D
    inline const Color SuccessLight    = Color(220, 252, 231);  // #DCFCE7

    inline const Color Warning         = Color(245, 158, 11);   // #F59E0B
    inline const Color WarningDark     = Color(180, 83, 9);     // #B45309
    inline const Color WarningLight    = Color(254, 243, 199);  // #FEF3C7

    inline const Color Danger          = Color(239, 68, 68);    // #EF4444
    inline const Color DangerDark      = Color(185, 28, 28);    // #B91C1C
    inline const Color DangerLight     = Color(254, 226, 226);  // #FEE2E2

    inline const Color Info            = Color(2, 132, 199);    // #0284C7
    inline const Color InfoDark        = Color(3, 105, 161);    // #0369A1
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

        float textX = pos.x + (withDot ? 20.f : paddingH) - b.position.x;
        float textY = pos.y + (height - b.size.y) / 2.f - b.position.y - 1.f;
        txt.setPosition({textX, textY});
        target.draw(txt);
    }

    // Standard Top Bar Navigation & Admin Profile Header
    inline void drawTopBar(RenderTarget& target, float startX, float panelY, float panelWidth,
                           const wstring& sectionName, const Font& font, const Font& boldFont) {
        (void)font;
        // 1. Breadcrumb Container (Pill badge)
        float bcY = panelY + 20.f;
        RoundedRectRenderer::draw(target, {startX, bcY}, {185.f, 28.f}, 14.f, PrimaryLight, 1.2f, PrimaryBorder);

        // Checkmark / Pin Icon
        CircleShape pin(3.5f, 10);
        pin.setFillColor(PrimaryHover);
        pin.setPosition({startX + 12.f, bcY + 10.5f});
        target.draw(pin);

        Text bc1(boldFont, L"CỔNG QUẢN TRỊ", BreadcrumbSize - 1);
        bc1.setFillColor(PrimaryHover);
        bc1.setPosition({startX + 24.f, bcY + 6.f});
        target.draw(bc1);

        Text bc2(boldFont, L"/   HỆ THỐNG   /   " + sectionName, BreadcrumbSize);
        bc2.setFillColor(Color(51, 65, 85));
        bc2.setPosition({startX + 196.f, bcY + 6.f});
        target.draw(bc2);

        // 2. Right-side Status, Bell & Admin Profile Pill
        float rightEdge = startX + panelWidth - 70.f;

        // Notification Bell
        float bellX = rightEdge - 245.f;
        RoundedRectRenderer::draw(target, {bellX, bcY - 2.f}, {36.f, 36.f}, 18.f, Surface, 1.2f, BorderMedium);
        // Simple bell vector icon
        RectangleShape bBody({10.f, 8.f});
        bBody.setFillColor(TextSecondary);
        bBody.setPosition({bellX + 13.f, bcY + 12.f});
        target.draw(bBody);
        CircleShape bTop(2.f, 8);
        bTop.setFillColor(TextSecondary);
        bTop.setPosition({bellX + 16.f, bcY + 9.f});
        target.draw(bTop);
        // Active notification dot
        CircleShape bDot(3.5f, 8);
        bDot.setFillColor(Danger);
        bDot.setPosition({bellX + 23.f, bcY + 6.f});
        target.draw(bDot);

        // User Avatar Pill
        float userX = rightEdge - 198.f;
        RoundedRectRenderer::draw(target, {userX, bcY - 4.f}, {198.f, 40.f}, 20.f, Surface, 1.2f, BorderMedium);

        CircleShape av(15.f, 16);
        av.setFillColor(Primary);
        av.setPosition({userX + 5.f, bcY + 1.f});
        target.draw(av);

        Text avTxt(boldFont, L"AD", 12);
        avTxt.setFillColor(TextWhite);
        avTxt.setPosition({userX + 12.f, bcY + 8.f});
        target.draw(avTxt);

        Text uName(boldFont, L"Admin CineCore", 13);
        uName.setFillColor(TextPrimary);
        uName.setPosition({userX + 42.f, bcY + 4.f});
        target.draw(uName);

        Text uRole(boldFont, L"Toàn quyền • SuperAdmin", 11.5f);
        uRole.setFillColor(TextSecondary);
        uRole.setPosition({userX + 42.f, bcY + 20.f});
        target.draw(uRole);
    }

    // Standard Page Title & Optional Subtitle
    inline void drawPageHeader(RenderTarget& target, float startX, float posY,
                               const wstring& title, const wstring& subtitle,
                               const Font& boldFont, const Font& font) {
        Text heading(boldFont, title, PageTitleSize);
        heading.setFillColor(TextPrimary);
        heading.setPosition({startX, posY});
        target.draw(heading);

        if (!subtitle.empty()) {
            Text sub(font, subtitle, HelperTextSize + 1);
            sub.setFillColor(TextSecondary);
            sub.setPosition({startX, posY + 36.f});
            target.draw(sub);
        }
    }

    // Standard KPI Summary Stat Card
    inline void drawKpiCard(RenderTarget& target, Vector2f pos, Vector2f size,
                            const wstring& title, const wstring& value, const wstring& subtext,
                            Color iconBg, Color iconColor, const Font& boldFont) {
        // Card Body
        RoundedRectRenderer::draw(target, pos, size, 12.f, Surface, 1.2f, BorderLight);

        // Icon Circle (Top-Right)
        float iconR = 19.f;
        CircleShape ic(iconR, 20);
        ic.setFillColor(iconBg);
        ic.setPosition({pos.x + size.x - 48.f, pos.y + 14.f});
        target.draw(ic);

        // Title
        Text t(boldFont, title, TableHeaderSize);
        t.setFillColor(TextSecondary);
        t.setPosition({pos.x + 18.f, pos.y + 14.f});
        target.draw(t);

        // Value
        Text v(boldFont, value, KpiValueSize);
        v.setFillColor(TextPrimary);
        v.setPosition({pos.x + 18.f, pos.y + 36.f});
        target.draw(v);

        // Subtext / Badge
        Text s(boldFont, subtext, HelperTextSize);
        s.setFillColor(iconColor);
        s.setPosition({pos.x + 18.f, pos.y + size.y - 28.f});
        target.draw(s);
    }
}
