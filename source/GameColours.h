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

    // TimingHistogram
    juce::Colour histogramBackground = juce::Colours::black.withAlpha (0.25f);
    juce::Colour histogramAxis = juce::Colours::white; // The perfect-timing line; category edges are drawn fainter
    juce::Colour histogramMedian = juce::Colours::white; // The median's dotted line and its label
    juce::Colour histogramQuartile = juce::Colours::white.withAlpha (0.5f); // The quartiles' dotted lines and labels
    // One per tolerance category, from 100 to Ok: the colour of each one's hit image.
    std::array<juce::Colour, 3> histogramBars {
        juce::Colour (0xffc6f9df), // hit_100.png: the average of its oval's gradient
        juce::Colour (0xfff00000), // hit_fresh.png
        juce::Colour (0xffe6b05a), // hit_ok.png
    };

    // LineGraph
    juce::Colour graphBackground = juce::Colours::black;
    juce::Colour graphForeground = juce::Colours::white; // Axes, ticks and tick labels
    std::array<juce::Colour, 8> graphSeriesPalette {
        juce::Colours::red,
        juce::Colours::dodgerblue,
        juce::Colour (0xffc6f9df), // hit_100.png: the average of its oval's gradient
        juce::Colour (0xffe6b05a), // hit_ok.png
        juce::Colours::mediumpurple,
        juce::Colours::cyan,
        juce::Colours::yellow,
        juce::Colours::hotpink,
    };
};

#endif //UPBEAT_GAMECOLOURS_H
