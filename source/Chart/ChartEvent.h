//
// Created by Arden on 7/9/2026.
//

#ifndef UPBEAT_CHARTEVENT_H
#define UPBEAT_CHARTEVENT_H
#include "juce_core/juce_core.h"
#include <vector>

struct ChartEvent
{
    enum eventType
    {
        NULL_EVENT = 0,
        BARLINE,
        BEAT,
        NOTE,
        BACKGROUND_NOTE,
        GUI_CHANGE
    };

    ChartEvent(long long timeMs, eventType type) : timeMs (timeMs), type (type), lengthMs (0), midiNote (0), inputButton (-1)
    {
    }

    long long timeMs;
    eventType type;
    long long lengthMs;
    int midiNote;
    // The lane a NOTE belongs to; -1 for every other event type.
    int inputButton;
    int velocity = 0;
    // The id of the chart synth a NOTE or BACKGROUND_NOTE plays on; -1 otherwise.
    int synthId = -1;

    // GUI_CHANGE: the GUI track name (e.g. "section titles") and the clip name.
    juce::String guiEventType;
    juce::String guiValue;

    std::vector<long long> performanceTimings;
};

#endif //UPBEAT_CHARTEVENT_H
