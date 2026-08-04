//
// Created by Arden on 7/27/2026.
//

#ifndef UPBEAT_SQUAREWAVESYNTH_H
#define UPBEAT_SQUAREWAVESYNTH_H

#include "OscillatorSynth.h"

class SquareWaveSynth : public OscillatorSynth
{
protected:
    float getSample (double phase) const override;
};

#endif //UPBEAT_SQUAREWAVESYNTH_H
