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
    constexpr int toleranceLabelWidth = 80;
    constexpr int toleranceLabelHeight = 20;

    // One lane takes the whole home row, two lanes split it between the hands (asdf / jkl;),
    // and anything wider gets one home-row key per lane, left to right.
    std::vector<std::vector<int>> keysForLanes (int numLanes)
    {
        if (numLanes == 1)
            return { { homeRowKeyCodes.begin(), homeRowKeyCodes.end() } };

        if (numLanes == 2)
            return { { 65, 83, 68, 70 }, { 74, 75, 76, 59 } };

        std::vector<std::vector<int>> result ((size_t) numLanes);
        for (size_t i = 0; i < result.size() && i < homeRowKeyCodes.size(); ++i)
            result[i] = { homeRowKeyCodes[i] };
        return result;
    }

    bool isPlayed (const ChartEvent& event)
    {
        return !event.performanceTimings.empty() && event.performanceTimings.back() != UNPLAYED_NOTE;
    }

    // The part of a row notes, beat lines and the playhead are placed within, inset so
    // notes at the very start or end of a bar, or at the pitch extremes, aren't clipped.
    juce::Rectangle<float> rowInner (juce::Rectangle<int> row)
    {
        return row.toFloat().reduced (noteRadius + 2.f);
    }
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
    timeMs = newTimeMs;
}

