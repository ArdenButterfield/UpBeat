#include <Audio/ChartSynthBank.h>
#include <Audio/OdinSynth.h>
#include <BundledResources.h>

#include <juce_audio_processors/juce_audio_processors.h>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

namespace
{
    // Odin stores each automatable parameter as a PARAM child with "id" and "value" properties.
    juce::var getParamValue (const juce::ValueTree& patch, const juce::String& id)
    {
        return patch.getChildWithProperty ("id", id)["value"];
    }
}

TEST_CASE ("OdinSynth::loadPreset loads a .odin file", "[odin]")
{
    auto presetData = BundledResources::loadFile ("odin_presets/test_preset.odin");
    REQUIRE (presetData.getSize() > 0);

    auto preset = juce::ValueTree::readFromData (presetData.getData(), presetData.getSize());
    REQUIRE (preset.hasType ("Odin"));

    OdinSynth synth;
    synth.prepareToPlay (44100.0, 512);

    auto before = synth.getPreset();
    REQUIRE (synth.loadPreset (presetData));
    auto after = synth.getPreset();

    int numParams = 0;
    int numChangedParams = 0;
    for (const auto& child : preset)
    {
        if (! child.hasType ("PARAM"))
            continue;

        auto id = child["id"].toString();
        // Odin deliberately doesn't load these performance controls from patches.
        if (id == "modwheel" || id == "pitchbend")
            continue;

        INFO ("parameter: " << id);
        auto expected = static_cast<double> (child["value"]);
        CHECK_THAT (static_cast<double> (getParamValue (after, id)), Catch::Matchers::WithinAbs (expected, 1e-4));

        ++numParams;
        if (std::abs (static_cast<double> (getParamValue (before, id)) - expected) > 1e-4)
            ++numChangedParams;
    }

    CHECK (numParams > 0);
    // Otherwise the preset is indistinguishable from Odin's default patch and the checks above prove nothing.
    CHECK (numChangedParams > 0);

    SECTION ("the loaded preset makes sound")
    {
        juce::AudioBuffer<float> buffer (2, 512);
        buffer.clear();
        synth.noteOn (60, 100, 0.5);

        float peak = 0.0f;
        for (int block = 0; block < 40; ++block)
        {
            buffer.clear();
            synth.renderNextBlock (buffer, 0, buffer.getNumSamples());
            peak = std::max (peak, buffer.getMagnitude (0, buffer.getNumSamples()));
        }

        CHECK (peak > 0.0f);
    }
}

TEST_CASE ("OdinSynth::loadPreset rejects data that isn't an Odin patch", "[odin]")
{
    OdinSynth synth;
    synth.prepareToPlay (44100.0, 512);
    auto before = synth.getPreset();

    SECTION ("garbage bytes")
    {
        const char garbage[] = "definitely not an odin patch";
        CHECK_FALSE (synth.loadPreset (juce::MemoryBlock (garbage, sizeof (garbage))));
    }

    SECTION ("a patch from a newer version of Odin")
    {
        auto presetData = BundledResources::loadFile ("odin_presets/test_preset.odin");
        auto preset = juce::ValueTree::readFromData (presetData.getData(), presetData.getSize());
        preset.getChildWithName ("misc").setProperty ("patch_migration_version", 1000, nullptr);

        juce::MemoryOutputStream newerData;
        preset.writeToStream (newerData);
        CHECK_FALSE (synth.loadPreset (newerData.getMemoryBlock()));
    }

    SECTION ("a missing file")
    {
        CHECK_FALSE (synth.loadPreset (juce::File::getCurrentWorkingDirectory().getChildFile ("does_not_exist.odin")));
    }

    CHECK (synth.getPreset().isEquivalentTo (before));
}

TEST_CASE ("OdinSynth::loadState loads every bundled chart's synth states", "[odin]")
{
    for (auto& entry : BundledResources::listEntries ("default_charts/yaml"))
    {
        auto chart = Chart::fromYaml (BundledResources::loadFile (entry).toString());
        REQUIRE (chart.has_value());

        for (auto& chartSynth : chart->synths)
        {
            INFO (entry << ": " << chartSynth.name);
            OdinSynth synth;
            synth.prepareToPlay (44100.0, 512);
            CHECK (synth.loadState (chartSynth.state));

            // The state's patch name survives the load, so it really was this synth's patch.
            auto expected = juce::AudioProcessor::getXmlFromBinary (chartSynth.state.getData(), (int) chartSynth.state.getSize());
            REQUIRE (expected != nullptr);
            CHECK (synth.getPreset().getChildWithName ("misc")["patch_name"].toString()
                   == expected->getChildByName ("misc")->getStringAttribute ("patch_name"));
        }
    }
}

