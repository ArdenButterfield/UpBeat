//
// Created by Arden on 9/7/2026.
//

#ifndef UPBEAT_BARPROGRESSDISPLAY_H
#define UPBEAT_BARPROGRESSDISPLAY_H

#include "juce_gui_basics/juce_gui_basics.h"
#include <vector>

// The small square-per-bar progress graphic shown next to the lanes during chart
// practice, one row per bar in a single column: each square starts out transparent, turns yellow once its bar has been
// introduced for practice, and turns green once the bar is fully learned. The bar
// currently being practiced is drawn with a heavier outline.
class BarProgressDisplay : public juce::Component
{
public:
    enum class BarState
    {
        NotStarted,
        InProgress,
        Learned
    };

    explicit BarProgressDisplay (int numBars);

    void setBarState (int barIndex, BarState state);

    // Highlights barIndex as the bar currently being practiced; pass -1 for none.
    void setCurrentBar (int barIndex);

    // Per-rep results for barIndex, drawn as a row of green (success) / red (failure) dots
    // to the right of its square.
    void setSuccessHistory (int barIndex, const std::vector<bool>& history);

    void paint (juce::Graphics& g) override;

private:
    std::vector<BarState> barStates;
    std::vector<std::vector<bool>> successHistories;
    int currentBar = -1;
};

#endif //UPBEAT_BARPROGRESSDISPLAY_H
