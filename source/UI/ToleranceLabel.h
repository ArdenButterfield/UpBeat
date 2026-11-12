//
// Created by Arden on 7/8/2026.
//

#ifndef UPBEAT_TOLERANCELABEL_H
#define UPBEAT_TOLERANCELABEL_H

#include "juce_gui_basics/juce_gui_basics.h"

// The image shown where a note was hit, saying how well it was timed. It fades out after a
// moment.
class ToleranceLabel : public juce::ImageComponent
{
public:
    // category is a GameState tolerance category, or GameState::NUM_TOLERANCE_CATEGORIES
    // for a miss.
    explicit ToleranceLabel (int category);

    // Advances the label's lifetime by elapsedMs and updates its opacity accordingly.
    // Returns true once the label has fully faded out and should be removed.
    bool advance (double elapsedMs);

    // The image for a tolerance category (see the constructor).
    static juce::Image imageForCategory (int category);

private:
    static constexpr double visibleDurationMs = 500.0;
    static constexpr double fadeDurationMs = 1000.0;

    double ageMs = 0.0;
};

#endif //UPBEAT_TOLERANCELABEL_H
