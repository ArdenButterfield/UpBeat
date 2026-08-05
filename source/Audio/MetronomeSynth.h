//
// Created by Arden on 8/5/2026.
//

#ifndef UPBEAT_METRONOMESYNTH_H
#define UPBEAT_METRONOMESYNTH_H

#include "OscillatorSynth.h"

// Click synth for a metronome. Rather than an arbitrary midi note, noteOn()
// takes which kind of click to play: a low pitch for ordinary beats, or a
// high pitch for barlines.
class MetronomeSynth : public OscillatorSynth
{
public:
    enum ClickType
    {
        BEAT,
        BARLINE
    };

    void noteOn (ClickType clickType);

protected:
    float getSample (double phase) const override;
    float getDecaySeconds() const override { return clickDecaySeconds; }

private:
    static constexpr int lowPitchMidiNote = 69;  // BEAT click (A4)
    static constexpr int highPitchMidiNote = 81; // BARLINE click (A5)
    static constexpr float clickDecaySeconds = 0.02f;
};

#endif //UPBEAT_METRONOMESYNTH_H
