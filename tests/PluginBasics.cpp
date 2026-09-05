#include "helpers/test_helpers.h"
#include <PluginProcessor.h>
#include <BundledResources.h>

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
