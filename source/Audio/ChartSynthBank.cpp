//
// Created by Arden on 10/20/2026.
//

#include "ChartSynthBank.h"

ChartSynthBank::ChartSynthBank (const Chart& chart)
{
    for (auto& chartSynth : chart.synths)
    {
        auto synth = std::make_unique<OdinSynth>();
        // A synth whose state can't be read still plays, with Odin's default patch.
        if (! synth->loadState (chartSynth.state))
            DBG ("Couldn't load the Odin state for chart synth " << chartSynth.name);

        synths.push_back ({ chartSynth.id, std::move (synth) });
    }

    for (auto& lane : chart.automation)
    {
        auto entry = std::find_if (synths.begin(), synths.end(), [&lane] (const Entry& e) { return e.synthId == lane.synthId; });
        if (entry == synths.end())
            continue;

        auto parameterIndex = entry->synth->getParameterIndex (lane.parameterId);
        if (parameterIndex < 0)
        {
            DBG ("Chart automates unknown Odin parameter " << lane.parameterId);
            continue;
        }

        automation.push_back ({ lane, entry->synth.get(), parameterIndex });
    }
}

void ChartSynthBank::applyAutomation (double chartTimeMs)
{
    for (auto& automated : automation)
    {
        auto value = static_cast<float> (automated.lane.valueAt (chartTimeMs));
        if (value != automated.lastValue)
        {
            automated.synth->setParameterValue (automated.parameterIndex, value);
            automated.lastValue = value;
        }
    }
}

void ChartSynthBank::prepareToPlay (double sampleRate, int maximumBlockSize)
{
    for (auto& entry : synths)
        entry.synth->prepareToPlay (sampleRate, maximumBlockSize);
}

void ChartSynthBank::noteOn (int synthId, int midiNote, int velocity, double durationSeconds)
{
    for (auto& entry : synths)
        if (entry.synthId == synthId)
            entry.synth->noteOn (midiNote, velocity, durationSeconds);
}

void ChartSynthBank::renderNextBlock (juce::AudioBuffer<float>& buffer, int startSample, int numSamples)
{
    for (auto& entry : synths)
        entry.synth->renderNextBlock (buffer, startSample, numSamples);
}
