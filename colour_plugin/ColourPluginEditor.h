#ifndef UPBEAT_COLOURPLUGINEDITOR_H
#define UPBEAT_COLOURPLUGINEDITOR_H

#include "ColourPluginProcessor.h"
#include "juce_gui_basics/juce_gui_basics.h"

// One colour in the grid: its title and a vertical slider for each Oklab channel, on a
// background of the colour itself.
class ColourSquare : public juce::Component
{
public:
    ColourSquare (ColourPluginProcessor& processor, int colour);

    juce::Colour getCurrentColour() const;
    juce::Slider& getSlider (int channel) { return sliders[(size_t) channel]; }

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    // The colour that stays readable on the current background.
    juce::Colour contrastingColour() const;
    void colourChanged();

    ColourPluginProcessor& processor;
    int colour;
    juce::String title;

    std::array<juce::Slider, LaneColours::numChannels> sliders;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, LaneColours::numChannels> attachments;
};

class ColourPluginEditor : public juce::AudioProcessorEditor
{
public:
    explicit ColourPluginEditor (ColourPluginProcessor& processor);

    ColourSquare& getSquare (int colour) { return *squares[colour]; }

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    juce::OwnedArray<ColourSquare> squares;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ColourPluginEditor)
};

#endif //UPBEAT_COLOURPLUGINEDITOR_H
