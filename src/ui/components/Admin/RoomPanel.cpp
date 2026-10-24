#include "UI/components/Admin/RoomPanel.h"
#include "UI/components/Admin/RoundedRectRenderer.h"
#include "utils/FileUtils.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <ctime>
#include <array>

using namespace std::chrono;

namespace {
sf::String utf8(const string& text) {
    return sf::String::fromUtf8(text.begin(), text.end());
}

}

RoomPanel::RoomPanel(Font& f, float w, float h)
    :   font(f), width(w), height(h), 
        reloadTexture("../assets/elements/reload.png") 
{
    setupUI();
    loadData();
}

void RoomPanel::setupUI() {
    background.setSize(Vector2f(width, height));
    background.setFillColor(bgColor);

    titleText = make_unique<Text>(font);
    titleText->setCharacterSize(26);
    titleText->setFillColor(Color(27, 38, 59));
    titleText->setStyle(Text::Bold);
    titleText->setString(utf8("Quản lý phòng chiếu"));

    tableHeaderBg.setSize(Vector2f(TABLE_WIDTH, HEADER_HEIGHT));
    tableHeaderBg.setFillColor(headerColor);

    reloadButtonBg.setSize(Vector2f(44.f, 44.f));
    reloadButtonBg.setFillColor(Color(20, 118, 172));

    reloadTexture.setSmooth(true);
    reloadSprite = make_unique<Sprite>(reloadTexture);
    reloadSprite->setScale({0.1f, 0.1f});

    notificationBg.setSize(Vector2f(400.f, 60.f));
    notificationBg.setFillColor(Color(211, 47, 47, 230));
    notificationText = make_unique<Text>(font);
    notificationText->setCharacterSize(18);
    notificationText->setFillColor(Color::White);
}

void RoomPanel::showNotification(const string& message) {
    notificationMessage = message;
    notificationVisible = !message.empty();
    if (notificationVisible) {
        notificationClock.restart();
    }
    if (notificationText) {
        notificationText->setString(utf8(message));
    }
}

void RoomPanel::renderNotification(RenderWindow& window) {
    if (!notificationVisible) return;
    if (notificationClock.getElapsedTime().asSeconds() > 3.0f) {
        notificationVisible = false;
        return;
    }

    notificationBg.setPosition(Vector2f(position.x + width - notificationBg.getSize().x - 30.f, position.y + 20.f));
    window.draw(notificationBg);

    if (notificationText) {
        FloatRect bounds = notificationText->getLocalBounds();
        notificationText->setPosition(Vector2f(
            notificationBg.getPosition().x + (notificationBg.getSize().x - bounds.size.x) / 2.f - bounds.position.x,
            notificationBg.getPosition().y + (notificationBg.getSize().y - bounds.size.y) / 2.f - bounds.position.y
        ));
        window.draw(*notificationText);
    }
}

void RoomPanel::setPosition(Vector2f pos) {
    position = pos;
    background.setPosition(pos);
    if (titleText) {
        titleText->setPosition(Vector2f(pos.x + 40.f, pos.y + 20.f));
    }
    tableHeaderBg.setPosition(Vector2f(pos.x + TABLE_X, pos.y + TABLE_Y));

    reloadButtonBg.setPosition(Vector2f(pos.x + TABLE_X + TABLE_WIDTH - reloadButtonBg.getSize().x, pos.y + TABLE_Y - 64.f));
}

void RoomPanel::loadData() {
    const string roomsPath = FileUtils::resolveDataPath("data/rooms.txt");
    const string moviesPath = FileUtils::resolveDataPath("data/movies.txt");
    const string showtimesPath = FileUtils::resolveDataPath("data/showtimes.txt");
    const string showtimesHistoryPath = FileUtils::resolveDataPath("data/showtimes_history.txt");

    rooms = loadRooms(roomsPath);
    auto movies = loadMovies(moviesPath);
    liveSchedules.clear();
    cachedSchedules.clear();
    roomSchedules.clear();
    cacheDirty = false;

    loadShowtimes(showtimesPath, movies);
    // Include history so rooms can still show the currently-playing movie even if the
    // active showtimes file only contains upcoming entries.
    loadShowtimes(showtimesHistoryPath, movies);
    roomSchedules = liveSchedules;
    loadCache();
    cacheDirty = removeExpiredCachedShows();
    updateRoomStatuses();
    if (ensureUpcomingShowsCached()) {
        cacheDirty = true;
    }
    if (cacheDirty) {
        saveCache();
    }
    hoveredRow = -1;
    selectedRow = -1;
    statusRefreshClock.restart();
}

