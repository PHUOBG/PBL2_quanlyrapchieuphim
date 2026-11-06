#pragma once

#include <SFML/Graphics.hpp>
#include "repositories/admin/AdminMovieRepository.h"
#include "UI/components/Admin/RoundedRectRenderer.h"
#include <string>
#include <vector>
#include <map>
#include <memory>

using namespace sf;
using namespace std;

class MoviePanel {
public:
    MoviePanel(Font& font, float width, float height);

    void setPosition(Vector2f pos);
    void handleEvent(const Event& event, const RenderWindow& window);
    void update(Vector2f mousePos, bool mousePressed);
    void render(RenderWindow& window);

private:
    Font& font;
    Font boldFont;
    float width, height;
    Vector2f position;

    map<string, Texture> posterCache;
    const Texture* getPosterTexture(const string& rawPath, const string& movieId = "");

    unique_ptr<AdminMovieRepository> repository;
    vector<vector<string>> allMovies;
    vector<int> filteredIndices;

    // Search and filter states
    wstring searchQuery;
    bool searchActive = false;
    bool caretVisible = true;
    Clock caretClock;

    string selectedGenre = "all";
    string selectedStatus = "all";
    bool genreDropdownOpen = false;
    bool statusDropdownOpen = false;
    int hoveredGenreIndex = -1;
    int hoveredStatusIndex = -1;

    // Selection & pagination/scroll
    int selectedRow = -1;
    int hoveredRow = -1;
    int scrollOffset = 0;
    int hoveredEditRow = -1;
    int hoveredDeleteRow = -1;

    // Toolbar buttons hover
    bool btnAddHover = false;
    bool btnEditHover = false;
    bool btnDeleteHover = false;
    bool btnReloadHover = false;
    bool btnSearchHover = false;
    bool btnGenreHover = false;
    bool btnStatusHover = false;
    bool bellHover = false;

    // Popup modal states
    enum PopupType { NONE, ADD, EDIT, DELETE_CONFIRM };
    PopupType currentPopup = NONE;

    // Popup Form Inputs
    wstring inputTitle;
    wstring inputGenres;
    wstring inputFormat;
    wstring inputDirector;
    wstring inputCountry;
    wstring inputDuration;
    wstring inputReleaseDate;
    wstring inputStatus;
    int activeInputBox = -1; // 0 to 7
    bool btnPopupSaveHover = false;
    bool btnPopupCancelHover = false;
    bool btnDeleteOkHover = false;
    bool btnDeleteCancelHover = false;

    // Toast notification
    wstring notificationMessage;
    Color notificationColor = Color(16, 185, 129);
    Clock notificationClock;
    bool notificationVisible = false;

    void refreshData();
    void applyFilter();
    void showToast(const wstring& msg, Color col = Color(16, 185, 129));

    void openAddModal();
    void openEditModal(int recordIndex);
    void openDeleteModal(int recordIndex);
    void closePopupModal();
    void saveMovie();
    void confirmDelete();

    // Vector drawing helpers
    void drawPlayIcon(RenderTarget& target, Vector2f pos, Color col);
    void drawCalendarIcon(RenderTarget& target, Vector2f pos, Color col);
    void drawClockIcon(RenderTarget& target, Vector2f pos, Color col);
    void drawFilmIcon(RenderTarget& target, Vector2f pos, Color col);
    void drawSearchIcon(RenderTarget& target, Vector2f pos, Color col);
    void drawPencilIcon(RenderTarget& target, Vector2f pos, Color col);
    void drawTrashIcon(RenderTarget& target, Vector2f pos, Color col);
    void drawPlusIcon(RenderTarget& target, Vector2f pos, Color col);
    void drawReloadIcon(RenderTarget& target, Vector2f pos, Color col);
    void drawBellIcon(RenderTarget& target, Vector2f pos, Color col);
    void drawFunnelIcon(RenderTarget& target, Vector2f pos, Color col);
    void drawCheckmark(RenderTarget& target, Vector2f pos, Color col);
    void drawDownArrow(RenderTarget& target, Vector2f pos, Color col);
};
