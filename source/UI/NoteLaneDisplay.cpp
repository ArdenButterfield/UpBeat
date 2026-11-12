//
// Created by Arden on 9/7/2026.
//

#include "NoteLaneDisplay.h"

#include <cstdlib>

namespace
{
    // Central row of the keyboard: 'a' 's' 'd' 'f' 'g' 'h' 'j' 'k' 'l' ';'.
    constexpr std::array<int, 10> homeRowKeyCodes = { 65, 83, 68, 70, 71, 72, 74, 75, 76, 59 };

    constexpr float noteRadius = 7.f;
    constexpr int toleranceLabelWidth = 80;  // the hit images are fitted inside this, keeping their shape
    constexpr int toleranceLabelHeight = 44;

    // Charts set colours in Oklab, with no alpha, so how see-through these are is fixed here.
    constexpr float rowBackgroundAlpha = 0.25f;
    constexpr float barlineAlpha = 0.3f;
    constexpr float beatLineAlpha = 0.15f;
    constexpr float laneLineAlpha = 0.5f;

    // One lane takes the whole home row; two lanes (Chart::MAX_LANES) split it between the hands (asdf / jkl;).
    std::vector<std::vector<int>> keysForLanes (int numLanes)
    {
        if (numLanes == 1)
            return { { homeRowKeyCodes.begin(), homeRowKeyCodes.end() } };

        if (numLanes == 2)
            return { { 65, 83, 68, 70 }, { 74, 75, 76, 59 } };

        return {};
    }

    bool isPlayed (const ChartEvent& event)
    {
        return !event.performanceTimings.empty() && event.performanceTimings.back() != UNPLAYED_NOTE;
    }

    // How far notes, beat lines and the playhead are kept from a border.
    constexpr float borderInset = noteRadius + 2.f;
}

NoteLaneDisplay::NoteLaneDisplay (GameState* gs, int numLanes_) : gameState (gs), numLanes (numLanes_)
{
    // All input is via the owning scene's keyPresses, never the mouse - and this display
    // sits on top of scene controls (like the start button) that do need clicks to land.
    setInterceptsMouseClicks (false, false);

    buttonIndicators.resize ((size_t) numLanes);
    indicatorLighting.assign ((size_t) numLanes, 0.f);
    keys = keysForLanes (numLanes);
}

void NoteLaneDisplay::setEvents (const std::multimap<long long, ChartEvent*>* newEvents)
{
    events = newEvents;
}

void NoteLaneDisplay::setBars (std::vector<BarSpan> newBars, int newFirstBarNumber)
{
    bars = std::move (newBars);
    firstBarNumber = newFirstBarNumber;
}

void NoteLaneDisplay::setPlayheadTimeMs (long long newTimeMs)
{
    // The playhead only goes back when a new run starts, whose notes all need checking for misses.
    if (newTimeMs < timeMs)
        missesLabelledBeforeMs = std::numeric_limits<long long>::min();

    timeMs = newTimeMs;
}

void NoteLaneDisplay::labelMissedNotesBefore (long long time)
{
    if (events == nullptr || time <= missesLabelledBeforeMs)
        return;

    for (auto it = events->lower_bound (missesLabelledBeforeMs); it != events->end() && it->first < time; ++it)
    {
        auto& [eventTime, event] = *it;
        if (event->type == ChartEvent::NOTE && !event->performanceTimings.empty() && !isPlayed (*event))
        {
            auto position = notePosition (eventTime, *event);
            addToleranceLabel (GameState::NUM_TOLERANCE_CATEGORIES, position ? position : playheadPosition());
        }
    }

    missesLabelledBeforeMs = time;
}

void NoteLaneDisplay::addToleranceLabel (int category, std::optional<juce::Point<float>> centre)
{
    auto* label = toleranceLabels.add (new ToleranceLabel (category));
    label->setBounds (juce::Rectangle<int> (toleranceLabelWidth, toleranceLabelHeight)
                          .withCentre (centre.value_or (getLocalBounds().getCentre().toFloat()).toInt()));
    addAndMakeVisible (label);
}

void NoteLaneDisplay::setChartTimeMs (double newChartTimeMs)
{
    chartTimeMs = newChartTimeMs;
}

