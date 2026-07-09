//
// Created by Arden on 7/9/2026.
//

#include "Chart.h"

#include "juce_audio_basics/juce_audio_basics.h"

Chart::Chart (const juce::MemoryBlock& _midiData) : midiData (_midiData)
{
    juce::MidiFile midiFile;
    auto inputStream = juce::MemoryInputStream(midiData, false);
    midiFile.readFrom (inputStream);

    juce::MidiMessageSequence tempoChangeEvents, timeSigEvents;
    midiFile.findAllTimeSigEvents (timeSigEvents);
    midiFile.findAllTempoEvents (tempoChangeEvents);

    midiFile.convertTimestampTicksToSeconds();

    auto activeTrack = midiFile.getTrack(0); // TODO: in the future, let the user pick the track to perform, with the other tracks as background.
    if (activeTrack == nullptr)
    {
        DBG("No active tracks found!");
    } else
    {
        for (int i = 0; i < activeTrack->getNumEvents(); ++i)
        {
            auto event = activeTrack->getEventPointer (i);
            std::cout << event->message.getDescription() << std::endl;
        }
    }

}
Chart::~Chart()
{
}