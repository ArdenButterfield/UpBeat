//
// Created by Arden on 9/7/2026.
//

#ifndef UPBEAT_NOTELANEDISPLAY_H
#define UPBEAT_NOTELANEDISPLAY_H

#include "../GameState.h"
#include "ToleranceLabel.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include <functional>
#include <map>
#include <optional>

// The bar-by-bar note display shared by chart performance and chart practice. It shows
// four bars at once in a 2x2 grid of rows (filled left to right, then top to bottom),
// with a playhead sweeping across the current one. The two rows side by side touch, and
// share one border and one set of lane lines, like a line of sheet music. The grid line
// being played and the one after it are shown. Once a whole grid line has been played,
// it's cleared, and the bars after the next line fade into its place over the first bar
// of the next line: with bars 1,2 on top and 3,4 below, bars 5,6 fade in on top while
// bar 3 plays. Each row has a horizontal line per lane,
// and notes are lane-coloured circles on their lane's line, placed by their position in the bar.
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
    static constexpr int gridColumns = 2;
    static constexpr int gridLines = (numRows + gridColumns - 1) / gridColumns;

    // A bar in the event map's time coordinates.
    struct BarSpan
    {
        long long startMs;
        long long lengthMs;
    };

    NoteLaneDisplay (GameState* gameState, int numLanes);

    // The image tiled behind the rows, under their background colour; none by default.
    void setBackgroundTile (juce::Image newTile);
    // The game's own lane background tile, from BinaryData.
    static juce::Image defaultBackgroundTile();

    void setEvents (const std::multimap<long long, ChartEvent*>* newEvents);

    // newBars must be consecutive and in time order. firstBarNumber counts how many bars
    // came before newBars[0] in the sequence the player has been shown, and decides which
    // row each bar fills: bar n of the sequence always goes in row n % numRows, and in grid
    // line n / gridColumns of the sequence.
    void setBars (std::vector<BarSpan> newBars, int firstBarNumber);

    void setPlayheadTimeMs (long long newTimeMs);

    // The time on the chart's own timeline, which the chart's colour automation follows. It's
    // separate from the playhead because the event map may use another timeline (see above).
    void setChartTimeMs (double newChartTimeMs);

    // Decays key-hit lighting, ages/removes key press ripples and tolerance labels, and labels notes whose hit
    // window has closed without them being hit as missed. Call once per frame.
    void advance (double elapsedMs);

    // Labels every unplayed note before time (in the event map's time coordinates) as missed,
    // unless it already has been. advance does this as hit windows close; call it directly
    // before notes are taken out of the event map while they could still be hit.
    void labelMissedNotesBefore (long long time);

    // Looks up the lane for this key, finds the closest unplayed note in tolerance,
    // records the hit on it, and returns it so the caller can trigger playback -
    // or nullptr if the key isn't mapped to a lane.
    ChartEvent* registerKeyPress (const juce::KeyPress& key, long long hitTimeMs);

    // Called with each hit's timing error (negative for early) once registerKeyPress has
    // recorded it on its note.
    std::function<void (long long timingErrorMs)> onNoteTimed;

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    ChartEvent* findClosestNoteForHit (int lane, long long time);
    // Where a key press in a lane at a time is drawn: on the lane's line, where the playhead
    // was at that time (or in the playhead's bar, if the time is outside every bar).
    std::optional<juce::Point<float>> keyPressPosition (int lane, long long time) const;
    // Shows a tolerance category's image centred on centre, or on the display if there's none.
    ToleranceLabel* addToleranceLabel (int category, std::optional<juce::Point<float>> centre);
    // The line from where a hit note was placed to where its label is centred.
    std::vector<juce::Line<float>> hitLineSegments (long long eventTime, juce::Point<float> placed, juce::Point<float> centre) const;

    // The bar the playhead is in, clamped to the first/last bar outside the bars' range;
    // nullopt only when there are no bars.
    std::optional<size_t> playheadBarIndex() const;
    // The index of the bar containing time, or nullopt if it's outside every bar.
    std::optional<size_t> barIndexAt (long long time) const;
    int rowForBar (size_t barIndex) const;
    // Which grid line of the sequence a bar is in (see setBars).
    int sequenceLineForBar (size_t barIndex) const;
    // How opaque a bar's notes and beat lines are drawn: 1 for the grid line being played
    // and the one after it (once it has faded in), 0 for bars that aren't shown.
    float barOpacity (size_t barIndex) const;
    float xForTime (size_t barIndex, long long time) const;
    // The y of a lane's line within a row's inner area: lanes are spread evenly down the
    // row, lane 0 at the top. Always on a boundary between pixel rows, so the lane line
    // (GuiStyle::thinLine, an even number of pixels) drawn centred on it stays crisp, and
    // the notes on it are centred on it.
    float laneY (juce::Rectangle<float> area, int lane) const;
    // The part of a row notes, beat lines and the playhead are placed within. It's inset from
    // the border so notes at the very start or end of a line aren't clipped, but not where
    // the row meets its neighbour, so a line's bars run on from one to the next.
    juce::Rectangle<float> rowInner (int row) const;
    // The same for a whole grid line, which its lane lines run along.
    juce::Rectangle<float> gridLineInner (int gridLine) const;
    // Where a note is drawn: in its own bar's row, or riding the playhead once it has passed.
    std::optional<juce::Point<float>> notePosition (long long eventTime, const ChartEvent& event) const;
    // Where a note sits in its own bar's row, even once the playhead has passed it or the
    // row has been cleared; nullopt if it's outside every bar.
    std::optional<juce::Point<float>> notePlacedPosition (long long eventTime, const ChartEvent& event) const;
    std::optional<juce::Point<float>> playheadPosition() const;

    GameState* gameState;
    int numLanes;

    std::array<juce::Rectangle<int>, numRows> rows;
    std::array<juce::Rectangle<int>, gridLines> gridLineBounds; // the rows of each grid line together
    std::vector<juce::Rectangle<int>> buttonIndicators;
    std::vector<float> indicatorLighting;
    std::vector<std::vector<int>> keys; // key codes accepted by each lane

    const std::multimap<long long, ChartEvent*>* events = nullptr;
    std::vector<BarSpan> bars;
    int firstBarNumber = 0;
    long long timeMs = 0;
    double chartTimeMs = 0.0;
    // Every note before this time has been checked for a miss.
    long long missesLabelledBeforeMs = std::numeric_limits<long long>::min();

    juce::OwnedArray<ToleranceLabel> toleranceLabels;

public:
    // A circle spreading out from where a key was pressed, in its lane's colour, which grows
    // and fades over a second.
    struct Ripple
    {
        juce::Point<float> centre;
        int lane;
        double ageMs;

        float diameter() const;
        float alpha() const;
    };
    const std::vector<Ripple>& getRipples() const { return ripples; }

private:
    std::vector<Ripple> ripples;
    juce::Image backgroundTile;
};

#endif //UPBEAT_NOTELANEDISPLAY_H
