#include "UI/components/Admin/UserPanel.h"
#include "UI/components/Admin/FontManager.h"
#include "UI/components/Admin/RoundedRectRenderer.h"
#include "UI/components/Admin/UITheme.h"
#include "utils/FileUtils.h"
#include "utils/Validator.h"
#include "repositories/admin/AdminTicketRepository.h"
#include <unordered_map>

#include <algorithm>
#include <array>
#include <cmath>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#endif

using namespace sf;
using namespace std;

static wstring s2ws(const string& s) {
    if (s.empty()) return L"";
    auto sfStr = String::fromUtf8(s.begin(), s.end());
    return sfStr.toWideString();
}


// Period filter: whole current year + current month and the 3 previous months (derived from today's date).
static void periodMonth(int back, int& month, int& year) {
    time_t now = time(nullptr);
    tm ltm{};
#if defined(_WIN32)
    localtime_s(&ltm, &now);
#else
    localtime_r(&now, &ltm);
#endif
    month = ltm.tm_mon + 1 - back;
    year = ltm.tm_year + 1900;
    while (month < 1) { month += 12; --year; }
}

static array<string, 5> dynamicTimePatterns() {
    array<string, 5> out;
    out[0] = "";
    for (int i = 0; i < 4; ++i) {
        int m, y;
        periodMonth(i, m, y);
        char buf[16];
        snprintf(buf, sizeof(buf), "/%02d/%04d", m, y);
        out[i + 1] = buf;
    }
    return out;
}

static array<wstring, 5> dynamicTimeLabels() {
    array<wstring, 5> out;
    out[0] = L"Thời gian: Tất cả";
    for (int i = 0; i < 4; ++i) {
        int m, y;
        periodMonth(i, m, y);
        wchar_t buf[48];
        swprintf(buf, 48, L"Kỳ: Tháng %02d/%04d", m, y);
        out[i + 1] = buf;
    }
    return out;
}
static string formatCurrency(long long amount) {
    string s = to_string(amount);
    string res = "";
    int cnt = 0;
    for (int i = (int)s.size() - 1; i >= 0; --i) {
        res += s[i];
        cnt++;
        if (cnt == 3 && i > 0) {
            res += '.';
            cnt = 0;
        }
    }
    reverse(res.begin(), res.end());
    return res;
}

UserPanel::UserPanel(Font& fontRef, float w, float h)
    : font(fontRef),
      boldFont(FontManager::instance().bold()),
      width(w),
      height(h),
      position(280.f, 0.f),
      repository(make_unique<UserRepository>(FileUtils::resolveDataPath("data/users.txt")))
{
    loadData();
    applyFilter();
}

void UserPanel::setPosition(const Vector2f& pos) {
    position = pos;
    width = 1728.f - pos.x;
}

void UserPanel::loadData() {
    allCustomers.clear();
    const auto users = repository->getAllUsers();

    const array<Color, 6> avatarColors = {
        Color(219, 234, 254), // blue
        Color(220, 252, 231), // green
        Color(254, 226, 226), // red
        Color(237, 233, 254), // purple
        Color(254, 243, 199), // amber
        Color(207, 250, 254)  // cyan
    };

    // Aggregate real ticket data per customer email (number of bookings, total paid)
    unordered_map<string, pair<int, long long>> stats;
    {
        AdminTicketRepository ticketRepo(FileUtils::resolveDataPath("data/tickets.txt"));
        DLL<Ticket> tickets = ticketRepo.loadAll();
        for (int i = 0; i < tickets.getSize(); ++i) {
            string key = tickets[i].email;
            transform(key.begin(), key.end(), key.begin(), ::tolower);
            auto& s = stats[key];
            s.first += 1;
            s.second += tickets[i].price;
        }
    }

    int colorIdx = 0;
    for (const auto& u : users) {
        if (u.getRole() != AppRole::Customer) continue;

        CustomerItem item;
        item.email = u.getEmail();
        item.fullName = u.getFullName();
        item.displayFullName = s2ws(u.getFullName());
        item.phone = u.getPhone();
        item.birthDate = u.getBirthDate();
        item.registeredAt = u.getRegisteredAt();

        // Format registeredDate from registeredAt (timestamp)
        time_t reg = u.getRegisteredAt();
        if (reg > 0) {
            tm ltm{};
#if defined(_WIN32)
            localtime_s(&ltm, &reg);
#else
            localtime_r(&reg, &ltm);
#endif
            char buf[32];
            strftime(buf, sizeof(buf), "%d/%m/%Y", &ltm);
            item.registeredDate = buf;
        } else {
            item.registeredDate = "";
        }

        // Generate clean UID
        char uidBuf[32];
        unsigned long uh = 5381; for (unsigned char ch : u.getEmail()) uh = uh * 33 + ch;
        snprintf(uidBuf, sizeof(uidBuf), "CXN-%06lu", uh % 1000000UL);
        item.uid = uidBuf;

        // Tickets & tiers: derived from real ticket records (tickets.txt)
        {
            string key = u.getEmail();
            transform(key.begin(), key.end(), key.begin(), ::tolower);
            auto it = stats.find(key);
            int tk = it != stats.end() ? it->second.first : 0;
            item.ticketsBought = tk;
            item.totalSpent = it != stats.end() ? it->second.second : 0;
            if (tk >= 7) item.tier = CustomerTier::GOLD;
            else if (tk >= 4) item.tier = CustomerTier::SILVER;
            else item.tier = CustomerTier::STANDARD;
        }
        item.avatarBg = avatarColors[colorIdx % avatarColors.size()];
        colorIdx++;

        // Generate initials
        stringstream ss(item.fullName);
        string word;
        vector<string> words;
        while (ss >> word) words.push_back(word);
        wstring inStr = L"KH";
        if (words.size() >= 2) {
            auto wFirst = s2ws(words[0]);
            auto wLast = s2ws(words.back());
            if (!wFirst.empty() && !wLast.empty()) {
                inStr = wstring(1, towupper(wFirst[0])) + wstring(1, towupper(wLast[0]));
            }
        }
        item.initials = inStr;
        allCustomers.push_back(item);
    }

    calculateCustomerStats();
}

void UserPanel::calculateCustomerStats() {
    totalCount = (int)allCustomers.size();
    activeCount = 0;
    goldCount = 0;
    silverCount = 0;
    standardCount = 0;
    long long totalSp = 0;
    int totalTk = 0;

    for (const auto& c : allCustomers) {
        if (c.ticketsBought > 0) activeCount++;
        if (c.tier == CustomerTier::GOLD) goldCount++;
        else if (c.tier == CustomerTier::SILVER) silverCount++;
        else standardCount++;

        totalSp += c.totalSpent;
        totalTk += c.ticketsBought;
    }

    avgSpending = totalCount > 0 ? totalSp / totalCount : 0;
    avgTicketsPerUser = totalCount > 0 ? (float)totalTk / (float)totalCount : 0.f;
}

void UserPanel::applyFilter() {
    filteredCustomers.clear();
    wstring wSearch = s2ws(searchText);

    const array<string, 5> timePatterns = dynamicTimePatterns();
    string activeTimePattern = timePatterns[timeFilterIndex % 5];

    for (auto& c : allCustomers) {
        // Tier filter
        if (tierFilterIndex == 1 && c.tier != CustomerTier::GOLD) continue;
        if (tierFilterIndex == 2 && c.tier != CustomerTier::SILVER) continue;
        if (tierFilterIndex == 3 && c.tier != CustomerTier::STANDARD) continue;

        // Time filter
        if (!activeTimePattern.empty() && c.registeredDate.find(activeTimePattern) == string::npos) {
            continue;
        }

        // Search text filter (hỗ trợ tiếng Việt có dấu & không dấu)
        if (!wSearch.empty()) {
            bool match = UITheme::matchesViQuery(c.displayFullName, wSearch) ||
                         UITheme::matchesViQuery(s2ws(c.email), wSearch) ||
                         UITheme::matchesViQuery(s2ws(c.phone), wSearch) ||
                         UITheme::matchesViQuery(s2ws(c.uid), wSearch);
            if (!match) continue;
        }

        filteredCustomers.push_back(&c);
    }

    page = 0;
    if (!wSearch.empty() && !filteredCustomers.empty()) {
        selectedIndex = 0;
    } else if (selectedIndex >= (int)filteredCustomers.size()) {
        selectedIndex = -1;
    }
}

void UserPanel::refreshData() {
    searchText.clear();
    tierFilterIndex = 0;
    timeFilterIndex = 0;
    loadData();
    applyFilter();
    toastActive = true;
    toastTimer = 2.5f;
    toastMessage = L"Đã làm mới danh sách khách hàng & hạng thẻ!";
}

