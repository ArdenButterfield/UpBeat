//
// Created by Arden on 7/27/2026.
//

#ifndef UPBEAT_OSCILLATORSYNTH_H
#define UPBEAT_OSCILLATORSYNTH_H

#include "juce_audio_basics/juce_audio_basics.h"
#include <array>

// Base class for simple multivoice oscillator synthesizers. Each triggered note
// plays at full amplitude and decays linearly to silence over decaySeconds.
// Subclasses provide the waveform shape via getSample().
class OscillatorSynth
{
public:
    virtual ~OscillatorSynth() = default;

    void prepareToPlay (double newSampleRate);
    void noteOn (int midiNote);
    void renderNextBlock (juce::AudioBuffer<float>& buffer, int startSample, int numSamples);

protected:
    virtual float getSample (double phase) const = 0;

    // Subclasses can override to shape their envelope, e.g. a short click
    // instead of the default sustained, slowly-decaying tone.
    virtual float getDecaySeconds() const { return 1.0f; }

private:
    struct Voice
    {
        bool active = false;
        double phase = 0.0;
        double phaseIncrement = 0.0;
        float amplitude = 0.0f;
    };

    static constexpr int maxVoices = 16;
    static constexpr float outputGain = 0.15f;

    static double midiNoteToFrequency (int midiNote);

    std::array<Voice, maxVoices> voices;
    double sampleRate = 44100.0;
    float amplitudeDecrementPerSample = 0.0f;
};

#endif //UPBEAT_OSCILLATORSYNTH_H