void NoteLaneDisplay::advance (double elapsedMs)
{
    for (auto& indicator : indicatorLighting)
        indicator = std::max (0.f, indicator - static_cast<float> (elapsedMs) * 0.001f);

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

    auto* message = gameState->getMessage (closeness);

    // The label takes the hit note's place on screen (found before the note is marked
    // played); a key press that hit nothing is labelled at the playhead.
    auto labelCentre = closestNote != nullptr ? notePosition (closestEventTime, *closestNote) : playheadPosition();
    auto* label = toleranceLabels.add (new ToleranceLabel (*message));
    label->setBounds (juce::Rectangle<int> (toleranceLabelWidth, toleranceLabelHeight)
                          .withCentre (labelCentre.value_or (getLocalBounds().getCentre().toFloat()).toInt()));
    addAndMakeVisible (label);

    if (closestNote != nullptr)
        closestNote->performanceTimings.back() = time - closestEventTime;

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

float NoteLaneDisplay::xForTime (size_t barIndex, long long time) const
{
    auto& bar = bars[barIndex];
    auto fraction = juce::jlimit (0.f, 1.f, static_cast<float> (time - bar.startMs) / static_cast<float> (std::max (1LL, bar.lengthMs)));
    auto area = rowInner (rows[(size_t) rowForBar (barIndex)]);
    return area.getX() + fraction * area.getWidth();
}

float NoteLaneDisplay::pitchFraction (size_t barIndex, int midiNote) const
{
    if (events == nullptr)
        return 0.5f;

    auto& bar = bars[barIndex];
    std::vector<int> pitches;
    for (auto it = events->lower_bound (bar.startMs); it != events->end() && it->first < bar.startMs + bar.lengthMs; ++it)
        if (it->second->type == ChartEvent::NOTE)
            pitches.push_back (it->second->midiNote);

    std::sort (pitches.begin(), pitches.end());
    pitches.erase (std::unique (pitches.begin(), pitches.end()), pitches.end());

    if (pitches.size() <= 1)
        return 0.5f;

    auto rank = std::lower_bound (pitches.begin(), pitches.end(), midiNote) - pitches.begin();
    return static_cast<float> (rank) / static_cast<float> (pitches.size() - 1);
}

std::optional<juce::Point<float>> NoteLaneDisplay::notePosition (long long eventTime, const ChartEvent& event) const
{
    auto barIndex = barIndexAt (eventTime);
    auto currentBar = playheadBarIndex();
    if (!barIndex || !currentBar)
        return std::nullopt;

    auto yFraction = pitchFraction (*barIndex, event.midiNote);

    // Once the playhead has passed a note, it rides along with the playhead - into the
    // next bar's row if need be - keeping the height it had in its own bar.
    auto drawnBar = eventTime < timeMs ? *currentBar : *barIndex;
    auto drawnTime = eventTime < timeMs ? timeMs : eventTime;

    if (drawnBar < *currentBar || drawnBar >= *currentBar + numRows)
        return std::nullopt;

    auto area = rowInner (rows[(size_t) rowForBar (drawnBar)]);
    return juce::Point<float> (xForTime (drawnBar, drawnTime), area.getBottom() - yFraction * area.getHeight());
}

std::optional<juce::Point<float>> NoteLaneDisplay::playheadPosition() const
{
    auto currentBar = playheadBarIndex();
    if (!currentBar)
        return std::nullopt;

    return juce::Point<float> (xForTime (*currentBar, timeMs), rows[(size_t) rowForBar (*currentBar)].toFloat().getCentreY());
}

juce::Colour NoteLaneDisplay::laneColour (int lane) const
{
    static const std::array<juce::Colour, 10> palette {
        juce::Colours::hotpink,
        juce::Colours::deepskyblue,
        juce::Colours::gold,
        juce::Colours::limegreen,
        juce::Colours::orange,
        juce::Colours::mediumpurple,
        juce::Colours::turquoise,
        juce::Colours::tomato,
        juce::Colours::khaki,
        juce::Colours::lightsteelblue,
    };
    return palette[(size_t) std::max (0, lane) % palette.size()];
}

void NoteLaneDisplay::paint (juce::Graphics& g)
{
    auto currentBar = playheadBarIndex();
    auto currentRow = currentBar ? rowForBar (*currentBar) : -1;

    for (int i = 0; i < numRows; ++i)
    {
        g.setColour (juce::Colours::black.withAlpha (0.25f));
        g.fillRect (rows[(size_t) i]);
        g.setColour (i == currentRow ? juce::Colours::white : juce::Colours::grey);
        g.drawRect (rows[(size_t) i], i == currentRow ? 2 : 1);
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

    auto drawNote = [&] (long long eventTime, const ChartEvent& event) {
        if (auto position = notePosition (eventTime, event))
        {
            g.setColour (laneColour (event.inputButton));
            g.fillEllipse (juce::Rectangle<float> (noteRadius * 2.f, noteRadius * 2.f).withCentre (*position));
        }
    };

    // The current bar and the ones after it that still fit on screen. Notes the playhead
    // has already passed are left to the riding-notes pass below.
    for (auto barIndex = *currentBar; barIndex < bars.size() && barIndex < *currentBar + numRows; ++barIndex)
    {
        auto& bar = bars[barIndex];
        auto area = rowInner (rows[(size_t) rowForBar (barIndex)]);

        for (auto it = events->lower_bound (bar.startMs); it != events->end() && it->first < bar.startMs + bar.lengthMs; ++it)
        {
            auto& [eventTime, event] = *it;
            if (event->type == ChartEvent::BEAT || event->type == ChartEvent::BARLINE)
            {
                g.setColour (juce::Colours::white.withAlpha (event->type == ChartEvent::BARLINE ? 0.3f : 0.15f));
                g.drawVerticalLine (juce::roundToInt (xForTime (barIndex, eventTime)), area.getY(), area.getBottom());
            }
        }

        for (auto it = events->lower_bound (std::max (bar.startMs, timeMs)); it != events->end() && it->first < bar.startMs + bar.lengthMs; ++it)
            if (it->second->type == ChartEvent::NOTE && !isPlayed (*it->second))
                drawNote (it->first, *it->second);
    }

    // Unplayed notes the playhead has passed travel with it until their hit window closes.
    auto totalHitWindow = gameState->tolerances[GameState::NUM_TOLERANCE_CATEGORIES - 1];
    for (auto it = events->lower_bound (timeMs - totalHitWindow); it != events->end() && it->first < timeMs; ++it)
        if (it->second->type == ChartEvent::NOTE && !isPlayed (*it->second))
            drawNote (it->first, *it->second);

    if (barIndexAt (timeMs))
    {
        auto row = rows[(size_t) currentRow].toFloat();
        g.setColour (juce::Colours::white);
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

    auto rowHeight = (bounds.getHeight() - rowGap * (numRows - 1)) / numRows;
    for (int i = 0; i < numRows; ++i)
        rows[(size_t) i] = bounds.withHeight (rowHeight).withY (bounds.getY() + i * (rowHeight + rowGap));

    auto indicatorW = buttonIndicators.empty() ? indicatorsArea.getWidth() : indicatorsArea.getWidth() / (int) buttonIndicators.size();
    for (size_t i = 0; i < buttonIndicators.size(); ++i)
        buttonIndicators[i] = indicatorsArea.withWidth (indicatorW).withX (indicatorsArea.getX() + (int) i * indicatorW).reduced (2, 0);
}
