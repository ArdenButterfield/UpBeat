//
// Created by Arden on 7/9/2026.
//

#ifndef UPBEAT_GAMESTATE_H
#define UPBEAT_GAMESTATE_H
#include "Chart/Chart.h"
#include "GameColours.h"

#define UNPLAYED_NOTE (-99999)

struct GameState
{
    std::vector<Chart> charts;
    Chart* currentChart;
    GameColours colours;
    static constexpr int NUM_TOLERANCE_CATEGORIES = 3;

    // The tolerance category a hit this far from its note falls in, or NUM_TOLERANCE_CATEGORIES
    // for a miss.
    [[nodiscard]] int getCategory (long long difference) const
    {
        for (int i = 0; i < NUM_TOLERANCE_CATEGORIES; ++i)
        {
            if (std::abs (difference) < tolerances[(size_t) i])
                return i;
        }
        return NUM_TOLERANCE_CATEGORIES;
    }
    std::array<long long, NUM_TOLERANCE_CATEGORIES> tolerances {30, 60, 140};
    std::array<std::string, NUM_TOLERANCE_CATEGORIES+1> toleranceLabels {"100", "Fresh", "Ok", "Miss"};
};

static_assert (std::tuple_size_v<decltype (GameColours::histogramBars)> == GameState::NUM_TOLERANCE_CATEGORIES, "Each tolerance category needs its own histogram colour");

#endif //UPBEAT_GAMESTATE_H
