#include "UI/screens/BaseScreen.h"

// Định nghĩa static variables
string BaseScreen::loggedInUsername = "";
string BaseScreen::loggedInUserEmail = "";
bool BaseScreen::logoutRequested = false;

BaseScreen::BaseScreen(Font& f)
	:   font(f),
		background_tex("../assets/elements/background.png"),
		background_sprite(background_tex),
		searchBar_tex("../assets/elements/search_bar.png"),
		searchBar_sprite(searchBar_tex),
		accountButton(f, L"Thông tin cá nhân", 18, {0.f, 0.f}),
		logoutButton(f, L"Đăng xuất", 18, {0.f, 0.f})
{
	buttons.push_back(TextButton(font, L"CiNeXíNè", 50, {90.f, 40.f}));
	buttons.push_back(TextButton(font, L"Đặt vé ngay", 23, {1150.f, 50.f}));
	buttons.push_back(TextButton(font, L"Đăng nhập | Đăng ký", 22, {1360.f, 50.f}));
	for (int i = 0; i < buttons.getSize(); i++) {
		buttons[i].setOutlineThickness(i == 0 ? 5.f : 2.f);
		buttons[i].setOutlineColor(Color(20, 118, 172));
	}
	searchBar_sprite.setScale({0.2f, 0.2f});
	searchBar_sprite.setPosition({700.f, 50.f});

	// Setup dropdown box
	dropdownBox.setSize({250.f, 100.f});
	dropdownBox.setFillColor(Color(40, 40, 40, 240));
	dropdownBox.setOutlineColor(Color(100, 100, 100));
	dropdownBox.setOutlineThickness(1.f);

	// Setup dropdown buttons (styled)
	accountButton.setOutlineThickness(0.f);
	logoutButton.setOutlineThickness(0.f);

	// Initialize global search bar
	FloatRect searchBarBounds = searchBar_sprite.getGlobalBounds();
	float searchBoxX = searchBarBounds.position.x + 40.f;
	float searchBoxY = searchBarBounds.position.y + 8.f;
	float searchBoxWidth = searchBarBounds.size.x - 50.f;

	globalSearchBar = make_unique<GlobalSearchBar>(font, Vector2f(searchBoxX, searchBoxY), Vector2f(searchBoxWidth, 40.f));
}

void BaseScreen::update(Vector2f mousePos, bool mousePressed, AppState& state) {
	// Handle global search bar updates
	if (globalSearchEnabled && globalSearchBar) {
		globalSearchBar->update(mousePos, mousePressed);

		// Check if a movie was selected from search
		int movieIdx;
		if (globalSearchBar->hasSelectedMovie(movieIdx)) {
			selectedMovieIndexFromSearch = movieIdx;
			state = AppState::MOVIE_DETAILS;
			return;
		}

		// Don't process other UI if search box is active
		if (globalSearchBar->isInputActive()) {
			return;
		}
	}

	// Tự động cập nhật text nút đăng nhập dựa vào trạng thái
	if (isUserLoggedIn()) {
		// Extract first name (last word in Vietnamese full name)
		string firstName = getFirstName(loggedInUsername);
		buttons[2].setString(L"Xin chào, " + String::fromUtf8(firstName.begin(), firstName.end()) + L"!");
	}
	else
		buttons[2].setString(L"Đăng nhập | Đăng ký");

	// Xử lý dropdown menu nếu user đã đăng nhập
	if (isUserLoggedIn() && showDropdown) {
		updateDropdownPosition();
		accountButton.update(mousePos);
		logoutButton.update(mousePos);

		if (mousePressed) {
			if (accountButton.isClicked(mousePos, mousePressed)) {
				state = AppState::ACCOUNT;
				showDropdown = false;
				return;
			}
			else if (logoutButton.isClicked(mousePos, mousePressed)) {
				handleLogout();
				return;
			}
			// Click bên ngoài dropdown -> đóng dropdown
			else if (!dropdownBox.getGlobalBounds().contains(mousePos) && !buttons[2].getGlobalBounds().contains(mousePos)) {
				showDropdown = false;
			}
		}
	}

	for (int i = 0; i < buttons.getSize(); i++) {
		buttons[i].update(mousePos);

		// Handle button clicks
		if (buttons[i].isClicked(mousePos, mousePressed)) {
			switch (i) {
				case 0:  // "CiNeXíNè" logo - go to HOME
					state = AppState::HOME;
					break;
				case 1:  // "Đặt vé ngay" - go to BOOKING
					if (state != AppState::BOOKING) {
						state = AppState::BOOKING;
					}
					break;
				case 2:  // "Đăng nhập | Đăng ký" hoặc "Xin chào, ..."
					if (isUserLoggedIn()) {
						// Toggle dropdown menu
						showDropdown = !showDropdown;
						if (showDropdown) {
							updateDropdownPosition();
						}
					} else {
						state = AppState::LOGIN;
					}
					break;
			}
		}
	}
}