void NoteLaneDisplay::advance (double elapsedMs)
{
    for (auto& indicator : indicatorLighting)
        indicator = std::max (0.f, indicator - static_cast<float> (elapsedMs) * 0.001f);

    labelMissedNotesBefore (timeMs - gameState->tolerances[GameState::NUM_TOLERANCE_CATEGORIES - 1]);

    for (int i = toleranceLabels.size() - 1; i >= 0; --i)
    {
        if (toleranceLabels[i]->advance (elapsedMs))
            toleranceLabels.remove (i);
    }

    for (size_t i = 0; i < keys.size(); ++i)
    {
        if (std::any_of (keys[i].begin(), keys[i].end(), [] (int k) { return juce::KeyPress::isKeyCurrentlyDown (k); }))
            indicatorLighting[i] = 1;
        else if (indicatorLighting[i] > 0.5f)
            indicatorLighting[i] = 0.5f;
    }
}

ChartEvent* NoteLaneDisplay::registerKeyPress (const juce::KeyPress& key, long long hitTimeMs)
{
    for (size_t i = 0; i < keys.size(); ++i)
    {
        if (std::find (keys[i].begin(), keys[i].end(), key.getKeyCode()) != keys[i].end())
        {
            indicatorLighting[i] = 1;
            return findClosestNoteForHit ((int) i, hitTimeMs);
        }
    }
    return nullptr;
}

ChartEvent* NoteLaneDisplay::findClosestNoteForHit (int lane, long long time)
{
    if (events == nullptr)
        return nullptr;

    auto totalHitWindow = gameState->tolerances[GameState::NUM_TOLERANCE_CATEGORIES - 1];
    auto closeness = totalHitWindow + 1;
    ChartEvent* closestNote = nullptr;
    long long closestEventTime = 0;

    for (auto it = events->lower_bound (time - totalHitWindow);
         it != events->end() && it->first < time + totalHitWindow;
         ++it)
    {
        auto* event = it->second;
        if (event->type == ChartEvent::NOTE
            && event->inputButton == lane
            && std::abs (it->first - time) < closeness
            && !event->performanceTimings.empty()
            && event->performanceTimings.back() == UNPLAYED_NOTE)
        {
            closeness = std::abs (it->first - time);
            closestNote = event;
            closestEventTime = it->first;
        }
    }

    // A key press that hits no note isn't labelled: misses are labelled on the notes that
    // were never hit, once their hit window closes (see labelMissedNotesBefore).
    if (closestNote != nullptr)
    {
        // The label takes the hit note's place on screen (found before the note is marked played).
        addToleranceLabel (gameState->getCategory (closeness), notePosition (closestEventTime, *closestNote));
        closestNote->performanceTimings.back() = time - closestEventTime;
        if (onNoteTimed)
            onNoteTimed (time - closestEventTime);
    }

    return closestNote;
}

std::optional<size_t> NoteLaneDisplay::playheadBarIndex() const
{
    if (bars.empty())
        return std::nullopt;

    for (size_t i = 0; i < bars.size(); ++i)
        if (timeMs < bars[i].startMs + bars[i].lengthMs)
            return i;

    return bars.size() - 1;
}

std::optional<size_t> NoteLaneDisplay::barIndexAt (long long time) const
{
    for (size_t i = 0; i < bars.size(); ++i)
        if (time >= bars[i].startMs && time < bars[i].startMs + bars[i].lengthMs)
            return i;

    return std::nullopt;
}

int NoteLaneDisplay::rowForBar (size_t barIndex) const
{
    return (firstBarNumber + (int) barIndex) % numRows;
}

int NoteLaneDisplay::sequenceLineForBar (size_t barIndex) const
{
    return (firstBarNumber + (int) barIndex) / gridColumns;
}

float NoteLaneDisplay::barOpacity (size_t barIndex) const
{
    auto currentBar = playheadBarIndex();
    if (! currentBar || barIndex >= bars.size())
        return 0.f;

    auto currentLine = sequenceLineForBar (*currentBar);
    auto line = sequenceLineForBar (barIndex);
    if (line < currentLine || line >= currentLine + gridLines)
        return 0.f;

    // The last line shown takes the place of the line just finished, so it fades in over the
    // first bar of the current line - except at the very start, when it replaces nothing.
    if (line < currentLine + gridLines - 1 || currentLine == 0)
        return 1.f;

    auto lineStartIndex = currentLine * gridColumns - firstBarNumber;
    if (lineStartIndex < 0)
        return 1.f; // That first bar finished before these bars were handed in.

    auto& firstBar = bars[(size_t) lineStartIndex];
    auto progress = static_cast<float> (timeMs - firstBar.startMs) / static_cast<float> (std::max (1LL, firstBar.lengthMs));
    return juce::jlimit (0.f, 1.f, progress);
}

