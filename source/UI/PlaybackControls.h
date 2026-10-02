//
// Created by Arden on 10/2/2026.
//

#ifndef UPBEAT_PLAYBACKCONTROLS_H
#define UPBEAT_PLAYBACKCONTROLS_H

#include "juce_gui_basics/juce_gui_basics.h"

// Side-by-side vertical tempo-scale and note-velocity sliders shown to the left of the
// note lanes in chart performance and practice. They stay usable during playback, and
// never take keyboard focus so key presses keep reaching the scene's lanes.
class PlaybackControls : public juce::Component
{
public:
    PlaybackControls (double initialTempoScale, double initialNoteVelocity);

    std::function<void (double)> onTempoScaleChange;
    std::function<void (double)> onNoteVelocityChange;

    void resized() override;

private:
    void configureSlider (juce::Slider& slider, juce::Label& label, const juce::String& labelText);

    juce::Slider tempoScaleSlider;
    juce::Label tempoScaleLabel;
    juce::Slider noteVelocitySlider;
    juce::Label noteVelocityLabel;
};

#endif //UPBEAT_PLAYBACKCONTROLS_H
