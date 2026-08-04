//
// Created by Arden on 7/27/2026.
//

#ifndef UPBEAT_SINEWAVESYNTH_H
#define UPBEAT_SINEWAVESYNTH_H

#include "OscillatorSynth.h"

class SineWaveSynth : public OscillatorSynth
{
protected:
    float getSample (double phase) const override;
};

#endif //UPBEAT_SINEWAVESYNTH_H
