//
// Created by Arden on 9/7/2026.
//

#include "NoteLaneDisplay.h"

#include <cstdlib>

namespace
{
    // Central row of the keyboard: lane 0 maps to 'a', lane 1 to 's', and so on.
    constexpr std::array<int, 10> laneKeyCodes = { 65, 83, 68, 70, 71, 72, 74, 75, 76, 59 };
}

NoteLaneDisplay::NoteLaneDisplay (GameState* gs, int numLanes_) : gameState (gs), numLanes (numLanes_)
{
    // All input is via the owning scene's keyPresses, never the mouse - and this display
    // sits on top of scene controls (like the start button) that do need clicks to land.
    setInterceptsMouseClicks (false, false);

    lanes.resize ((size_t) numLanes);
    buttonIndicators.resize ((size_t) numLanes);
    indicatorLighting.assign ((size_t) numLanes, 0.f);
    keys.resize ((size_t) numLanes);
    for (int i = 0; i < numLanes; ++i)
        keys[(size_t) i] = (i < (int) laneKeyCodes.size()) ? laneKeyCodes[(size_t) i] : -1;
}

void NoteLaneDisplay::setEvents (const std::multimap<long long, ChartEvent*>* newEvents)
{
    events = newEvents;
}

void NoteLaneDisplay::setNoteOnScreenVelocity (double pixelsPerMs)
{
    noteOnScreenVelocity = pixelsPerMs;
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
        if (juce::KeyPress::isKeyCurrentlyDown (keys[i]))
            indicatorLighting[i] = 1;
        else if (indicatorLighting[i] > 0.5f)
            indicatorLighting[i] = 0.5f;
    }
}

ChartEvent* NoteLaneDisplay::registerKeyPress (const juce::KeyPress& key, long long hitTimeMs)
{
    for (size_t i = 0; i < keys.size(); ++i)
    {
        if (keys[i] == key.getKeyCode())
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

    constexpr int toleranceLabelHeight = 20;
    auto laneBounds = lanes[(size_t) lane];
    auto* label = toleranceLabels.add (new ToleranceLabel (*message));
    label->setBounds (laneBounds.getX(), laneBounds.getBottom() - toleranceLabelHeight, laneBounds.getWidth(), toleranceLabelHeight);
    addAndMakeVisible (label);

    if (closestNote != nullptr)
        closestNote->performanceTimings.back() = time - closestEventTime;

    return closestNote;
}

void NoteLaneDisplay::paint (juce::Graphics& g)
{
    g.setColour (juce::Colours::lightgrey);
    g.drawRect (laneOutline);
    for (auto& lane : lanes)
        g.drawRect (lane);

    for (size_t i = 0; i < buttonIndicators.size(); ++i)
    {
        g.setColour (juce::Colours::white.withAlpha (indicatorLighting[i]));
        g.fillRect (buttonIndicators[i]);
    }

    if (events == nullptr || lanes.empty())
        return;

    for (auto& [eventTime, event] : *events)
    {
        long eventYPosition;
        if (event->type == ChartEvent::NOTE)
        {
            // Notes arrive at the bottom of the screen exactly on time, and hang there
            // until the "Perfect" tolerance window closes rather than sliding past.
            auto departureTime = eventTime + gameState->tolerances[0];

            if (timeMs > departureTime)
                continue;

            auto clampedTime = std::min (timeMs, eventTime);
            eventYPosition = static_cast<long> ((clampedTime - eventTime) * noteOnScreenVelocity) + lanes[0].getBottom();
        }
        else
        {
            eventYPosition = static_cast<long> ((timeMs - eventTime) * noteOnScreenVelocity) + lanes[0].getBottom();
        }

        if (eventYPosition > lanes[0].getBottom() || eventYPosition < lanes[0].getY())
            continue;

        if (event->type == ChartEvent::NOTE)
        {
            g.setColour (juce::Colours::pink);
            g.drawRect (lanes[(size_t) event->inputButton].withY (eventYPosition - 3L).withHeight (6));
        }
        else if (event->type == ChartEvent::BEAT)
        {
            g.setColour (juce::Colours::grey);
            g.drawHorizontalLine (eventYPosition, static_cast<float> (lanes[0].getX()), static_cast<float> (lanes.back().getRight()));
        }
        else if (event->type == ChartEvent::BARLINE)
        {
            g.setColour (juce::Colours::white);
            g.drawHorizontalLine (eventYPosition, static_cast<float> (lanes[0].getX()), static_cast<float> (lanes.back().getRight()));
        }
    }
}

void NoteLaneDisplay::resized()
{
    laneOutline = getLocalBounds();
    auto lanesInner = laneOutline.reduced (5).withTrimmedBottom (30);
    auto indicatorsInner = laneOutline.reduced (5).withTop (lanesInner.getBottom() + 5);
    auto laneW = lanes.empty() ? lanesInner.getWidth() : lanesInner.getWidth() / (int) lanes.size();
    for (size_t i = 0; i < lanes.size(); ++i)
    {
        lanes[i] = lanesInner.withWidth (laneW).withX (lanesInner.getX() + (int) i * laneW);
        buttonIndicators[i] = indicatorsInner.withWidth (laneW).withX (lanesInner.getX() + (int) i * laneW);
    }
}
