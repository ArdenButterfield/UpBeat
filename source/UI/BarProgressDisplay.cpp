//
// Created by Arden on 9/7/2026.
//

#include "BarProgressDisplay.h"

#include <algorithm>

BarProgressDisplay::BarProgressDisplay (int numBars)
{
    setInterceptsMouseClicks (false, false);
    barStates.assign ((size_t) numBars, BarState::NotStarted);
}

void BarProgressDisplay::setBarState (int barIndex, BarState state)
{
    if (barIndex < 0 || barIndex >= (int) barStates.size())
        return;

    barStates[(size_t) barIndex] = state;
    repaint();
}

void BarProgressDisplay::paint (juce::Graphics& g)
{
    constexpr int squareSize = 14;
    constexpr int spacing = 4;

    auto bounds = getLocalBounds();
    auto rowsPerColumn = std::max (1, bounds.getHeight() / (squareSize + spacing));

    for (int i = 0; i < (int) barStates.size(); ++i)
    {
        auto column = i / rowsPerColumn;
        auto row = i % rowsPerColumn;
        juce::Rectangle<int> square (bounds.getX() + column * (squareSize + spacing),
                                      bounds.getY() + row * (squareSize + spacing),
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
        g.drawRect (square);
    }
}
