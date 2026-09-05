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

    // Converts real elapsed time (ms since performance start) into chart time (ms), applying
    // tempo scale and the fixed count-in offset. This must be the only place that does this
    // conversion, and it must always be computed fresh from the total elapsed time rather than
    // accumulated frame-by-frame: accumulating rounded per-frame deltas loses a fraction of a
    // millisecond every call whenever tempoScale isn't exactly 1, and that loss compounds over
    // time, making whichever clock accumulates deltas drift away from a clock (like the audio
    // thread's sample counter) that recomputes from scratch each time.
    static long long chartTimeForRealElapsedMs (long long realElapsedMs, double tempoScale, long long countInTime)
    {
        return static_cast<long long> (static_cast<double> (realElapsedMs) * tempoScale) - countInTime;
    }

    juce::String name;
    juce::MemoryBlock midiData;
    std::multimap<long long, ChartEvent> events;
    int numLanes;

    long long countInTime = 2000;

    // Multiplies real elapsed time when advancing chart playback, letting the player
    // practice at a slower or faster speed than the chart's original tempo.
    double tempoScale = 1.0;

    // Pixels per millisecond that notes travel down the screen during performance.
    double noteOnScreenVelocity = 0.2;
};

#endif //UPBEAT_CHART_H
