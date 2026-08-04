//
// Created by Arden on 7/27/2026.
//

#include "OscillatorSynth.h"

void OscillatorSynth::prepareToPlay (double newSampleRate)
{
    sampleRate = newSampleRate;
    amplitudeDecrementPerSample = 1.0f / (decaySeconds * static_cast<float>(sampleRate));
}

void OscillatorSynth::noteOn (int midiNote)
{
    auto voiceToUse = std::find_if (voices.begin(), voices.end(), [] (const Voice& v) { return !v.active; });

    if (voiceToUse == voices.end())
    {
        voiceToUse = std::min_element (voices.begin(), voices.end(), [] (const Voice& a, const Voice& b) {
            return a.amplitude < b.amplitude;
        });
    }

    voiceToUse->active = true;
    voiceToUse->phase = 0.0;
    voiceToUse->phaseIncrement = midiNoteToFrequency (midiNote) / sampleRate;
    voiceToUse->amplitude = 1.0f;
}

void OscillatorSynth::renderNextBlock (juce::AudioBuffer<float>& buffer, int startSample, int numSamples)
{
    for (auto& voice : voices)
    {
        if (!voice.active)
            continue;

        for (int sample = 0; sample < numSamples; ++sample)
        {
            auto value = getSample (voice.phase) * voice.amplitude * outputGain;

            for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
                buffer.addSample (channel, startSample + sample, value);

            voice.phase += voice.phaseIncrement;
            if (voice.phase >= 1.0)
                voice.phase -= 1.0;

            voice.amplitude -= amplitudeDecrementPerSample;
            if (voice.amplitude <= 0.0f)
            {
                voice.amplitude = 0.0f;
                voice.active = false;
                break;
            }
        }
    }
}

double OscillatorSynth::midiNoteToFrequency (int midiNote)
{
    return 440.0 * std::pow (2.0, (midiNote - 69) / 12.0);
}
