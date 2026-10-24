#include "services/MovieSearchManager.h"
#include <algorithm>
#include <cctype>
#include <locale>
#include <codecvt>

int MovieSearchManager::getMovieCount() const {
    return movies.getSize();
}

// Map Vietnamese characters to lowercase without diacritics
string MovieSearchManager::normalizeString(const string& str) {
    string result;
    
    for (size_t i = 0; i < str.length(); ) {
        unsigned char c = str[i];
        
        // ASCII characters - just lowercase
        if (c < 128) {
            result += tolower(c);
            i++;
        }
        // 2-byte UTF-8 sequences
        else if ((c & 0xE0) == 0xC0 && i + 1 < str.length()) {
            unsigned char c1 = str[i], c2 = str[i + 1];
            
            // Đ/đ (U+0110/U+0111) -> d
            if (c1 == 0xC4 && (c2 == 0x90 || c2 == 0x91)) {
                result += 'd';
            }
            // À/à (U+00C0/U+00E0) -> a
            else if (c1 == 0xC3 && (c2 == 0x80 || c2 == 0xA0)) {
                result += 'a';
            }
            // Á/á (U+00C1/U+00E1) -> a
            else if (c1 == 0xC3 && (c2 == 0x81 || c2 == 0xA1)) {
                result += 'a';
            }
            // Â/â (U+00C2/U+00E2) -> a
            else if (c1 == 0xC3 && (c2 == 0x82 || c2 == 0xA2)) {
                result += 'a';
            }
            // Ã/ã (U+00C3/U+00E3) -> a
            else if (c1 == 0xC3 && (c2 == 0x83 || c2 == 0xA3)) {
                result += 'a';
            }
            // È/è (U+00C8/U+00E8) -> e
            else if (c1 == 0xC3 && (c2 == 0x88 || c2 == 0xA8)) {
                result += 'e';
            }
            // É/é (U+00C9/U+00E9) -> e
            else if (c1 == 0xC3 && (c2 == 0x89 || c2 == 0xA9)) {
                result += 'e';
            }
            // Ê/ê (U+00CA/U+00EA) -> e
            else if (c1 == 0xC3 && (c2 == 0x8A || c2 == 0xAA)) {
                result += 'e';
            }
            // Ì/ì (U+00CC/U+00EC) -> i
            else if (c1 == 0xC3 && (c2 == 0x8C || c2 == 0xAC)) {
                result += 'i';
            }
            // Í/í (U+00CD/U+00ED) -> i
            else if (c1 == 0xC3 && (c2 == 0x8D || c2 == 0xAD)) {
                result += 'i';
            }
            // Ò/ò (U+00D2/U+00F2) -> o
            else if (c1 == 0xC3 && (c2 == 0x92 || c2 == 0xB2)) {
                result += 'o';
            }
            // Ó/ó (U+00D3/U+00F3) -> o
            else if (c1 == 0xC3 && (c2 == 0x93 || c2 == 0xB3)) {
                result += 'o';
            }
            // Ô/ô (U+00D4/U+00F4) -> o
            else if (c1 == 0xC3 && (c2 == 0x94 || c2 == 0xB4)) {
                result += 'o';
            }
            // Õ/õ (U+00D5/U+00F5) -> o
            else if (c1 == 0xC3 && (c2 == 0x95 || c2 == 0xB5)) {
                result += 'o';
            }
            // Ù/ù (U+00D9/U+00F9) -> u
            else if (c1 == 0xC3 && (c2 == 0x99 || c2 == 0xB9)) {
                result += 'u';
            }
            // Ú/ú (U+00DA/U+00FA) -> u
            else if (c1 == 0xC3 && (c2 == 0x9A || c2 == 0xBA)) {
                result += 'u';
            }
            // Ý/ý (U+00DD/U+00FD) -> y
            else if (c1 == 0xC3 && (c2 == 0x9D || c2 == 0xBD)) {
                result += 'y';
            }
            // Ơ/ơ (U+01A0/U+01A1) -> o
            else if (c1 == 0xC6 && (c2 == 0xA0 || c2 == 0xA1)) {
                result += 'o';
            }
            // Ư/ư (U+01AF/U+01B0) -> u
            else if (c1 == 0xC6 && (c2 == 0xAF || c2 == 0xB0)) {
                result += 'u';
            }
            // Ă/ă (U+0102/U+0103) -> a
            else if (c1 == 0xC4 && (c2 == 0x82 || c2 == 0x83)) {
                result += 'a';
            }
            else {
                // Keep other 2-byte sequences as-is (lowercase if possible)
                result += tolower(c1);
                result += tolower(c2);
            }
            i += 2;
        }
        // 3-byte UTF-8 sequences (Vietnamese with tone marks)
        else if ((c & 0xF0) == 0xE0 && i + 2 < str.length()) {
            unsigned char c1 = str[i], c2 = str[i + 1], c3 = str[i + 2];
            
            // Vietnamese characters in E1 BA/BB range
            if (c1 == 0xE1) {
                // Ạ-Ặ, ạ-ặ (U+1EA0-U+1EB7) -> a
                if ((c2 == 0xBA && c3 >= 0xA0) || (c2 == 0xBB && c3 <= 0x97)) {
                    result += 'a';
                }
                // Ẹ-Ệ, ẹ-ệ (U+1EB8-U+1EC7) -> e
                else if (c2 == 0xBA && c3 >= 0xB8) {
                    result += 'e';
                }
                else if (c2 == 0xBB && c3 <= 0x87) {
                    result += 'e';
                }
                // Ỉ-Ị, ỉ-ị (U+1EC8-U+1ECB) -> i
                else if (c2 == 0xBB && c3 >= 0x88 && c3 <= 0x8B) {
                    result += 'i';
                }
                // Ọ-Ợ, ọ-ợ (U+1ECC-U+1EE3) -> o
                else if (c2 == 0xBB && c3 >= 0x8C && c3 <= 0xA3) {
                    result += 'o';
                }
                // Ụ-Ự, ụ-ự (U+1EE4-U+1EF1) -> u
                else if (c2 == 0xBB && c3 >= 0xA4 && c3 <= 0xB1) {
                    result += 'u';
                }
                // Ỳ-Ỵ, ỳ-ỵ (U+1EF2-U+1EF9) -> y
                else if (c2 == 0xBB && c3 >= 0xB2 && c3 <= 0xB9) {
                    result += 'y';
                }
                else {
                    // Keep as-is
                    result += c1; result += c2; result += c3;
                }
            }
            else {
                // Keep as-is
                result += c1; result += c2; result += c3;
            }
            i += 3;
        }
        // 4-byte UTF-8
        else if ((c & 0xF8) == 0xF0 && i + 3 < str.length()) {
            result += str[i]; result += str[i+1]; result += str[i+2]; result += str[i+3];
            i += 4;
        }
        else {
            result += c;
            i++;
        }
    }
    
    return result;
}