void UserPanel::handleEvent(const Event& event, const RenderWindow& window) {
    // 1. ƯU TIÊN XỬ LÝ DIALOG (VIEW / EDIT / DELETE_CONFIRM / ADD_NEW)
    if (activeDialog != DialogType::NONE) {
        float dW = 480.f;
        float dH = 350.f;
        if (activeDialog == DialogType::VIEW) {
            dW = 520.f;
            dH = 370.f;
        } else if (activeDialog == DialogType::DELETE_CONFIRM) {
            dW = 500.f;
            dH = 210.f;
        }
        Vector2f dPos(position.x + (width - dW) / 2.f, position.y + (height - dH) / 2.f);

        auto commitDialogAction = [&]() {
            if (activeDialog == DialogType::VIEW) {
                activeDialog = DialogType::NONE;
                return;
            }
            if (activeDialog == DialogType::DELETE_CONFIRM && dialogUserIndex >= 0 && dialogUserIndex < (int)filteredCustomers.size()) {
                string delEmail = filteredCustomers[dialogUserIndex]->email;
                toastActive = true;
                toastTimer = 3.f;
                // Tickets keep their own copy of customer name/email, so revenue history is unaffected.
                if (repository->deleteUser(delEmail)) {
                    allCustomers.erase(remove_if(allCustomers.begin(), allCustomers.end(),
                        [&](const CustomerItem& it) { return it.email == delEmail; }), allCustomers.end());
                    applyFilter();
                    calculateCustomerStats();
                    toastMessage = L"Đã xóa tài khoản hội viên (lịch sử vé được giữ nguyên)!";
                } else {
                    toastMessage = L"Không thể xóa tài khoản (lỗi ghi dữ liệu)!";
                }
            } else if (activeDialog == DialogType::ADD_NEW) {
                // Validate and PERSIST a real customer account (no placeholder data).
                auto fail = [&](const wchar_t* msg) {
                    toastActive = true; toastTimer = 3.f; toastMessage = msg;
                };
                if (inputName.empty() || !Validator::isValidFullName(inputName)) { fail(L"Họ tên không hợp lệ (không để trống, không chứa số)!"); return; }
                if (!Validator::isValidEmail(inputEmail)) { fail(L"Email không hợp lệ!"); return; }
                if (repository->exists(inputEmail)) { fail(L"Email đã tồn tại!"); return; }
                if (!inputPhone.empty() && !Validator::isValidPhone(inputPhone)) { fail(L"Số điện thoại không hợp lệ (10 số nhà mạng VN)!"); return; }
                // Hash that can never match any password: the customer sets one via password reset.
                User created(inputEmail, "!unset", inputName, "", inputPhone, time(nullptr), AppRole::Customer);
                if (!repository->addUser(created)) { fail(L"Không thể lưu hội viên (lỗi ghi dữ liệu)!"); return; }
                loadData();
                applyFilter();
                toastActive = true;
                toastTimer = 3.f;
                toastMessage = L"Đã thêm hội viên mới thành công!";
            } else if (activeDialog == DialogType::EDIT && dialogUserIndex >= 0 && dialogUserIndex < (int)filteredCustomers.size()) {
                CustomerItem* target = filteredCustomers[dialogUserIndex];
                toastActive = true;
                toastTimer = 3.f;
                User* stored = repository->findByEmail(target->email);
                if (!stored) {
                    toastMessage = L"Không tìm thấy tài khoản!";
                } else if (!inputName.empty() && !Validator::isValidFullName(inputName)) {
                    toastMessage = L"Họ tên không hợp lệ (không chứa số)!";
                } else if (!inputPhone.empty() && !Validator::isValidPhone(inputPhone)) {
                    toastMessage = L"Số điện thoại không hợp lệ (10 số nhà mạng VN)!";
                } else if (!inputEmail.empty() && inputEmail != target->email) {
                    toastMessage = L"Email là khóa tài khoản, không thể thay đổi!";
                } else {
                    User updated = *stored;
                    if (!inputName.empty()) updated.setFullName(inputName);
                    updated.setPhone(inputPhone);
                    if (repository->updateUser(updated)) {
                        loadData();
                        applyFilter();
                        toastMessage = L"Đã cập nhật hồ sơ khách hàng thành công!";
                    } else {
                        toastMessage = L"Không thể lưu thay đổi (lỗi ghi dữ liệu)!";
                    }
                }
            }
            activeDialog = DialogType::NONE;
        };

        if (auto mousePress = event.getIf<Event::MouseButtonPressed>()) {
            if (mousePress->button == Mouse::Button::Left) {
                Vector2f mPos = window.mapPixelToCoords(Mouse::getPosition(window));

                if (activeDialog == DialogType::VIEW) {
                    FloatRect closeBtn({dPos.x + dW - 130.f, dPos.y + dH - 50.f}, {105.f, 36.f});
                    if (closeBtn.contains(mPos) || !FloatRect(dPos, {dW, dH}).contains(mPos)) {
                        activeDialog = DialogType::NONE;
                    }
                    return;
                }

                if (activeDialog == DialogType::ADD_NEW || activeDialog == DialogType::EDIT) {
                    float inY = dPos.y + 65.f;
                    for (int f = 0; f < 3; ++f) {
                        FloatRect boxR({dPos.x + 24.f, inY + 22.f + f * 64.f}, {dW - 48.f, 36.f});
                        if (boxR.contains(mPos)) {
                            activeInputField = f;
                            return;
                        }
                    }
                }

                FloatRect cancelRect({dPos.x + dW - 230.f, dPos.y + dH - 50.f}, {95.f, 36.f});
                if (cancelRect.contains(mPos) || !FloatRect(dPos, {dW, dH}).contains(mPos)) {
                    activeDialog = DialogType::NONE;
                    return;
                }

                FloatRect confirmRect({dPos.x + dW - 120.f, dPos.y + dH - 50.f}, {95.f, 36.f});
                if (confirmRect.contains(mPos)) {
                    commitDialogAction();
                    return;
                }
            }
            return;
        }

        if (auto keyEvt = event.getIf<Event::KeyPressed>()) {
            if (keyEvt->code == Keyboard::Key::Escape) {
                activeDialog = DialogType::NONE;
                return;
            }
            if (keyEvt->code == Keyboard::Key::Enter) {
                commitDialogAction();
                return;
            }
            if (keyEvt->code == Keyboard::Key::Tab && (activeDialog == DialogType::ADD_NEW || activeDialog == DialogType::EDIT)) {
                activeInputField = (activeInputField + 1) % 3;
                return;
            }
            if (keyEvt->code == Keyboard::Key::Backspace && (activeDialog == DialogType::ADD_NEW || activeDialog == DialogType::EDIT)) {
                auto popUtf8 = [](string& s) {
                    if (s.empty()) return;
                    wstring ws = s2ws(s);
                    if (!ws.empty()) ws.pop_back();
                    s = UITheme::wstrToUtf8(ws);
                };
                if (activeInputField == 0) popUtf8(inputName);
                else if (activeInputField == 1) popUtf8(inputEmail);
                else if (activeInputField == 2) popUtf8(inputPhone);
                return;
            }
        }

        if (auto textEvt = event.getIf<Event::TextEntered>()) {
            if (activeDialog == DialogType::ADD_NEW || activeDialog == DialogType::EDIT) {
                char32_t u = textEvt->unicode;
                if (u >= 32 && u != 127) {
                    if (activeInputField == 0) {
                        wstring ws = s2ws(inputName);
                        if (ws.size() < 32) { ws += static_cast<wchar_t>(u); inputName = UITheme::wstrToUtf8(ws); }
                    } else if (activeInputField == 1 && u < 127 && inputEmail.size() < 35) {
                        inputEmail.push_back(static_cast<char>(u));
                    } else if (activeInputField == 2 && u >= '0' && u <= '9' && inputPhone.size() < 12) {
                        inputPhone.push_back(static_cast<char>(u));
                    }
                }
            }
        }
        return;
    }

    // 2. XỬ LÝ BÀN PHÍM CHO Ô TÌM KIẾM KHÁCH HÀNG
    if (searchActive) {
        if (auto keyEvt = event.getIf<Event::KeyPressed>()) {
            if (keyEvt->code == Keyboard::Key::Escape) {
                searchText.clear();
                searchActive = false;
                applyFilter();
                return;
            }
            if (keyEvt->code == Keyboard::Key::Backspace) {
                if (!searchText.empty()) {
                    wstring ws = s2ws(searchText);
                    if (!ws.empty()) ws.pop_back();
                    searchText = UITheme::wstrToUtf8(ws);
                    applyFilter();
                }
                return;
            }
        }
        if (auto textEvt = event.getIf<Event::TextEntered>()) {
            char32_t u = textEvt->unicode;
            if (u >= 32 && u != 127) {
                wstring ws = s2ws(searchText);
                if (ws.size() < 30) {
                    ws += static_cast<wchar_t>(u);
                    searchText = UITheme::wstrToUtf8(ws);
                    applyFilter();
                }
            }
            return;
        }
    }

    // 3. XỬ LÝ CLICK CHUỘT TRÊN MÀN HÌNH CHÍNH
    if (auto mousePress = event.getIf<Event::MouseButtonPressed>()) {
        if (mousePress->button == Mouse::Button::Left) {
            Vector2f mPos = window.mapPixelToCoords(Mouse::getPosition(window));

            // 1. Header Buttons (khớp drawHeader: cardY = position.y + 14.f, btnY = position.y + 37.f)
            FloatRect expRect({position.x + width - 308.f, position.y + 37.f}, {105.f, 38.f});
            if (expRect.contains(mPos)) {
                string outPath = FileUtils::resolveDataPath("data/khach_hang_report.csv");
                ofstream out(outPath, ios::binary);
                if (!out.is_open()) {
                    outPath = "khach_hang_report.csv";
                    out.open(outPath, ios::binary);
                }
                if (out.is_open()) {
                    const unsigned char bom[] = {0xEF, 0xBB, 0xBF};
                    out.write(reinterpret_cast<const char*>(bom), sizeof(bom));
                    out << "DANH SÁCH KHÁCH HÀNG & HỘI VIÊN RẠP CINECORE\n\n";
                    out << "STT,Mã khách hàng,Họ và tên,Email,Số điện thoại,Hạng thẻ,Ngày đăng ký,Số vé đã mua,Tổng chi tiêu (VNĐ)\n";
                    for (size_t i = 0; i < allCustomers.size(); ++i) {
                        string tName = (allCustomers[i].tier == CustomerTier::GOLD) ? "Hạng Vàng" :
                                       (allCustomers[i].tier == CustomerTier::SILVER ? "Hạng Bạc" : "Tiêu chuẩn");
                        out << (i + 1) << "," << allCustomers[i].uid << ",\"" << allCustomers[i].fullName << "\","
                            << allCustomers[i].email << ",\"" << allCustomers[i].phone << "\",\""
                            << tName << "\"," << allCustomers[i].registeredDate << ","
                            << allCustomers[i].ticketsBought << "," << allCustomers[i].totalSpent << "\n";
                    }
                    out.close();

#ifdef _WIN32
                    ShellExecuteA(NULL, "open", outPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
#else
                    system(("start \"\" \"" + outPath + "\"").c_str());
#endif
                }

                toastActive = true;
                toastTimer = 3.f;
                toastMessage = L"Đã xuất dữ liệu và mở file danh sách khách hàng trong Excel!";
                return;
            }

            FloatRect addRect({position.x + width - 193.f, position.y + 37.f}, {155.f, 38.f});
            if (addRect.contains(mPos)) {
                activeDialog = DialogType::ADD_NEW;
                inputName = "";
                inputEmail = "";
                inputPhone = "";
                activeInputField = 0;
                return;
            }

            // 2. Toolbar Action Buttons (khớp drawActionToolbar: actY = position.y + 226.f)
            float actX = position.x + 20.f;
            float actY = position.y + 226.f;

            // Ô tìm kiếm khách hàng
            FloatRect searchRect({position.x + width - 620.f, actY}, {220.f, 34.f});
            if (searchRect.contains(mPos)) {
                if (!searchText.empty() && mPos.x >= position.x + width - 425.f) {
                    searchText.clear();
                    applyFilter();
                }
                searchActive = true;
                return;
            } else {
                searchActive = false;
            }

            // Xem thông tin
            FloatRect viewRect({actX, actY}, {125.f, 34.f});
            if (viewRect.contains(mPos)) {
                if (selectedIndex >= 0 && selectedIndex < (int)filteredCustomers.size()) {
                    activeDialog = DialogType::VIEW;
                    dialogUserIndex = selectedIndex;
                } else {
                    toastActive = true;
                    toastTimer = 2.5f;
                    toastMessage = L"Vui lòng chọn một khách hàng trong bảng để xem thông tin!";
                }
                return;
            }

            // Xóa tài khoản
            FloatRect delRect({actX + 135.f, actY}, {115.f, 34.f});
            if (delRect.contains(mPos)) {
                if (selectedIndex >= 0 && selectedIndex < (int)filteredCustomers.size()) {
                    activeDialog = DialogType::DELETE_CONFIRM;
                    dialogUserIndex = selectedIndex;
                } else {
                    toastActive = true;
                    toastTimer = 2.5f;
                    toastMessage = L"Vui lòng chọn tài khoản khách hàng cần xóa!";
                }
                return;
            }

            // Đặt lại mật khẩu
            FloatRect pwdRect({actX + 260.f, actY}, {135.f, 34.f});
            if (pwdRect.contains(mPos)) {
                if (selectedIndex >= 0 && selectedIndex < (int)filteredCustomers.size()) {
                    toastActive = true;
                    toastTimer = 3.f;
                    toastMessage = L"Đã gửi email khôi phục mật khẩu mặc định tới khách hàng!";
                } else {
                    toastActive = true;
                    toastTimer = 2.5f;
                    toastMessage = L"Vui lòng chọn khách hàng cần đặt lại mật khẩu!";
                }
                return;
            }

            // Tier dropdown filter
            FloatRect tierRect({position.x + width - 390.f, actY}, {135.f, 34.f});
            if (tierRect.contains(mPos)) {
                tierFilterIndex = (tierFilterIndex + 1) % 4;
                applyFilter();
                return;
            }

            // Time filter dropdown
            FloatRect timeRect({position.x + width - 245.f, actY}, {180.f, 34.f});
            if (timeRect.contains(mPos)) {
                timeFilterIndex = (timeFilterIndex + 1) % 5;
                applyFilter();
                const array<wstring, 5> timeLabels = dynamicTimeLabels();
                toastActive = true;
                toastTimer = 2.f;
                toastMessage = L"Đã lọc: " + timeLabels[timeFilterIndex % 5];
                return;
            }

            // Reload button on toolbar
            FloatRect relToolbarRect({position.x + width - 56.f, actY}, {36.f, 34.f});
            if (relToolbarRect.contains(mPos)) {
                refreshData();
                return;
            }

            // 3. Table Rows click & row action icons
            float tTop = position.y + 274.f + 44.f;
            int startIdx = page * rowsPerPage;
            int endIdx = min((int)filteredCustomers.size(), startIdx + rowsPerPage);

            for (int i = startIdx; i < endIdx; ++i) {
                int rowIdx = i - startIdx;
                float ry = tTop + rowIdx * 52.f;

                FloatRect rRect({position.x + 20.f, ry}, {width - 40.f, 50.f});
                if (rRect.contains(mPos)) {
                    selectedIndex = i;
                    filteredCustomers[i]->selected = !filteredCustomers[i]->selected;

                    float col_act = position.x + 20.f + (width - 40.f) - 144.f;
                    if (FloatRect({col_act, ry + 12.f}, {40.f, 28.f}).contains(mPos)) {
                        activeDialog = DialogType::VIEW;
                        dialogUserIndex = i;
                        return;
                    }
                    if (FloatRect({col_act + 46.f, ry + 12.f}, {40.f, 28.f}).contains(mPos)) {
                        activeDialog = DialogType::EDIT;
                        dialogUserIndex = i;
                        activeInputField = 0;
                        inputName = filteredCustomers[i]->fullName;
                        inputEmail = filteredCustomers[i]->email;
                        inputPhone = filteredCustomers[i]->phone;
                        return;
                    }
                    if (FloatRect({col_act + 92.f, ry + 12.f}, {40.f, 28.f}).contains(mPos)) {
                        activeDialog = DialogType::DELETE_CONFIRM;
                        dialogUserIndex = i;
                        return;
                    }
                    return;
                }
            }

            // Pagination buttons
            int totalPages = max(1, (static_cast<int>(filteredCustomers.size()) + rowsPerPage - 1) / rowsPerPage);
            float pageY = position.y + 274.f + 44.f + rowsPerPage * 52.f + 12.f;
            float tableW = width - 40.f;
            float rightEdge = position.x + 20.f + tableW;

            int numPageBtns = min(5, totalPages);
            int startP = max(0, min(page - 2, totalPages - numPageBtns));
            int endP = min(totalPages - 1, startP + numPageBtns - 1);

            float btnPrevW = 32.f;
            float btnNextW = 32.f;
            float btnPageW = 32.f;
            float gap = 6.f;
            float totalBtnsW = btnPrevW + gap + (endP - startP + 1) * (btnPageW + gap) - gap + gap + btnNextW;
            float curBtnX = rightEdge - totalBtnsW;

            // Nút Prev <
            FloatRect prevRect({curBtnX, pageY}, {btnPrevW, 30.f});
            if (prevRect.contains(mPos) && page > 0) {
                page--;
                return;
            }
            curBtnX += btnPrevW + gap;

            // Nút các trang
            for (int p = startP; p <= endP; ++p) {
                FloatRect btnP({curBtnX, pageY}, {btnPageW, 30.f});
                if (btnP.contains(mPos)) {
                    page = p;
                    return;
                }
                curBtnX += btnPageW + gap;
            }

            // Nút Next >
            FloatRect nextRect({curBtnX, pageY}, {btnNextW, 30.f});
            if (nextRect.contains(mPos) && page < totalPages - 1) {
                page++;
                return;
            }
        }
    }
}

void UserPanel::update(const Vector2f& mousePos, bool /*mouseDown*/) {
    // Check row hover
    hoveredRow = -1;
    float tTop = position.y + 274.f + 44.f;
    int startIdx = page * rowsPerPage;
    int endIdx = min((int)filteredCustomers.size(), startIdx + rowsPerPage);

    for (int i = startIdx; i < endIdx; ++i) {
        int rowIdx = i - startIdx;
        float ry = tTop + rowIdx * 52.f;
        FloatRect rRect({position.x + 20.f, ry}, {width - 40.f, 50.f});
        if (rRect.contains(mousePos)) {
            hoveredRow = i;
            break;
        }
    }

    if (toastActive) {
        toastTimer -= 0.016f;
        if (toastTimer <= 0.f) toastActive = false;
    }
}

void UserPanel::render(RenderTarget& target) const {
    // 1. Canvas Background
    RectangleShape bg(Vector2f(width, height));
    bg.setPosition(position);
    bg.setFillColor(Color(241, 245, 249));
    target.draw(bg);

    // 2. Header
    drawHeader(target);

    // 3. 4 KPI Metric Cards
    drawKpiCards(target);

    // 4. Action Toolbar & Filters
    drawActionToolbar(target);

    // 5. Customer Table
    drawCustomerTable(target);

    // 6. Bottom Status Bar
    drawBottomBar(target);

    // 7. Dialog / Modal
    if (activeDialog != DialogType::NONE) {
        drawDialog(target);
    }

    // 8. Toast Notification
    if (toastActive) {
        drawToast(target);
    }
}

void UserPanel::drawHeader(RenderTarget& target) const {
    int totalCount = static_cast<int>(allCustomers.size());

    UITheme::drawSectionHeaderCard(target, position.x + 20.f, position.y + 14.f, width - 40.f,
                                   L"Quản lý khách hàng",
                                   L"Tổng " + to_wstring(totalCount) + L" hội viên",
                                   Color(224, 242, 254), Color(8, 125, 181),
                                   L"Tra cứu, quản lý hồ sơ tài khoản khách hàng, lịch sử vé và cấp bậc thẻ thành viên.",
                                   boldFont, font, false);

    // Right action buttons inside header card:
    // 1. Xuất Excel
    Vector2f expPos(position.x + width - 308.f, position.y + 37.f);
    RoundedRectRenderer::draw(target, expPos, {105.f, 38.f}, 8.f, Color::White, 1.f, Color(203, 213, 225));
    Text expTxt(boldFont);
    expTxt.setString(L"Xuất Excel");
    expTxt.setCharacterSize(14);
    expTxt.setFillColor(Color(22, 101, 52));
    FloatRect expB = expTxt.getLocalBounds();
    expTxt.setPosition({std::round(expPos.x + (105.f - expB.size.x) / 2.f - expB.position.x), std::round(expPos.y + (38.f - expB.size.y) / 2.f - expB.position.y)});
    target.draw(expTxt);

    // 2. + Thêm hội viên mới
    Vector2f addPos(position.x + width - 193.f, position.y + 37.f);
    RoundedRectRenderer::draw(target, addPos, {155.f, 38.f}, 8.f, Color(8, 125, 181));
    Text addTxt(boldFont);
    addTxt.setString(L"+ Thêm hội viên mới");
    addTxt.setCharacterSize(14);
    addTxt.setFillColor(Color::White);
    FloatRect addB = addTxt.getLocalBounds();
    addTxt.setPosition({std::round(addPos.x + (155.f - addB.size.x) / 2.f - addB.position.x), std::round(addPos.y + (38.f - addB.size.y) / 2.f - addB.position.y)});
    target.draw(addTxt);
}

void UserPanel::drawKpiCards(RenderTarget& target) const {
    const float cardY = position.y + 110.f;
    const float cardH = 88.f;
    const float margin = 20.f;
    const float gap = 12.f;
    const float availableW = width - margin * 2.f;
    const float cardW = (availableW - gap * 3.f) / 4.f; // ~343px

    // Card 1: TỔNG KHÁCH HÀNG
    {
        Vector2f cPos(position.x + margin, cardY);
        RoundedRectRenderer::draw(target, cPos, {cardW, cardH}, 8.f, Color::White, 1.f, Color(226, 232, 240));

        Text lbl(boldFont);
        lbl.setString(L"TỔNG KHÁCH HÀNG");
        lbl.setCharacterSize(14);
        lbl.setFillColor(Color(100, 116, 139));
        lbl.setPosition({cPos.x + 16.f, cPos.y + 14.f});
        target.draw(lbl);

        Text val(boldFont);
        val.setString(to_string(totalCount));
        val.setCharacterSize(24);
        val.setFillColor(Color(23, 32, 51));
        val.setPosition({cPos.x + 16.f, cPos.y + 40.f});
        target.draw(val);

        Text badge(boldFont);
        badge.setString(L"\u2191 +2 tuần này");
        badge.setCharacterSize(17);
        badge.setFillColor(Color(21, 128, 61));
        badge.setPosition({std::round(val.getPosition().x + val.getGlobalBounds().size.x + 10.f), std::round(cPos.y + 45.f)});
        target.draw(badge);
    }

    // Card 2: ĐANG HOẠT ĐỘNG (ACTIVE)
    {
        Vector2f cPos(position.x + margin + (cardW + gap), cardY);
        RoundedRectRenderer::draw(target, cPos, {cardW, cardH}, 8.f, Color::White, 1.f, Color(226, 232, 240));

        Text lbl(boldFont);
        lbl.setString(L"ĐANG HOẠT ĐỘNG (ACTIVE)");
        lbl.setCharacterSize(14);
        lbl.setFillColor(Color(100, 116, 139));
        lbl.setPosition({cPos.x + 16.f, cPos.y + 14.f});
        target.draw(lbl);

        Text val(boldFont);
        val.setString(to_string(activeCount));
        val.setCharacterSize(24);
        val.setFillColor(Color(23, 32, 51));
        val.setPosition({cPos.x + 16.f, cPos.y + 40.f});
        target.draw(val);

        Text unit(boldFont);
        unit.setString(L"/ " + to_wstring(totalCount) + L" tài khoản");
        unit.setCharacterSize(22);
        unit.setFillColor(Color(23, 32, 51));
        unit.setPosition({std::round(val.getPosition().x + val.getGlobalBounds().size.x + 8.f), std::round(cPos.y + 42.f)});
        target.draw(unit);
    }

    // Card 3: HẠNG THẺ HỘI VIÊN
    {
        Vector2f cPos(position.x + margin + 2 * (cardW + gap), cardY);
        RoundedRectRenderer::draw(target, cPos, {cardW, cardH}, 8.f, Color::White, 1.f, Color(226, 232, 240));

        Text lbl(boldFont);
        lbl.setString(L"HẠNG THẺ HỘI VIÊN");
        lbl.setCharacterSize(14);
        lbl.setFillColor(Color(100, 116, 139));
        lbl.setPosition({cPos.x + 16.f, cPos.y + 14.f});
        target.draw(lbl);

        // 3 Badges in a row: 6 Vàng, 7 Bạc, 2 Chuẩn
        float pW = (cardW - 32.f - 16.f) / 3.f;

        auto drawTierPill = [&](int idx, int count, const wstring& name, Color bg, Color txt) {
            Vector2f pPos(cPos.x + 16.f + idx * (pW + 8.f), cPos.y + 42.f);
            RoundedRectRenderer::draw(target, pPos, {pW, 28.f}, 4.f, bg);
            Text pTxt(boldFont);
            pTxt.setString(to_wstring(count) + L" " + name);
            pTxt.setCharacterSize(16);
            pTxt.setFillColor(txt);
            FloatRect pb = pTxt.getLocalBounds();
            float posX = std::round(pPos.x + (pW - pb.size.x) / 2.f - pb.position.x);
            float posY = std::round(pPos.y + (28.f - pb.size.y) / 2.f - pb.position.y);
            if (idx == 2) {
                // "Chuẩn" with diacritic hook on 'ẩ' shifts bounding box; adjust optical center
                posX -= 3.5f;
                posY = std::round(pPos.y + 2.f);
            }
            pTxt.setPosition({posX, posY});
            target.draw(pTxt);
        };

        drawTierPill(0, goldCount, L"Vàng", Color(254, 243, 199), Color(180, 83, 9));
        drawTierPill(1, silverCount, L"Bạc", Color(241, 245, 249), Color(71, 85, 105));
        drawTierPill(2, standardCount, L"Chuẩn", Color(224, 242, 254), Color(8, 125, 181));
    }

    // Card 4: TỔNG CHI TIÊU BÌNH QUÂN
    {
        Vector2f cPos(position.x + margin + 3 * (cardW + gap), cardY);
        RoundedRectRenderer::draw(target, cPos, {cardW, cardH}, 8.f, Color::White, 1.f, Color(226, 232, 240));

        Text lbl(boldFont);
        lbl.setString(L"TỔNG CHI TIÊU BÌNH QUÂN");
        lbl.setCharacterSize(14);
        lbl.setFillColor(Color(100, 116, 139));
        lbl.setPosition({cPos.x + 16.f, cPos.y + 14.f});
        target.draw(lbl);

        Text val(boldFont);
        val.setString(s2ws(formatCurrency(avgSpending)));
        val.setCharacterSize(24);
        val.setFillColor(Color(23, 32, 51));
        val.setPosition({cPos.x + 16.f, cPos.y + 40.f});
        target.draw(val);

        Text unit(boldFont);
        unit.setString(L"VNĐ");
        unit.setCharacterSize(22);
        unit.setFillColor(Color(23, 32, 51));
        unit.setPosition({std::round(val.getPosition().x + val.getGlobalBounds().size.x + 8.f), std::round(cPos.y + 42.f)});
        target.draw(unit);
    }
}

void UserPanel::drawActionToolbar(RenderTarget& target) const {
    float actX = position.x + 20.f;
    float actY = position.y + 226.f;

    // Action buttons:
    // 1. Xem thông tin (blue)
    RoundedRectRenderer::draw(target, {actX, actY}, {125.f, 34.f}, 6.f, Color(8, 125, 181));
    Text vTxt(boldFont); vTxt.setString(L"Xem thông tin"); vTxt.setCharacterSize(14); vTxt.setFillColor(Color::White); vTxt.setPosition({actX + 16.f, actY + 8.f}); target.draw(vTxt);

    // 2. Xóa tài khoản (red)
    RoundedRectRenderer::draw(target, {actX + 135.f, actY}, {115.f, 34.f}, 6.f, Color(220, 38, 38));
    Text dTxt(boldFont); dTxt.setString(L"Xóa tài khoản"); dTxt.setCharacterSize(14); dTxt.setFillColor(Color::White); dTxt.setPosition({actX + 148.f, actY + 8.f}); target.draw(dTxt);

    // 3. Đặt lại mật khẩu (white with border)
    RoundedRectRenderer::draw(target, {actX + 260.f, actY}, {135.f, 34.f}, 6.f, Color::White, 1.f, Color(203, 213, 225));
    Text pTxt(boldFont); pTxt.setString(L"Đặt lại mật khẩu"); pTxt.setCharacterSize(14); pTxt.setFillColor(Color(51, 65, 85)); pTxt.setPosition({actX + 270.f, actY + 8.f}); target.draw(pTxt);

    // Right Filters:
    // 1. Search Box
    Vector2f searchPos(position.x + width - 620.f, actY);
    RoundedRectRenderer::draw(target, searchPos, {220.f, 34.f}, 6.f, Color::White, searchActive ? 1.8f : 1.f, searchActive ? Color(8, 125, 181) : Color(203, 213, 225));

    // Kính lúp tìm kiếm
    Vector2f uIconPos(searchPos.x + 10.f, searchPos.y + 10.f);
    Color uIcCol = searchActive ? Color(8, 125, 181) : Color(148, 163, 184);
    CircleShape uLens(5.f, 16);
    uLens.setFillColor(Color::Transparent);
    uLens.setOutlineThickness(1.6f);
    uLens.setOutlineColor(uIcCol);
    uLens.setPosition(uIconPos);
    target.draw(uLens);

    RectangleShape uStem({5.5f, 1.8f});
    uStem.setFillColor(uIcCol);
    uStem.setRotation(degrees(45.f));
    uStem.setPosition({uIconPos.x + 8.f, uIconPos.y + 8.f});
    target.draw(uStem);

    Text sTxt(font);
    wstring dispS = searchText.empty()
        ? (searchActive ? L"|" : L"Tìm theo Tên, Email, SĐT...")
        : (s2ws(searchText) + (searchActive ? L"|" : L""));
    sTxt.setString(dispS);
    sTxt.setCharacterSize(15);
    sTxt.setFillColor(searchText.empty() && !searchActive ? Color(148, 163, 184) : Color(23, 32, 51));
    sTxt.setPosition({searchPos.x + 28.f, std::round(searchPos.y + 7.f)});
    target.draw(sTxt);

    // 2. Tier Dropdown
    Vector2f tierPos(position.x + width - 390.f, actY);
    RoundedRectRenderer::draw(target, tierPos, {135.f, 34.f}, 6.f, Color::White, 1.f, Color(203, 213, 225));
    const array<wstring, 4> tierLabels = {L"Tất cả hạng thẻ", L"Hạng Vàng", L"Hạng Bạc", L"Tiêu chuẩn"};
    Text tTxt(font);
    tTxt.setString(tierLabels[tierFilterIndex % 4]);
    tTxt.setCharacterSize(15);
    tTxt.setFillColor(Color(51, 65, 85));
    tTxt.setPosition({tierPos.x + 10.f, std::round(tierPos.y + 7.f)});
    target.draw(tTxt);
    UITheme::drawChevronDown(target, {tierPos.x + 123.f, tierPos.y + 17.f}, 4.2f, Color(100, 116, 139));

    // 3. Time Filter Dropdown
    Vector2f timePos(position.x + width - 245.f, actY);
    RoundedRectRenderer::draw(target, timePos, {180.f, 34.f}, 6.f, Color::White, 1.f, Color(203, 213, 225));
    const array<wstring, 5> timeLabels = dynamicTimeLabels();
    Text tmTxt(font);
    tmTxt.setString(timeLabels[timeFilterIndex % 5]);
    tmTxt.setCharacterSize(15);
    tmTxt.setFillColor(Color(51, 65, 85));
    tmTxt.setPosition({timePos.x + 10.f, std::round(timePos.y + 7.f)});
    target.draw(tmTxt);
    UITheme::drawChevronDown(target, {timePos.x + 165.f, timePos.y + 17.f}, 4.2f, Color(100, 116, 139));

    // 4. Reload Button
    Vector2f relPos(position.x + width - 56.f, actY);
    RoundedRectRenderer::draw(target, relPos, {36.f, 34.f}, 6.f, Color::White, 1.f, Color(203, 213, 225));
    UITheme::drawReloadIcon(target, {relPos.x + 18.f, relPos.y + 17.f}, 7.f, Color(71, 85, 105));
}

void UserPanel::drawCustomerTable(RenderTarget& target) const {
    const float tableX = position.x + 20.f;
    const float tableY = position.y + 274.f;
    const float tableW = width - 40.f;
    const float headerH = 44.f;

    // Ocean Blue Header Bar (synchronized with system design)
    RoundedRectRenderer::draw(target, {tableX, tableY}, {tableW, headerH}, 6.f, Color(8, 125, 181));

    // Column divider positions
    float div0 = tableX + 80.f;                          // STT | HỌ VÀ TÊN
    float div1 = tableX + std::floor(tableW * 0.27f);     // HỌ VÀ TÊN | EMAIL
    float div2 = tableX + std::floor(tableW * 0.45f);     // EMAIL | SỐ ĐIỆN THOẠI
    float div3 = tableX + std::floor(tableW * 0.57f);     // SĐT | HẠNG THẺ
    float div4 = tableX + std::floor(tableW * 0.69f);     // HẠNG THẺ | NGÀY ĐĂNG KÝ
    float div5 = tableX + std::floor(tableW * 0.80f);     // NGÀY ĐĂNG KÝ | ĐÃ MUA
    float div6 = tableX + tableW - 156.f;                 // ĐÃ MUA | THAO TÁC

    // Header column dividers
    for (float dx : {div0, div1, div2, div3, div4, div5, div6}) {
        RectangleShape hDiv(Vector2f(1.f, headerH));
        hDiv.setPosition({dx, tableY});
        hDiv.setFillColor(Color(255, 255, 255, 65));
        target.draw(hDiv);
    }

    // Column content starting positions
    float col_stt    = tableX + 44.f;
    float col_name   = div0 + 14.f;
    float col_action = div6 + 12.f;

    float thY = std::round(tableY + 11.f);
    Text th_stt(boldFont); th_stt.setString(L"STT"); th_stt.setCharacterSize(16); th_stt.setFillColor(Color::White); th_stt.setPosition({col_stt, thY}); target.draw(th_stt);
    Text th_name(boldFont); th_name.setString(L"HỌ VÀ TÊN KHÁCH HÀNG"); th_name.setCharacterSize(16); th_name.setFillColor(Color::White); th_name.setPosition({col_name, thY}); target.draw(th_name);

    auto drawColHdr = [&](float xStart, float xEnd, const wstring& str) {
        Text th(boldFont, str, 16);
        th.setFillColor(Color::White);
        FloatRect thb = th.getLocalBounds();
        th.setPosition({
            std::round(xStart + (xEnd - xStart - thb.size.x) / 2.f - thb.position.x),
            thY
        });
        target.draw(th);
    };
    drawColHdr(div1, div2, L"EMAIL TÀI KHOẢN");
    drawColHdr(div2, div3, L"SỐ ĐIỆN THOẠI");
    drawColHdr(div3, div4, L"HẠNG THẺ");
    drawColHdr(div4, div5, L"NGÀY ĐĂNG KÝ");
    drawColHdr(div5, div6, L"ĐÃ MUA");
    drawColHdr(div6, tableX + tableW, L"THAO TÁC");

    // Customer Rows
    float rTop = tableY + headerH;
    float rH = 52.f;
    int startIdx = page * rowsPerPage;
    int endIdx = min((int)filteredCustomers.size(), startIdx + rowsPerPage);

    for (int i = startIdx; i < endIdx; ++i) {
        int rIdx = i - startIdx;
        float ry = rTop + rIdx * rH;
        const auto* c = filteredCustomers[i];

        // Row background
        bool isSel = (selectedIndex == i || c->selected);
        bool isHov = (hoveredRow == i);
        Color rBg = isSel ? Color(239, 246, 255) : (isHov ? Color(248, 250, 252) : Color::White);

        RoundedRectRenderer::draw(target, {tableX, ry}, {tableW, rH - 2.f}, 4.f, rBg, 1.f, isSel ? Color(186, 230, 253) : Color(241, 245, 249));

        // Row column dividers
        for (float dx : {div0, div1, div2, div3, div4, div5, div6}) {
            RectangleShape rDiv(Vector2f(1.f, rH - 2.f));
            rDiv.setPosition({dx, ry});
            rDiv.setFillColor(Color(241, 245, 249));
            target.draw(rDiv);
        }

        // Checkbox
        Vector2f rcPos(tableX + 16.f, ry + 16.f);
        RoundedRectRenderer::draw(target, rcPos, {16.f, 16.f}, 3.f, c->selected ? Color(8, 125, 181) : Color::White, 1.f, Color(203, 213, 225));
        if (c->selected) {
            UITheme::drawCheckmark(target, {rcPos.x + 8.f, rcPos.y + 8.f}, 0.72f, Color::White, 2.f);
        }

        // STT
        Text sttTxt(font); sttTxt.setString(to_string(i + 1)); sttTxt.setCharacterSize(17); sttTxt.setFillColor(Color(100, 116, 139)); sttTxt.setPosition({col_stt + 4.f, std::round(ry + 14.f)}); target.draw(sttTxt);

        // Avatar Circle with Initials
        Vector2f avPos(col_name, ry + 8.f);
        CircleShape av(16.f); av.setPosition(avPos); av.setFillColor(c->avatarBg); target.draw(av);
        Text avTxt(boldFont); avTxt.setString(c->initials); avTxt.setCharacterSize(17); avTxt.setFillColor(Color(30, 41, 59));
        FloatRect avb = avTxt.getLocalBounds(); avTxt.setPosition({std::round(avPos.x + 16.f - avb.size.x / 2.f - avb.position.x), std::round(avPos.y + 16.f - avb.size.y / 2.f - avb.position.y)}); target.draw(avTxt);

        // Name & UID
        Text nameTxt(boldFont); nameTxt.setString(c->displayFullName); nameTxt.setCharacterSize(17); nameTxt.setFillColor(Color(23, 32, 51)); nameTxt.setPosition({col_name + 40.f, std::round(ry + 5.f)}); target.draw(nameTxt);
        Text uidTxt(font); uidTxt.setString(s2ws("UID: " + c->uid)); uidTxt.setCharacterSize(17); uidTxt.setFillColor(Color(148, 163, 184)); uidTxt.setPosition({col_name + 40.f, std::round(ry + 26.f)}); target.draw(uidTxt);

        // Email (căn giữa)
        Text emailTxt(font); emailTxt.setString(s2ws(c->email)); emailTxt.setCharacterSize(16); emailTxt.setFillColor(Color(51, 65, 85));
        FloatRect emb = emailTxt.getLocalBounds();
        emailTxt.setPosition({std::round(div1 + (div2 - div1 - emb.size.x) / 2.f - emb.position.x), std::round(ry + 14.f)});
        target.draw(emailTxt);

        // Phone (căn giữa)
        Text phoneTxt(font); phoneTxt.setString(s2ws(c->phone)); phoneTxt.setCharacterSize(16); phoneTxt.setFillColor(Color(51, 65, 85));
        FloatRect phb = phoneTxt.getLocalBounds();
        phoneTxt.setPosition({std::round(div2 + (div3 - div2 - phb.size.x) / 2.f - phb.position.x), std::round(ry + 14.f)});
        target.draw(phoneTxt);

        // Tier Pill (căn giữa)
        auto drawRowTierPill = [&](float pillW, const wstring& lbl, Color bg, Color bor, Color fg) {
            float tierX = std::round(div3 + (div4 - div3 - pillW) / 2.f);
            Vector2f tierPos(tierX, ry + 11.f);
            RoundedRectRenderer::draw(target, tierPos, {pillW, 28.f}, 14.f, bg, 1.f, bor);
            Text tTxt(boldFont); tTxt.setString(lbl); tTxt.setCharacterSize(15); tTxt.setFillColor(fg);
            FloatRect tb = tTxt.getLocalBounds();
            tTxt.setPosition({std::round(tierPos.x + (pillW - tb.size.x) / 2.f - tb.position.x), std::round(tierPos.y + (28.f - tb.size.y) / 2.f - tb.position.y)});
            target.draw(tTxt);
        };
        if (c->tier == CustomerTier::GOLD) {
            drawRowTierPill(110.f, L"Hạng Vàng", Color(254, 243, 199), Color(253, 230, 138), Color(180, 83, 9));
        } else if (c->tier == CustomerTier::SILVER) {
            drawRowTierPill(104.f, L"Hạng Bạc", Color(241, 245, 249), Color(226, 232, 240), Color(71, 85, 105));
        } else {
            drawRowTierPill(104.f, L"Tiêu chuẩn", Color(224, 242, 254), Color(186, 230, 253), Color(8, 125, 181));
        }

        // Registration Date (căn giữa)
        Text dtTxt(font); dtTxt.setString(s2ws(c->registeredDate)); dtTxt.setCharacterSize(17); dtTxt.setFillColor(Color(100, 116, 139));
        FloatRect dtb = dtTxt.getLocalBounds();
        dtTxt.setPosition({std::round(div4 + (div5 - div4 - dtb.size.x) / 2.f - dtb.position.x), std::round(ry + 14.f)});
        target.draw(dtTxt);

        // Tickets Bought (căn giữa)
        Text tkTxt(boldFont); tkTxt.setString(to_wstring(c->ticketsBought) + L" vé"); tkTxt.setCharacterSize(16); tkTxt.setFillColor(Color(8, 125, 181));
        FloatRect tkb = tkTxt.getLocalBounds();
        tkTxt.setPosition({std::round(div5 + (div6 - div5 - tkb.size.x) / 2.f - tkb.position.x), std::round(ry + 14.f)});
        target.draw(tkTxt);

        // Action buttons: Xem, Sửa, Xóa (cỡ chữ tăng 20%, thoáng đãng không dính nhau)
        RoundedRectRenderer::draw(target, {col_action, ry + 12.f}, {40.f, 28.f}, 4.f, Color(241, 245, 249), 1.f, Color(203, 213, 225));
        Text aView(boldFont); aView.setString(L"Xem"); aView.setCharacterSize(17); aView.setFillColor(Color(8, 125, 181));
        FloatRect vb = aView.getLocalBounds();
        aView.setPosition({std::round(col_action + (40.f - vb.size.x) / 2.f - vb.position.x), std::round(ry + 12.f + (28.f - vb.size.y) / 2.f - vb.position.y)});
        target.draw(aView);

        RoundedRectRenderer::draw(target, {col_action + 46.f, ry + 12.f}, {40.f, 28.f}, 4.f, Color(254, 243, 199), 1.f, Color(253, 230, 138));
        Text aEdit(boldFont); aEdit.setString(L"S\u1eeda"); aEdit.setCharacterSize(17); aEdit.setFillColor(Color(180, 83, 9));
        FloatRect eb = aEdit.getLocalBounds();
        aEdit.setPosition({std::round(col_action + 46.f + (40.f - eb.size.x) / 2.f - eb.position.x), std::round(ry + 12.f + (28.f - eb.size.y) / 2.f - eb.position.y)});
        target.draw(aEdit);

        RoundedRectRenderer::draw(target, {col_action + 92.f, ry + 12.f}, {40.f, 28.f}, 4.f, Color(254, 242, 242), 1.f, Color(254, 202, 202));
        Text aDel(boldFont); aDel.setString(L"X\u00f3a"); aDel.setCharacterSize(17); aDel.setFillColor(Color(220, 38, 38));
        FloatRect db = aDel.getLocalBounds();
        aDel.setPosition({std::round(col_action + 92.f + (40.f - db.size.x) / 2.f - db.position.x), std::round(ry + 12.f + (28.f - db.size.y) / 2.f - db.position.y)});
        target.draw(aDel);
    }

    // Pagination Footer
    float pageY = rTop + rowsPerPage * rH + 12.f;
    Text pageInfo(font);
    wstring pageInfoStr = filteredCustomers.empty()
        ? L"Không có khách hàng nào phù hợp"
        : (L"Hiển thị " + to_wstring(startIdx + 1) + L" - " + to_wstring(endIdx) + L" trên tổng số " + to_wstring(filteredCustomers.size()) + L" khách hàng");
    pageInfo.setString(pageInfoStr);
    pageInfo.setCharacterSize(14);
    pageInfo.setFillColor(Color(100, 116, 139));
    pageInfo.setPosition({tableX + 16.f, pageY + 6.f});
    target.draw(pageInfo);

    int totalPages = max(1, (static_cast<int>(filteredCustomers.size()) + rowsPerPage - 1) / rowsPerPage);
    float rightEdge = tableX + tableW;

    int numPageBtns = min(5, totalPages);
    int startP = max(0, min(page - 2, totalPages - numPageBtns));
    int endP = min(totalPages - 1, startP + numPageBtns - 1);

    float btnPrevW = 32.f;
    float btnNextW = 32.f;
    float btnPageW = 32.f;
    float gap = 6.f;
    float totalBtnsW = btnPrevW + gap + (endP - startP + 1) * (btnPageW + gap) - gap + gap + btnNextW;
    float curBtnX = rightEdge - totalBtnsW;

    // Prev Button
    bool canPrev = (page > 0);
    RoundedRectRenderer::draw(target, {curBtnX, pageY}, {btnPrevW, 30.f}, 4.f, Color::White, 1.f, Color(203, 213, 225));
    Text prevT(boldFont);
    prevT.setString(L"<");
    prevT.setCharacterSize(14);
    prevT.setFillColor(canPrev ? Color(71, 85, 105) : Color(203, 213, 225));
    FloatRect prb = prevT.getLocalBounds();
    prevT.setPosition({curBtnX + (btnPrevW - prb.size.x) / 2.f - prb.position.x, pageY + (30.f - prb.size.y) / 2.f - prb.position.y});
    target.draw(prevT);
    curBtnX += btnPrevW + gap;

    // Number Buttons
    for (int p = startP; p <= endP; ++p) {
        bool isAct = (p == page);
        Color pBg = isAct ? Color(8, 125, 181) : Color::White;
        Color pBorder = isAct ? Color(8, 125, 181) : Color(203, 213, 225);
        Color pFg = isAct ? Color::White : Color(71, 85, 105);

        RoundedRectRenderer::draw(target, {curBtnX, pageY}, {btnPageW, 30.f}, 4.f, pBg, 1.f, pBorder);
        Text pTxt(boldFont);
        pTxt.setString(to_string(p + 1));
        pTxt.setCharacterSize(14);
        pTxt.setFillColor(pFg);
        FloatRect pb = pTxt.getLocalBounds();
        pTxt.setPosition({curBtnX + (btnPageW - pb.size.x) / 2.f - pb.position.x, pageY + (30.f - pb.size.y) / 2.f - pb.position.y});
        target.draw(pTxt);
        curBtnX += btnPageW + gap;
    }

    // Next Button
    bool canNext = (page < totalPages - 1);
    RoundedRectRenderer::draw(target, {curBtnX, pageY}, {btnNextW, 30.f}, 4.f, Color::White, 1.f, Color(203, 213, 225));
    Text nextT(boldFont);
    nextT.setString(L">");
    nextT.setCharacterSize(14);
    nextT.setFillColor(canNext ? Color(71, 85, 105) : Color(203, 213, 225));
    FloatRect nxb = nextT.getLocalBounds();
    nextT.setPosition({curBtnX + (btnNextW - nxb.size.x) / 2.f - nxb.position.x, pageY + (30.f - nxb.size.y) / 2.f - nxb.position.y});
    target.draw(nextT);
}

void UserPanel::drawDialog(RenderTarget& target) const {
    // Dim Overlay
    RectangleShape dim(Vector2f(width, height));
    dim.setPosition(position);
    dim.setFillColor(Color(15, 23, 42, 160));
    target.draw(dim);

    float dW = 480.f;
    float dH = 350.f;
    if (activeDialog == DialogType::VIEW) {
        dW = 520.f;
        dH = 370.f;
    } else if (activeDialog == DialogType::DELETE_CONFIRM) {
        dW = 500.f;
        dH = 210.f;
    }
    Vector2f dPos(position.x + (width - dW) / 2.f, position.y + (height - dH) / 2.f);

    RoundedRectRenderer::draw(target, dPos, {dW, dH}, 8.f, Color::White, 1.f, Color(226, 232, 240));

    if (activeDialog == DialogType::VIEW && dialogUserIndex >= 0 && dialogUserIndex < (int)filteredCustomers.size()) {
        const auto* c = filteredCustomers[dialogUserIndex];

        Text dTitle(boldFont);
        dTitle.setString(L"Thông tin chi tiết hội viên");
        dTitle.setCharacterSize(20);
        dTitle.setFillColor(Color(23, 32, 51));
        dTitle.setPosition({dPos.x + 24.f, dPos.y + 20.f});
        target.draw(dTitle);

        Text dSub(font);
        dSub.setString(L"Hồ sơ thành viên CineCore Rewards ID: " + s2ws(c->uid));
        dSub.setCharacterSize(17.5f);
        dSub.setFillColor(Color(100, 116, 139));
        dSub.setPosition({dPos.x + 24.f, dPos.y + 48.f});
        target.draw(dSub);

        float lineY = dPos.y + 82.f;
        auto drawField = [&](const wstring& l, const wstring& v, Color vCol) {
            Text fl(font); fl.setString(l); fl.setCharacterSize(17.5f); fl.setFillColor(Color(100, 116, 139)); fl.setPosition({dPos.x + 24.f, lineY}); target.draw(fl);
            Text fv(boldFont); fv.setString(v); fv.setCharacterSize(17.5f); fv.setFillColor(vCol); fv.setPosition({dPos.x + 210.f, lineY}); target.draw(fv);
            lineY += 34.f;
        };

        drawField(L"Họ và tên:", c->displayFullName, Color(23, 32, 51));
        drawField(L"Email tài khoản:", s2ws(c->email), Color(37, 99, 235));
        drawField(L"Số điện thoại:", s2ws(c->phone), Color(23, 32, 51));
        wstring tierName = (c->tier == CustomerTier::GOLD) ? L"Hạng Vàng (VIP)" : (c->tier == CustomerTier::SILVER ? L"Hạng Bạc" : L"Tiêu chuẩn");
        drawField(L"Cấp bậc thẻ:", tierName, Color(180, 83, 9));
        drawField(L"Số vé đã mua:", to_wstring(c->ticketsBought) + L" vé", Color(21, 128, 61));
        drawField(L"Tổng tích lũy chi tiêu:", s2ws(formatCurrency(c->totalSpent)) + L" VNĐ", Color(225, 29, 72));

        Vector2f btnP(dPos.x + dW - 130.f, dPos.y + dH - 50.f);
        RoundedRectRenderer::draw(target, btnP, {105.f, 36.f}, 6.f, Color(8, 125, 181));
        Text btnT(boldFont); btnT.setString(L"Đóng"); btnT.setCharacterSize(17.5f); btnT.setFillColor(Color::White);
        FloatRect btb = btnT.getLocalBounds();
        btnT.setPosition({std::round(btnP.x + (105.f - btb.size.x) / 2.f - btb.position.x), std::round(btnP.y + (36.f - btb.size.y) / 2.f - btb.position.y)});
        target.draw(btnT);
    } else if (activeDialog == DialogType::DELETE_CONFIRM && dialogUserIndex >= 0 && dialogUserIndex < (int)filteredCustomers.size()) {
        const auto* c = filteredCustomers[dialogUserIndex];

        Text dTitle(boldFont);
        dTitle.setString(L"Xác nhận xóa tài khoản khách hàng");
        dTitle.setCharacterSize(18);
        dTitle.setFillColor(Color(220, 38, 38));
        dTitle.setPosition({dPos.x + 24.f, dPos.y + 20.f});
        target.draw(dTitle);

        Text warn(font);
        warn.setString(L"Bạn có chắc chắn muốn xóa tài khoản của khách hàng:\n" + c->displayFullName + L" (" + s2ws(c->email) + L")?\n\nHành động này sẽ xóa dữ liệu tài khoản và tích lũy điểm thưởng!");
        warn.setCharacterSize(16);
        warn.setFillColor(Color(51, 65, 85));
        warn.setPosition({dPos.x + 24.f, dPos.y + 54.f});
        target.draw(warn);

        Vector2f cancelP(dPos.x + dW - 230.f, dPos.y + dH - 50.f);
        RoundedRectRenderer::draw(target, cancelP, {95.f, 36.f}, 6.f, Color::White, 1.f, Color(203, 213, 225));
        Text cT(boldFont); cT.setString(L"Hủy bỏ"); cT.setCharacterSize(16); cT.setFillColor(Color(71, 85, 105));
        FloatRect cb = cT.getLocalBounds();
        cT.setPosition({std::round(cancelP.x + (95.f - cb.size.x) / 2.f - cb.position.x), std::round(cancelP.y + (36.f - cb.size.y) / 2.f - cb.position.y)});
        target.draw(cT);

        Vector2f delP(dPos.x + dW - 120.f, dPos.y + dH - 50.f);
        RoundedRectRenderer::draw(target, delP, {95.f, 36.f}, 6.f, Color(220, 38, 38));
        Text dT(boldFont); dT.setString(L"Xác nhận"); dT.setCharacterSize(16); dT.setFillColor(Color::White);
        FloatRect db = dT.getLocalBounds();
        dT.setPosition({std::round(delP.x + (95.f - db.size.x) / 2.f - db.position.x), std::round(delP.y + (36.f - db.size.y) / 2.f - db.position.y)});
        target.draw(dT);
    } else if (activeDialog == DialogType::ADD_NEW || activeDialog == DialogType::EDIT) {
        bool isEdit = (activeDialog == DialogType::EDIT);
        float titleSize = isEdit ? 17.f : 15.5f;
        float labelSize = isEdit ? 17.f : 15.5f;
        float inputSize = isEdit ? 17.f : 15.5f;
        float btnSize   = isEdit ? 17.f : 15.5f;

        Text dTitle(boldFont);
        dTitle.setString(isEdit ? L"Chỉnh sửa hồ sơ hội viên" : L"Thêm hội viên CineCore Rewards mới");
        dTitle.setCharacterSize(titleSize);
        dTitle.setFillColor(Color(23, 32, 51));
        dTitle.setPosition({dPos.x + 24.f, dPos.y + 20.f});
        target.draw(dTitle);

        float inY = dPos.y + 65.f;
        int fIdx = 0;
        auto drawInput = [&](const wstring& lbl, const string& val, const wstring& placeholder) {
            bool isAct = (activeInputField == fIdx);
            Text fl(boldFont); fl.setString(lbl); fl.setCharacterSize(labelSize); fl.setFillColor(Color(71, 85, 105)); fl.setPosition({dPos.x + 24.f, inY}); target.draw(fl);
            Vector2f boxP(dPos.x + 24.f, inY + 22.f);
            RoundedRectRenderer::draw(target, boxP, {dW - 48.f, 36.f}, 4.f,
                                      isAct ? Color::White : Color(248, 250, 252),
                                      isAct ? 1.8f : 1.f,
                                      isAct ? Color(8, 125, 181) : Color(203, 213, 225));
            wstring disp = val.empty()
                ? (isAct ? L"|" : placeholder)
                : (s2ws(val) + (isAct ? L"|" : L""));
            Text fv(font);
            fv.setString(disp);
            fv.setCharacterSize(inputSize);
            fv.setFillColor(val.empty() && !isAct ? Color(148, 163, 184) : Color(23, 32, 51));
            FloatRect fvb = fv.getLocalBounds();
            fv.setPosition({boxP.x + 10.f, std::round(boxP.y + (36.f - fvb.size.y) / 2.f - fvb.position.y)});
            target.draw(fv);
            inY += 64.f;
            fIdx++;
        };

        drawInput(L"Họ và tên khách hàng *", inputName, L"Ví dụ: Trần Văn Nam");
        drawInput(L"Email tài khoản *", inputEmail, L"namtran@gmail.com");
        drawInput(L"Số điện thoại *", inputPhone, L"0912345678");

        Vector2f cancelP(dPos.x + dW - 230.f, dPos.y + dH - 50.f);
        RoundedRectRenderer::draw(target, cancelP, {95.f, 36.f}, 6.f, Color::White, 1.f, Color(203, 213, 225));
        Text cT(boldFont); cT.setString(L"Hủy bỏ"); cT.setCharacterSize(btnSize); cT.setFillColor(Color(71, 85, 105));
        FloatRect cb = cT.getLocalBounds();
        cT.setPosition({std::round(cancelP.x + (95.f - cb.size.x) / 2.f - cb.position.x), std::round(cancelP.y + (36.f - cb.size.y) / 2.f - cb.position.y)});
        target.draw(cT);

        Vector2f okP(dPos.x + dW - 120.f, dPos.y + dH - 50.f);
        RoundedRectRenderer::draw(target, okP, {95.f, 36.f}, 6.f, Color(8, 125, 181));
        Text okT(boldFont);
        okT.setString(isEdit ? L"Lưu lại" : L"Thêm mới");
        okT.setCharacterSize(btnSize);
        okT.setFillColor(Color::White);
        FloatRect ob = okT.getLocalBounds();
        okT.setPosition({std::round(okP.x + (95.f - ob.size.x) / 2.f - ob.position.x), std::round(okP.y + (36.f - ob.size.y) / 2.f - ob.position.y)});
        target.draw(okT);
    }
}

void UserPanel::drawBottomBar(RenderTarget& target) const {
    float barY = height - 28.f;

    RectangleShape div(Vector2f(width - 40.f, 1.f));
    div.setPosition({position.x + 20.f, barY - 4.f});
    div.setFillColor(Color(226, 232, 240));
    target.draw(div);

    Text authText(font);
    authText.setString(L"Admin CineCore (Toàn quyền)  •  1728x972 HD");
    authText.setCharacterSize(11.f);
    authText.setFillColor(Color(100, 116, 139));
    FloatRect ab = authText.getLocalBounds();
    authText.setPosition({position.x + width - ab.size.x - 20.f, barY});
    target.draw(authText);
}

void UserPanel::drawToast(RenderTarget& target) const {
    Text toast(boldFont);
    toast.setString(toastMessage);
    toast.setCharacterSize(14);
    toast.setFillColor(Color::White);
    FloatRect tb = toast.getLocalBounds();

    float tw = max(380.f, min(width - 80.f, tb.size.x + 48.f));
    float th = 42.f;
    Vector2f tPos(position.x + (width - tw) / 2.f, position.y + height - 60.f);

    RoundedRectRenderer::draw(target, tPos, {tw, th}, 8.f, Color(22, 163, 74, 245), 1.5f, Color(21, 128, 61));
    toast.setPosition({std::round(tPos.x + (tw - tb.size.x) / 2.f - tb.position.x), std::round(tPos.y + (th - tb.size.y) / 2.f - tb.position.y)});
    target.draw(toast);
}
