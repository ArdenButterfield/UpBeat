//
// Created by Arden on 9/7/2026.
//

#ifndef UPBEAT_BARPROGRESSDISPLAY_H
#define UPBEAT_BARPROGRESSDISPLAY_H

#include "juce_gui_basics/juce_gui_basics.h"
#include <vector>

// The small square-per-bar progress graphic shown next to the lanes during chart
// practice: each square starts out transparent, turns yellow once its bar has been
// introduced for practice, and turns green once the bar is fully learned.
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

    void paint (juce::Graphics& g) override;

private:
    std::vector<BarState> barStates;
};

#endif //UPBEAT_BARPROGRESSDISPLAY_H
