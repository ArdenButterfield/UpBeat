//
// Created by Arden on 9/7/2026.
//

#include "BarProgressDisplay.h"

#include <algorithm>

BarProgressDisplay::BarProgressDisplay (int numBars)
{
    setInterceptsMouseClicks (false, false);
    barStates.assign ((size_t) numBars, BarState::NotStarted);
    successHistories.resize ((size_t) numBars);
}

void BarProgressDisplay::setBarState (int barIndex, BarState state)
{
    if (barIndex < 0 || barIndex >= (int) barStates.size())
        return;

    barStates[(size_t) barIndex] = state;
    repaint();
}

void BarProgressDisplay::setCurrentBar (int barIndex)
{
    if (barIndex == currentBar)
        return;

    currentBar = barIndex;
    repaint();
}

void BarProgressDisplay::setSuccessHistory (int barIndex, const std::vector<bool>& history)
{
    if (barIndex < 0 || barIndex >= (int) successHistories.size())
        return;

    successHistories[(size_t) barIndex] = history;
    repaint();
}

void BarProgressDisplay::paint (juce::Graphics& g)
{
    constexpr int squareSize = 14;
    constexpr int spacing = 4;
    constexpr int dotSize = 6;
    constexpr int dotSpacing = 2;

    auto bounds = getLocalBounds();
    auto dotsStartX = bounds.getX() + squareSize + spacing;
    auto maxDots = std::max (0, (bounds.getRight() - dotsStartX + dotSpacing) / (dotSize + dotSpacing));

    for (int i = 0; i < (int) barStates.size(); ++i)
    {
        juce::Rectangle<int> square (bounds.getX(),
                                      bounds.getY() + i * (squareSize + spacing),
                                      squareSize,
                                      squareSize);

        switch (barStates[(size_t) i])
        {
            case BarState::NotStarted:
                g.setColour (juce::Colours::transparentWhite);
                break;
            case BarState::InProgress:
                g.setColour (juce::Colours::yellow);
                break;
            case BarState::Learned:
                g.setColour (juce::Colours::green);
                break;
        }
        g.fillRect (square);
        g.setColour (juce::Colours::lightgrey);
        g.drawRect (square, i == currentBar ? 3 : 1);

        // If the history is longer than fits, show only the most recent reps.
        auto& history = successHistories[(size_t) i];
        auto firstShown = std::max (0, (int) history.size() - maxDots);
        for (int rep = firstShown; rep < (int) history.size(); ++rep)
        {
            auto dotX = dotsStartX + (rep - firstShown) * (dotSize + dotSpacing);
            auto dotY = square.getCentreY() - dotSize / 2;
            g.setColour (history[(size_t) rep] ? juce::Colours::green : juce::Colours::red);
            g.fillEllipse ((float) dotX, (float) dotY, (float) dotSize, (float) dotSize);
        }
    }
}