float NoteLaneDisplay::xForTime (size_t barIndex, long long time) const
{
    auto& bar = bars[barIndex];
    auto fraction = juce::jlimit (0.f, 1.f, static_cast<float> (time - bar.startMs) / static_cast<float> (std::max (1LL, bar.lengthMs)));
    auto area = rowInner (rowForBar (barIndex));
    return area.getX() + fraction * area.getWidth();
}

juce::Rectangle<float> NoteLaneDisplay::rowInner (int row) const
{
    auto area = rows[(size_t) row].toFloat().reduced (0.f, borderInset);
    if (row % gridColumns == 0)
        area.removeFromLeft (borderInset);
    if (row % gridColumns == gridColumns - 1)
        area.removeFromRight (borderInset);
    return area;
}

juce::Rectangle<float> NoteLaneDisplay::gridLineInner (int gridLine) const
{
    return gridLineBounds[(size_t) gridLine].toFloat().reduced (borderInset);
}

float NoteLaneDisplay::laneY (juce::Rectangle<float> area, int lane) const
{
    auto lanes = std::max (1, numLanes);
    auto fraction = static_cast<float> (juce::jlimit (0, lanes - 1, lane) + 1) / static_cast<float> (lanes + 1);
    return std::floor (area.getY() + fraction * area.getHeight()) + 0.5f;
}

std::optional<juce::Point<float>> NoteLaneDisplay::notePosition (long long eventTime, const ChartEvent& event) const
{
    auto barIndex = barIndexAt (eventTime);
    auto currentBar = playheadBarIndex();
    if (!barIndex || !currentBar)
        return std::nullopt;

    // Once the playhead has passed a note, it rides along with the playhead - into the
    // next bar's row if need be - staying on its lane's line.
    auto drawnBar = eventTime < timeMs ? *currentBar : *barIndex;
    auto drawnTime = eventTime < timeMs ? timeMs : eventTime;

    if (barOpacity (drawnBar) <= 0.f)
        return std::nullopt;

    auto area = rowInner (rowForBar (drawnBar));
    return juce::Point<float> (xForTime (drawnBar, drawnTime), laneY (area, event.inputButton));
}

std::optional<juce::Point<float>> NoteLaneDisplay::playheadPosition() const
{
    auto currentBar = playheadBarIndex();
    if (!currentBar)
        return std::nullopt;

    return juce::Point<float> (xForTime (*currentBar, timeMs), rows[(size_t) rowForBar (*currentBar)].toFloat().getCentreY());
}