void BaseScreen::handleEvent(const Event& event) {
	if (globalSearchEnabled && globalSearchBar) {
		globalSearchBar->handleEvent(event);
	}
}

void BaseScreen::draw(RenderWindow& window) {
	window.draw(background_sprite);
	if (globalSearchEnabled) {
		window.draw(searchBar_sprite);
	}
	for (int i = 0; i < buttons.getSize(); i++)
		buttons[i].draw(window);

}

void BaseScreen::drawOverlay(RenderWindow& window) {
	// Draw dropdown menu first (below search bar)
	if (isUserLoggedIn() && showDropdown) {
		window.draw(dropdownBox);
		accountButton.draw(window);
		logoutButton.draw(window);
	}

	// Draw global search bar and suggestions on top of all content
	if (globalSearchEnabled && globalSearchBar) {
		globalSearchBar->draw(window);
	}
}

void BaseScreen::setAccountButtonText(const String& text) {
	buttons[2].setString(text);
}

void BaseScreen::initializeGlobalSearch(const DLL<MovieDetail>& movies) {
	if (!globalSearchEnabled) {
		return;
	}
	if (!globalSearchManager) {
		globalSearchManager = make_unique<MovieSearchManager>();
	}
	globalSearchManager->loadMovies(movies);
	if (globalSearchBar) {
		globalSearchBar->setSearchManager(globalSearchManager.get());
	}
}

void BaseScreen::setGlobalSearchEnabled(bool enabled) {
	globalSearchEnabled = enabled;
	selectedMovieIndexFromSearch = -1;

	if (!globalSearchEnabled) {
		if (globalSearchBar) {
			globalSearchBar->clear();
		}
		globalSearchManager.reset();
	}
}

int BaseScreen::getSelectedMovieIndexFromSearch() const {
	return selectedMovieIndexFromSearch;
}

void BaseScreen::clearSelectedMovieIndexFromSearch() {
	selectedMovieIndexFromSearch = -1;
}

void BaseScreen::setLoggedInUser(const string& username, const string& email) {
	loggedInUsername = username;
	loggedInUserEmail = email;
}

string BaseScreen::getLoggedInUser() {
	return loggedInUsername;
}

string BaseScreen::getLoggedInUserEmail() {
	return loggedInUserEmail;
}

bool BaseScreen::isUserLoggedIn() {
	return !loggedInUsername.empty();
}

void BaseScreen::handleLogout() {
	loggedInUsername = "";
	loggedInUserEmail = "";
	logoutRequested = true;
}

void BaseScreen::closeDropdown() {
	showDropdown = false;
}

bool BaseScreen::isLogoutRequested() {
	return logoutRequested;
}

void BaseScreen::clearLogoutFlag() {
	logoutRequested = false;
}

string BaseScreen::getFirstName(const string& fullName) {
	if (fullName.empty()) return fullName;

	// Find the last space to get the first name (Vietnamese naming: last word is first name)
	size_t lastSpace = fullName.find_last_of(" ");
	if (lastSpace != string::npos && lastSpace < fullName.length() - 1) {
		return fullName.substr(lastSpace + 1);
	}

	// If no space found, return the full name
	return fullName;
}

void BaseScreen::updateDropdownPosition() {
	FloatRect loginButtonBounds = buttons[2].getGlobalBounds();
	float dropdownX = loginButtonBounds.position.x + loginButtonBounds.size.x - dropdownBox.getSize().x;
	float dropdownY = loginButtonBounds.position.y + loginButtonBounds.size.y + 10.f;

	dropdownBox.setPosition({dropdownX, dropdownY});
	accountButton.setPosition({dropdownX + 15.f, dropdownY + 15.f});
	logoutButton.setPosition({dropdownX + 15.f, dropdownY + 55.f});
}
