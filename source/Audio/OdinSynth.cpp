//
// Created by Arden on 10/8/2026.
//

#include "OdinSynth.h"

// Odin is third-party code; keep its header warnings out of this file. (Elsewhere its include
// directories are SYSTEM, which does the same; see modules/odin2_integration/Odin2.cmake.)
#if JUCE_MSVC
    #pragma warning(push, 0)
#endif
#include <Source/PluginProcessor.h>
#if JUCE_MSVC
    #pragma warning(pop)
#endif

OdinSynth::OdinSynth() : processor (std::make_unique<OdinAudioProcessor>())
{
}

OdinSynth::~OdinSynth() = default;

void OdinSynth::prepareToPlay (double newSampleRate, int maximumBlockSize)
{
    sampleRate = newSampleRate;
    renderBuffer.setSize (2, maximumBlockSize);
    // Each note on/off event takes well under 16 bytes in a MidiBuffer; reserving up front
    // keeps addEvent from allocating on the audio thread.
    midiBuffer.ensureSize (maxMidiEventsPerBlock * 16);
    pendingNoteOffs.fill ({});

    processor->setRateAndBufferSizeDetails (newSampleRate, maximumBlockSize);
    processor->prepareToPlay (newSampleRate, maximumBlockSize);
}

void OdinSynth::noteOn (int midiNote, double durationSeconds)
{
    auto durationSamples = std::max<juce::int64> (1, static_cast<juce::int64> (durationSeconds * sampleRate));

    // A retriggered note gets a fresh duration rather than being cut short by the earlier
    // trigger's release.
    auto slot = std::find_if (pendingNoteOffs.begin(), pendingNoteOffs.end(), [midiNote] (const PendingNoteOff& p) {
        return p.active && p.midiNote == midiNote;
    });
    if (slot == pendingNoteOffs.end())
        slot = std::find_if (pendingNoteOffs.begin(), pendingNoteOffs.end(), [] (const PendingNoteOff& p) { return !p.active; });
    if (slot == pendingNoteOffs.end())
    {
        // Every slot is busy: release whichever note was going to end soonest right away.
        slot = std::min_element (pendingNoteOffs.begin(), pendingNoteOffs.end(), [] (const PendingNoteOff& a, const PendingNoteOff& b) {
            return a.samplesRemaining < b.samplesRemaining;
        });
        midiBuffer.addEvent (juce::MidiMessage::noteOff (1, slot->midiNote), 0);
    }

    *slot = { true, midiNote, durationSamples };
    midiBuffer.addEvent (juce::MidiMessage::noteOn (1, midiNote, 0.8f), 0);
}

void OdinSynth::renderNextBlock (juce::AudioBuffer<float>& buffer, int startSample, int numSamples)
{
    jassert (numSamples <= renderBuffer.getNumSamples()); // block larger than prepareToPlay promised
    numSamples = std::min (numSamples, renderBuffer.getNumSamples());

    for (auto& pending : pendingNoteOffs)
    {
        if (!pending.active)
            continue;

        if (pending.samplesRemaining < numSamples)
        {
            midiBuffer.addEvent (juce::MidiMessage::noteOff (1, pending.midiNote), static_cast<int> (pending.samplesRemaining));
            pending.active = false;
        }
        else
        {
            pending.samplesRemaining -= numSamples;
        }
    }

    // A view of the first numSamples of renderBuffer; Odin overwrites (rather than adds to)
    // whatever buffer it's given, so it can't render straight into the shared output.
    juce::AudioBuffer<float> block (renderBuffer.getArrayOfWritePointers(), renderBuffer.getNumChannels(), numSamples);
    processor->processBlock (block, midiBuffer);
    midiBuffer.clear();

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        buffer.addFrom (channel, startSample, block, std::min (channel, block.getNumChannels() - 1), 0, numSamples, outputGain);
}