vector<RoomPanel::RoomInfo> RoomPanel::loadRooms(const string& path) {
    vector<RoomInfo> result;
    ifstream file(path);
    if (!file.is_open()) {
        showNotification("Không thể mở file: " + path);
        return result;
    }

    string line;
    getline(file, line); // Skip header
    while (getline(file, line)) {
        auto parts = splitLine(line, '|');
        if (parts.size() < 2) continue;
        RoomInfo info;
        info.id = trim(parts[0]);
        info.name = trim(parts[1]);
        if (!info.id.empty()) {
            result.push_back(info);
        }
    }
    return result;
}

unordered_map<string, RoomPanel::MovieInfo> RoomPanel::loadMovies(const string& path) {
    unordered_map<string, MovieInfo> movies;
    ifstream file(path);
    if (!file.is_open()) {
        showNotification("Không thể mở file: " + path);
        return movies;
    }
    string line;
    getline(file, line);
    while (getline(file, line)) {
        auto parts = splitLine(line, '|');
        if (parts.size() < 7) continue;
        MovieInfo info;
        info.title = trim(parts[1]);
        try {
            info.durationMinutes = stoi(trim(parts[6]));
        } catch (...) {
            info.durationMinutes = 0;
        }
        movies[trim(parts[0])] = info;
    }
    return movies;
}

void RoomPanel::loadShowtimes(const string& path, const unordered_map<string, MovieInfo>& movies) {
    ifstream file(path);
    if (!file.is_open()) {
        showNotification("Không thể mở file: " + path);
        return;
    }
    string line;
    getline(file, line);
    while (getline(file, line)) {
        auto parts = splitLine(line, '|');
        if (parts.size() < 5) continue;
        string movieId = trim(parts[1]);
        string roomId = trim(parts[2]);
        string dateStr = trim(parts[3]);
        string timeStr = trim(parts[4]);

        auto startOpt = parseDateTime(dateStr, timeStr);
        if (!startOpt.has_value()) continue;
        auto movieIt = movies.find(movieId);
        string movieTitle = movieIt != movies.end() ? movieIt->second.title : movieId;
        int duration = movieIt != movies.end() ? movieIt->second.durationMinutes : 0;
        system_clock::time_point start = startOpt.value();
        int clampedDuration = duration <= 0 ? 1 : duration;
        system_clock::time_point end = start + minutes(clampedDuration);

        ShowtimeSlot slot{start, end, movieTitle};
        auto& list = liveSchedules[roomId];
        if (!hasSlot(list, slot)) {
            list.push_back(slot);
        }
    }

    for (auto& [roomId, slots] : liveSchedules) {
        sort(slots.begin(), slots.end(), [](const ShowtimeSlot& a, const ShowtimeSlot& b) {
            return a.start < b.start;
        });
    }
}

vector<string> RoomPanel::splitLine(const string& line, char delimiter) {
    vector<string> parts;
    string part;
    stringstream ss(line);
    while (getline(ss, part, delimiter)) {
        parts.push_back(part);
    }
    return parts;
}

string RoomPanel::trim(const string& text) {
    size_t start = 0;
    while (start < text.size() && (isspace(static_cast<unsigned char>(text[start])) || text[start] == '\r')) {
        ++start;
    }
    size_t end = text.size();
    while (end > start && (isspace(static_cast<unsigned char>(text[end - 1])) || text[end - 1] == '\r')) {
        --end;
    }
    return text.substr(start, end - start);
}

optional<system_clock::time_point> RoomPanel::parseDateTime(const string& dateStr, const string& timeStr) {
    if (dateStr.size() < 10 || timeStr.size() < 5) return nullopt;
    int year = 0, month = 0, day = 0, hour = 0, minute = 0;
    try {
        year = stoi(dateStr.substr(0, 4));
        month = stoi(dateStr.substr(5, 2));
        day = stoi(dateStr.substr(8, 2));
        hour = stoi(timeStr.substr(0, 2));
        minute = stoi(timeStr.substr(3, 2));
    } catch (...) {
        return nullopt;
    }

    tm tmTime{};
    tmTime.tm_year = year - 1900;
    tmTime.tm_mon = month - 1;
    tmTime.tm_mday = day;
    tmTime.tm_hour = hour;
    tmTime.tm_min = minute;
    tmTime.tm_sec = 0;
    tmTime.tm_isdst = -1;

    time_t localTime = mktime(&tmTime);
    if (localTime == -1) return nullopt;
    return system_clock::from_time_t(localTime);
}

