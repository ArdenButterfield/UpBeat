#include <Audio/OdinSynth.h>
#include <BundledResources.h>

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
        synth.noteOn (60, 0.5);

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
