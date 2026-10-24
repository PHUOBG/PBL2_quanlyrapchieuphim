#pragma once
#include <string>
#include <vector>
#include <map>
#include <SFML/Graphics.hpp>
#include "data-structures/DLL.h"
#include "../include/data-structures/Trie.h"

using namespace std;
using namespace sf;

struct MovieDetail {
    String movie_id;
    wstring title, age_rating, country, language, genres, duration_min, release_date, director, cast, synopsis, posterPath;
    wstring status;
};

struct SearchResult {
    string title;
    int movieIndex;
};

class MovieSearchManager {
private:
    Trie trie;
    DLL<MovieDetail> movies;
    map<string, vector<int>> titleToIndexMap;
    
    string normalizeString(const string& str);
    
public:
    MovieSearchManager();
    void loadMovies(const DLL<MovieDetail>& movieList);
    vector<SearchResult> searchMovies(const string& query, int limit = 5);
    int getMovieCount() const;
};
