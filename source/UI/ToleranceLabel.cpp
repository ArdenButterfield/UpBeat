//
// Created by Arden on 7/8/2026.
//

#include "ToleranceLabel.h"

ToleranceLabel::ToleranceLabel (const juce::String& message) : juce::Label ({}, message)
{
    setJustificationType (juce::Justification::centred);
    setInterceptsMouseClicks (false, false);
}

bool ToleranceLabel::advance (double elapsedMs)
{
    ageMs += elapsedMs;

    if (ageMs <= visibleDurationMs)
    {
        setAlpha (1.0f);
    }
    else
    {
        auto fadeElapsed = ageMs - visibleDurationMs;
        setAlpha (juce::jlimit (0.0f, 1.0f, 1.0f - static_cast<float> (fadeElapsed / fadeDurationMs)));
    }

    return ageMs >= visibleDurationMs + fadeDurationMs;
}
