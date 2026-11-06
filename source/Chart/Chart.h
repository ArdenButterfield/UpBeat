//
// Created by Arden on 7/9/2026.
//

#ifndef UPBEAT_CHART_H
#define UPBEAT_CHART_H
#include "Automation.h"
#include "ChartColours.h"
#include "ChartEvent.h"
#include "juce_core/juce_core.h"
#include <map>
#include <optional>
#include <vector>

// One Odin2 synth from the chart's Ableton set, in either the PLAYABLE or BACKGROUND group.
struct ChartSynth
{
    enum Group
    {
        PLAYABLE,
        BACKGROUND
    };

    int id;
    juce::String name;
    Group group;
    juce::MemoryBlock state; // Odin2 plugin state, decoded from the chart's base64
};

class Chart
{
public:
    // Parses a chart in the YAML format described in Ableton Charts/chart_format.md.
    // The notes of PLAYABLE synths are the playable notes; each note's lane comes from its own
    // "lane" field (default 0), independent of which synth plays it. numLanes is the chart's
    // "lanes" field, grown to fit every note's lane. Both are limited to MAX_LANES: a higher lane count
    // is reduced, and notes in higher lanes move to the last lane. Returns std::nullopt if the data
    // isn't a valid chart.
    static std::optional<Chart> fromYaml (const juce::String& yamlText);

    static constexpr int MAX_LANES = 2;

    // Whether the metronome clicks on this beat (counted from 1) of a bar in this time
    // signature. Compound time (6/8, 9/8, 12/8) clicks only on the first of each group of
    // three beats; every other time signature clicks on every beat.
    static bool metronomeClicksOnBeat (int numerator, int denominator, int beat);
    static_assert (LaneColours::lane1 - LaneColours::lane0 + 1 == MAX_LANES, "Each lane needs its own colour");

    juce::String name;
    std::vector<ChartSynth> synths;
    std::multimap<long long, ChartEvent> events;
    // One lane per automated (synth, parameter) pair, built from the chart's AutomationEvents.
    std::vector<AutomationLane> automation;
    int numLanes = 0;
    ChartColours colours;

    // Multiplies real elapsed time when advancing chart playback, letting the player
    // practice at a slower or faster speed than the chart's original tempo.
    double tempoScale = 1.0;
};

#endif //UPBEAT_CHART_H