void NoteLaneDisplay::paint (juce::Graphics& g)
{
    auto currentBar = playheadBarIndex();
    auto currentRow = currentBar ? rowForBar (*currentBar) : -1;

    const auto palette = gameState->currentChart->colours.paletteAt (chartTimeMs);
    auto laneColour = [&palette] (int lane) {
        return palette[(size_t) (LaneColours::lane0 + juce::jlimit (0, Chart::MAX_LANES - 1, lane))];
    };

    for (int i = 0; i < gridLines; ++i)
    {
        auto isCurrent = currentRow >= 0 && currentRow / gridColumns == i;
        g.setColour (palette[LaneColours::rowBackground].withAlpha (rowBackgroundAlpha));
        g.fillRect (gridLineBounds[(size_t) i]);
        g.setColour (palette[isCurrent ? LaneColours::rowActiveOutline : LaneColours::rowOutline]);
        g.drawRect (gridLineBounds[(size_t) i], isCurrent ? 2 : 1);

        auto area = gridLineInner (i);
        for (int lane = 0; lane < numLanes; ++lane)
        {
            g.setColour (laneColour (lane).withAlpha (laneLineAlpha));
            g.drawHorizontalLine (static_cast<int> (laneY (area, lane)), area.getX(), area.getRight());
        }
    }

    for (size_t i = 0; i < buttonIndicators.size(); ++i)
    {
        auto colour = laneColour ((int) i);
        g.setColour (colour.withAlpha (indicatorLighting[i]));
        g.fillRect (buttonIndicators[i]);
        g.setColour (colour);
        g.drawRect (buttonIndicators[i]);
    }

    if (events == nullptr || !currentBar)
        return;

    auto drawNote = [&] (long long eventTime, const ChartEvent& event, float opacity) {
        if (auto position = notePosition (eventTime, event))
        {
            g.setColour (laneColour (event.inputButton).withMultipliedAlpha (opacity));
            g.fillEllipse (juce::Rectangle<float> (noteRadius * 2.f, noteRadius * 2.f).withCentre (*position));
        }
    };

    // Every bar of the grid lines shown, from the start of the current one, so the bars
    // already played in it keep their beat lines. Notes the playhead has already passed are
    // left to the riding-notes pass below.
    auto firstShownBar = (size_t) std::max (0, sequenceLineForBar (*currentBar) * gridColumns - firstBarNumber);
    for (auto barIndex = firstShownBar; barIndex < bars.size(); ++barIndex)
    {
        auto opacity = barOpacity (barIndex);
        if (opacity <= 0.f)
        {
            if (barIndex > *currentBar)
                break;
            continue;
        }

        auto& bar = bars[barIndex];
        auto area = rowInner (rowForBar (barIndex));

        for (auto it = events->lower_bound (bar.startMs); it != events->end() && it->first < bar.startMs + bar.lengthMs; ++it)
        {
            auto& [eventTime, event] = *it;
            if (event->type == ChartEvent::BEAT || event->type == ChartEvent::BARLINE)
            {
                g.setColour (event->type == ChartEvent::BARLINE ? palette[LaneColours::barline].withAlpha (barlineAlpha * opacity) : palette[LaneColours::beatLine].withAlpha (beatLineAlpha * opacity));
                g.drawVerticalLine (juce::roundToInt (xForTime (barIndex, eventTime)), area.getY(), area.getBottom());
            }
        }

        for (auto it = events->lower_bound (std::max (bar.startMs, timeMs)); it != events->end() && it->first < bar.startMs + bar.lengthMs; ++it)
            if (it->second->type == ChartEvent::NOTE && !isPlayed (*it->second))
                drawNote (it->first, *it->second, opacity);
    }

    // Unplayed notes the playhead has passed travel with it until their hit window closes.
    auto totalHitWindow = gameState->tolerances[GameState::NUM_TOLERANCE_CATEGORIES - 1];
    for (auto it = events->lower_bound (timeMs - totalHitWindow); it != events->end() && it->first < timeMs; ++it)
        if (it->second->type == ChartEvent::NOTE && !isPlayed (*it->second))
            drawNote (it->first, *it->second, 1.f);

    if (barIndexAt (timeMs))
    {
        auto row = rows[(size_t) currentRow].toFloat();
        g.setColour (palette[LaneColours::playhead]);
        g.drawLine (xForTime (*currentBar, timeMs), row.getY(), xForTime (*currentBar, timeMs), row.getBottom(), 2.f);
    }
}

void NoteLaneDisplay::resized()
{
    constexpr int rowGap = 6;
    constexpr int indicatorHeight = 20;

    auto bounds = getLocalBounds().reduced (5);
    auto indicatorsArea = bounds.removeFromBottom (indicatorHeight);
    bounds.removeFromBottom (rowGap);

    // Row i sits in grid column i % gridColumns and grid line i / gridColumns, so bars
    // read left to right, then top to bottom. Grid lines have a gap between them; the rows
    // within one touch.
    auto rowWidth = bounds.getWidth() / gridColumns;
    auto rowHeight = (bounds.getHeight() - rowGap * (gridLines - 1)) / gridLines;
    for (int line = 0; line < gridLines; ++line)
        gridLineBounds[(size_t) line] = bounds.withHeight (rowHeight).withY (bounds.getY() + line * (rowHeight + rowGap));

    for (int i = 0; i < numRows; ++i)
    {
        auto line = gridLineBounds[(size_t) (i / gridColumns)];
        auto column = i % gridColumns;
        // The last column takes any width left over from dividing the line.
        auto width = column == gridColumns - 1 ? line.getWidth() - column * rowWidth : rowWidth;
        rows[(size_t) i] = line.withX (line.getX() + column * rowWidth).withWidth (width);
    }

    auto indicatorW = buttonIndicators.empty() ? indicatorsArea.getWidth() : indicatorsArea.getWidth() / (int) buttonIndicators.size();
    for (size_t i = 0; i < buttonIndicators.size(); ++i)
        buttonIndicators[i] = indicatorsArea.withWidth (indicatorW).withX (indicatorsArea.getX() + (int) i * indicatorW).reduced (2, 0);
}
