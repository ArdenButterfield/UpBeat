//
// Created by Arden on 10/2/2026.
//

#include "PlaybackControls.h"

PlaybackControls::PlaybackControls (double initialTempoScale)
{
    setWantsKeyboardFocus (false);
    setMouseClickGrabsKeyboardFocus (false);

    configureSlider (tempoScaleSlider, tempoScaleLabel, "Tempo");
    tempoScaleSlider.setRange (0.25, 2.0, 0.01);
    tempoScaleSlider.setTextValueSuffix ("x");
    tempoScaleSlider.setValue (initialTempoScale, juce::dontSendNotification);
    tempoScaleSlider.onValueChange = [this] {
        if (onTempoScaleChange)
            onTempoScaleChange (tempoScaleSlider.getValue());
    };
}

void PlaybackControls::configureSlider (juce::Slider& slider, juce::Label& label, const juce::String& labelText)
{
    slider.setSliderStyle (juce::Slider::LinearVertical);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, true, 60, 20);
    slider.setWantsKeyboardFocus (false);
    slider.setMouseClickGrabsKeyboardFocus (false);

    label.setText (labelText, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setWantsKeyboardFocus (false);

    addAndMakeVisible (slider);
    addAndMakeVisible (label);
}

void PlaybackControls::resized()
{
    auto bounds = getLocalBounds();
    tempoScaleLabel.setBounds (bounds.removeFromTop (20));
    tempoScaleSlider.setBounds (bounds);
}
