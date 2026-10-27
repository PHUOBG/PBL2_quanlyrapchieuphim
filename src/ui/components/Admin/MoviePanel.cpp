#include "UI/components/Admin/MoviePanel.h"
#include "utils/FileUtils.h"
#include <algorithm>
#include <cwctype>
#include <iostream>

static wstring toLowerW(const wstring& s) {
    wstring r = s;
    for (auto& c : r) c = (wchar_t)towlower(c);
    return r;
}

static wstring s2ws(const string& s) {
    if (s.empty()) return L"";
    auto sfStr = String::fromUtf8(s.begin(), s.end());
    return sfStr.toWideString();
}

static string ws2s(const wstring& ws) {
    if (ws.empty()) return "";
    String sfStr(ws);
    auto u8 = sfStr.toUtf8();
    return string(u8.begin(), u8.end());
}

MoviePanel::MoviePanel(Font& fontRef, float w, float h)
    : font(fontRef),
      boldFont("../assets/fonts/segoeuib.ttf"),
      width(w), height(h), position(350.f, 0.f)
{
    repository = make_unique<AdminMovieRepository>(FileUtils::resolveDataPath("data/movies.txt"));
    refreshData();
}

void MoviePanel::setPosition(Vector2f pos) {
    position = pos;
}

void MoviePanel::refreshData() {
    if (!repository) return;
    repository->loadFromFile();
    allMovies = repository->getAllData();
    applyFilter();
}

void MoviePanel::applyFilter() {
    filteredIndices.clear();
    wstring q = toLowerW(searchQuery);

    for (size_t i = 0; i < allMovies.size(); ++i) {
        const auto& row = allMovies[i];
        if (row.size() < 8) continue;

        string title = row[1];
        string country = row[3];
        string genres = row[5];
        string director = (row.size() > 9) ? row[9] : "";
        string status = (row.size() >= 14) ? row[13] : "Đang chiếu";
        if (status.empty()) status = "Đang chiếu";

        // 1. Text search
        if (!q.empty()) {
            wstring wt = toLowerW(s2ws(title));
            wstring wc = toLowerW(s2ws(country));
            wstring wd = toLowerW(s2ws(director));
            wstring wg = toLowerW(s2ws(genres));
            if (wt.find(q) == wstring::npos &&
                wc.find(q) == wstring::npos &&
                wd.find(q) == wstring::npos &&
                wg.find(q) == wstring::npos) {
                continue;
            }
        }

        // 2. Genre filter
        if (selectedGenre != "all") {
            wstring wg = toLowerW(s2ws(genres));
            wstring targetG = toLowerW(s2ws(selectedGenre));
            if (wg.find(targetG) == wstring::npos) continue;
        }

        // 3. Status filter
        if (selectedStatus != "all") {
            if (selectedStatus == "dang_chieu" && status != "Đang chiếu") continue;
            if (selectedStatus == "sap_chieu" && status != "Sắp chiếu") continue;
        }

        filteredIndices.push_back(static_cast<int>(i));
    }

    if (selectedRow >= static_cast<int>(filteredIndices.size())) {
        selectedRow = -1;
    }
}

void MoviePanel::showToast(const wstring& msg, Color col) {
    notificationMessage = msg;
    notificationColor = col;
    notificationVisible = true;
    notificationClock.restart();
}

void MoviePanel::openAddModal() {
    currentPopup = ADD;
    inputTitle = L"";
    inputGenres = L"Hành động, Phiêu lưu";
    inputFormat = L"2D / Atmos";
    inputDirector = L"";
    inputCountry = L"Việt Nam";
    inputDuration = L"120";
    inputReleaseDate = L"28/04/2024";
    inputStatus = L"Đang chiếu";
    activeInputBox = 0;
}

void MoviePanel::openEditModal(int recordIndex) {
    if (recordIndex < 0 || recordIndex >= static_cast<int>(allMovies.size())) return;
    currentPopup = EDIT;
    selectedRow = recordIndex;

    const auto& row = allMovies[recordIndex];
    inputTitle = s2ws(row.size() > 1 ? row[1] : "");
    inputCountry = s2ws(row.size() > 3 ? row[3] : "Việt Nam");
    inputGenres = s2ws(row.size() > 5 ? row[5] : "");
    inputDuration = s2ws(row.size() > 6 ? row[6] : "120");
    inputReleaseDate = s2ws(row.size() > 7 ? row[7] : "");
    inputDirector = s2ws(row.size() > 9 ? row[9] : "");
    inputStatus = s2ws(row.size() >= 14 ? row[13] : "Đang chiếu");
    if (inputStatus.empty()) inputStatus = L"Đang chiếu";
    inputFormat = L"2D / Atmos";
    activeInputBox = 0;
}

void MoviePanel::openDeleteModal(int recordIndex) {
    if (recordIndex < 0 || recordIndex >= static_cast<int>(allMovies.size())) return;
    currentPopup = DELETE_CONFIRM;
    selectedRow = recordIndex;
}

void MoviePanel::closePopupModal() {
    currentPopup = NONE;
    activeInputBox = -1;
}

void MoviePanel::saveMovie() {
    if (inputTitle.empty()) {
        showToast(L"Vui lòng nhập tên phim!", Color(239, 68, 68));
        return;
    }

    if (currentPopup == ADD) {
        string newId = "#PBL-" + to_string(allMovies.size() + 1);
        if (newId.size() < 8) newId.insert(5, string(8 - newId.size(), '0'));

        vector<string> newRow = {
            newId,
            ws2s(inputTitle),
            "T16",
            ws2s(inputCountry),
            "Tiếng Việt",
            ws2s(inputGenres),
            ws2s(inputDuration),
            ws2s(inputReleaseDate),
            "30/12/2024",
            ws2s(inputDirector),
            "Diễn viên chính",
            "Mô tả phim...",
            "../assets/posters/default.jpg",
            ws2s(inputStatus)
        };
        repository->addRecord(newRow);
        repository->saveToFile();
        refreshData();
        closePopupModal();
        showToast(L"Thêm phim mới thành công!");
    }
    else if (currentPopup == EDIT) {
        if (selectedRow >= 0 && selectedRow < static_cast<int>(allMovies.size())) {
            auto row = allMovies[selectedRow];
            while (row.size() < 14) row.push_back("");
            row[1] = ws2s(inputTitle);
            row[3] = ws2s(inputCountry);
            row[5] = ws2s(inputGenres);
            row[6] = ws2s(inputDuration);
            row[7] = ws2s(inputReleaseDate);
            row[9] = ws2s(inputDirector);
            row[13] = ws2s(inputStatus);

            repository->updateRecord(selectedRow, row);
            repository->saveToFile();
            refreshData();
            closePopupModal();
            showToast(L"Cập nhật thông tin phim thành công!");
        }
    }
}

void MoviePanel::confirmDelete() {
    if (selectedRow >= 0 && selectedRow < static_cast<int>(allMovies.size())) {
        repository->deleteRecord(selectedRow);
        repository->saveToFile();
        refreshData();
        closePopupModal();
        selectedRow = -1;
        showToast(L"Đã xóa phim thành công!");
    }
}

// Vector Icon Helpers
void MoviePanel::drawPlayIcon(RenderTarget& target, Vector2f pos, Color col) {
    ConvexShape tri(3);
    tri.setPoint(0, {pos.x, pos.y});
    tri.setPoint(1, {pos.x + 9.f, pos.y + 5.5f});
    tri.setPoint(2, {pos.x, pos.y + 11.f});
    tri.setFillColor(col);
    target.draw(tri);
}

