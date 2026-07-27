//
// Created by Arden on 7/9/2026.
//

#ifndef UPBEAT_GAMESTATE_H
#define UPBEAT_GAMESTATE_H
#include "Chart/Chart.h"

struct GameState
{
    std::vector<Chart> charts;
    Chart* currentChart;
    static constexpr int numberOfInputLanes = 4;
    static constexpr int NUM_TOLERANCE_CATEGORIES = 4;
    std::array<long, NUM_TOLERANCE_CATEGORIES> tolerances {100, 200, 400, 800};
    std::array<std::string, NUM_TOLERANCE_CATEGORIES> toleranceLabels {"Perfect", "Great", "Good", "Ok"};
};

#endif //UPBEAT_GAMESTATE_H