TEST_CASE ("OdinSynth::loadState rejects data that isn't Odin state", "[odin]")
{
    OdinSynth synth;
    synth.prepareToPlay (44100.0, 512);

    const char garbage[] = "definitely not odin state";
    CHECK_FALSE (synth.loadState (juce::MemoryBlock (garbage, sizeof (garbage))));
    CHECK_FALSE (synth.loadState ({}));
}

TEST_CASE ("ChartSynthBank plays notes only on the addressed synth", "[odin]")
{
    auto chart = Chart::fromYaml (BundledResources::loadFile ("default_charts/yaml/test.yaml").toString());
    REQUIRE (chart.has_value());
    REQUIRE (! chart->synths.empty());

    ChartSynthBank bank (*chart);
    bank.prepareToPlay (44100.0, 512);

    auto renderPeak = [&bank]
    {
        juce::AudioBuffer<float> buffer (2, 512);
        float peak = 0.0f;
        for (int block = 0; block < 40; ++block)
        {
            buffer.clear();
            bank.renderNextBlock (buffer, 0, buffer.getNumSamples());
            peak = std::max (peak, buffer.getMagnitude (0, buffer.getNumSamples()));
        }
        return peak;
    };

    bank.noteOn (-1, 60, 100, 0.5);
    CHECK (renderPeak() == 0.0f);

    bank.noteOn (chart->synths.front().id, 60, 100, 0.5);
    CHECK (renderPeak() > 0.0f);
}

TEST_CASE ("OdinSynth parameters can be set by id", "[odin][automation]")
{
    OdinSynth synth;
    synth.prepareToPlay (44100.0, 512);

    CHECK (synth.getParameterIndex ("not_an_odin_parameter") == -1);

    auto index = synth.getParameterIndex ("fil1_freq");
    REQUIRE (index >= 0);

    auto before = static_cast<double> (getParamValue (synth.getPreset(), "fil1_freq"));
    auto target = synth.getParameterValue (index) < 0.5f ? 0.75f : 0.25f;
    synth.setParameterValue (index, target);
    CHECK_THAT (synth.getParameterValue (index), Catch::Matchers::WithinAbs (target, 1e-6));
    // Odin's own patch follows (in the parameter's real units), as it would for host automation.
    CHECK (static_cast<double> (getParamValue (synth.getPreset(), "fil1_freq")) != before);
}

TEST_CASE ("Every parameter the bundled charts automate exists in Odin", "[odin][automation]")
{
    OdinSynth synth;
    for (auto& entry : BundledResources::listEntries ("default_charts/yaml"))
    {
        auto chart = Chart::fromYaml (BundledResources::loadFile (entry).toString());
        REQUIRE (chart.has_value());

        for (auto& lane : chart->automation)
        {
            INFO (entry << ": " << lane.parameterId);
            CHECK (synth.getParameterIndex (lane.parameterId) >= 0);
        }
    }
}

TEST_CASE ("OdinSynth::noteOn plays louder at higher velocities", "[odin]")
{
    auto peakAtVelocity = [] (int velocity)
    {
        OdinSynth synth;
        synth.prepareToPlay (44100.0, 512);
        // Odin's default patch ignores velocity; make the amplifier fully velocity sensitive.
        synth.setParameterValue (synth.getParameterIndex ("amp_velocity"), 1.0f);
        synth.noteOn (60, velocity, 0.5);

        juce::AudioBuffer<float> buffer (2, 512);
        float peak = 0.0f;
        for (int block = 0; block < 40; ++block)
        {
            buffer.clear();
            synth.renderNextBlock (buffer, 0, buffer.getNumSamples());
            peak = std::max (peak, buffer.getMagnitude (0, buffer.getNumSamples()));
        }
        return peak;
    };

    auto quiet = peakAtVelocity (20);
    auto loud = peakAtVelocity (127);
    CHECK (quiet > 0.0f);
    CHECK (loud > quiet * 1.5f);
}