void MoviePanel::drawCalendarIcon(RenderTarget& target, Vector2f pos, Color col) {
    RoundedRectRenderer::draw(target, pos, {14.f, 13.f}, 2.f, Color::Transparent, 1.4f, col);
    RectangleShape bar({14.f, 3.f});
    bar.setFillColor(col);
    bar.setPosition(pos);
    target.draw(bar);
}

void MoviePanel::drawClockIcon(RenderTarget& target, Vector2f pos, Color col) {
    CircleShape c(7.f, 16);
    c.setFillColor(Color::Transparent);
    c.setOutlineThickness(1.4f);
    c.setOutlineColor(col);
    c.setPosition(pos);
    target.draw(c);

    RectangleShape h({4.f, 1.4f});
    h.setFillColor(col);
    h.setPosition({pos.x + 7.f, pos.y + 7.f});
    target.draw(h);

    RectangleShape m({1.4f, 5.f});
    m.setFillColor(col);
    m.setPosition({pos.x + 7.f, pos.y + 3.f});
    target.draw(m);
}

void MoviePanel::drawFilmIcon(RenderTarget& target, Vector2f pos, Color col) {
    RoundedRectRenderer::draw(target, pos, {15.f, 12.f}, 2.f, Color::Transparent, 1.4f, col);
    RectangleShape l({3.f, 6.f});
    l.setFillColor(col);
    l.setPosition({pos.x + 6.f, pos.y + 3.f});
    target.draw(l);
}

void MoviePanel::drawSearchIcon(RenderTarget& target, Vector2f pos, Color col) {
    CircleShape c(5.f, 14);
    c.setFillColor(Color::Transparent);
    c.setOutlineThickness(1.5f);
    c.setOutlineColor(col);
    c.setPosition(pos);
    target.draw(c);

    RectangleShape stem({5.f, 1.8f});
    stem.setFillColor(col);
    stem.setRotation(degrees(45.f));
    stem.setPosition({pos.x + 8.f, pos.y + 8.f});
    target.draw(stem);
}

void MoviePanel::drawPencilIcon(RenderTarget& target, Vector2f pos, Color col) {
    RectangleShape b({9.f, 3.f});
    b.setFillColor(col);
    b.setRotation(degrees(-45.f));
    b.setPosition({pos.x + 2.f, pos.y + 8.f});
    target.draw(b);

    ConvexShape tip(3);
    tip.setPoint(0, {pos.x, pos.y + 11.f});
    tip.setPoint(1, {pos.x + 3.f, pos.y + 11.f});
    tip.setPoint(2, {pos.x, pos.y + 8.f});
    tip.setFillColor(col);
    target.draw(tip);
}

void MoviePanel::drawTrashIcon(RenderTarget& target, Vector2f pos, Color col) {
    RoundedRectRenderer::draw(target, {pos.x + 1.f, pos.y + 3.f}, {10.f, 9.f}, 1.5f, Color::Transparent, 1.4f, col);
    RectangleShape top({12.f, 1.8f});
    top.setFillColor(col);
    top.setPosition({pos.x, pos.y + 1.5f});
    target.draw(top);
    RectangleShape handle({4.f, 1.4f});
    handle.setFillColor(col);
    handle.setPosition({pos.x + 4.f, pos.y});
    target.draw(handle);
}

void MoviePanel::drawPlusIcon(RenderTarget& target, Vector2f pos, Color col) {
    RectangleShape h({10.f, 2.f});
    h.setFillColor(col);
    h.setPosition({pos.x, pos.y + 4.f});
    target.draw(h);
    RectangleShape v({2.f, 10.f});
    v.setFillColor(col);
    v.setPosition({pos.x + 4.f, pos.y});
    target.draw(v);
}

void MoviePanel::drawReloadIcon(RenderTarget& target, Vector2f pos, Color col) {
    CircleShape arc(6.f, 20);
    arc.setFillColor(Color::Transparent);
    arc.setOutlineThickness(1.6f);
    arc.setOutlineColor(col);
    arc.setPosition(pos);
    target.draw(arc);

    ConvexShape arrow(3);
    arrow.setPoint(0, {pos.x + 10.f, pos.y - 1.f});
    arrow.setPoint(1, {pos.x + 13.f, pos.y + 3.f});
    arrow.setPoint(2, {pos.x + 8.f, pos.y + 3.f});
    arrow.setFillColor(col);
    target.draw(arrow);
}

void MoviePanel::drawBellIcon(RenderTarget& target, Vector2f pos, Color col) {
    RoundedRectRenderer::draw(target, {pos.x + 2.f, pos.y + 2.f}, {11.f, 8.f}, 3.5f, Color::Transparent, 1.5f, col);
    RectangleShape btm({15.f, 1.8f});
    btm.setFillColor(col);
    btm.setPosition({pos.x, pos.y + 10.f});
    target.draw(btm);
    CircleShape clapper(1.8f, 8);
    clapper.setFillColor(col);
    clapper.setPosition({pos.x + 5.5f, pos.y + 11.5f});
    target.draw(clapper);
}

void MoviePanel::drawFunnelIcon(RenderTarget& target, Vector2f pos, Color col) {
    ConvexShape top(3);
    top.setPoint(0, {pos.x, pos.y});
    top.setPoint(1, {pos.x + 10.f, pos.y});
    top.setPoint(2, {pos.x + 5.f, pos.y + 6.f});
    top.setFillColor(col);
    target.draw(top);

    RectangleShape stem({2.f, 5.f});
    stem.setFillColor(col);
    stem.setPosition({pos.x + 4.f, pos.y + 5.f});
    target.draw(stem);
}

void MoviePanel::drawCheckmark(RenderTarget& target, Vector2f pos, Color col) {
    RectangleShape c1({4.f, 1.8f});
    c1.setFillColor(col);
    c1.setRotation(degrees(45.f));
    c1.setPosition(pos);
    target.draw(c1);

    RectangleShape c2({8.f, 1.8f});
    c2.setFillColor(col);
    c2.setRotation(degrees(-55.f));
    c2.setPosition({pos.x + 2.8f, pos.y + 2.8f});
    target.draw(c2);
}

void MoviePanel::drawDownArrow(RenderTarget& target, Vector2f pos, Color col) {
    ConvexShape a(3);
    a.setPoint(0, {pos.x, pos.y});
    a.setPoint(1, {pos.x + 7.f, pos.y});
    a.setPoint(2, {pos.x + 3.5f, pos.y + 4.f});
    a.setFillColor(col);
    target.draw(a);
}

