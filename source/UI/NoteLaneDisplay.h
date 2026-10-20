//
// Created by Arden on 9/7/2026.
//

#ifndef UPBEAT_NOTELANEDISPLAY_H
#define UPBEAT_NOTELANEDISPLAY_H

#include "../GameState.h"
#include "ToleranceLabel.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include <map>
#include <optional>

// The bar-by-bar note display shared by chart performance and chart practice. It shows
// four bars at once as stacked rows, with a playhead sweeping across the current one;
// once a bar is complete its row is refilled with the next bar to come, so the rows
// cycle (bars 1,2,3,4 -> 5,2,3,4 -> 5,6,3,4 ...). Notes are lane-coloured circles placed
// by their position in the bar (x) and their pitch relative to the bar's other notes (y).
// It also lights up key indicators and resolves key presses to the closest unplayed note
// in each lane.
//
// The events it displays are handed in by the owning scene as a time-keyed map of pointers
// into that scene's own chart data, along with the bars those times fall into, so the owner
// decides what time coordinate system to use (a chart's own time in performance mode, or a
// practice queue's concatenated bar timeline in practice mode) - this component only ever
// compares against the map's keys, never against an event's own timeMs field, so it stays
// correct regardless of that choice.
class NoteLaneDisplay : public juce::Component
{
public:
    static constexpr int numRows = 4;

    // A bar in the event map's time coordinates.
    struct BarSpan
    {
        long long startMs;
        long long lengthMs;
    };

    NoteLaneDisplay (GameState* gameState, int numLanes);

    void setEvents (const std::multimap<long long, ChartEvent*>* newEvents);

    // newBars must be consecutive and in time order. firstBarNumber counts how many bars
    // came before newBars[0] in the sequence the player has been shown, and decides which
    // row each bar fills: bar n of the sequence always goes in row n % numRows.
    void setBars (std::vector<BarSpan> newBars, int firstBarNumber);

    void setPlayheadTimeMs (long long newTimeMs);

    // Decays key-hit lighting and ages/removes tolerance labels. Call once per frame.
    void advance (double elapsedMs);

    // Looks up the lane for this key, finds the closest unplayed note in tolerance,
    // records the hit (or miss) on it, and returns it so the caller can trigger playback -
    // or nullptr if the key isn't mapped to a lane.
    ChartEvent* registerKeyPress (const juce::KeyPress& key, long long hitTimeMs);

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    ChartEvent* findClosestNoteForHit (int lane, long long time);

    // The bar the playhead is in, clamped to the first/last bar outside the bars' range;
    // nullopt only when there are no bars.
    std::optional<size_t> playheadBarIndex() const;
    // The index of the bar containing time, or nullopt if it's outside every bar.
    std::optional<size_t> barIndexAt (long long time) const;
    int rowForBar (size_t barIndex) const;
    float xForTime (size_t barIndex, long long time) const;
    // 0 for the lowest distinct pitch among the bar's notes, 1 for the highest.
    float pitchFraction (size_t barIndex, int midiNote) const;
    // Where a note is drawn: in its own bar's row, or riding the playhead once it has passed.
    std::optional<juce::Point<float>> notePosition (long long eventTime, const ChartEvent& event) const;
    std::optional<juce::Point<float>> playheadPosition() const;
    juce::Colour laneColour (int lane) const;

    GameState* gameState;
    int numLanes;

    std::array<juce::Rectangle<int>, numRows> rows;
    std::vector<juce::Rectangle<int>> buttonIndicators;
    std::vector<float> indicatorLighting;
    std::vector<std::vector<int>> keys; // key codes accepted by each lane

    const std::multimap<long long, ChartEvent*>* events = nullptr;
    std::vector<BarSpan> bars;
    int firstBarNumber = 0;
    long long timeMs = 0;

    juce::OwnedArray<ToleranceLabel> toleranceLabels;
};

#endif //UPBEAT_NOTELANEDISPLAY_H
