//
// Created by Arden on 10/2/2026.
//

#include "PlaybackControls.h"

PlaybackControls::PlaybackControls (double initialTempoScale, double initialNoteVelocity)
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

    configureSlider (noteVelocitySlider, noteVelocityLabel, "Velocity");
    noteVelocitySlider.setRange (0.05, 0.5, 0.01);
    noteVelocitySlider.setValue (initialNoteVelocity, juce::dontSendNotification);
    noteVelocitySlider.onValueChange = [this] {
        if (onNoteVelocityChange)
            onNoteVelocityChange (noteVelocitySlider.getValue());
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
    auto columnWidth = bounds.getWidth() / 2;

    auto tempoColumn = bounds.removeFromLeft (columnWidth);
    tempoScaleLabel.setBounds (tempoColumn.removeFromTop (20));
    tempoScaleSlider.setBounds (tempoColumn);

    noteVelocityLabel.setBounds (bounds.removeFromTop (20));
    noteVelocitySlider.setBounds (bounds);
}
