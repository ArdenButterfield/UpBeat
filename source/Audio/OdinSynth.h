//
// Created by Arden on 10/8/2026.
//

#ifndef UPBEAT_ODINSYNTH_H
#define UPBEAT_ODINSYNTH_H

#include "juce_audio_basics/juce_audio_basics.h"
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
