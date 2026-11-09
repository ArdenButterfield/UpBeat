#include "ColourPluginEditor.h"

namespace
{
    constexpr int gridColumns = 4;
    constexpr int squareWidth = 170;
    constexpr int squareHeight = 220;
    constexpr int gridGap = 6;
    constexpr int titleHeight = 30;
    constexpr int channelLabelHeight = 20;
}

ColourSquare::ColourSquare (ColourPluginProcessor& processor_, int colour_)
    : processor (processor_), colour (colour_), title (LaneColours::infos[(size_t) colour_].title)
{
    for (int channel = 0; channel < LaneColours::numChannels; ++channel)
    {
        auto& slider = sliders[(size_t) channel];
        slider.setSliderStyle (juce::Slider::LinearVertical);
        slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        slider.setPopupDisplayEnabled (true, false, this);
        addAndMakeVisible (slider);

        attachments[(size_t) channel] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            processor.getParameters(), LaneColours::parameterId (colour, channel), slider);

        // Attached sliders also follow the host's automation.
        slider.onValueChange = [this] { colourChanged(); };
    }
    colourChanged();
}

juce::Colour ColourSquare::getCurrentColour() const
{
    return toColour (processor.getOklab (colour));
}

juce::Colour ColourSquare::contrastingColour() const
{
    return processor.getOklab (colour).L > 0.6 ? juce::Colours::black : juce::Colours::white;
}

void ColourSquare::colourChanged()
{
    auto contrast = contrastingColour();
    for (auto& slider : sliders)
    {
        slider.setColour (juce::Slider::thumbColourId, contrast);
        slider.setColour (juce::Slider::trackColourId, contrast.withAlpha (0.6f));
        slider.setColour (juce::Slider::backgroundColourId, contrast.withAlpha (0.25f));
    }
    repaint();
}

void ColourSquare::paint (juce::Graphics& g)
{
    g.fillAll (getCurrentColour());

    g.setColour (contrastingColour());
    g.setFont (15.0f);
    g.drawText (title, getLocalBounds().removeFromTop (titleHeight), juce::Justification::centred, true);

    g.setFont (13.0f);
    for (int channel = 0; channel < LaneColours::numChannels; ++channel)
    {
        auto sliderBounds = sliders[(size_t) channel].getBounds();
        g.drawText (LaneColours::channels[(size_t) channel].name,
            sliderBounds.getX(),
            sliderBounds.getBottom(),
            sliderBounds.getWidth(),
            channelLabelHeight,
            juce::Justification::centred);
    }
}

void ColourSquare::resized()
{
    auto bounds = getLocalBounds().reduced (6);
    bounds.removeFromTop (titleHeight);
    bounds.removeFromBottom (channelLabelHeight);

    auto sliderWidth = bounds.getWidth() / LaneColours::numChannels;
    for (auto& slider : sliders)
        slider.setBounds (bounds.removeFromLeft (sliderWidth));
}

ColourPluginEditor::ColourPluginEditor (ColourPluginProcessor& processor)
    : AudioProcessorEditor (processor)
{
    for (int colour = 0; colour < LaneColours::numColours; ++colour)
        addAndMakeVisible (squares.add (new ColourSquare (processor, colour)));

    auto rows = (LaneColours::numColours + gridColumns - 1) / gridColumns;
    setSize (gridColumns * (squareWidth + gridGap) + gridGap, rows * (squareHeight + gridGap) + gridGap);
}

void ColourPluginEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);
}

void ColourPluginEditor::resized()
{
    for (int i = 0; i < squares.size(); ++i)
        squares[i]->setBounds (gridGap + (i % gridColumns) * (squareWidth + gridGap),
            gridGap + (i / gridColumns) * (squareHeight + gridGap),
            squareWidth,
            squareHeight);
}
