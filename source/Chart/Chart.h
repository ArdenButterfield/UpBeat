//
// Created by Arden on 7/9/2026.
//

#ifndef UPBEAT_CHART_H
#define UPBEAT_CHART_H
#include "ChartEvent.h"
#include "juce_core/juce_core.h"
#include <map>
#include <utility>
#include <vector>

// Describes one (track, channel) pair found in a MIDI file, and how many notes it contains.
struct TrackChannelInfo
{
    int track;
    int channel;
    int noteCount;
};

class Chart
{
public:
    // selectedTrackChannels lists the (track, channel) pairs the user chose to make playable,
    // in order: the first pair becomes lane 0, the second lane 1, and so on. Every other note
    // in the file becomes a background note.
    Chart (const juce::MemoryBlock& midiData, const juce::String& name, const std::vector<std::pair<int, int>>& selectedTrackChannels);
    ~Chart();

    // Scans a MIDI file and reports the (track, channel) pairs it contains and how many
    // note-on events are in each, so the UI can offer them to the user for selection.
    static std::vector<TrackChannelInfo> analyzeTrackChannels (const juce::MemoryBlock& midiData);

    juce::String name;
    juce::MemoryBlock midiData;
    std::multimap<long long, ChartEvent> events;
    int numLanes;

    long long countInTime = 2000;
};

#endif //UPBEAT_CHART_H
