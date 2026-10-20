//
// Created by Arden on 7/9/2026.
//

#ifndef UPBEAT_CHART_H
#define UPBEAT_CHART_H
#include "Automation.h"
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
    // Each PLAYABLE synth becomes a lane, in the order the synths are listed: its notes are
    // the playable notes of that lane. Returns std::nullopt if the data isn't a valid chart.
    static std::optional<Chart> fromYaml (const juce::String& yamlText);

    juce::String name;
    std::vector<ChartSynth> synths;
    std::multimap<long long, ChartEvent> events;
    // One lane per automated (synth, parameter) pair, built from the chart's AutomationEvents.
    std::vector<AutomationLane> automation;
    int numLanes = 0;

    long long countInTime = 2000;

    // Multiplies real elapsed time when advancing chart playback, letting the player
    // practice at a slower or faster speed than the chart's original tempo.
    double tempoScale = 1.0;
};

#endif //UPBEAT_CHART_H
