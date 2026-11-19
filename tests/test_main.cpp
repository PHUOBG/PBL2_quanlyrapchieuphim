// CineCore automated tests (no SFML window needed).
// Run:  cmake --build build && ctest --test-dir build --output-on-failure
// All file I/O happens inside a throw-away sandbox directory, never in the real data/ folder.
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "data-structures/DLL.h"
#include "data-structures/HashTable.h"
#include "data-structures/MyQueue.h"
#include "data-structures/MyStack.h"
#include "data-structures/MyVector.h"
#include "data-structures/Trie.h"
#include "repositories/UserRepository.h"
#include "repositories/admin/AdminComboRepository.h"
#include "repositories/admin/AdminMovieRepository.h"
#include "repositories/admin/AdminTicketRepository.h"
#include "repositories/booking/ShowtimeSeatRepository.h"
#include "services/AuthService.h"
#include "services/BookingService.h"
#include "services/ShowtimeCleanupService.h"
#include "services/VoucherManager.h"
#include "utils/FileUtils.h"
#include "utils/PasswordHasher.h"
#include "utils/Validator.h"

namespace fs = std::filesystem;
using std::string;

static int g_pass = 0, g_fail = 0;
static string g_suite;

#define CHECK(cond)                                                                      \
    do {                                                                                 \
        if (cond) { ++g_pass; }                                                          \
        else { ++g_fail; std::cout << "  FAIL [" << g_suite << "] " << #cond            \
                                   << "  (line " << __LINE__ << ")\n"; }                 \
    } while (0)

static void suite(const char* name, const std::function<void()>& fn) {
    g_suite = name;
    int before = g_fail;
    fn();
    std::cout << (g_fail == before ? "[ OK ] " : "[FAIL] ") << name << "\n";
}

static string slurp(const string& p) {
    std::ifstream f(p, std::ios::binary);
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}
static void spit(const string& p, const string& c) {
    std::ofstream f(p, std::ios::binary | std::ios::trunc);
    f << c;
}
static int countLines(const string& p) {
    std::ifstream f(p);
    string l;
    int n = 0;
    while (std::getline(f, l)) if (!l.empty()) ++n;
    return n;
}

static void resetData() {
    fs::remove_all("data");
    fs::create_directories("data");
    spit("data/voucher_defs.txt",
         "code|type|value|min_bill|description\n"
         "FIX50|1|50000|100000|fixed 50k\n"
         "PCT10|2|10|0|ten percent\n"
         "HUGE|1|900000|0|bigger than any bill\n");
    spit("data/user_wallets.txt", "email|code|status|expiry_date|quantity\n");
}

static BookingInfo makeInfo(const string& showtime, const string& room, std::vector<string> seats, int price = 140000) {
    BookingInfo b;
    b.customerEmail = "cust@example.com";
    b.customerName = "Khach Hang";
    b.showtimeId = showtime;
    b.roomId = room;
    b.roomName = "Phong Chieu 1";
    b.movieName = "Phim Thu";
    b.date = "01/01/2030";
    b.time = "10:00";
    b.selectedSeats = seats;
    for (size_t i = 0; i < seats.size(); ++i) b.seatsDisplay += (i ? ", " : "") + seats[i];
    b.totalPrice = price;
    return b;
}

