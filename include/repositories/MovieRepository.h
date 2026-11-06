#pragma once

#include "UI/components/PosterSlider/IPosterProvider.h"
#include "data-structures/DLL.h"
#include "services/MovieSearchManager.h"

#include <SFML/System/String.hpp>

#include <string>

class MovieRepository : public IPosterProvider {
private:
    String filePath;
    DLL<MovieDetail> movies;
    int selectedIndex = -1;

    void loadMovies();

public:
    MovieRepository(const String& path);
    String getMovieTitleById(const String& movieId) const;

    DLL<String> getPosterPaths() override;

    void setSelectedIndex(int index);
    int getSelectedIndex() const;

    MovieDetail getMovieDetailbyIndex(int index) const;

    const DLL<MovieDetail>& getAllMovies() const;
};