void MoviePanel::handleEvent(const Event& event, const RenderWindow& window) {
    // Caret blinking
    if (caretClock.getElapsedTime().asSeconds() >= 0.5f) {
        caretVisible = !caretVisible;
        caretClock.restart();
    }

    if (currentPopup != NONE) {
        if (const auto* keyEvent = event.getIf<Event::KeyPressed>()) {
            if (keyEvent->code == Keyboard::Key::Escape) {
                closePopupModal();
                return;
            }
            if (keyEvent->code == Keyboard::Key::Enter) {
                if (currentPopup == DELETE_CONFIRM) confirmDelete();
                else saveMovie();
                return;
            }
            if (keyEvent->code == Keyboard::Key::Tab) {
                if (currentPopup != DELETE_CONFIRM) {
                    activeInputBox = (activeInputBox + 1) % 8;
                }
            }
            if (keyEvent->code == Keyboard::Key::Backspace) {
                if (activeInputBox == 0 && !inputTitle.empty()) inputTitle.pop_back();
                else if (activeInputBox == 1 && !inputGenres.empty()) inputGenres.pop_back();
                else if (activeInputBox == 2 && !inputFormat.empty()) inputFormat.pop_back();
                else if (activeInputBox == 3 && !inputDirector.empty()) inputDirector.pop_back();
                else if (activeInputBox == 4 && !inputCountry.empty()) inputCountry.pop_back();
                else if (activeInputBox == 5 && !inputDuration.empty()) inputDuration.pop_back();
                else if (activeInputBox == 6 && !inputReleaseDate.empty()) inputReleaseDate.pop_back();
                else if (activeInputBox == 7 && !inputStatus.empty()) inputStatus.pop_back();
            }
        }
        if (const auto* textEvent = event.getIf<Event::TextEntered>()) {
            char32_t uc = textEvent->unicode;
            if (uc >= 32 && uc != 127) {
                if (activeInputBox == 0 && inputTitle.size() < 60) inputTitle.push_back((wchar_t)uc);
                else if (activeInputBox == 1 && inputGenres.size() < 60) inputGenres.push_back((wchar_t)uc);
                else if (activeInputBox == 2 && inputFormat.size() < 30) inputFormat.push_back((wchar_t)uc);
                else if (activeInputBox == 3 && inputDirector.size() < 40) inputDirector.push_back((wchar_t)uc);
                else if (activeInputBox == 4 && inputCountry.size() < 30) inputCountry.push_back((wchar_t)uc);
                else if (activeInputBox == 5 && inputDuration.size() < 10) inputDuration.push_back((wchar_t)uc);
                else if (activeInputBox == 6 && inputReleaseDate.size() < 20) inputReleaseDate.push_back((wchar_t)uc);
                else if (activeInputBox == 7 && inputStatus.size() < 20) inputStatus.push_back((wchar_t)uc);
            }
        }
        return;
    }

    // Search input typing
    if (const auto* keyEvent = event.getIf<Event::KeyPressed>()) {
        if (searchActive) {
            if (keyEvent->code == Keyboard::Key::Backspace && !searchQuery.empty()) {
                searchQuery.pop_back();
                applyFilter();
            }
            if (keyEvent->code == Keyboard::Key::Escape) {
                searchActive = false;
            }
        }
    }
    if (const auto* textEvent = event.getIf<Event::TextEntered>()) {
        if (searchActive) {
            char32_t uc = textEvent->unicode;
            if (uc >= 32 && uc != 127 && searchQuery.size() < 50) {
                searchQuery.push_back((wchar_t)uc);
                applyFilter();
            }
        }
    }

    // Mouse wheel scroll for table
    if (const auto* wheel = event.getIf<Event::MouseWheelScrolled>()) {
        if (wheel->delta > 0 && scrollOffset > 0) {
            scrollOffset--;
        } else if (wheel->delta < 0 && scrollOffset + 5 < static_cast<int>(filteredIndices.size())) {
            scrollOffset++;
        }
    }
}

void MoviePanel::update(Vector2f mouse, bool mousePressed) {
    if (currentPopup != NONE) {
        const float popupW = 680.f;
        const float popupH = (currentPopup == DELETE_CONFIRM) ? 240.f : 530.f;
        const float popupX = (1728.f - popupW) / 2.f;
        const float popupY = (972.f - popupH) / 2.f;

        if (currentPopup == DELETE_CONFIRM) {
            FloatRect cancelBtn({popupX + 40.f, popupY + 160.f}, {280.f, 44.f});
            FloatRect okBtn({popupX + 360.f, popupY + 160.f}, {280.f, 44.f});
            btnDeleteCancelHover = cancelBtn.contains(mouse);
            btnDeleteOkHover = okBtn.contains(mouse);

            if (mousePressed) {
                if (btnDeleteCancelHover) closePopupModal();
                else if (btnDeleteOkHover) confirmDelete();
            }
        } else {
            // Input box bounds (2 columns: left: 0,1,2,3 - right: 4,5,6,7)
            const float leftX = popupX + 40.f;
            const float rightX = popupX + 360.f;
            const float startY = popupY + 95.f;
            const float gapY = 68.f;

            if (mousePressed) {
                for (int c = 0; c < 4; ++c) {
                    FloatRect lb({leftX, startY + c * gapY + 22.f}, {280.f, 40.f});
                    FloatRect rb({rightX, startY + c * gapY + 22.f}, {280.f, 40.f});
                    if (lb.contains(mouse)) activeInputBox = c;
                    if (rb.contains(mouse)) activeInputBox = c + 4;
                }
            }

            FloatRect cancelBtn({popupX + 40.f, popupY + 450.f}, {280.f, 44.f});
            FloatRect saveBtn({popupX + 360.f, popupY + 450.f}, {280.f, 44.f});
            btnPopupCancelHover = cancelBtn.contains(mouse);
            btnPopupSaveHover = saveBtn.contains(mouse);

            if (mousePressed) {
                if (btnPopupCancelHover) closePopupModal();
                else if (btnPopupSaveHover) saveMovie();
            }
        }
        return;
    }

    const float startX = position.x + 35.f; // 385.f
    const float toolY = 252.f;

    FloatRect searchBox({startX, toolY}, {360.f, 44.f});
    FloatRect genreBtn({startX + 375.f, toolY}, {160.f, 44.f});
    FloatRect statusBtn({startX + 550.f, toolY}, {160.f, 44.f});
    FloatRect reloadBtn({startX + 725.f, toolY}, {44.f, 44.f});
    FloatRect editBtn({startX + 785.f, toolY}, {150.f, 44.f});
    FloatRect deleteBtn({startX + 950.f, toolY}, {125.f, 44.f});
    FloatRect addBtn({startX + 1090.f, toolY}, {165.f, 44.f});

    btnSearchHover = searchBox.contains(mouse);
    btnGenreHover = genreBtn.contains(mouse);
    btnStatusHover = statusBtn.contains(mouse);
    btnReloadHover = reloadBtn.contains(mouse);
    btnEditHover = editBtn.contains(mouse);
    btnDeleteHover = deleteBtn.contains(mouse);
    btnAddHover = addBtn.contains(mouse);

    // Dropdown open interaction
    if (genreDropdownOpen) {
        vector<string> genres = {"Tất cả thể loại", "Hành động", "Hoạt hình", "Tội phạm", "Viễn tưởng", "Tâm lý"};
        hoveredGenreIndex = -1;
        for (size_t g = 0; g < genres.size(); ++g) {
            FloatRect gb({startX + 375.f, toolY + 46.f + g * 34.f}, {160.f, 34.f});
            if (gb.contains(mouse)) {
                hoveredGenreIndex = static_cast<int>(g);
                if (mousePressed) {
                    selectedGenre = (g == 0) ? "all" : genres[g];
                    genreDropdownOpen = false;
                    applyFilter();
                    return;
                }
            }
        }
        if (mousePressed && !genreBtn.contains(mouse)) {
            genreDropdownOpen = false;
        }
    }

    if (statusDropdownOpen) {
        vector<pair<string, string>> statuses = {{"all", "Trạng thái: Tất cả"}, {"dang_chieu", "Đang chiếu"}, {"sap_chieu", "Sắp chiếu"}};
        hoveredStatusIndex = -1;
        for (size_t s = 0; s < statuses.size(); ++s) {
            FloatRect sb({startX + 550.f, toolY + 46.f + s * 34.f}, {160.f, 34.f});
            if (sb.contains(mouse)) {
                hoveredStatusIndex = static_cast<int>(s);
                if (mousePressed) {
                    selectedStatus = statuses[s].first;
                    statusDropdownOpen = false;
                    applyFilter();
                    return;
                }
            }
        }
        if (mousePressed && !statusBtn.contains(mouse)) {
            statusDropdownOpen = false;
        }
    }

    if (mousePressed) {
        if (btnSearchHover) {
            searchActive = true;
            genreDropdownOpen = false;
            statusDropdownOpen = false;
        } else {
            searchActive = false;
        }

        if (btnGenreHover) {
            genreDropdownOpen = !genreDropdownOpen;
            statusDropdownOpen = false;
        }

        if (btnStatusHover) {
            statusDropdownOpen = !statusDropdownOpen;
            genreDropdownOpen = false;
        }

        if (btnReloadHover) {
            searchQuery = L"";
            selectedGenre = "all";
            selectedStatus = "all";
            refreshData();
            showToast(L"Đã làm mới dữ liệu!");
        }

        if (btnAddHover) {
            openAddModal();
        }

        if (btnEditHover) {
            if (selectedRow >= 0 && selectedRow < static_cast<int>(filteredIndices.size())) {
                openEditModal(filteredIndices[selectedRow]);
            } else {
                showToast(L"Vui lòng chọn 1 phim để sửa!", Color(245, 158, 11));
            }
        }

        if (btnDeleteHover) {
            if (selectedRow >= 0 && selectedRow < static_cast<int>(filteredIndices.size())) {
                openDeleteModal(filteredIndices[selectedRow]);
            } else {
                showToast(L"Vui lòng chọn 1 phim để xóa!", Color(239, 68, 68));
            }
        }
    }

    // Table rows hover & interaction
    const float tableY = 352.f;
    const float hdrH = 48.f;
    const float rowH = 80.f;
    const float tableW = 1244.f;

    hoveredRow = -1;
    hoveredEditRow = -1;
    hoveredDeleteRow = -1;

    int displayCount = min(5, static_cast<int>(filteredIndices.size()) - scrollOffset);
    for (int r = 0; r < displayCount; ++r) {
        int idx = scrollOffset + r;
        float rY = tableY + hdrH + r * rowH;
        FloatRect rBounds({startX, rY}, {tableW, rowH});

        FloatRect editIconBtn({startX + tableW - 80.f, rY + 24.f}, {32.f, 32.f});
        FloatRect delIconBtn({startX + tableW - 40.f, rY + 24.f}, {32.f, 32.f});

        if (editIconBtn.contains(mouse)) hoveredEditRow = idx;
        if (delIconBtn.contains(mouse)) hoveredDeleteRow = idx;

        if (rBounds.contains(mouse)) {
            hoveredRow = idx;
            if (mousePressed) {
                if (editIconBtn.contains(mouse)) {
                    openEditModal(filteredIndices[idx]);
                } else if (delIconBtn.contains(mouse)) {
                    openDeleteModal(filteredIndices[idx]);
                } else {
                    selectedRow = (selectedRow == idx) ? -1 : idx;
                }
            }
        }
    }
}