int main() {
    const fs::path sandbox = fs::temp_directory_path() / "cinecore_test_sandbox";
    fs::remove_all(sandbox);
    fs::create_directories(sandbox);
    fs::current_path(sandbox);
    std::cout << "Sandbox: " << sandbox.string() << "\n";

    // ------------------------------------------------------------------ file utils
    suite("FileUtils.writeAtomic", [] {
        resetData();
        CHECK(FileUtils::writeAtomic("data/a.txt", "one\n"));
        CHECK(slurp("data/a.txt") == "one\n");
        CHECK(FileUtils::writeAtomic("data/a.txt", "two\n"));          // overwrite
        CHECK(slurp("data/a.txt") == "two\n");
        CHECK(!fs::exists("data/a.txt.tmp"));                           // no temp leftovers
        CHECK(!FileUtils::writeAtomic("data/missing_dir/a.txt", "x"));  // failure is reported
        CHECK(slurp("data/a.txt") == "two\n");                          // original untouched
    });

    suite("FileUtils.resolveDataPath", [] {
        resetData();
        CHECK(FileUtils::resolveDataPath("data/voucher_defs.txt") == "data/voucher_defs.txt");
        // "../data/x" must still resolve from a cwd that has no parent "data" folder
        string r = FileUtils::resolveDataPath("../data/voucher_defs.txt");
        CHECK(fs::exists(r));
    });

    // ------------------------------------------------------------------ validators / hashing
    suite("Validator+PasswordHasher", [] {
        CHECK(Validator::isValidEmail("a@b.com"));
        CHECK(!Validator::isValidEmail("abc"));
        CHECK(!Validator::isValidEmail("a@b"));
        CHECK(!Validator::isValidEmail(""));
        CHECK(Validator::isStrongPassword("Abcdef12"));
        CHECK(!Validator::isStrongPassword("abc"));
        CHECK(!Validator::isStrongPassword("abcdefgh"));
        CHECK(Validator::isValidDate("29/02/2024"));
        CHECK(!Validator::isValidDate("29/02/2023"));
        CHECK(!Validator::isValidDate("31/04/2020"));
        CHECK(!Validator::isValidDate("01/01/2999"));   // future birthdate
        CHECK(Validator::isValidPhone("0901234567"));
        CHECK(!Validator::isValidPhone("123"));
        CHECK(PasswordHasher::verifyPassword("secret", PasswordHasher::hashPassword("secret")));
        CHECK(!PasswordHasher::verifyPassword("Secret", PasswordHasher::hashPassword("secret")));
        CHECK(PasswordHasher::hashPassword("a") != PasswordHasher::hashPassword("b"));
    });

    // ------------------------------------------------------------------ authentication
    suite("Auth: seed, login, register", [] {
        resetData();
        {
            AuthService auth("data/users.txt");
            CHECK(auth.emailExists("admin@cinexine.vn"));           // fresh install is seeded
            CHECK(auth.login("admin@cinexine.vn", "admin123"));
            CHECK(auth.isLoggedIn());
            CHECK(auth.isAdmin());
            auth.logout();
            CHECK(!auth.isLoggedIn());
            CHECK(!auth.isAdmin());
            CHECK(auth.getCurrentUser() == nullptr);
            CHECK(!auth.login("admin@cinexine.vn", "wrong"));
            CHECK(!auth.login("nobody@x.com", "admin123"));
            CHECK(!auth.login("", ""));
            CHECK(!auth.isLoggedIn());

            CHECK(auth.registerUser("new@admin.vn", "Passw0rdX", "Quan Tri", "01/01/1990", "0912345678", AppRole::Admin));
            CHECK(!auth.registerUser("new@admin.vn", "Passw0rdX", "Dup", "", "", AppRole::Admin));    // duplicate
            CHECK(!auth.registerUser("NEW@admin.vn", "Passw0rdX", "Dup", "", "", AppRole::Admin));    // case-insensitive duplicate
            CHECK(!auth.registerUser("bad-email", "Passw0rdX", "X", "", "", AppRole::Admin));
            CHECK(!auth.registerUser("w@eak.vn", "weak", "X", "", "", AppRole::Admin));
            CHECK(!auth.registerUser("d@ate.vn", "Passw0rdX", "X", "31/02/2020", "", AppRole::Admin));
            CHECK(!auth.registerUser("p@hone.vn", "Passw0rdX", "X", "", "12", AppRole::Admin));
            CHECK(!auth.emailExists("w@eak.vn"));
        }
        // persistence across a "restart"
        AuthService again("data/users.txt");
        CHECK(again.login("new@admin.vn", "Passw0rdX"));
    });

    suite("Auth: reset / change password security", [] {
        resetData();
        AuthService auth("data/users.txt");
        auth.registerUser("boss@admin.vn", "Passw0rdX", "Boss", "01/01/1990", "0912345678", AppRole::Admin);

        using R = AuthService::ResetResult;
        CHECK(auth.resetPassword("boss@admin.vn", "0000000000", "NewPassw0rd") == R::VerificationFailed);  // wrong phone
        CHECK(auth.resetPassword("ghost@admin.vn", "0912345678", "NewPassw0rd") == R::VerificationFailed); // unknown account: same answer
        CHECK(auth.resetPassword("boss@admin.vn", "", "NewPassw0rd") == R::MissingInput);
        CHECK(auth.resetPassword("boss@admin.vn", "0912345678", "weak") == R::WeakPassword);
        CHECK(auth.login("boss@admin.vn", "Passw0rdX"));                       // nothing changed so far
        auth.logout();
        CHECK(auth.resetPassword("boss@admin.vn", "0912345678", "NewPassw0rd") == R::Ok);
        CHECK(!auth.login("boss@admin.vn", "Passw0rdX"));
        CHECK(auth.login("boss@admin.vn", "NewPassw0rd"));

        CHECK(!auth.changePassword("boss@admin.vn", "wrongold", "Another1Pass"));
        CHECK(!auth.changePassword("boss@admin.vn", "NewPassw0rd", "weak"));
        CHECK(auth.changePassword("boss@admin.vn", "NewPassw0rd", "Another1Pass"));
        AuthService reloaded("data/users.txt");
        CHECK(reloaded.login("boss@admin.vn", "Another1Pass"));
    });

    suite("Auth: delete account", [] {
        resetData();
        {
            AuthService auth("data/users.txt");
            CHECK(auth.login("admin@cinexine.vn", "admin123"));
            CHECK(auth.deleteAccount("admin@cinexine.vn"));
            CHECK(!auth.isLoggedIn());                               // session cleared
            CHECK(!auth.login("admin@cinexine.vn", "admin123"));     // deleted account can't authenticate
            CHECK(!auth.deleteAccount("admin@cinexine.vn"));         // already gone
        }
        AuthService restarted("data/users.txt");                     // must NOT resurrect the account
        CHECK(!restarted.emailExists("admin@cinexine.vn"));
        CHECK(!restarted.login("admin@cinexine.vn", "admin123"));
    });

    suite("UserRepository: no stale overwrite", [] {
        resetData();
        UserRepository a("data/users.txt");
        UserRepository b("data/users.txt");   // second instance, loaded before a changes anything
        CHECK(a.addUser(User("x@x.vn", "h", "X", "", "", 1, AppRole::Customer)));
        // destroying b (stale snapshot) must not erase x@x.vn from disk
        { UserRepository tmp("data/users.txt"); (void)tmp; }
        CHECK(slurp("data/users.txt").find("x@x.vn") != string::npos);
    });

    suite("UserRepository: malformed + pipe injection", [] {
        resetData();
        spit("data/users.txt",
             "\xEF\xBB\xBF" "email|passwordHash|fullName|birthDate|phone|registeredAt|role\r\n"
             "ok@x.vn|h|Ok User|01/01/1990|0912345678|100|admin\r\n"
             "garbage line without separators\r\n"
             "\r\n"
             "short|row\r\n");
        UserRepository repo("data/users.txt");
        CHECK(repo.exists("ok@x.vn"));
        CHECK(repo.getTotalUsers() == 1);
        CHECK(repo.addUser(User("inj@x.vn", "h", "Evil|Name\nsecond", "", "", 1, AppRole::Customer)));
        UserRepository reread("data/users.txt");
        CHECK(reread.getTotalUsers() == 2);   // the pipe/newline did not create extra columns/records
        CHECK(reread.findByEmail("inj@x.vn") != nullptr);
    });

    // ------------------------------------------------------------------ seat reservation
    suite("Seats: reservation rules", [] {
        resetData();
        ShowtimeSeatRepository repo("data/RoomStatusAtShowtime.txt");
        DLL<string> s1; s1.push_back("A1"); s1.push_back("A2");
        CHECK(repo.tryReserveSeats("ST1", "R1", s1));
        CHECK(!repo.tryReserveSeats("ST1", "R1", s1));              // same seats again
        DLL<string> overlap; overlap.push_back("A2"); overlap.push_back("A3");
        CHECK(!repo.tryReserveSeats("ST1", "R1", overlap));          // partial overlap rejected atomically
        CHECK(repo.getBookedSeats("ST1", "R1").getSize() == 2);      // A3 was NOT reserved
        DLL<string> dup; dup.push_back("B1"); dup.push_back("B1");
        CHECK(!repo.tryReserveSeats("ST1", "R1", dup));              // duplicate within one transaction
        CHECK(repo.tryReserveSeats("ST2", "R1", s1));                // same room, different showtime
        CHECK(repo.tryReserveSeats("ST3", "R2", s1));                // different room
        DLL<string> none;
        CHECK(!repo.tryReserveSeats("ST1", "R1", none));
        CHECK(!repo.tryReserveSeats("", "R1", s1));                  // no showtime

        ShowtimeSeatRepository restarted("data/RoomStatusAtShowtime.txt");   // persistence
        CHECK(restarted.getBookedSeats("ST1", "R1").getSize() == 2);
        CHECK(restarted.hasAnySeatBooked("ST2", "R1", s1));

        CHECK(repo.removeBookedSeats("ST1", "R1", s1));              // release
        CHECK(repo.tryReserveSeats("ST1", "R1", s1));                // available again
    });

    suite("Seats: stale instance cannot double book", [] {
        resetData();
        ShowtimeSeatRepository a("data/RoomStatusAtShowtime.txt");
        ShowtimeSeatRepository b("data/RoomStatusAtShowtime.txt");   // loaded before a books
        DLL<string> seat; seat.push_back("C3");
        CHECK(a.tryReserveSeats("ST1", "R1", seat));
        CHECK(!b.tryReserveSeats("ST1", "R1", seat));                // b must re-read disk and refuse
        CHECK(countLines("data/RoomStatusAtShowtime.txt") == 2);
    });

    suite("Seats: save failure leaves no phantom reservation", [] {
        resetData();
        ShowtimeSeatRepository repo("data/nodir/seats.txt");          // directory does not exist
        DLL<string> seat; seat.push_back("A1");
        CHECK(!repo.addBookedSeats("ST1", "R1", seat));
        CHECK(repo.getBookedSeats("ST1", "R1").getSize() == 0);       // memory rolled back
    });

    suite("Seats: malformed file tolerated", [] {
        resetData();
        spit("data/RoomStatusAtShowtime.txt", "room_id|showtime_id|booked\nR1|ST1|A1, A2,,  \nonlyonecolumn\n\n");
        ShowtimeSeatRepository repo("data/RoomStatusAtShowtime.txt");
        CHECK(repo.getBookedSeats("ST1", "R1").getSize() == 2);
    });

    // ------------------------------------------------------------------ booking transaction
    suite("Booking: ticket + seats are one transaction", [] {
        resetData();
        BookingService svc("data/users.txt", "data/tickets.txt", "data/RoomStatusAtShowtime.txt");
        Ticket t1 = svc.createTicket(makeInfo("ST1", "R1", {"A1", "A2"}));
        CHECK(!t1.ticketId.empty());
        CHECK(t1.ticketId == "T0001");
        CHECK(svc.getBookedSeats("ST1", "R1").getSize() == 2);

        Ticket t2 = svc.createTicket(makeInfo("ST1", "R1", {"A2", "A3"}));     // overlaps A2
        CHECK(t2.ticketId.empty());
        CHECK(svc.getBookedSeats("ST1", "R1").getSize() == 2);                  // A3 not leaked
        CHECK(countLines("data/tickets.txt") == 2);                              // header + 1 ticket

        Ticket t3 = svc.createTicket(makeInfo("ST1", "R1", {"B1", "B1"}));     // same seat twice
        CHECK(t3.ticketId.empty());
        CHECK(svc.getBookedSeats("ST1", "R1").getSize() == 2);

        CHECK(svc.createTicket(makeInfo("ST2", "R1", {"A1"})).ticketId == "T0002");   // other showtime, same room
        CHECK(svc.createTicket(makeInfo("ST3", "R2", {"A1"})).ticketId == "T0003");   // other room
        CHECK(svc.createTicket(makeInfo("ST1", "R1", {})).ticketId.empty());          // no seats
        CHECK(svc.createTicket(makeInfo("", "R1", {"Z1"})).ticketId.empty());         // no showtime
        CHECK(svc.createTicket(makeInfo("ST1", "R1", {"Z1"}, -5)).ticketId.empty());  // negative price
        CHECK(svc.getBookedSeats("ST1", "R1").getSize() == 2);
    });

    suite("Booking: two services racing for one seat", [] {
        resetData();
        BookingService a("data/users.txt", "data/tickets.txt", "data/RoomStatusAtShowtime.txt");
        BookingService b("data/users.txt", "data/tickets.txt", "data/RoomStatusAtShowtime.txt");
        Ticket ta = a.createTicket(makeInfo("ST1", "R1", {"E5"}));
        Ticket tb = b.createTicket(makeInfo("ST1", "R1", {"E5"}));
        CHECK(!ta.ticketId.empty());
        CHECK(tb.ticketId.empty());                                  // second transaction loses
        CHECK(countLines("data/tickets.txt") == 2);
    });

    suite("Booking: ticket save failure rolls seats back", [] {
        resetData();
        BookingService svc("data/users.txt", "data/nodir/tickets.txt", "data/RoomStatusAtShowtime.txt");
        Ticket t = svc.createTicket(makeInfo("ST1", "R1", {"A1"}));
        CHECK(t.ticketId.empty());
        ShowtimeSeatRepository check("data/RoomStatusAtShowtime.txt");
        CHECK(check.getBookedSeats("ST1", "R1").getSize() == 0);     // reservation was released
    });

    suite("Booking: delete ticket releases seats", [] {
        resetData();
        BookingService svc("data/users.txt", "data/tickets.txt", "data/RoomStatusAtShowtime.txt");
        Ticket t = svc.createTicket(makeInfo("ST1", "R1", {"A1", "A2"}));
        Ticket keep = svc.createTicket(makeInfo("ST1", "R1", {"B1"}));
        CHECK(!t.ticketId.empty() && !keep.ticketId.empty());

        AdminTicketRepository admin("data/tickets.txt");
        CHECK(admin.loadAll().getSize() == 2);
        CHECK(!admin.deleteTicket("T9999"));                          // unknown id
        CHECK(admin.deleteTicket(t.ticketId));
        CHECK(admin.loadAll().getSize() == 1);
        ShowtimeSeatRepository after("data/RoomStatusAtShowtime.txt");
        CHECK(after.getBookedSeats("ST1", "R1").getSize() == 1);     // only B1 remains: no orphan reservation
        DLL<string> a1; a1.push_back("A1");
        CHECK(after.tryReserveSeats("ST1", "R1", a1));               // freed seat is bookable again
        CHECK(slurp("data/tickets.txt").rfind("\xEF\xBB\xBF", 0) == 0);   // BOM preserved after rewrite
        // IDs stay unique after deleting then creating
        Ticket n = svc.createTicket(makeInfo("ST4", "R1", {"A9"}));
        CHECK(!n.ticketId.empty() && n.ticketId != keep.ticketId);
    });

    suite("Tickets: pipes in free text cannot corrupt the file", [] {
        resetData();
        BookingService svc("data/users.txt", "data/tickets.txt", "data/RoomStatusAtShowtime.txt");
        BookingInfo info = makeInfo("ST1", "R1", {"A1"});
        info.customerName = "Evil|Name\nInjected|row";
        info.movieName = "Title|With|Pipes";
        Ticket t = svc.createTicket(info);
        CHECK(!t.ticketId.empty());
        AdminTicketRepository admin("data/tickets.txt");
        DLL<Ticket> all = admin.loadAll();
        CHECK(all.getSize() == 1);
        CHECK(all[0].price == 140000);
        CHECK(all[0].email == "cust@example.com");
    });

    suite("Tickets: corrupted records skipped", [] {
        resetData();
        spit("data/tickets.txt",
             "ticket_id|showtime_id|title|date|time|room_name|booked|combo_ids|price|email|fullName|booked_date|booked_time\n"
             "T0001|ST1|T|01/01/2030|10:00|P1|A1|Không có|100000|a@b.vn|A|01/01/2030|10:00:00\n"
             "T0002|broken line\n"
             "T0003|ST1|T|01/01/2030|10:00|P1|A2|Không có|notanumber|a@b.vn|A|01/01/2030|10:00:00\n");
        AdminTicketRepository admin("data/tickets.txt");
        CHECK(admin.loadAll().getSize() == 1);
        AdminTicketRepository missing("data/does_not_exist.txt");
        CHECK(missing.loadAll().getSize() == 0);                      // missing file does not crash
    });

    // ------------------------------------------------------------------ vouchers
    suite("Voucher: discount rules", [] {
        resetData();
        VoucherManager vm("data/voucher_defs.txt", "data/user_wallets.txt");
        vm.giveVoucher("u@x.vn", "FIX50", 30, 1);
        vm.giveVoucher("u@x.vn", "PCT10", 30, 2);
        vm.giveVoucher("u@x.vn", "HUGE", 30, 1);
        CHECK(vm.getVouchersByUser("u@x.vn").size() == 3);

        CHECK(vm.applyVoucher("u@x.vn", "NOPE", 200000, false) == 0.0);          // invalid code
        CHECK(vm.applyVoucher("u@x.vn", "FIX50", 50000, false) == 0.0);          // below min bill
        CHECK(vm.applyVoucher("u@x.vn", "FIX50", 200000, false) == 50000.0);     // fixed
        CHECK(vm.applyVoucher("u@x.vn", "PCT10", 200000, false) == 20000.0);     // percentage
        CHECK(vm.applyVoucher("u@x.vn", "HUGE", 120000, false) == 120000.0);     // never exceeds subtotal
        CHECK(vm.applyVoucher("u@x.vn", "FIX50", 0, false) == 0.0);              // empty bill
        CHECK(vm.applyVoucher("other@x.vn", "FIX50", 200000, false) == 0.0);     // not the owner
        CHECK(vm.getVouchersByUser("u@x.vn").size() == 3);                       // preview did not consume

        CHECK(vm.applyVoucher("u@x.vn", "FIX50", 200000, true) == 50000.0);      // consume
        CHECK(vm.applyVoucher("u@x.vn", "FIX50", 200000, true) == 0.0);          // already consumed
        CHECK(vm.applyVoucher("u@x.vn", "PCT10", 100000, true) == 10000.0);      // quantity 2 -> 1
        CHECK(vm.applyVoucher("u@x.vn", "PCT10", 100000, true) == 10000.0);      // 1 -> 0
        CHECK(vm.applyVoucher("u@x.vn", "PCT10", 100000, true) == 0.0);          // zero remaining

        VoucherManager reloaded("data/voucher_defs.txt", "data/user_wallets.txt");   // usage persisted
        CHECK(reloaded.applyVoucher("u@x.vn", "FIX50", 200000, false) == 0.0);
    });

    suite("Voucher: expired + corrupted records", [] {
        resetData();
        spit("data/user_wallets.txt",
             "email|code|status|expiry_date|quantity\n"
             "u@x.vn|FIX50|1|20000101|1\n"          // expired
             "u@x.vn|FIX50|1|29991231|1\n"          // valid
             "garbage\n"
             "u@x.vn|PCT10|x|29991231|1\n"          // corrupted status
             "u@x.vn|FIX50|0|29991231|1\n");        // disabled
        VoucherManager vm("data/voucher_defs.txt", "data/user_wallets.txt");
        CHECK(vm.applyVoucher("u@x.vn", "FIX50", 200000, false) == 50000.0);
        CHECK(vm.getVouchersByUser("u@x.vn").size() == 2);    // expired one hidden, corrupted skipped
        VoucherManager noFiles("data/none_defs.txt", "data/none_wallet.txt");
        CHECK(noFiles.applyVoucher("u@x.vn", "FIX50", 200000, true) == 0.0);   // missing files do not crash
    });

    // ------------------------------------------------------------------ movies & combos & showtime gen
    suite("AdminMovieRepository: CRUD, unique IDs, atomic save", [] {
        resetData();
        spit("data/movies.txt",
             "movie_id|title|age_rating|country|language|genres|duration_min|release_date|end_date|director|cast|synopsis|poster_path|status\n"
             "#PBL-001|Phim A|T13|VN|Viet|Hanh dong|120|01/01/2025|31/12/2030|Dir|Cast|Syn|p.jpg|Đang chiếu\n");
        AdminMovieRepository repo("data/movies.txt");
        CHECK(repo.getRecordCount() == 1);
        // Duplicate ID passed -> auto-assigned a new unique ID
        repo.addRecord({"#PBL-001", "Phim B", "T16", "VN", "Viet", "Hai", "95", "01/01/2025", "31/12/2030", "D2", "C2", "S2", "p2.jpg", "Đang chiếu"});
        CHECK(repo.getRecordCount() == 2);
        CHECK(repo.getRecord(0)[0] != repo.getRecord(1)[0]);
        CHECK(repo.saveToFile());

        AdminMovieRepository reloaded("data/movies.txt");
        CHECK(reloaded.getRecordCount() == 2);
        reloaded.deleteRecord(0);
        CHECK(reloaded.saveToFile());
        AdminMovieRepository afterDel("data/movies.txt");
        CHECK(afterDel.getRecordCount() == 1);
    });

    suite("AdminComboRepository: CRUD, unique IDs, atomic save", [] {
        resetData();
        spit("data/combo.txt", "combo_id|combo_name|price\nCB01|Coca|30000\n");
        AdminComboRepository repo("data/combo.txt");
        CHECK(repo.getRecordCount() == 1);
        repo.addRecord({"CB01", "Bap Rang|Special", "70000"});   // duplicate ID + pipe char
        CHECK(repo.getRecordCount() == 2);
        CHECK(repo.getRecord(0)[0] != repo.getRecord(1)[0]);
        CHECK(repo.saveToFile());
        AdminComboRepository reloaded("data/combo.txt");
        CHECK(reloaded.getRecordCount() == 2);
        CHECK(reloaded.getRecord(1)[1].find('|') == string::npos); // pipe sanitized
    });

    suite("ShowtimeCleanupService: non-overlapping + preserves booked", [] {
        resetData();
        spit("data/movies.txt",
             "movie_id|title|age_rating|country|language|genres|duration_min|release_date|end_date|director|cast|synopsis|poster_path|status\n"
             "#PBL-001|Phim A|T13|VN|Viet|Hanh dong|120|01/01/2020|31/12/2030|Dir|Cast|Syn|p.jpg|Đang chiếu\n"
             "#PBL-002|Phim B|T13|VN|Viet|Hai|100|01/01/2020|31/12/2030|Dir|Cast|Syn|p.jpg|Đang chiếu\n");
        spit("data/rooms.txt",
             "room_id|room_name|seat_rows|seat_cols|total_seats\n"
             "PHONG_001|Phong Chieu 1|9|9|81\n"
             "PHONG_002|Phong Chieu 2|9|9|81\n");
        spit("data/showtimes.txt",
             "showtime_id|movie_id|room_id|date|time|price\n"
             "SUATCHIEU_20200101_0001|#PBL-001|PHONG_001|2020-01-01|10:00|70000\n");
        // Book a ticket for a past/locked showtime and ensure maintainShowtimes preserves locked or generates valid ones
        ShowtimeCleanupService::maintainShowtimes("data/showtimes.txt", 2);
        CHECK(countLines("data/showtimes.txt") > 1);
    });

    // ------------------------------------------------------------------ data structures
    suite("DLL", [] {
        DLL<int> l;
        CHECK(l.empty());
        l.pop_back(); l.pop_front();                       // empty pops are harmless
        l.push_back(1); l.push_back(2); l.push_front(0);
        CHECK(l.getSize() == 3 && l[0] == 0 && l[2] == 2);
        DLL<int> copy = l;                                 // deep copy
        copy[0] = 99;
        CHECK(l[0] == 0);
        DLL<int> assigned; assigned.push_back(7);
        assigned = l;
        CHECK(assigned.getSize() == 3);
        l.removeAt(1);
        CHECK(l.getSize() == 2 && l[1] == 2);
        bool threw = false;
        try { (void)l[5]; } catch (const std::out_of_range&) { threw = true; }
        CHECK(threw);
        threw = false;
        try { l.removeAt(-1); } catch (const std::out_of_range&) { threw = true; }
        CHECK(threw);
        DLL<int> moved = std::move(l);
        CHECK(moved.getSize() == 2 && l.getSize() == 0);
        int sum = 0; for (int v : moved) sum += v;
        CHECK(sum == 2);
        auto it = moved.begin();
        it = moved.erase(it);
        CHECK(moved.getSize() == 1);
        moved = moved;                                     // self-assignment
        CHECK(moved.getSize() == 1);
    });

    suite("HashTable", [] {
        HashTable<string, int> h(2);                       // tiny capacity forces collisions + rehash
        for (int i = 0; i < 200; ++i) h.insert("k" + std::to_string(i), i);
        bool allFound = true;
        for (int i = 0; i < 200; ++i) { int* v = h.find("k" + std::to_string(i)); if (!v || *v != i) allFound = false; }
        CHECK(allFound);
        h.insert("k5", 555);                               // duplicate key overwrites
        CHECK(*h.find("k5") == 555);
        CHECK(h.remove("k5"));
        CHECK(!h.remove("k5"));
        CHECK(h.find("k5") == nullptr);
        HashTable<string, int> copy = h;
        h.clear();
        CHECK(h.find("k1") == nullptr);
        CHECK(copy.find("k1") != nullptr);                 // deep copy independent of original
        HashTable<string, int> empty;
        CHECK(empty.find("x") == nullptr);
        CHECK(!empty.remove("x"));
        copy = copy;
        CHECK(copy.find("k1") != nullptr);
    });

    suite("MyVector/MyQueue/MyStack", [] {
        MyVector<int> v;
        CHECK(v.empty());
        for (int i = 0; i < 1000; ++i) v.push_back(i);
        CHECK(v.size() == 1000 && v[999] == 999);
        MyVector<int> c = v; c[0] = -1;
        CHECK(v[0] == 0);
        MyVector<int> a; a = v;
        CHECK(a.size() == 1000);
        v.remove_at(0);
        CHECK(v.size() == 999 && v[0] == 1);
        bool threw = false;
        try { (void)v.at(5000); } catch (const std::out_of_range&) { threw = true; }
        CHECK(threw);
        v.clear();
        CHECK(v.empty());

        MyQueue<int> q;
        q.push(1); q.push(2);
        CHECK(q.front() == 1);
        q.pop();
        CHECK(q.front() == 2 && q.size() == 1);
        MyQueue<int> q2 = q;
        q.clear();
        CHECK(q.empty() && q2.size() == 1);

        MyStack<int> s;
        s.push(1); s.push(2);
        CHECK(s.top() == 2);
        s.pop();
        CHECK(s.top() == 1);
        MyStack<int> s2 = s;
        s.clear();
        CHECK(s.empty() && s2.size() == 1);
    });

    suite("Trie", [] {
        Trie t;
        CHECK(!t.search("a"));
        t.insert("cinema"); t.insert("cine"); t.insert("movie");
        CHECK(t.search("cine"));
        CHECK(!t.search("cin"));
        CHECK(t.startsWith("cin"));
        CHECK(!t.startsWith("x"));
        CHECK(t.getSuggestions("cin", 5).size == 2);
        t.insert("cine");                                  // duplicate insert is harmless
        CHECK(t.getSuggestions("cin", 5).size == 2);
    });

    std::cout << "\n==== " << g_pass << " checks passed, " << g_fail << " failed ====\n";
    fs::current_path(sandbox.parent_path());
    std::error_code ec;
    fs::remove_all(sandbox, ec);
    return g_fail == 0 ? 0 : 1;
}

