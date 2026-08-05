//
// Created by Arden on 7/8/2026.
//

#ifndef UPBEAT_TOLERANCELABEL_H
#define UPBEAT_TOLERANCELABEL_H

#include "juce_gui_basics/juce_gui_basics.h"

class ToleranceLabel : public juce::Label
{
public:
    explicit ToleranceLabel (const juce::String& message);

    // Advances the label's lifetime by elapsedMs and updates its opacity accordingly.
    // Returns true once the label has fully faded out and should be removed.
    bool advance (double elapsedMs);

private:
    static constexpr double visibleDurationMs = 500.0;
    static constexpr double fadeDurationMs = 1000.0;

    double ageMs = 0.0;
};

#endif //UPBEAT_TOLERANCELABEL_H