MovieSearchManager::MovieSearchManager() {}

void MovieSearchManager::loadMovies(const DLL<MovieDetail>& movieList) {
    movies = movieList;
    titleToIndexMap.clear();
    
    // Build the Trie and index map
    for (int i = 0; i < movies.getSize(); i++) {
        // Convert sf::String to std::string UTF-8
        sf::String sfTitle = movies[i].title;
        auto utf8 = sfTitle.toUtf8();
        string title(reinterpret_cast<const char*>(utf8.data()), utf8.size());
        
        string normalized = normalizeString(title);
        
        // Insert into Trie
        trie.insert(normalized);
        
        // Map normalized title to movie index
        titleToIndexMap[normalized].push_back(i);
    }
}

vector<SearchResult> MovieSearchManager::searchMovies(const string& query, int limit) {
    vector<SearchResult> results;
    
    if (query.empty()) {
        return results;
    }
    
    string normalizedQuery = normalizeString(query);
    
    // Try prefix search with Trie first
    StringArray suggestions = trie.getSuggestions(normalizedQuery, limit * 3);
    
    // Convert suggestions to SearchResult with movie indices
    for (int i = 0; i < suggestions.size; i++) {
        const char* suggestion = suggestions.get(i);
        if (titleToIndexMap.find(suggestion) != titleToIndexMap.end()) {
            const vector<int>& indices = titleToIndexMap[suggestion];
            for (int idx : indices) {
                if (results.size() >= (size_t)limit) break;
                SearchResult result;
                
                // Convert sf::String to std::string UTF-8
                sf::String sfTitle = movies[idx].title;
                auto utf8 = sfTitle.toUtf8();
                result.title = string(reinterpret_cast<const char*>(utf8.data()), utf8.size());
                
                result.movieIndex = idx;
                results.push_back(result);
            }
        }
        if (results.size() >= (size_t)limit) break;
    }
    
    // If no results from Trie, fallback to substring matching
    if (results.empty()) {
        for (int i = 0; i < movies.getSize() && results.size() < (size_t)limit; i++) {
            // Convert sf::String to std::string UTF-8
            sf::String sfTitle = movies[i].title;
            auto utf8 = sfTitle.toUtf8();
            string title(reinterpret_cast<const char*>(utf8.data()), utf8.size());
            
            string normalized = normalizeString(title);
            if (normalized.find(normalizedQuery) != string::npos) {
                SearchResult result;
                result.title = title;
                result.movieIndex = i;
                results.push_back(result);
            }
        }
    }
    
    return results;
}
