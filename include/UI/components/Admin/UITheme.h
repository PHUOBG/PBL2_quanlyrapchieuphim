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
        (void)sectionName;
        (void)font;

        // 1. "CỔNG QUẢN TRỊ" Pill Badge (Tự co giãn chuẩn xác theo nội dung, căn giữa hoàn hảo)
        float bcY = panelY + 18.f;
        Text bc1(boldFont, L"CỔNG QUẢN TRỊ", 11.5f);
        bc1.setFillColor(Color(2, 132, 199));
        FloatRect bcb = bc1.getLocalBounds();

        float pillH = 28.f;
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
        float userW = 205.f;
        float userH = 42.f;
        float userY = panelY + 12.f;
        float userX = startX + panelWidth - userW;

        RoundedRectRenderer::draw(target, {userX, userY}, {userW, userH}, userH / 2.f, Color::White, 1.2f, Color(203, 213, 225));

        // Avatar tròn đường kính 32px
        float avRadius = 16.f;
        CircleShape av(avRadius, 24);
        av.setFillColor(Color(0, 119, 182));
        av.setPosition({userX + 5.f, userY + 5.f});
        target.draw(av);

        // Ký tự "AD" căn giữa tuyệt đối trong vòng tròn
        Text avTxt(boldFont, L"AD", 12);
        avTxt.setFillColor(Color::White);
        FloatRect atb = avTxt.getLocalBounds();
        avTxt.setPosition({
            userX + 5.f + avRadius - atb.size.x / 2.f - atb.position.x,
            userY + 5.f + avRadius - atb.size.y / 2.f - atb.position.y - 1.f
        });
        target.draw(avTxt);

        // Khối văn bản Admin CineCore & Toàn quyền
        float textStartX = userX + 44.f;
        Text uName(boldFont, L"Admin CineCore", 13);
        uName.setFillColor(Color(15, 23, 42));
        FloatRect unb = uName.getLocalBounds();
        uName.setPosition({
            textStartX - unb.position.x,
            userY + 7.f - unb.position.y
        });
        target.draw(uName);

        Text uRole(boldFont, L"Toàn quyền • SuperAdmin", 11.5f);
        uRole.setFillColor(Color(100, 116, 139));
        FloatRect urb = uRole.getLocalBounds();
        uRole.setPosition({
            textStartX - urb.position.x,
            userY + 23.f - urb.position.y
        });
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
