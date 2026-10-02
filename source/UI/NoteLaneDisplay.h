//
// Created by Arden on 9/7/2026.
//

#ifndef UPBEAT_NOTELANEDISPLAY_H
#define UPBEAT_NOTELANEDISPLAY_H

#include "../GameState.h"
#include "ToleranceLabel.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include <map>

// The falling-note lane display shared by chart performance and chart practice: draws the
// lanes, notes, beats and barlines, lights up key indicators, and resolves key presses to
// the closest unplayed note in each lane.
//
// The events it displays are handed in by the owning scene as a time-keyed map of pointers
// into that scene's own chart data, so the owner decides what time coordinate system to use
// (a chart's own time in performance mode, or a practice queue's concatenated bar timeline
// in practice mode) - this component only ever compares against the map's keys, never
// against an event's own timeMs field, so it stays correct regardless of that choice.
class NoteLaneDisplay : public juce::Component
{
public:
    NoteLaneDisplay (GameState* gameState, int numLanes);

    void setEvents (const std::multimap<long long, ChartEvent*>* newEvents);
    void setNoteOnScreenVelocity (double pixelsPerMs);
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

    GameState* gameState;
    int numLanes;

    juce::Rectangle<int> laneOutline;
    std::vector<juce::Rectangle<int>> lanes;
    std::vector<juce::Rectangle<int>> buttonIndicators;
    std::vector<float> indicatorLighting;
    std::vector<std::vector<int>> keys; // key codes accepted by each lane

    const std::multimap<long long, ChartEvent*>* events = nullptr;
    double noteOnScreenVelocity = 0.2;
    long long timeMs = 0;

    juce::OwnedArray<ToleranceLabel> toleranceLabels;
};

#endif //UPBEAT_NOTELANEDISPLAY_H
