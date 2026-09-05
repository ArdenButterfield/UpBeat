#include "helpers/test_helpers.h"
#include <PluginProcessor.h>
#include <BundledResources.h>
#include <Chart/Chart.h>

#include <juce_audio_basics/juce_audio_basics.h>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <map>
#include <set>

TEST_CASE ("one is equal to one", "[dummy]")
{
    REQUIRE (1 == 1);
}

TEST_CASE ("Plugin instance", "[instance]")
{
    PluginProcessor testPlugin;

    SECTION ("name")
    {
        CHECK_THAT (testPlugin.getName().toStdString(),
            Catch::Matchers::Equals ("UpBeat"));
    }
}

TEST_CASE ("Bundled Resources")
{
    int i = 0;
    for (auto& entry : BundledResources::listEntries ("default_charts"))
    {
        i += 1;
        DBG(entry);
    }

    REQUIRE (i > 1);

    auto helloMid = BundledResources::loadFile ("default_charts/midi/hello.mid");
    REQUIRE (helloMid.getSize() > 0);
    REQUIRE (juce::String::toHexString (helloMid.getData(), (int) helloMid.getSize()).length() > 0);
}

TEST_CASE ("invent6.mid has notes on 2 channels", "[midi]")
{
    auto invent6Mid = BundledResources::loadFile ("default_charts/midi/invent6.mid");
    REQUIRE (invent6Mid.getSize() > 0);

    juce::MidiFile midiFile;
    auto inputStream = juce::MemoryInputStream (invent6Mid, false);
    REQUIRE (midiFile.readFrom (inputStream));

    std::map<int, int> noteOnCountByChannel;
    for (int trackIndex = 0; trackIndex < midiFile.getNumTracks(); ++trackIndex)
    {
        auto* track = midiFile.getTrack (trackIndex);
        for (int i = 0; i < track->getNumEvents(); ++i)
        {
            auto& message = track->getEventPointer (i)->message;
            if (message.isNoteOn())
                noteOnCountByChannel[message.getChannel()] += 1;
        }
    }

    REQUIRE (noteOnCountByChannel.size() == 2);
    for (auto& [channel, count] : noteOnCountByChannel)
        CHECK (count > 0);
}


TEST_CASE ("Chart assigns lanes based on selected track/channel pairs", "[chart]")
{
    juce::MidiMessageSequence sequence;
    sequence.addEvent (juce::MidiMessage::noteOn (1, 60, (juce::uint8) 100).withTimeStamp (0));
    sequence.addEvent (juce::MidiMessage::noteOff (1, 60).withTimeStamp (100));
    sequence.addEvent (juce::MidiMessage::noteOn (2, 61, (juce::uint8) 100).withTimeStamp (200));
    sequence.addEvent (juce::MidiMessage::noteOff (2, 61).withTimeStamp (300));
    sequence.addEvent (juce::MidiMessage::noteOn (5, 62, (juce::uint8) 100).withTimeStamp (400));
    sequence.addEvent (juce::MidiMessage::noteOff (5, 62).withTimeStamp (500));
    sequence.updateMatchedPairs();

    juce::MidiFile midiFile;
    midiFile.setTicksPerQuarterNote (960);
    midiFile.addTrack (sequence);

    juce::MemoryBlock midiData;
    juce::MemoryOutputStream outStream (midiData, false);
    REQUIRE (midiFile.writeTo (outStream));

    // Only channels 1 and 2 are selected as playable lanes; channel 5 becomes a background note.
    Chart chart (midiData, "test chart", { { 0, 1 }, { 0, 2 } });

    REQUIRE (chart.numLanes == 2);

    std::map<int, int> countByLane;
    int backgroundCount = 0;
    for (auto& [time, event] : chart.events)
    {
        if (event.type == ChartEvent::NOTE)
            countByLane[event.inputButton] += 1;
        else if (event.type == ChartEvent::BACKGROUND_NOTE)
            backgroundCount += 1;
    }

    CHECK (countByLane[0] == 1);
    CHECK (countByLane[1] == 1);
    CHECK (backgroundCount == 1);
}

TEST_CASE ("Chart::analyzeTrackChannels reports note counts per track/channel", "[chart]")
{
    auto invent6Mid = BundledResources::loadFile ("default_charts/midi/invent6.mid");
    auto trackChannels = Chart::analyzeTrackChannels (invent6Mid);

    REQUIRE (! trackChannels.empty());

    std::set<std::pair<int, int>> seen;
    for (auto& info : trackChannels)
    {
        CHECK (info.noteCount > 0);
        CHECK (seen.insert ({ info.track, info.channel }).second);
    }
}

#ifdef PAMPLEJUCE_IPP
    #include <ipp.h>

TEST_CASE ("IPP version", "[ipp]")
{
    #if defined(__APPLE__)
        // macOS uses 2021.9.1 from pip wheel (only x86_64 version available)
        CHECK_THAT (ippsGetLibVersion()->Version, Catch::Matchers::Equals ("2021.9.1 (r0x7e208212)"));
    #else
        CHECK_THAT (ippsGetLibVersion()->Version, Catch::Matchers::Equals ("2026.0.0 (r0xa7ad6ebc)"));
    #endif
}
#endif