optional<system_clock::time_point> RoomPanel::parseCacheDateTime(const string& value) {
    if (value.size() < 16) return nullopt;
    string date = value.substr(0, 10);
    string time = value.substr(11, 5);
    return parseDateTime(date, time);
}

string RoomPanel::formatDateTime(system_clock::time_point tp) {
    time_t raw = system_clock::to_time_t(tp);
    tm local{};
#ifdef _WIN32
    localtime_s(&local, &raw);
#else
    localtime_r(&raw, &local);
#endif
    char buffer[20];
    strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M", &local);
    return string(buffer);
}

void RoomPanel::loadCache() {
    cachedSchedules.clear();
    ifstream file(cacheFilePath);
    if (!file.is_open()) {
        return;
    }

    string line;
    while (getline(file, line)) {
        auto parts = splitLine(line, '|');
        if (parts.size() < 4) continue;
        string roomId = trim(parts[0]);
        string movieTitle = trim(parts[1]);
        auto startOpt = parseCacheDateTime(trim(parts[2]));
        auto endOpt = parseCacheDateTime(trim(parts[3]));
        if (!startOpt.has_value() || !endOpt.has_value()) continue;

        ShowtimeSlot slot{startOpt.value(), endOpt.value(), movieTitle};
        auto& cachedList = cachedSchedules[roomId];
        if (!hasSlot(cachedList, slot)) {
            cachedList.push_back(slot);
        }
        auto& combined = roomSchedules[roomId];
        if (!hasSlot(combined, slot)) {
            combined.push_back(slot);
        }
    }

    for (auto& [roomId, slots] : cachedSchedules) {
        sort(slots.begin(), slots.end(), [](const ShowtimeSlot& a, const ShowtimeSlot& b) {
            return a.start < b.start;
        });
    }
    for (auto& [roomId, slots] : roomSchedules) {
        sort(slots.begin(), slots.end(), [](const ShowtimeSlot& a, const ShowtimeSlot& b) {
            return a.start < b.start;
        });
    }
}

void RoomPanel::saveCache() {
    ofstream file(cacheFilePath, ios::trunc);
    if (!file.is_open()) {
        showNotification("Không thể ghi file cache: " + cacheFilePath);
        return;
    }

    for (const auto& [roomId, slots] : cachedSchedules) {
        for (const auto& slot : slots) {
            file << roomId << '|' << slot.movieTitle << '|' << formatDateTime(slot.start)
                 << '|' << formatDateTime(slot.end) << '\n';
        }
    }
}

bool RoomPanel::hasSlot(const vector<ShowtimeSlot>& slots, const ShowtimeSlot& slot) const {
    for (const auto& existing : slots) {
        if (slotsEqual(existing, slot)) {
            return true;
        }
    }
    return false;
}

bool RoomPanel::slotsEqual(const ShowtimeSlot& a, const ShowtimeSlot& b) {
    return a.movieTitle == b.movieTitle && a.start == b.start;
}

bool RoomPanel::pruneSchedules(unordered_map<string, vector<ShowtimeSlot>>& schedules,
                               chrono::system_clock::time_point now) {
    bool removed = false;
    for (auto it = schedules.begin(); it != schedules.end();) {
        auto& list = it->second;
        size_t before = list.size();
        list.erase(remove_if(list.begin(), list.end(), [&](const ShowtimeSlot& slot) {
            return slot.end <= now;
        }), list.end());

        if (list.empty()) {
            it = schedules.erase(it);
        } else {
            ++it;
        }

        if (list.size() != before) {
            removed = true;
        }
    }
    return removed;
}

bool RoomPanel::removeExpiredCachedShows() {
    auto now = system_clock::now();
    bool removed = pruneSchedules(cachedSchedules, now);
    removed = pruneSchedules(roomSchedules, now) || removed;
    return removed;
}

