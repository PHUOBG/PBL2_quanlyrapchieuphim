#include "repositories/admin/AdminMovieRepository.h"
#include <filesystem>
#include <iostream>
#include <stdexcept>
int main() {
    namespace fs = std::filesystem;
    const fs::path temp = fs::path("build_audit_gnu/cinecore_movies_smoke.txt");
    fs::copy_file("data/movies.txt", temp, fs::copy_options::overwrite_existing);
    AdminMovieRepository repo(temp.string());
    auto data = repo.getAllData();
    if (data.size() != 9) throw std::runtime_error("expected 10 movie records");
    for (const auto& row : data) if (row.size() != 15) throw std::runtime_error("movie row schema is not 15 columns");
    bool found2025 = false;
    for (const auto& row : data) if (row[0] == "#PBL-009" && row[6] == "118" && row[12] == "../assets/posters/tu-chien-tren-khong.png") found2025 = true;
    if (!found2025) throw std::runtime_error("2025 record not loaded with its local poster");
    repo.addRecord({"#PBL-009", "Test duplicate handling", "T16", "VN", "VI", "Drama", "100", "01/01/2026", "", "Director", "Cast", "Synopsis", "", "Sắp chiếu", "2D"});
    if (repo.getAllData().back()[0] == "#PBL-009") throw std::runtime_error("duplicate movie ID accepted");
    if (!repo.saveToFile()) throw std::runtime_error("save failed");
    AdminMovieRepository reload(temp.string());
    if (reload.getAllData().size() != 10 || reload.getAllData().back()[1] != "Test duplicate handling") throw std::runtime_error("saved record failed roundtrip");
    fs::remove(temp);
    std::cout << "movie repository smoke: PASS (load real project data, 15-column schema, duplicate ID, save/reload)\n";
}


