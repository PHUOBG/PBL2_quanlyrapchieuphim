#pragma once
#include <SFML/Graphics.hpp>
#include "core/AppState.h"
#include "UI/components/TextButton.h"
#include "UI/components/GlobalSearchBar.h"
#include "services/MovieSearchManager.h"
#include "data-structures/DLL.h"
#include <memory>
using namespace std;
using namespace sf;

class BaseScreen {
    protected:
        Font& font;
        Texture background_tex;
        Sprite background_sprite;
        Texture searchBar_tex;
        Sprite searchBar_sprite;
        DLL<TextButton> buttons;

        bool globalSearchEnabled = true;
        
        // Global search bar for all screens
        unique_ptr<GlobalSearchBar> globalSearchBar;
        unique_ptr<MovieSearchManager> globalSearchManager;
        
        // Track selected movie from search
        int selectedMovieIndexFromSearch = -1;
        
        // Static variables để lưu trạng thái đăng nhập chung cho tất cả screen
        static string loggedInUsername;
        static string loggedInUserEmail;
        static bool logoutRequested;  // Flag để báo cho App biết cần reset
        
        // Dropdown menu components
        bool showDropdown = false;
        RectangleShape dropdownBox;
        TextButton accountButton;
        TextButton logoutButton;
    public:
        BaseScreen(Font& f);

        virtual ~BaseScreen() = default;

        virtual void update(Vector2f mousePos, bool mousePressed, AppState& state);
        
        // Method to handle events (for search bar keyboard input)
        virtual void handleEvent(const Event& event);

        virtual void draw(RenderWindow& window);
        
        // Draw UI overlay elements that should be on top of everything
        virtual void drawOverlay(RenderWindow& window);

        void setAccountButtonText(const String& text);
        
        // Initialize search functionality (to be called by derived classes with movie data)
        void initializeGlobalSearch(const DLL<MovieDetail>& movies);

        // Enable/disable search UI and behavior for specific screens
        void setGlobalSearchEnabled(bool enabled);
        
        // Get selected movie index from global search
        int getSelectedMovieIndexFromSearch() const;
        
        // Clear selected movie index after use
        void clearSelectedMovieIndexFromSearch();
        
        // Static methods để set/get username và email cho tất cả screens
        static void setLoggedInUser(const string& username, const string& email);
        static string getLoggedInUser();
        static string getLoggedInUserEmail();
        static bool isUserLoggedIn();
        
        // Method để logout user
        static void handleLogout();
        
        // Close dropdown menu (should be called before state changes)
        void closeDropdown();
        
        // Check if logout was requested
        static bool isLogoutRequested();
        
        // Clear logout flag after handling
        static void clearLogoutFlag();
        
    protected:
        // Helper to extract first name (last word) from full name
        static string getFirstName(const string& fullName);
        
        // Update vị trí dropdown ngay bên dưới nút "Xin chào"
        void updateDropdownPosition();
};