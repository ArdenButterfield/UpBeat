//
// Created by Arden on 7/9/2026.
//

#include "Chart.h"
#include "../GameState.h"

#include "juce_audio_basics/juce_audio_basics.h"

Chart::Chart (const juce::MemoryBlock& _midiData) : midiData (_midiData)
{
    juce::MidiFile midiFile;
    auto inputStream = juce::MemoryInputStream(midiData, false);
    midiFile.readFrom (inputStream);

    midiFile.convertTimestampTicksToSeconds();

    // Must be found after convertTimestampTicksToSeconds(), so that their timestamps are in seconds
    // rather than ticks, matching the note events below.
    juce::MidiMessageSequence tempoChangeEvents, timeSigEvents;
    midiFile.findAllTempoEvents (tempoChangeEvents);
    midiFile.findAllTimeSigEvents (timeSigEvents);

    auto activeTrack = midiFile.getTrack(0); // TODO: in the future, let the user pick the track to perform, with the other tracks as background.
    if (activeTrack == nullptr)
    {
        DBG("No active tracks found!");
    } else
    {
        for (int i = 0; i < activeTrack->getNumEvents(); ++i)
        {
            auto event = activeTrack->getEventPointer (i);

            if (event->message.isNoteOn ())
            {
                auto chartEvent = ChartEvent(event, GameState::numberOfInputLanes);
                std::cout << "create event at " << chartEvent.timeMs << ", button " << chartEvent.inputButton << std::endl;
                events.insert ({chartEvent.timeMs, std::move(chartEvent)});
            }
        }
    }

    if (tempoChangeEvents.getNumEvents() > 0 && timeSigEvents.getNumEvents() > 0)
    {
        double secondsPerQuarterNote = 0.5; // default 120 BPM, per the MIDI spec
        int timeSigNumerator = 4;
        int timeSigDenominator = 4; // default 4/4, per the MIDI spec

        int tempoIndex = 0;
        int timeSigIndex = 0;
        int beatInBar = 0;

        constexpr double epsilonSeconds = 1.0e-6;
        const double endTimeSeconds = midiFile.getLastTimestamp();

        for (double t = 0.0; t <= endTimeSeconds;)
        {
            bool timeSigChanged = false;

            while (tempoIndex < tempoChangeEvents.getNumEvents()
                   && tempoChangeEvents.getEventPointer (tempoIndex)->message.getTimeStamp() <= t + epsilonSeconds)
            {
                secondsPerQuarterNote = tempoChangeEvents.getEventPointer (tempoIndex)->message.getTempoSecondsPerQuarterNote();
                ++tempoIndex;
            }

            while (timeSigIndex < timeSigEvents.getNumEvents()
                   && timeSigEvents.getEventPointer (timeSigIndex)->message.getTimeStamp() <= t + epsilonSeconds)
            {
                timeSigEvents.getEventPointer (timeSigIndex)->message.getTimeSignatureInfo (timeSigNumerator, timeSigDenominator);
                ++timeSigIndex;
                timeSigChanged = true;
            }

            if (timeSigNumerator < 1)
                timeSigNumerator = 4;
            if (timeSigDenominator < 1)
                timeSigDenominator = 4;

            // Realign to the start of a bar whenever the time signature changes.
            if (timeSigChanged)
                beatInBar = 0;

            auto type = (beatInBar == 0) ? ChartEvent::BARLINE : ChartEvent::BEAT;
            auto chartEvent = ChartEvent (static_cast<long long> (t * 1000.0), type);
            events.insert ({ chartEvent.timeMs, std::move (chartEvent) });

            beatInBar = (beatInBar + 1) % timeSigNumerator;

            const double beatDurationSeconds = secondsPerQuarterNote * 4.0 / static_cast<double> (timeSigDenominator);
            if (beatDurationSeconds <= 0.0)
                break;

            t += beatDurationSeconds;
        }
    }
}
Chart::~Chart()
{
}