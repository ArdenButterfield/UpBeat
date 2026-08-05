//
// Created by Arden on 7/9/2026.
//

#ifndef UPBEAT_CHARTEVENT_H
#define UPBEAT_CHARTEVENT_H
#include "Chart.h"
#include "juce_audio_basics/juce_audio_basics.h"
#include <vector>

struct ChartEvent
{
    enum eventType
    {
        NULL_EVENT = 0,
        BARLINE,
        BEAT,
        NOTE,
        BACKGROUND_NOTE
    };

    ChartEvent(const juce::MidiMessageSequence::MidiEventHolder* event, int numInputLanes)
    {
        midiNote = event->message.getNoteNumber();
        lengthMs = static_cast<long long>(event->noteOffObject->message.getTimeStamp() * 1000 - event->message.getTimeStamp() * 1000);
        timeMs = static_cast<long long>(event->message.getTimeStamp() * 1000);
        inputButton = (event->message.getNoteNumber() % numInputLanes);
        type = NOTE;
    }

    ChartEvent(long long timeMs, eventType type) : timeMs (timeMs), type (type), lengthMs (0), midiNote (0), inputButton (-1)
    {
    }

    long long timeMs;
    eventType type;
    long long lengthMs;
    int midiNote;
    int inputButton;
    std::vector<long long> performanceTimings;
};

#endif //UPBEAT_CHARTEVENT_H
