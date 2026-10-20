//
// Created by Arden on 10/2/2026.
//

#ifndef UPBEAT_PLAYBACKCONTROLS_H
#define UPBEAT_PLAYBACKCONTROLS_H

#include "juce_gui_basics/juce_gui_basics.h"

// A vertical tempo-scale slider shown to the left of the note display in chart
// performance and practice. It stays usable during playback, and never takes keyboard
// focus so key presses keep reaching the scene's lanes.
class PlaybackControls : public juce::Component
{
public:
    explicit PlaybackControls (double initialTempoScale);

    std::function<void (double)> onTempoScaleChange;

    void resized() override;

private:
    void configureSlider (juce::Slider& slider, juce::Label& label, const juce::String& labelText);

    juce::Slider tempoScaleSlider;
    juce::Label tempoScaleLabel;
};

#endif //UPBEAT_PLAYBACKCONTROLS_H
