#include "helpers/test_helpers.h"
#include <PluginProcessor.h>
#include <BundledResources.h>
#include <Chart/Chart.h>

#include <juce_audio_basics/juce_audio_basics.h>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <map>

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


TEST_CASE ("Chart::fromYaml reads note lanes independently of synths", "[chart]")
{
    auto yaml = R"(
title: lanes
lanes: 2
synths:
- id: 0
  name: lead
  group: PLAYABLE
  state: ""
- id: 1
  name: pad
  group: BACKGROUND
  state: ""
- id: 2
  name: bass
  group: PLAYABLE
  state: ""
events:
- type: BarEvent
  time: 0.0
  bar: 1
  numerator: 4
  denominator: 4
- type: BeatEvent
  time: 0.0
  bar: 1
  beat: 1
- type: PlayableNoteEvent
  time: 0.0
  synth: 0
  lane: 1
  pitch: 60
  velocity: 100
  length_seconds: 0.5
  length_beats: 1.0
- type: BeatEvent
  time: 0.5
  bar: 1
  beat: 2
- type: PlayableNoteEvent
  time: 0.5
  synth: 2
  lane: 0
  pitch: 36
  velocity: 90
  length_seconds: 0.25
  length_beats: 0.5
- type: BackgroundNoteEvent
  time: 0.5
  synth: 1
  pitch: 64
  velocity: 80
  length_seconds: 1.0
  length_beats: 2.0
)";

    auto chart = Chart::fromYaml (yaml);
    REQUIRE (chart.has_value());

    CHECK (chart->name == "lanes");
    REQUIRE (chart->synths.size() == 3);
    REQUIRE (chart->numLanes == 2);

    std::map<int, int> pitchByLane, synthByLane;
    int backgroundCount = 0, barCount = 0, beatCount = 0;
    for (auto& [time, event] : chart->events)
    {
        CHECK (time == event.timeMs);
        if (event.type == ChartEvent::NOTE)
        {
            pitchByLane[event.inputButton] = event.midiNote;
            synthByLane[event.inputButton] = event.synthId;
        }
        else if (event.type == ChartEvent::BACKGROUND_NOTE)
        {
            backgroundCount += 1;
            CHECK (event.inputButton == -1);
            CHECK (event.lengthMs == 1000);
        }
        else if (event.type == ChartEvent::BARLINE)
            barCount += 1;
        else if (event.type == ChartEvent::BEAT)
            beatCount += 1;
    }

    CHECK (pitchByLane[1] == 60);
    CHECK (synthByLane[1] == 0);
    CHECK (pitchByLane[0] == 36);
    CHECK (synthByLane[0] == 2);
    CHECK (backgroundCount == 1);
    CHECK (barCount == 1);
    // The downbeat's BeatEvent is covered by its BarEvent.
    CHECK (beatCount == 1);
}

TEST_CASE ("Chart::fromYaml puts notes without a lane in the first lane", "[chart]")
{
    auto yaml = R"(
title: no lanes
synths:
- id: 0
  name: lead
  group: PLAYABLE
  state: ""
- id: 1
  name: bass
  group: PLAYABLE
  state: ""
events:
- type: PlayableNoteEvent
  time: 0.0
  synth: 0
  pitch: 60
  velocity: 100
  length_seconds: 0.5
  length_beats: 1.0
- type: PlayableNoteEvent
  time: 0.5
  synth: 1
  pitch: 36
  velocity: 100
  length_seconds: 0.5
  length_beats: 1.0
)";

    auto chart = Chart::fromYaml (yaml);
    REQUIRE (chart.has_value());
    CHECK (chart->numLanes == 1);

    int noteCount = 0;
    for (auto& [time, event] : chart->events)
    {
        if (event.type != ChartEvent::NOTE)
            continue;
        CHECK (event.inputButton == 0);
        noteCount += 1;
    }
    CHECK (noteCount == 2);
}

TEST_CASE ("Chart::fromYaml rejects data that isn't a chart", "[chart]")
{
    CHECK_FALSE (Chart::fromYaml ("").has_value());
    CHECK_FALSE (Chart::fromYaml ("just a string").has_value());
    CHECK_FALSE (Chart::fromYaml ("events: [unclosed").has_value());
}

TEST_CASE ("Every bundled YAML chart parses", "[chart]")
{
    int chartCount = 0;
    for (auto& entry : BundledResources::listEntries ("default_charts/yaml"))
    {
        INFO (entry);
        auto chart = Chart::fromYaml (BundledResources::loadFile (entry).toString());
        REQUIRE (chart.has_value());
        CHECK (chart->numLanes > 0);
        for (auto& synth : chart->synths)
            CHECK (synth.state.getSize() > 0);
        chartCount += 1;
    }

    CHECK (chartCount > 0);
}