void MoviePanel::render(RenderWindow& window) {
    const float startX = position.x + 35.f; // 385.f

    // 1. Breadcrumbs
    RoundedRectRenderer::draw(window, {startX, 38.f}, {195.f, 28.f}, 14.f, Color(239, 246, 255), 1.2f, Color(186, 230, 253));
    drawCheckmark(window, {startX + 14.f, 49.f}, Color(2, 132, 199));

    Text b1(boldFont, L"C\u1ed4NG QU\u1ea2N TR\u1eca N\u1ed8I B\u1ed8", 12);
    b1.setFillColor(Color(2, 132, 199));
    b1.setPosition({startX + 30.f, 43.f});
    window.draw(b1);

    Text b2(boldFont, L"/   H\u1ec6 TH\u1ed0NG   /   QU\u1ea2N L\u00dd PHIM", 13);
    b2.setFillColor(Color(51, 65, 85));
    b2.setPosition({startX + 208.f, 42.f});
    window.draw(b2);

    // 2. Heading
    Text heading(boldFont, L"DANH S\u00c1CH & \u0110I\u1ec0U PH\u1ed0I PHIM", 26);
    heading.setFillColor(Color(15, 23, 42));
    heading.setPosition({startX, 74.f});
    window.draw(heading);

    // 3. Top-Right Bell and Admin Avatar
    RoundedRectRenderer::draw(window, {position.x + width - 270.f, 40.f}, {42.f, 42.f}, 21.f, Color::White, 1.2f, Color(203, 213, 225));
    drawBellIcon(window, {position.x + width - 257.f, 53.f}, Color(51, 65, 85));

    // User pill
    RoundedRectRenderer::draw(window, {position.x + width - 215.f, 36.f}, {200.f, 50.f}, 25.f, Color::White, 1.2f, Color(203, 213, 225));
    CircleShape av(18.f, 20);
    av.setFillColor(Color(2, 132, 199));
    av.setPosition({position.x + width - 209.f, 43.f});
    window.draw(av);

    Text avText(boldFont, L"AD", 13);
    avText.setFillColor(Color::White);
    avText.setPosition({position.x + width - 200.f, 51.f});
    window.draw(avText);

    Text uName(boldFont, L"Admin Danang01", 14);
    uName.setFillColor(Color(15, 23, 42));
    uName.setPosition({position.x + width - 165.f, 42.f});
    window.draw(uName);

    Text uRole(boldFont, L"ADM-9842 (SuperAdmin)", 11);
    uRole.setFillColor(Color(71, 85, 105));
    uRole.setPosition({position.x + width - 165.f, 62.f});
    window.draw(uRole);

    // 4. 4 KPI STAT CARDS
    const float cardY = 126.f;
    const float cardW = 296.f;
    const float cardH = 104.f;
    const float cardGap = 20.f;

    // Count statistics
    int countActive = 0, countUpcoming = 0;
    for (const auto& r : allMovies) {
        string st = (r.size() >= 14) ? r[13] : "Đang chiếu";
        if (st == "Đang chiếu" || st.empty()) countActive++;
        else countUpcoming++;
    }

    // Card 1: Phim đang chiếu
    RoundedRectRenderer::draw(window, {startX, cardY}, {cardW, cardH}, 12.f, Color::White, 1.2f, Color(226, 232, 240));
    CircleShape i1(19.f, 20);
    i1.setFillColor(Color(220, 252, 231));
    i1.setPosition({startX + cardW - 50.f, cardY + 14.f});
    window.draw(i1);
    drawPlayIcon(window, {startX + cardW - 36.f, cardY + 28.f}, Color(21, 128, 61));

    Text t1(boldFont, L"Phim \u0111ang chi\u1ebfu", 13);
    t1.setFillColor(Color(51, 65, 85));
    t1.setPosition({startX + 18.f, cardY + 14.f});
    window.draw(t1);

    Text n1(boldFont, to_wstring(countActive), 30);
    n1.setFillColor(Color(15, 23, 42));
    n1.setPosition({startX + 18.f, cardY + 34.f});
    window.draw(n1);

    Text s1(boldFont, L"\u0110ang t\u1ea1o doanh thu", 12);
    s1.setFillColor(Color(21, 128, 61));
    s1.setPosition({startX + 18.f, cardY + 74.f});
    window.draw(s1);

    // Card 2: Phim sắp khởi chiếu
    float c2X = startX + cardW + cardGap;
    RoundedRectRenderer::draw(window, {c2X, cardY}, {cardW, cardH}, 12.f, Color::White, 1.2f, Color(226, 232, 240));
    CircleShape i2(19.f, 20);
    i2.setFillColor(Color(224, 242, 254));
    i2.setPosition({c2X + cardW - 50.f, cardY + 14.f});
    window.draw(i2);
    drawCalendarIcon(window, {c2X + cardW - 38.f, cardY + 27.f}, Color(3, 105, 161));

    Text t2(boldFont, L"Phim s\u1eafp kh\u1edfi chi\u1ebfu", 13);
    t2.setFillColor(Color(51, 65, 85));
    t2.setPosition({c2X + 18.f, cardY + 14.f});
    window.draw(t2);

    Text n2(boldFont, to_wstring(countUpcoming), 30);
    n2.setFillColor(Color(15, 23, 42));
    n2.setPosition({c2X + 18.f, cardY + 34.f});
    window.draw(n2);

    Text s2(boldFont, L"\u0110\u00e3 m\u1edf b\u00e1n s\u1edbm", 12);
    s2.setFillColor(Color(3, 105, 161));
    s2.setPosition({c2X + 18.f, cardY + 74.f});
    window.draw(s2);

    // Card 3: Tổng suất hôm nay
    float c3X = c2X + cardW + cardGap;
    RoundedRectRenderer::draw(window, {c3X, cardY}, {cardW, cardH}, 12.f, Color::White, 1.2f, Color(226, 232, 240));
    CircleShape i3(19.f, 20);
    i3.setFillColor(Color(243, 232, 255));
    i3.setPosition({c3X + cardW - 50.f, cardY + 14.f});
    window.draw(i3);
    drawClockIcon(window, {c3X + cardW - 38.f, cardY + 26.f}, Color(126, 34, 206));

    Text t3(boldFont, L"T\u1ed5ng su\u1ea5t h\u00f4m nay", 13);
    t3.setFillColor(Color(51, 65, 85));
    t3.setPosition({c3X + 18.f, cardY + 14.f});
    window.draw(t3);

    Text n3(boldFont, L"42", 30);
    n3.setFillColor(Color(15, 23, 42));
    n3.setPosition({c3X + 18.f, cardY + 34.f});
    window.draw(n3);

    Text s3(boldFont, L"12 ph\u00f2ng chi\u1ebfu full", 12);
    s3.setFillColor(Color(126, 34, 206));
    s3.setPosition({c3X + 18.f, cardY + 74.f});
    window.draw(s3);

    // Card 4: Tổng phim quản lý
    float c4X = c3X + cardW + cardGap;
    RoundedRectRenderer::draw(window, {c4X, cardY}, {cardW, cardH}, 12.f, Color::White, 1.2f, Color(226, 232, 240));
    CircleShape i4(19.f, 20);
    i4.setFillColor(Color(241, 245, 249));
    i4.setPosition({c4X + cardW - 50.f, cardY + 14.f});
    window.draw(i4);
    drawFilmIcon(window, {c4X + cardW - 38.f, cardY + 27.f}, Color(51, 65, 85));

    Text t4(boldFont, L"T\u1ed5ng phim qu\u1ea3n l\u00fd", 13);
    t4.setFillColor(Color(51, 65, 85));
    t4.setPosition({c4X + 18.f, cardY + 14.f});
    window.draw(t4);

    Text n4(boldFont, to_wstring(allMovies.size()), 30);
    n4.setFillColor(Color(15, 23, 42));
    n4.setPosition({c4X + 18.f, cardY + 34.f});
    window.draw(n4);

    Text s4(boldFont, L"Kho d\u1eef li\u1ec7u 2024", 12);
    s4.setFillColor(Color(51, 65, 85));
    s4.setPosition({c4X + 18.f, cardY + 74.f});
    window.draw(s4);

    // 5. TOOLBAR (Search & Filters & Actions)
    const float toolY = 252.f;

    // Search Input
    Color searchBorder = searchActive ? Color(2, 132, 199) : Color(203, 213, 225);
    RoundedRectRenderer::draw(window, {startX, toolY}, {360.f, 44.f}, 10.f, Color::White, 1.2f, searchBorder);
    drawSearchIcon(window, {startX + 14.f, toolY + 14.f}, Color(100, 116, 139));

    if (searchQuery.empty()) {
        Text ph(boldFont, L"T\u00ecm t\u00ean phim, \u0111\u1ea1o di\u1ec5n, qu\u1ed1c gia...", 13);
        ph.setFillColor(Color(100, 116, 139));
        ph.setPosition({startX + 38.f, toolY + 12.f});
        window.draw(ph);
    } else {
        Text val(boldFont, searchQuery, 14);
        val.setFillColor(Color(15, 23, 42));
        val.setPosition({startX + 38.f, toolY + 11.f});
        window.draw(val);

        if (searchActive && caretVisible) {
            RectangleShape caret({2.f, 20.f});
            caret.setFillColor(Color(2, 132, 199));
            caret.setPosition({startX + 40.f + val.getLocalBounds().size.x, toolY + 12.f});
            window.draw(caret);
        }
    }

    // Genre Dropdown Button
    RoundedRectRenderer::draw(window, {startX + 375.f, toolY}, {160.f, 44.f}, 10.f, Color::White, 1.2f, Color(203, 213, 225));
    wstring gLabel = (selectedGenre == "all") ? L"T\u1ea5t c\u1ea3 th\u1ec3 lo\u1ea1i" : s2ws(selectedGenre);
    Text gt(boldFont, gLabel, 13);
    gt.setFillColor(Color(30, 41, 59));
    gt.setPosition({startX + 388.f, toolY + 12.f});
    window.draw(gt);
    drawDownArrow(window, {startX + 514.f, toolY + 20.f}, Color(51, 65, 85));

    // Status Dropdown Button
    RoundedRectRenderer::draw(window, {startX + 550.f, toolY}, {160.f, 44.f}, 10.f, Color::White, 1.2f, Color(203, 213, 225));
    wstring stLabel = (selectedStatus == "all") ? L"Tr\u1ea1ng th\u00e1i: T\u1ea5t c\u1ea3" : (selectedStatus == "dang_chieu" ? L"\u0110ang chi\u1ebfu" : L"S\u1eafp chi\u1ebfu");
    Text stt(boldFont, stLabel, 13);
    stt.setFillColor(Color(30, 41, 59));
    stt.setPosition({startX + 562.f, toolY + 12.f});
    window.draw(stt);
    drawDownArrow(window, {startX + 690.f, toolY + 20.f}, Color(51, 65, 85));

    // Reload Button
    Color relCol = btnReloadHover ? Color(241, 245, 249) : Color::White;
    RoundedRectRenderer::draw(window, {startX + 725.f, toolY}, {44.f, 44.f}, 10.f, relCol, 1.2f, Color(203, 213, 225));
    drawReloadIcon(window, {startX + 740.f, toolY + 15.f}, Color(30, 41, 59));

    // Edit Button (Amber/Orange)
    Color editCol = btnEditHover ? Color(217, 119, 6) : Color(245, 158, 11);
    RoundedRectRenderer::draw(window, {startX + 785.f, toolY}, {150.f, 44.f}, 10.f, editCol);
    drawPencilIcon(window, {startX + 800.f, toolY + 15.f}, Color::White);
    Text editTxt(boldFont, L"S\u1eeda th\u00f4ng tin", 14);
    editTxt.setFillColor(Color::White);
    editTxt.setPosition({startX + 822.f, toolY + 11.f});
    window.draw(editTxt);

    // Delete Button (Crimson)
    Color delCol = btnDeleteHover ? Color(220, 38, 38) : Color(239, 68, 68);
    RoundedRectRenderer::draw(window, {startX + 950.f, toolY}, {125.f, 44.f}, 10.f, delCol);
    drawTrashIcon(window, {startX + 964.f, toolY + 15.f}, Color::White);
    Text delTxt(boldFont, L"X\u00f3a phim", 14);
    delTxt.setFillColor(Color::White);
    delTxt.setPosition({startX + 986.f, toolY + 11.f});
    window.draw(delTxt);

    // Add Button (Ocean Blue)
    Color addCol = btnAddHover ? Color(3, 105, 161) : Color(0, 119, 182);
    RoundedRectRenderer::draw(window, {startX + 1090.f, toolY}, {165.f, 44.f}, 10.f, addCol);
    drawPlusIcon(window, {startX + 1106.f, toolY + 17.f}, Color::White);
    Text addTxt(boldFont, L"Th\u00eam phim m\u1edbi", 14);
    addTxt.setFillColor(Color::White);
    addTxt.setPosition({startX + 1126.f, toolY + 11.f});
    window.draw(addTxt);

    // 6. DATA TABLE SUBHEADER
    const float subY = 318.f;
    drawFunnelIcon(window, {startX + 2.f, subY + 3.f}, Color(2, 132, 199));

    Text subTitle(boldFont, L"DANH S\u00c1CH PHIM CHI\u1ebeU R\u1ea0P (THEO D\u1eee LI\u1ec6U TH\u1ef0C T\u1ebe PBL2)", 13);
    subTitle.setFillColor(Color(15, 23, 42));
    subTitle.setPosition({startX + 20.f, subY + 2.f});
    window.draw(subTitle);

    // Right count badge
    wstring countStr = L"\u0110ang hi\u1ec3n th\u1ecb: " + to_wstring(min(5, (int)filteredIndices.size())) + L" tr\u00ean " + to_wstring(allMovies.size()) + L" phim";
    RoundedRectRenderer::draw(window, {startX + 1244.f - 210.f, subY - 4.f}, {210.f, 28.f}, 14.f, Color(241, 245, 249), 1.2f, Color(203, 213, 225));
    Text cBadge(boldFont, countStr, 12);
    cBadge.setFillColor(Color(30, 41, 59));
    cBadge.setPosition({startX + 1244.f - 200.f, subY + 1.f});
    window.draw(cBadge);

    // 7. TABLE HEADER (Ocean Blue #0077B6)
    const float tblY = 352.f;
    const float tblW = 1244.f;
    const float hdrH = 48.f;

    RoundedRectRenderer::draw(window, {startX, tblY}, {tblW, hdrH}, 8.f, Color(0, 119, 182));

    // Column headers
    vector<pair<wstring, float>> cols = {
        {L"", 45.f},
        {L"ID PHIM", 95.f},
        {L"T\u00caN PHIM & TH\u1ec2 LO\u1ea0I", 360.f},
        {L"\u0110\u1ea0O DI\u1ec4N", 145.f},
        {L"QU\u1ed0C GIA", 125.f},
        {L"TH\u1edcI L\u01af\u1ee2NG", 100.f},
        {L"NG\u00c0Y CHI\u1ebeU", 120.f},
        {L"TR\u1ea0NG TH\u00c1I", 134.f},
        {L"THAO T\u00c1C", 120.f}
    };

    float curColX = startX;
    for (size_t c = 0; c < cols.size(); ++c) {
        if (!cols[c].first.empty()) {
            Text hText(boldFont, cols[c].first, 12);
            hText.setFillColor(Color::White);
            hText.setPosition({curColX + 12.f, tblY + 15.f});
            window.draw(hText);
        }
        curColX += cols[c].second;
    }

    // 8. TABLE ROWS
    const float rowH = 80.f;
    int displayCount = min(5, static_cast<int>(filteredIndices.size()) - scrollOffset);

    for (int r = 0; r < displayCount; ++r) {
        int idx = scrollOffset + r;
        int origIdx = filteredIndices[idx];
        const auto& row = allMovies[origIdx];
        float rY = tblY + hdrH + r * rowH;

        bool isSelected = (idx == selectedRow);
        bool isHovered = (idx == hoveredRow);

        Color rowBg = isSelected ? Color(224, 242, 254) : (isHovered ? Color(248, 250, 252) : Color::White);
        RoundedRectRenderer::draw(window, {startX, rY}, {tblW, rowH}, 0.f, rowBg);

        // Divider under row
        RectangleShape rLine({tblW, 1.2f});
        rLine.setFillColor(Color(226, 232, 240));
        rLine.setPosition({startX, rY + rowH - 1.f});
        window.draw(rLine);

        // Checkbox column
        Color cbBorder = isSelected ? Color(2, 132, 199) : Color(203, 213, 225);
        RoundedRectRenderer::draw(window, {startX + 14.f, rY + 30.f}, {20.f, 20.f}, 4.f, Color::White, 1.2f, cbBorder);
        if (isSelected) {
            drawCheckmark(window, {startX + 18.f, rY + 38.f}, Color(2, 132, 199));
        }

        // Col 1: ID Phim (e.g. #PBL-001)
        string idStr = (row.size() > 0) ? row[0] : "";
        Text idText(boldFont, s2ws(idStr), 13);
        idText.setFillColor(Color(2, 132, 199));
        idText.setPosition({startX + 50.f, rY + 31.f});
        window.draw(idText);

        // Col 2: Poster thumbnail + Title + Format & Genre
        float col2X = startX + 140.f;
        // Poster box
        RoundedRectRenderer::draw(window, {col2X, rY + 13.f}, {40.f, 54.f}, 6.f, Color(226, 232, 240));
        Text postTxt(boldFont, L"POSTER", 9);
        postTxt.setFillColor(Color(71, 85, 105));
        postTxt.setPosition({col2X + 4.f, rY + 31.f});
        window.draw(postTxt);

        // Title
        string titleStr = (row.size() > 1) ? row[1] : "";
        Text titleTxt(boldFont, s2ws(titleStr), 15);
        titleTxt.setFillColor(Color(15, 23, 42));
        titleTxt.setPosition({col2X + 50.f, rY + 14.f});
        window.draw(titleTxt);

        // Format Pill (e.g. 2D / Atmos, IMAX 3D, 4DX/3D)
        string fmtStr = (idx == 1) ? "IMAX 3D" : ((idx == 2) ? "2D Digital" : ((idx == 3) ? "4DX / 3D" : ((idx == 4) ? "2D Lồng tiếng" : "2D / Atmos")));
        RoundedRectRenderer::draw(window, {col2X + 50.f, rY + 41.f}, {72.f, 22.f}, 5.f, Color(241, 245, 249), 1.f, Color(203, 213, 225));
        Text fmtTxt(boldFont, s2ws(fmtStr), 11);
        fmtTxt.setFillColor(Color(30, 41, 59));
        fmtTxt.setPosition({col2X + 56.f, rY + 44.f});
        window.draw(fmtTxt);

        // Genre bullet & text
        string genreStr = (row.size() > 5) ? row[5] : "";
        Text gTxt(boldFont, L"\u2022  " + s2ws(genreStr), 12);
        gTxt.setFillColor(Color(71, 85, 105));
        gTxt.setPosition({col2X + 128.f, rY + 44.f});
        window.draw(gTxt);

        // Col 3: Director
        float col3X = startX + 500.f;
        string dirStr = (row.size() > 9) ? row[9] : "";
        Text dirTxt(boldFont, s2ws(dirStr), 13);
        dirTxt.setFillColor(Color(30, 41, 59));
        dirTxt.setPosition({col3X + 12.f, rY + 31.f});
        window.draw(dirTxt);

        // Col 4: Country pill (e.g. VN Việt Nam, US Mỹ, KR Hàn Quốc, RU Nga)
        float col4X = startX + 645.f;
        string ctry = (row.size() > 3) ? row[3] : "Việt Nam";

        Color ctryBg = Color(254, 226, 226);
        Color ctryText = Color(220, 38, 38);
        string code = "VN ";
        if (ctry.find("Mỹ") != string::npos) {
            ctryBg = Color(224, 242, 254);
            ctryText = Color(2, 132, 199);
            code = "US ";
        } else if (ctry.find("Hàn") != string::npos) {
            ctryBg = Color(243, 232, 255);
            ctryText = Color(124, 58, 237);
            code = "KR ";
        } else if (ctry.find("Nga") != string::npos) {
            ctryBg = Color(220, 252, 231);
            ctryText = Color(22, 163, 74);
            code = "RU ";
        }

        RoundedRectRenderer::draw(window, {col4X + 8.f, rY + 27.f}, {96.f, 26.f}, 13.f, ctryBg);
        Text ctryDisplay(boldFont, s2ws(code + ctry), 11);
        ctryDisplay.setFillColor(ctryText);
        ctryDisplay.setPosition({col4X + 18.f, rY + 31.f});
        window.draw(ctryDisplay);

        // Col 5: Duration
        float col5X = startX + 770.f;
        string dur = (row.size() > 6) ? row[6] : "120";
        Text durVal(boldFont, s2ws(dur), 14);
        durVal.setFillColor(Color(15, 23, 42));
        durVal.setPosition({col5X + 10.f, rY + 23.f});
        window.draw(durVal);
        Text durUnit(boldFont, L"ph\u00fat", 11);
        durUnit.setFillColor(Color(71, 85, 105));
        durUnit.setPosition({col5X + 10.f, rY + 41.f});
        window.draw(durUnit);

        // Col 6: Release Date
        float col6X = startX + 870.f;
        string relDate = (row.size() > 7) ? row[7] : "";
        Text dateTxt(boldFont, s2ws(relDate), 13);
        dateTxt.setFillColor(Color(30, 41, 59));
        dateTxt.setPosition({col6X + 10.f, rY + 31.f});
        window.draw(dateTxt);

        // Col 7: Status Pill (• Đang chiếu / • Sắp chiếu)
        float col7X = startX + 990.f;
        string st = (row.size() >= 14) ? row[13] : "Đang chiếu";
        if (st.empty()) st = "Đang chiếu";

        bool isPlaying = (st == "Đang chiếu");
        Color sPillBg = isPlaying ? Color(220, 252, 231) : Color(224, 242, 254);
        Color sPillText = isPlaying ? Color(21, 128, 61) : Color(3, 105, 161);
        Color sDot = isPlaying ? Color(21, 128, 61) : Color(3, 105, 161);

        RoundedRectRenderer::draw(window, {col7X + 6.f, rY + 25.f}, {112.f, 30.f}, 15.f, sPillBg);
        CircleShape dot(3.5f, 10);
        dot.setFillColor(sDot);
        dot.setPosition({col7X + 18.f, rY + 37.f});
        window.draw(dot);

        Text sPill(boldFont, isPlaying ? L"\u0110ang chi\u1ebfu" : L"S\u1eafp chi\u1ebfu", 12);
        sPill.setFillColor(sPillText);
        sPill.setPosition({col7X + 28.f, rY + 30.f});
        window.draw(sPill);

        // Col 8: Action Icons (Edit & Delete)
        float col8X = startX + 1125.f;
        RoundedRectRenderer::draw(window, {col8X + 8.f, rY + 24.f}, {32.f, 32.f}, 8.f, (idx == hoveredEditRow) ? Color(254, 243, 199) : Color(241, 245, 249));
        Color iEditCol = (idx == hoveredEditRow) ? Color(180, 83, 9) : Color(217, 119, 6);
        drawPencilIcon(window, {col8X + 16.f, rY + 32.f}, iEditCol);

        RoundedRectRenderer::draw(window, {col8X + 48.f, rY + 24.f}, {32.f, 32.f}, 8.f, (idx == hoveredDeleteRow) ? Color(254, 226, 226) : Color(241, 245, 249));
        Color iDelCol = (idx == hoveredDeleteRow) ? Color(185, 28, 28) : Color(220, 38, 38);
        drawTrashIcon(window, {col8X + 56.f, rY + 32.f}, iDelCol);
    }

    // 9. Dropdown menus (if open)
    if (genreDropdownOpen) {
        vector<string> genres = {"Tất cả thể loại", "Hành động", "Hoạt hình", "Tội phạm", "Viễn tưởng", "Tâm lý"};
        RoundedRectRenderer::draw(window, {startX + 375.f, toolY + 46.f}, {160.f, genres.size() * 36.f}, 8.f, Color::White, 1.2f, Color(203, 213, 225));
        for (size_t g = 0; g < genres.size(); ++g) {
            Color itemBg = (static_cast<int>(g) == hoveredGenreIndex) ? Color(241, 245, 249) : Color::White;
            RoundedRectRenderer::draw(window, {startX + 376.f, toolY + 47.f + g * 36.f}, {158.f, 34.f}, 4.f, itemBg);
            Text it(boldFont, s2ws(genres[g]), 13);
            it.setFillColor(Color(15, 23, 42));
            it.setPosition({startX + 386.f, toolY + 55.f + g * 36.f});
            window.draw(it);
        }
    }

    if (statusDropdownOpen) {
        vector<string> statuses = {"Trạng thái: Tất cả", "Đang chiếu", "Sắp chiếu"};
        RoundedRectRenderer::draw(window, {startX + 550.f, toolY + 46.f}, {160.f, statuses.size() * 36.f}, 8.f, Color::White, 1.2f, Color(203, 213, 225));
        for (size_t s = 0; s < statuses.size(); ++s) {
            Color itemBg = (static_cast<int>(s) == hoveredStatusIndex) ? Color(241, 245, 249) : Color::White;
            RoundedRectRenderer::draw(window, {startX + 551.f, toolY + 47.f + s * 36.f}, {158.f, 34.f}, 4.f, itemBg);
            Text it(boldFont, s2ws(statuses[s]), 13);
            it.setFillColor(Color(15, 23, 42));
            it.setPosition({startX + 561.f, toolY + 55.f + s * 36.f});
            window.draw(it);
        }
    }

    // 10. MODAL POPUP
    if (currentPopup != NONE) {
        RectangleShape overlay({1728.f, 972.f});
        overlay.setFillColor(Color(15, 23, 42, 140));
        window.draw(overlay);

        const float popupW = 680.f;
        const float popupH = (currentPopup == DELETE_CONFIRM) ? 240.f : 530.f;
        const float popupX = (1728.f - popupW) / 2.f;
        const float popupY = (972.f - popupH) / 2.f;

        RoundedRectRenderer::draw(window, {popupX, popupY}, {popupW, popupH}, 16.f, Color::White, 1.f, Color(226, 232, 240));

        if (currentPopup == DELETE_CONFIRM) {
            Text pTitle(boldFont, L"X\u00c1C NH\u1eacN X\u00d3A PHIM", 18);
            pTitle.setFillColor(Color(239, 68, 68));
            pTitle.setPosition({popupX + 40.f, popupY + 35.f});
            window.draw(pTitle);

            string delName = (selectedRow >= 0 && selectedRow < static_cast<int>(allMovies.size())) ? allMovies[selectedRow][1] : "";
            Text pMsg(font, L"B\u1ea1n c\u00f3 ch\u1eafc ch\u1eafn mu\u1ed1n x\u00f3a phim \"" + s2ws(delName) + L"\" kh\u00f4ng?\nThao t\u00e1c n\u00e0y kh\u00f4ng th\u1ec3 ho\u00e0n t\u00e1c.", 13);
            pMsg.setFillColor(Color(71, 85, 105));
            pMsg.setPosition({popupX + 40.f, popupY + 80.f});
            window.draw(pMsg);

            Color cBg = btnDeleteCancelHover ? Color(203, 213, 225) : Color(226, 232, 240);
            RoundedRectRenderer::draw(window, {popupX + 40.f, popupY + 160.f}, {280.f, 44.f}, 8.f, cBg);
            Text cTxt(boldFont, L"H\u1ee7y b\u1ecf", 13);
            cTxt.setFillColor(Color(51, 65, 85));
            cTxt.setPosition({popupX + 155.f, popupY + 172.f});
            window.draw(cTxt);

            Color okBg = btnDeleteOkHover ? Color(220, 38, 38) : Color(239, 68, 68);
            RoundedRectRenderer::draw(window, {popupX + 360.f, popupY + 160.f}, {280.f, 44.f}, 8.f, okBg);
            Text okTxt(boldFont, L"\u0110\u1ed3ng \u00fd x\u00f3a", 13);
            okTxt.setFillColor(Color::White);
            okTxt.setPosition({popupX + 465.f, popupY + 172.f});
            window.draw(okTxt);
        } else {
            wstring mTitle = (currentPopup == ADD) ? L"TH\u00caM PHIM M\u1edaI" : L"CH\u1ec8NH S\u1eecA TH\u00d4NG TIN PHIM";
            Text pTitle(boldFont, mTitle, 18);
            pTitle.setFillColor(Color(15, 23, 42));
            pTitle.setPosition({popupX + 40.f, popupY + 30.f});
            window.draw(pTitle);

            struct FormField {
                wstring label;
                wstring* val;
                int idx;
            };

            vector<FormField> leftFields = {
                {L"T\u00ean phim*", &inputTitle, 0},
                {L"Th\u1ec3 lo\u1ea1i*", &inputGenres, 1},
                {L"\u0110\u1ecbnh d\u1ea1ng", &inputFormat, 2},
                {L"\u0110\u1ea1o di\u1ec5n*", &inputDirector, 3}
            };
            vector<FormField> rightFields = {
                {L"Qu\u1ed1c gia*", &inputCountry, 4},
                {L"Th\u1eddi l\u01b0\u1ee3ng (ph\u00fat)*", &inputDuration, 5},
                {L"Ng\u00e0y chi\u1ebfu (DD/MM/YYYY)*", &inputReleaseDate, 6},
                {L"Tr\u1ea1ng th\u00e1i (\u0110ang chi\u1ebfu / S\u1eafp chi\u1ebfu)", &inputStatus, 7}
            };

            const float leftX = popupX + 40.f;
            const float rightX = popupX + 360.f;
            const float startY = popupY + 75.f;
            const float gapY = 68.f;

            for (int i = 0; i < 4; ++i) {
                // Left
                Text lbl1(boldFont, leftFields[i].label, 12);
                lbl1.setFillColor(Color(30, 41, 59));
                lbl1.setPosition({leftX, startY + i * gapY});
                window.draw(lbl1);

                Color b1 = (activeInputBox == leftFields[i].idx) ? Color(2, 132, 199) : Color(203, 213, 225);
                RoundedRectRenderer::draw(window, {leftX, startY + i * gapY + 20.f}, {280.f, 40.f}, 8.f, Color(248, 250, 252), 1.2f, b1);
                Text v1(boldFont, *leftFields[i].val, 13);
                v1.setFillColor(Color(15, 23, 42));
                v1.setPosition({leftX + 10.f, startY + i * gapY + 29.f});
                window.draw(v1);

                // Right
                Text lbl2(boldFont, rightFields[i].label, 12);
                lbl2.setFillColor(Color(30, 41, 59));
                lbl2.setPosition({rightX, startY + i * gapY});
                window.draw(lbl2);

                Color b2 = (activeInputBox == rightFields[i].idx) ? Color(2, 132, 199) : Color(203, 213, 225);
                RoundedRectRenderer::draw(window, {rightX, startY + i * gapY + 20.f}, {280.f, 40.f}, 8.f, Color(248, 250, 252), 1.2f, b2);
                Text v2(boldFont, *rightFields[i].val, 13);
                v2.setFillColor(Color(15, 23, 42));
                v2.setPosition({rightX + 10.f, startY + i * gapY + 29.f});
                window.draw(v2);
            }

            // Buttons
            Color cBg = btnPopupCancelHover ? Color(203, 213, 225) : Color(226, 232, 240);
            RoundedRectRenderer::draw(window, {leftX, popupY + 450.f}, {280.f, 44.f}, 8.f, cBg);
            Text cTxt(boldFont, L"\u0110\u00f3ng", 14);
            cTxt.setFillColor(Color(51, 65, 85));
            cTxt.setPosition({leftX + 120.f, popupY + 460.f});
            window.draw(cTxt);

            Color sBg = btnPopupSaveHover ? Color(3, 105, 161) : Color(0, 119, 182);
            RoundedRectRenderer::draw(window, {rightX, popupY + 450.f}, {280.f, 44.f}, 8.f, sBg);
            Text sTxt(boldFont, L"L\u01b0u th\u00f4ng tin", 14);
            sTxt.setFillColor(Color::White);
            sTxt.setPosition({rightX + 90.f, popupY + 460.f});
            window.draw(sTxt);
        }
    }

    // 11. TOAST NOTIFICATION
    if (notificationVisible) {
        if (notificationClock.getElapsedTime().asSeconds() > 3.0f) {
            notificationVisible = false;
        } else {
            const float toastW = 380.f;
            const float toastX = (1728.f - toastW) / 2.f;
            const float toastY = 972.f - 70.f;
            RoundedRectRenderer::draw(window, {toastX, toastY}, {toastW, 44.f}, 10.f, notificationColor);
            drawCheckmark(window, {toastX + 16.f, toastY + 16.f}, Color::White);
            Text nMsg(boldFont, notificationMessage, 12);
            nMsg.setFillColor(Color::White);
            nMsg.setPosition({toastX + 36.f, toastY + 12.f});
            window.draw(nMsg);
        }
    }
}
