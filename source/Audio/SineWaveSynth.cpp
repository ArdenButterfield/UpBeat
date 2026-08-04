//
// Created by Arden on 7/27/2026.
//

#include "SineWaveSynth.h"

float SineWaveSynth::getSample (double phase) const
{
    return static_cast<float> (std::sin (phase * juce::MathConstants<double>::twoPi));
}
