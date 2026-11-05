#ifndef UPBEAT_GAMECOLOURS_H
#define UPBEAT_GAMECOLOURS_H

#include "juce_graphics/juce_graphics.h"

// The colours the game draws with, owned by GameState. NoteLaneDisplay's colours belong to
// each chart instead (see ChartColours).
struct GameColours
{
    // Scene and panel backgrounds
    juce::Colour titleBackground = juce::Colours::darkblue;
    juce::Colour menuBackground = juce::Colours::black; // Chart selection and feedback scenes
    juce::Colour playBackground = juce::Colours::darkgrey; // Chart performance and practice scenes
    juce::Colour panelBackground = juce::Colours::black.withAlpha (0.9f);

    // Text
    juce::Colour text = juce::Colours::white;
    juce::Colour errorText = juce::Colours::red;

    // BarProgressDisplay
    juce::Colour barNotStarted = juce::Colours::transparentWhite;
    juce::Colour barInProgress = juce::Colours::yellow;
    juce::Colour barLearned = juce::Colours::green;
    juce::Colour barOutline = juce::Colours::lightgrey;
    juce::Colour repSuccess = juce::Colours::green;
    juce::Colour repFailure = juce::Colours::red;

    // LineGraph
    juce::Colour graphBackground = juce::Colours::black;
    juce::Colour graphForeground = juce::Colours::white; // Axes, ticks and tick labels
    std::array<juce::Colour, 8> graphSeriesPalette {
        juce::Colours::red,
        juce::Colours::dodgerblue,
        juce::Colours::limegreen,
        juce::Colours::orange,
        juce::Colours::mediumpurple,
        juce::Colours::cyan,
        juce::Colours::yellow,
        juce::Colours::hotpink,
    };
};

#endif //UPBEAT_GAMECOLOURS_H
