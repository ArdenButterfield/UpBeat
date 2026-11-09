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
// ChartSynth id that chart events carry. Construct on the message thread, or on a background
// thread via the loadingThread constructor; everything else is meant to be called from the
// audio thread.
class ChartSynthBank
{
public:
    explicit ChartSynthBank (const Chart& chart);

    // Builds the bank from loadingThread, a background thread. Each Odin instance's parameter
    // state (an AudioProcessorValueTreeState) is flushed by a timer on the message thread, so
    // every synth is built while holding the MessageManagerLock; the message thread runs freely
    // between synths. Stops early, leaving the bank incomplete, if loadingThread is asked to exit.
    //
    // Requesting that lock posts a message, and on some platforms (Windows) the message thread
    // only paints once it has no posted messages left, so asking for the lock straight away
    // would hold off painting until every synth had loaded. Instead, before each synth,
    // loadingThread waits to be notified (Thread::notify, e.g. once a frame has been painted),
    // or for maxWaitForMessageThreadMs if nothing does.
    ChartSynthBank (const Chart& chart, juce::Thread& loadingThread);

    static constexpr int maxWaitForMessageThreadMs = 200;

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
    ChartSynthBank (const Chart& chart, juce::Thread* loadingThread);

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
