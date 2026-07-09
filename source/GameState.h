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
    static const int numberOfInputLanes = 4;
};

#endif //UPBEAT_GAMESTATE_H