TEST_CASE ("AutomationLane::valueAt ramps between points", "[chart][automation]")
{
    AutomationLane lane { 0, "amp_gain", {} };
    lane.points.push_back ({ 1000.0, 0.2 });
    lane.points.push_back ({ 2000.0, 0.6 });
    // A jump: two points at the same time.
    lane.points.push_back ({ 3000.0, 0.6 });
    lane.points.push_back ({ 3000.0, 0.0 });
    AutomationPoint curved { 4000.0, 0.0 };
    curved.hasCurve = true;
    curved.curve = { 0.5, 0.0, 1.0, 0.5 }; // eases in: below the straight line in the middle
    lane.points.push_back (curved);
    lane.points.push_back ({ 5000.0, 1.0 });

    using Catch::Matchers::WithinAbs;
    CHECK_THAT (lane.valueAt (0.0), WithinAbs (0.2, 1e-9)); // held before the first point
    CHECK_THAT (lane.valueAt (1000.0), WithinAbs (0.2, 1e-9));
    CHECK_THAT (lane.valueAt (1500.0), WithinAbs (0.4, 1e-9));
    CHECK_THAT (lane.valueAt (2999.0), WithinAbs (0.6, 1e-9));
    CHECK_THAT (lane.valueAt (3000.0), WithinAbs (0.0, 1e-9)); // the jump's later point wins
    CHECK_THAT (lane.valueAt (3500.0), WithinAbs (0.0, 1e-9));
    CHECK (lane.valueAt (4500.0) < 0.5);
    CHECK_THAT (lane.valueAt (4999.999), WithinAbs (1.0, 1e-2));
    CHECK_THAT (lane.valueAt (9000.0), WithinAbs (1.0, 1e-9)); // held after the last point
}

TEST_CASE ("Chart::fromYaml limits charts to MAX_LANES lanes", "[chart]")
{
    auto yaml = R"(
title: too many lanes
lanes: 5
synths:
- id: 0
  name: lead
  group: PLAYABLE
  state: ""
events:
- type: PlayableNoteEvent
  time: 0.0
  synth: 0
  lane: 0
  pitch: 60
  velocity: 100
  length_seconds: 0.5
  length_beats: 1.0
- type: PlayableNoteEvent
  time: 0.5
  synth: 0
  lane: 4
  pitch: 62
  velocity: 100
  length_seconds: 0.5
  length_beats: 1.0
)";

    auto chart = Chart::fromYaml (yaml);
    REQUIRE (chart.has_value());
    CHECK (chart->numLanes == Chart::MAX_LANES);

    auto first = chart->events.find (0);
    auto second = chart->events.find (500);
    REQUIRE (first != chart->events.end());
    REQUIRE (second != chart->events.end());
    CHECK (first->second.inputButton == 0);
    CHECK (second->second.inputButton == Chart::MAX_LANES - 1);
}

TEST_CASE ("Chart::fromYaml groups AutomationEvents into lanes", "[chart][automation]")
{
    auto chart = Chart::fromYaml (R"(
title: automation
synths:
- id: 0
  name: lead
  group: PLAYABLE
  state: ""
events:
- type: AutomationEvent
  time: 0.0
  synth: 0
  parameter: amp_gain
  value: 0.25
- type: AutomationEvent
  time: 0.5
  synth: 0
  parameter: fil1_freq
  value: 0.1
- type: AutomationEvent
  time: 1.0
  synth: 0
  parameter: amp_gain
  value: 0.75
  curve: [0.25, 0.0, 0.75, 1.0]
- type: AutomationEvent
  time: 2.0
  synth: 0
  parameter: amp_gain
  value: 1.0
)");
    REQUIRE (chart.has_value());
    REQUIRE (chart->automation.size() == 2);

    auto& gain = chart->automation[0];
    CHECK (gain.synthId == 0);
    CHECK (gain.parameterId == "amp_gain");
    REQUIRE (gain.points.size() == 3);
    CHECK (gain.points[1].timeMs == 1000.0);
    CHECK_FALSE (gain.points[0].hasCurve);
    CHECK (gain.points[1].hasCurve);
    CHECK (gain.points[1].curve[2] == 0.75);
    CHECK_THAT (gain.valueAt (500.0), Catch::Matchers::WithinAbs (0.5, 1e-9));

    CHECK (chart->automation[1].parameterId == "fil1_freq");
}