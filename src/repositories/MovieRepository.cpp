#include "repositories/MovieRepository.h"

#include <fstream>
#include <sstream>
#include <vector>

void MovieRepository::loadMovies() {
    std::ifstream file(filePath.toAnsiString());
    if (!file.is_open()) return;

    std::string line;
    std::getline(file, line);

    while (std::getline(file, line)) {
        if (line.empty()) continue;

        std::stringstream ss(line);
        std::string token;
        std::vector<std::string> cols;
        while (std::getline(ss, token, '|')) {
            cols.push_back(token);
        }
        if (cols.size() < 12) continue;

        MovieDetail m;
        m.movie_id      = sf::String(cols[0]);
        m.title         = sf::String::fromUtf8(cols[1].begin(), cols[1].end());
        m.age_rating    = sf::String::fromUtf8(cols[2].begin(), cols[2].end());
        m.country       = sf::String::fromUtf8(cols[3].begin(), cols[3].end());
        m.language      = sf::String::fromUtf8(cols[4].begin(), cols[4].end());
        m.genres        = sf::String::fromUtf8(cols[5].begin(), cols[5].end());
        m.duration_min  = sf::String::fromUtf8(cols[6].begin(), cols[6].end());
        m.release_date  = sf::String::fromUtf8(cols[7].begin(), cols[7].end());

        // 14-col format inserts end_date at index 8
        const bool hasEndDate = (cols.size() >= 14);
        const size_t directorIdx = hasEndDate ? 9 : 8;
        const size_t castIdx     = hasEndDate ? 10 : 9;
        const size_t synIdx      = hasEndDate ? 11 : 10;
        const size_t posterIdx   = hasEndDate ? 12 : 11;
        const size_t statusIdx   = hasEndDate ? 13 : 12;

        if (cols.size() > posterIdx) {
            m.director   = sf::String::fromUtf8(cols[directorIdx].begin(), cols[directorIdx].end());
            m.cast       = sf::String::fromUtf8(cols[castIdx].begin(), cols[castIdx].end());
            m.synopsis   = sf::String::fromUtf8(cols[synIdx].begin(), cols[synIdx].end());
            m.posterPath = sf::String::fromUtf8(cols[posterIdx].begin(), cols[posterIdx].end());
        }

        // Optional status (index depends on schema)
        if (cols.size() > statusIdx) {
            m.status = sf::String::fromUtf8(cols[statusIdx].begin(), cols[statusIdx].end());
        } else {
            m.status = L"";
        }

        movies.push_back(m);
    }

    file.close();
}

MovieRepository::MovieRepository(const sf::String& path) : filePath(path) {
    loadMovies();
}

sf::String MovieRepository::getMovieTitleById(const sf::String& movieId) const {
    for (int i = 0; i < movies.getSize(); ++i) {
        if (movies[i].movie_id == movieId) {
            return movies[i].title;
        }
    }
    return movieId;
}

DLL<sf::String> MovieRepository::getPosterPaths() {
    DLL<sf::String> paths;
    for (int i = 0; i < movies.getSize(); i++)
        paths.push_back(movies[i].posterPath);
    return paths;
}

void MovieRepository::setSelectedIndex(int index) {
    selectedIndex = index;
}

int MovieRepository::getSelectedIndex() const {
    return selectedIndex;
}

MovieDetail MovieRepository::getMovieDetailbyIndex(int index) const {
    if (index >= 0 && index < (int)movies.getSize())
        return movies[index];
    return MovieDetail();
}

const DLL<MovieDetail>& MovieRepository::getAllMovies() const {
    return movies;
}
