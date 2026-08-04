//
// Created by Arden on 7/27/2026.
//

#include "SquareWaveSynth.h"

float SquareWaveSynth::getSample (double phase) const
{
    return phase < 0.5 ? 1.0f : -1.0f;
}
