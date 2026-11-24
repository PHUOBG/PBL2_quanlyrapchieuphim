#pragma once

#include <SFML/Graphics.hpp>
#include <string>
#include <cmath>
#include "UI/components/Admin/RoundedRectRenderer.h"

/**
 * @brief Hộp thoại xác nhận xóa dùng chung, đồng bộ thiết kế với MoviePanel.
 */
class DeleteConfirmModal {
public:
    bool isOpen = false;
    std::wstring title;
    std::wstring message;
    std::string targetId;
    int targetIndex = -1;
    bool cancelHover = false;
    bool okHover = false;
    mutable float currentPopupH = 240.f;

    static std::wstring wrapText(const std::wstring& text, const sf::Font& font, unsigned int charSize, float maxWidth) {
        std::wstring result;
        std::wstring line;
        std::wstring word;
        for (size_t i = 0; i <= text.size(); ++i) {
            wchar_t c = (i < text.size()) ? text[i] : L'\n';
            if (c == L'\n' || c == L' ') {
                std::wstring candidate = line.empty() ? word : (line + L" " + word);
                sf::Text probe(font, candidate, charSize);
                if (!line.empty() && probe.getLocalBounds().size.x > maxWidth) {
                    if (!result.empty()) result += L"\n";
                    result += line;
                    line = word;
                } else {
                    line = candidate;
                }
                word.clear();

                if (c == L'\n') {
                    if (!result.empty()) result += L"\n";
                    result += line;
                    line.clear();
                }
            } else {
                word += c;
            }
        }
        return result;
    }

    void open(const std::wstring& modalTitle,
              const std::wstring& modalMessage,
              const std::string& id = "",
              int idx = -1) {
        title = modalTitle;
        message = modalMessage;
        targetId = id;
        targetIndex = idx;
        isOpen = true;
        cancelHover = false;
        okHover = false;
        currentPopupH = 240.f;
    }

    void close() {
        isOpen = false;
        cancelHover = false;
        okHover = false;
    }

    /**
     * @return 0 nếu chưa thao tác, 1 nếu Hủy/Đóng, 2 nếu Đồng ý xóa.
     */
    int handleEvent(const sf::Event& event, const sf::RenderWindow& window) {
        if (!isOpen) return 0;

        const float popupW = 680.f;
        const float popupH = currentPopupH;
        const float popupX = (1728.f - popupW) / 2.f;
        const float popupY = (972.f - popupH) / 2.f;
        const float btnY = popupY + popupH - 80.f;

        if (const auto* mouseBtn = event.getIf<sf::Event::MouseButtonPressed>()) {
            if (mouseBtn->button == sf::Mouse::Button::Left) {
                sf::Vector2f mouse = window.mapPixelToCoords(sf::Mouse::getPosition(window));
                sf::FloatRect cancelBtn({popupX + 40.f, btnY}, {280.f, 44.f});
                sf::FloatRect okBtn({popupX + 360.f, btnY}, {280.f, 44.f});
                sf::FloatRect modalRect({popupX, popupY}, {popupW, popupH});

                if (cancelBtn.contains(mouse)) {
                    close();
                    return 1;
                }
                if (okBtn.contains(mouse)) {
                    isOpen = false; // giữ lại targetId/targetIndex cho caller xử lý xóa
                    cancelHover = false;
                    okHover = false;
                    return 2;
                }
                if (!modalRect.contains(mouse)) {
                    close();
                    return 1;
                }
            }
        } else if (const auto* key = event.getIf<sf::Event::KeyPressed>()) {
            if (key->code == sf::Keyboard::Key::Escape) {
                close();
                return 1;
            }
            if (key->code == sf::Keyboard::Key::Enter) {
                isOpen = false;
                cancelHover = false;
                okHover = false;
                return 2;
            }
        }
        return 0;
    }

    void update(const sf::Vector2f& mousePos) {
        if (!isOpen) return;
        const float popupW = 680.f;
        const float popupH = currentPopupH;
        const float popupX = (1728.f - popupW) / 2.f;
        const float popupY = (972.f - popupH) / 2.f;
        const float btnY = popupY + popupH - 80.f;

        cancelHover = sf::FloatRect({popupX + 40.f, btnY}, {280.f, 44.f}).contains(mousePos);
        okHover = sf::FloatRect({popupX + 360.f, btnY}, {280.f, 44.f}).contains(mousePos);
    }

    void render(sf::RenderTarget& target, const sf::Font& font, const sf::Font& boldFont) const {
        if (!isOpen) return;

        sf::RectangleShape overlay({1728.f, 972.f});
        overlay.setFillColor(sf::Color(15, 23, 42, 140));
        target.draw(overlay);

        const float popupW = 680.f;
        std::wstring wrapped = wrapText(message, font, 18, popupW - 80.f);
        int lineCount = 1;
        for (wchar_t c : wrapped) {
            if (c == L'\n') ++lineCount;
        }
        int extraLines = (lineCount > 2) ? (lineCount - 2) : 0;
        const float popupH = 240.f + static_cast<float>(extraLines) * 26.f;
        currentPopupH = popupH;

        const float popupX = (1728.f - popupW) / 2.f;
        const float popupY = (972.f - popupH) / 2.f;
        const float btnY = popupY + popupH - 80.f;

        RoundedRectRenderer::draw(target, {popupX, popupY}, {popupW, popupH}, 16.f,
                                  sf::Color::White, 1.f, sf::Color(226, 232, 240));

        sf::Text pTitle(boldFont, title, 24);
        pTitle.setFillColor(sf::Color(220, 38, 38));
        pTitle.setPosition({popupX + 40.f, popupY + 35.f});
        target.draw(pTitle);

        sf::Text pMsg(font, wrapped, 18);
        pMsg.setLineSpacing(1.25f);
        pMsg.setFillColor(sf::Color(71, 85, 105));
        pMsg.setPosition({popupX + 40.f, popupY + 76.f});
        target.draw(pMsg);

        sf::Color cBg = cancelHover ? sf::Color(203, 213, 225) : sf::Color(226, 232, 240);
        RoundedRectRenderer::draw(target, {popupX + 40.f, btnY}, {280.f, 44.f}, 8.f, cBg);
        sf::Text cTxt(boldFont, L"H\u1ee7y b\u1ecf", 17);
        cTxt.setFillColor(sf::Color(51, 65, 85));
        sf::FloatRect cb = cTxt.getLocalBounds();
        cTxt.setPosition({
            std::round(popupX + 40.f + (280.f - cb.size.x) / 2.f - cb.position.x),
            std::round(btnY + (44.f - cb.size.y) / 2.f - cb.position.y)
        });
        target.draw(cTxt);

        sf::Color okBg = okHover ? sf::Color(185, 28, 28) : sf::Color(220, 38, 38);
        RoundedRectRenderer::draw(target, {popupX + 360.f, btnY}, {280.f, 44.f}, 8.f, okBg);
        sf::Text okTxt(boldFont, L"\u0110\u1ed3ng \u00fd x\u00f3a", 17);
        okTxt.setFillColor(sf::Color::White);
        sf::FloatRect okb = okTxt.getLocalBounds();
        okTxt.setPosition({
            std::round(popupX + 360.f + (280.f - okb.size.x) / 2.f - okb.position.x),
            std::round(btnY + (44.f - okb.size.y) / 2.f - okb.position.y)
        });
        target.draw(okTxt);
    }
};

