//
// Created by Arden on 8/5/2026.
//

#include "MetronomeSynth.h"

void MetronomeSynth::noteOn (ClickType clickType)
{
    OscillatorSynth::noteOn (clickType == BARLINE ? highPitchMidiNote : lowPitchMidiNote);
}

float MetronomeSynth::getSample (double phase) const
{
    return phase < 0.5 ? 1.0f : -1.0f;
}
