//
// Created by Arden on 7/27/2026.
//

#ifndef UPBEAT_SQUAREWAVESYNTH_H
#define UPBEAT_SQUAREWAVESYNTH_H

#include "juce_audio_basics/juce_audio_basics.h"
#include <array>

// Simple multivoice square wave synthesizer. Each triggered note plays at
// full amplitude and decays linearly to silence over decaySeconds.
class SquareWaveSynth
{
public:
    SquareWaveSynth();

    void prepareToPlay (double sampleRate);
    void noteOn (int midiNote);
    void renderNextBlock (juce::AudioBuffer<float>& buffer, int startSample, int numSamples);

private:
    struct Voice
    {
        bool active = false;
        double phase = 0.0;
        double phaseIncrement = 0.0;
        float amplitude = 0.0f;
    };

    static constexpr int maxVoices = 16;
    static constexpr float decaySeconds = 1.0f;
    static constexpr float outputGain = 0.15f;

    static double midiNoteToFrequency (int midiNote);

    std::array<Voice, maxVoices> voices;
    double sampleRate = 44100.0;
    float amplitudeDecrementPerSample = 0.0f;
};

#endif //UPBEAT_SQUAREWAVESYNTH_H
