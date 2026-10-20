//
// Created by Arden on 10/20/2026.
//

#ifndef UPBEAT_CHARTSYNTHBANK_H
#define UPBEAT_CHARTSYNTHBANK_H

#include "../Chart/Chart.h"
#include "OdinSynth.h"
#include <memory>
#include <vector>

// One OdinSynth per synth in a chart, each loaded with that synth's state, addressed by the
// ChartSynth id that chart events carry. Construct on the message thread; everything else is
// meant to be called from the audio thread.
class ChartSynthBank
{
public:
    explicit ChartSynthBank (const Chart& chart);

    void prepareToPlay (double sampleRate, int maximumBlockSize);

    // Plays midiNote at velocity (1 to 127) on the synth with the given id; ignored if the
    // chart has no such synth.
    void noteOn (int synthId, int midiNote, int velocity, double durationSeconds);

    // Sets every automated parameter to its value at chartTimeMs (in the chart's own time,
    // unaffected by tempo scaling). Call once per block, before renderNextBlock.
    void applyAutomation (double chartTimeMs);

    // Adds every synth's output to buffer.
    void renderNextBlock (juce::AudioBuffer<float>& buffer, int startSample, int numSamples);

private:
    struct Entry
    {
        int synthId;
        std::unique_ptr<OdinSynth> synth;
    };

    struct AutomatedParameter
    {
        AutomationLane lane;
        OdinSynth* synth;
        int parameterIndex;
        float lastValue = -1.0f; // what was last set, so unchanged values aren't resent each block
    };

    std::vector<Entry> synths;
    std::vector<AutomatedParameter> automation;
};

#endif //UPBEAT_CHARTSYNTHBANK_H