bool RoomPanel::ensureUpcomingShowsCached() {
    auto now = system_clock::now();
    bool updated = false;

    for (const auto& [roomId, slots] : liveSchedules) {
        ShowtimeSlot candidate;
        bool found = false;
        for (const auto& slot : slots) {
            if (slot.end <= now) continue;
            candidate = slot;
            found = true;
            break;
        }
        if (!found) continue;

        auto& cachedList = cachedSchedules[roomId];
        if (!hasSlot(cachedList, candidate)) {
            cachedList.push_back(candidate);
            updated = true;
        }

        auto& combined = roomSchedules[roomId];
        if (!hasSlot(combined, candidate)) {
            combined.push_back(candidate);
            updated = true;
        }
    }

    if (updated) {
        for (auto& [roomId, slots] : cachedSchedules) {
            sort(slots.begin(), slots.end(), [](const ShowtimeSlot& a, const ShowtimeSlot& b) {
                return a.start < b.start;
            });
        }
        for (auto& [roomId, slots] : roomSchedules) {
            sort(slots.begin(), slots.end(), [](const ShowtimeSlot& a, const ShowtimeSlot& b) {
                return a.start < b.start;
            });
        }
    }

    return updated;
}

void RoomPanel::updateRoomStatuses() {
    rows.clear();
    rows.reserve(rooms.size());
    auto now = system_clock::now();

    for (const auto& room : rooms) {
        string current = "Không";
        string upcoming = "Không";
        auto it = roomSchedules.find(room.id);
        if (it != roomSchedules.end()) {
            const auto& slots = it->second;
            int currentIndex = -1;
            for (size_t i = 0; i < slots.size(); ++i) {
                // Current movie is playing when now is in [start, end)
                if (now >= slots[i].start && now < slots[i].end) {
                    currentIndex = static_cast<int>(i);
                    current = slots[i].movieTitle;
                    break;
                }
            }

            if (currentIndex >= 0) {
                // Upcoming is the next showtime after the currently-playing one (same room)
                if (static_cast<size_t>(currentIndex + 1) < slots.size()) {
                    upcoming = slots[static_cast<size_t>(currentIndex + 1)].movieTitle;
                }
            } else {
                // If nothing is playing now, upcoming is the first showtime that starts after now
                for (const auto& slot : slots) {
                    if (slot.start > now) {
                        upcoming = slot.movieTitle;
                        break;
                    }
                }
            }
        }
        rows.push_back(RoomRow{room.id, room.name, current, upcoming});
    }

    if (selectedRow >= static_cast<int>(rows.size())) {
        selectedRow = -1;
    }
}

void RoomPanel::handleEvent(const Event& event, const RenderWindow& window) {
    if (const auto* mouseEvent = event.getIf<Event::MouseButtonPressed>()) {
        if (mouseEvent->button == Mouse::Button::Left) {
            Vector2f mousePos = window.mapPixelToCoords(Mouse::getPosition(window));
            FloatRect reloadBounds = reloadButtonBg.getGlobalBounds();
            if (reloadBounds.contains(mousePos)) {
                reloadPressed = true;
                return;
            }

            float startY = position.y + TABLE_Y + HEADER_HEIGHT;
            for (size_t i = 0; i < rows.size(); ++i) {
                FloatRect rowBounds(Vector2f(position.x + TABLE_X, startY + i * ROW_HEIGHT), Vector2f(TABLE_WIDTH, ROW_HEIGHT));
                if (rowBounds.contains(mousePos)) {
                    selectedRow = static_cast<int>(i);
                    break;
                }
            }
        }
    }
}

void RoomPanel::update(Vector2f mousePos, bool mousePressed) {
    if (statusRefreshClock.getElapsedTime().asSeconds() >= statusRefreshInterval) {
        bool removed = removeExpiredCachedShows();
        updateRoomStatuses();
        bool added = ensureUpcomingShowsCached();
        if (removed || added) {
            saveCache();
        }
        statusRefreshClock.restart();
    }

    FloatRect reloadBounds = reloadButtonBg.getGlobalBounds();
    reloadHovered = reloadBounds.contains(mousePos);
    if (reloadHovered) {
        reloadButtonBg.setFillColor(Color(30, 138, 192));
    } else {
        reloadButtonBg.setFillColor(Color(20, 118, 172));
    }

    if (reloadPressed && !mousePressed) {
        if (reloadHovered) {
            loadData();
            showNotification("Đã tải lại dữ liệu");
        }
        reloadPressed = false;
    }

    hoveredRow = -1;
    float startY = position.y + TABLE_Y + HEADER_HEIGHT;
    for (size_t i = 0; i < rows.size(); ++i) {
        FloatRect rowBounds(Vector2f(position.x + TABLE_X, startY + i * ROW_HEIGHT), Vector2f(TABLE_WIDTH, ROW_HEIGHT));
        if (rowBounds.contains(mousePos)) {
            hoveredRow = static_cast<int>(i);
            break;
        }
    }
}

