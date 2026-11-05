#include "ColourPluginProcessor.h"

#include "ColourPluginEditor.h"

namespace
{
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
    {
        juce::AudioProcessorValueTreeState::ParameterLayout layout;
        for (int colour = 0; colour < LaneColours::numColours; ++colour)
        {
            auto defaultLab = LaneColours::defaultOklab (colour);
            for (int channel = 0; channel < LaneColours::numChannels; ++channel)
            {
                auto& info = LaneColours::channels[(size_t) channel];
                layout.add (std::make_unique<juce::AudioParameterFloat> (
                    juce::ParameterID { LaneColours::parameterId (colour, channel), 1 },
                    juce::String (LaneColours::infos[(size_t) colour].title) + " " + info.name,
                    juce::NormalisableRange<float> (info.minValue, info.maxValue),
                    static_cast<float> (defaultLab[channel])));
            }
        }
        return layout;
    }
}

ColourPluginProcessor::ColourPluginProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "UpBeatColours", createParameterLayout())
{
}

Oklab ColourPluginProcessor::getOklab (int colour) const
{
    Oklab lab;
    for (int channel = 0; channel < LaneColours::numChannels; ++channel)
        lab[channel] = parameters.getRawParameterValue (LaneColours::parameterId (colour, channel))->load();
    return lab;
}

void ColourPluginProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    buffer.clear();
    midi.clear();
}

juce::AudioProcessorEditor* ColourPluginProcessor::createEditor()
{
    return new ColourPluginEditor (*this);
}

void ColourPluginProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = parameters.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void ColourPluginProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes); xml != nullptr && xml->hasTagName (parameters.state.getType()))
        parameters.replaceState (juce::ValueTree::fromXml (*xml));
}
