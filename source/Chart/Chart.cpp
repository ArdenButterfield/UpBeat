//
// Created by Arden on 7/9/2026.
//

#include "Chart.h"

#include "juce_audio_basics/juce_audio_basics.h"

std::vector<TrackChannelInfo> Chart::analyzeTrackChannels (const juce::MemoryBlock& midiData)
{
    juce::MidiFile midiFile;
    auto inputStream = juce::MemoryInputStream (midiData, false);
    midiFile.readFrom (inputStream);

    std::map<std::pair<int, int>, int> noteCountByTrackAndChannel;
    for (int trackIndex = 0; trackIndex < midiFile.getNumTracks(); ++trackIndex)
    {
        auto* track = midiFile.getTrack (trackIndex);
        for (int i = 0; i < track->getNumEvents(); ++i)
        {
            auto& message = track->getEventPointer (i)->message;
            if (message.isNoteOn())
                noteCountByTrackAndChannel[{ trackIndex, message.getChannel() }] += 1;
        }
    }

    std::vector<TrackChannelInfo> result;
    for (auto& [trackAndChannel, count] : noteCountByTrackAndChannel)
        result.push_back ({ trackAndChannel.first, trackAndChannel.second, count });

    return result;
}

Chart::Chart (const juce::MemoryBlock& _midiData, const juce::String& _name, const std::vector<std::pair<int, int>>& selectedTrackChannels)
    : name (_name), midiData (_midiData), numLanes (static_cast<int> (selectedTrackChannels.size()))
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

    std::map<std::pair<int, int>, int> laneByTrackAndChannel;
    for (size_t lane = 0; lane < selectedTrackChannels.size(); ++lane)
        laneByTrackAndChannel[selectedTrackChannels[lane]] = static_cast<int> (lane);

    for (int trackIndex = 0; trackIndex < midiFile.getNumTracks(); ++trackIndex)
    {
        auto* track = midiFile.getTrack (trackIndex);
        for (int i = 0; i < track->getNumEvents(); ++i)
        {
            auto event = track->getEventPointer (i);

            if (event->message.isNoteOn ())
            {
                auto laneIt = laneByTrackAndChannel.find ({ trackIndex, event->message.getChannel() });
                auto lane = (laneIt != laneByTrackAndChannel.end()) ? laneIt->second : -1;
                auto chartEvent = ChartEvent (event, lane);
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