void RoomPanel::renderTable(RenderWindow& window) {
    window.draw(tableHeaderBg);

    struct ColumnSpec {
        string label;
        float ratio;
    };

    static const array<ColumnSpec, 4> columns = {{{"ID phòng", 0.15f},
        {"Tên phòng", 0.25f}, {"Phim đang chiếu", 0.30f}, {"Phim sắp chiếu", 0.30f}}};

    vector<float> columnWidths;
    columnWidths.reserve(columns.size());
    for (const auto& col : columns) {
        columnWidths.push_back(TABLE_WIDTH * col.ratio);
    }

    vector<float> columnLefts(columns.size(), position.x + TABLE_X);
    for (size_t i = 1; i < columns.size(); ++i) {
        columnLefts[i] = columnLefts[i - 1] + columnWidths[i - 1];
    }

    float headerTop = position.y + TABLE_Y;
    float startY = headerTop + HEADER_HEIGHT;
    float totalHeight = HEADER_HEIGHT + static_cast<float>(rows.size()) * ROW_HEIGHT;

    RectangleShape separator(Vector2f(1.f, totalHeight));
    separator.setFillColor(borderColor);
    separator.setPosition(Vector2f(position.x + TABLE_X, headerTop));
    window.draw(separator);
    for (size_t i = 0; i < columns.size(); ++i) {
        separator.setPosition(Vector2f(columnLefts[i] + columnWidths[i], headerTop));
        window.draw(separator);
    }

    for (size_t i = 0; i < columns.size(); ++i) {
        Text text(font, utf8(columns[i].label), 18);
        text.setFillColor(Color::White);
        text.setStyle(Text::Bold);
        FloatRect bounds = text.getLocalBounds();
        text.setPosition(Vector2f(
            columnLefts[i] + (columnWidths[i] - bounds.size.x) / 2.f - bounds.position.x,
            headerTop + (HEADER_HEIGHT - bounds.size.y) / 2.f - bounds.position.y
        ));
        window.draw(text);
    }

    for (size_t i = 0; i < rows.size(); ++i) {
        float rowY = startY + i * ROW_HEIGHT;
        RectangleShape rowBg(Vector2f(TABLE_WIDTH, ROW_HEIGHT));
        rowBg.setPosition(Vector2f(position.x + TABLE_X, rowY));
        if (static_cast<int>(i) == selectedRow) {
            rowBg.setFillColor(selectedColor);
        } else if (static_cast<int>(i) == hoveredRow) {
            rowBg.setFillColor(hoverColor);
        } else {
            rowBg.setFillColor(rowColor);
        }
        window.draw(rowBg);

        RectangleShape border(Vector2f(TABLE_WIDTH, 1.f));
        border.setPosition(Vector2f(position.x + TABLE_X, rowY + ROW_HEIGHT));
        border.setFillColor(borderColor);
        window.draw(border);

        array<string, 4> values = {
            rows[i].roomId,
            rows[i].roomName,
            rows[i].currentMovie.empty() ? string("Không") : rows[i].currentMovie,
            rows[i].upcomingMovie.empty() ? string("Không") : rows[i].upcomingMovie
        };

        for (size_t col = 0; col < columns.size(); ++col) {
            Text text(font, utf8(values[col]), 16);
            text.setFillColor(textColor);
            FloatRect bounds = text.getLocalBounds();
            text.setPosition(Vector2f(
                columnLefts[col] + 12.f - bounds.position.x,
                rowY + (ROW_HEIGHT - bounds.size.y) / 2.f - bounds.position.y
            ));
            window.draw(text);
        }
    }
}

void RoomPanel::render(RenderWindow& window) {
    window.draw(background);
    if (titleText) 
        window.draw(*titleText);

    renderTable(window);

    RoundedRectRenderer::draw(window, reloadButtonBg.getPosition(), reloadButtonBg.getSize(), 6.f, reloadButtonBg.getFillColor());
    FloatRect spriteBounds = reloadSprite->getLocalBounds();
    Vector2f scale = reloadSprite->getScale();
    float spriteWidth = spriteBounds.size.x * scale.x;
    float spriteHeight = spriteBounds.size.y * scale.y;
    reloadSprite->setPosition(Vector2f(reloadButtonBg.getPosition().x + (reloadButtonBg.getSize().x - spriteWidth) / 2.f, reloadButtonBg.getPosition().y + (reloadButtonBg.getSize().y - spriteHeight) / 2.f));
    reloadSprite->setColor(Color::White);
    window.draw(*reloadSprite);

    renderNotification(window);
}