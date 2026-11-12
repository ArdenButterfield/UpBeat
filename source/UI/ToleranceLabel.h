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

    // The line from where a hit note should have been played to this label, which the
    // display draws beneath it, fading with it. Several segments when it crosses rows.
    void setHitLine (std::vector<juce::Line<float>> segments, juce::Colour colour);
    const std::vector<juce::Line<float>>& getHitLine() const { return hitLine; }
    juce::Colour getHitLineColour() const { return hitLineColour; }

private:
    static constexpr double visibleDurationMs = 500.0;
    static constexpr double fadeDurationMs = 1000.0;

    double ageMs = 0.0;
    std::vector<juce::Line<float>> hitLine;
    juce::Colour hitLineColour;
};

#endif //UPBEAT_TOLERANCELABEL_H
