#ifndef UPBEAT_COLOURPLUGINPROCESSOR_H
#define UPBEAT_COLOURPLUGINPROCESSOR_H

#include "../source/Chart/LaneColours.h"
#include "juce_audio_processors/juce_audio_processors.h"

// UpBeat Colours: a helper plugin for choosing a chart's NoteLaneDisplay colours in Ableton.
// It makes no sound. Each colour has an L, a and b parameter, whose values and automation
// Ableton Charts/parse_als.py copies into the chart when the plugin is on a GUI track.
//
// It's an instrument because Ableton's GUI tracks are MIDI tracks, and Ableton only accepts
// instruments and MIDI effects on a MIDI track that has no instrument yet.
class ColourPluginProcessor : public juce::AudioProcessor
{
public:
    ColourPluginProcessor();

    // The colour's current value, from its three parameters.
    Oklab getOklab (int colour) const;

    juce::AudioProcessorValueTreeState& getParameters() { return parameters; }

    void prepareToPlay (double, int) override {}
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "UpBeat Colours"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

private:
    juce::AudioProcessorValueTreeState parameters;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ColourPluginProcessor)
};

#endif //UPBEAT_COLOURPLUGINPROCESSOR_H
