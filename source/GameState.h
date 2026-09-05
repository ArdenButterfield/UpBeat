//
// Created by Arden on 7/9/2026.
//

#ifndef UPBEAT_GAMESTATE_H
#define UPBEAT_GAMESTATE_H
#include "Chart/Chart.h"

#define UNPLAYED_NOTE (-99999)

struct GameState
{
    std::vector<Chart> charts;
    Chart* currentChart;
    static constexpr int NUM_TOLERANCE_CATEGORIES = 4;

    [[nodiscard]] const std::string* getMessage (long long difference) const
    {
        for (int i = 0; i < NUM_TOLERANCE_CATEGORIES; ++i)
        {
            if (abs(difference) < tolerances[i])
            {
                return &toleranceLabels[i];
            }
        }
        return &toleranceLabels.back();
    }
    std::array<long long, NUM_TOLERANCE_CATEGORIES> tolerances {100, 200, 400, 800};
    std::array<std::string, NUM_TOLERANCE_CATEGORIES+1> toleranceLabels {"Perfect", "Great", "Good", "Ok", "Miss"};
};

#endif //UPBEAT_GAMESTATE_H
