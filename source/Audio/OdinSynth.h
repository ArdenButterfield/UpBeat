//
// Created by Arden on 10/8/2026.
//

#ifndef UPBEAT_ODINSYNTH_H
#define UPBEAT_ODINSYNTH_H

#include "juce_audio_basics/juce_audio_basics.h"
#include "juce_data_structures/juce_data_structures.h"
#include <array>
#include <memory>

class OdinAudioProcessor;

// Hosts an instance of the Odin2 synth (modules/odin2) and drives it with notes that
// release on their own after a given duration, so callers only ever trigger notes.
// All methods except the constructor/destructor are meant to be called from the audio thread.
class OdinSynth
{
public:
    OdinSynth();
    ~OdinSynth();

    void prepareToPlay (double newSampleRate, int maximumBlockSize);

    // Starts midiNote at the beginning of the next rendered block and releases it after
    // durationSeconds.
    void noteOn (int midiNote, double durationSeconds);

    // Adds the synth's output to buffer.
    void renderNextBlock (juce::AudioBuffer<float>& buffer, int startSample, int numSamples);

    void setOutputGain (float newGain) { outputGain = newGain; }

    // Loads a patch saved by Odin (the contents of a .odin file), replacing the current sound.
    // Returns false, leaving the current patch untouched, if the data isn't an Odin patch or
    // was saved by a newer version of Odin than we bundle. Allocates, so call it from the
    // message thread while audio isn't being rendered.
    bool loadPreset (const juce::MemoryBlock& odinFileData);
    bool loadPreset (const juce::File& odinFile);

    // The current patch, in the same form a .odin file stores it.
    juce::ValueTree getPreset() const;

private:
    struct PendingNoteOff
    {
        bool active = false;
        int midiNote = 0;
        juce::int64 samplesRemaining = 0;
    };

    static constexpr int maxPendingNoteOffs = 64;
    static constexpr int maxMidiEventsPerBlock = 256;

    std::unique_ptr<OdinAudioProcessor> processor;
    juce::AudioBuffer<float> renderBuffer;
    juce::MidiBuffer midiBuffer;
    std::array<PendingNoteOff, maxPendingNoteOffs> pendingNoteOffs;

    double sampleRate = 44100.0;
    float outputGain = 1.0f;
};

#endif //UPBEAT_ODINSYNTH_H
