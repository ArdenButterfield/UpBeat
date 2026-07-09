//
// Created by Arden on 7/9/2026.
//

#ifndef UPBEAT_CHARTEVENT_H
#define UPBEAT_CHARTEVENT_H
#include <vector>

struct ChartEvent
{
    enum eventType
    {
        NULL_EVENT = 0,
        BARLINE,
        BEATLINE,
        NOTE
    };

    int timeMs;
    eventType type;
    int lengthMs;
    char midiNote;
    char inputButton;
    std::vector<int> performanceTimings;
};

#endif //UPBEAT_CHARTEVENT_H
