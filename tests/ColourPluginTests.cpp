#include <ColourPluginEditor.h>
#include <ColourPluginProcessor.h>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <set>

using Catch::Matchers::WithinAbs;

namespace
{
    juce::RangedAudioParameter& parameter (ColourPluginProcessor& processor, int colour, int channel)
    {
        auto* found = processor.getParameters().getParameter (LaneColours::parameterId (colour, channel));
        REQUIRE (found != nullptr);
        return *found;
    }

    void setOklab (ColourPluginProcessor& processor, int colour, Oklab lab)
    {
        for (int channel = 0; channel < LaneColours::numChannels; ++channel)
        {
            auto& param = parameter (processor, colour, channel);
            param.setValueNotifyingHost (param.convertTo0to1 (static_cast<float> (lab[channel])));
        }
    }
}

TEST_CASE ("UpBeat Colours has an L, a and b parameter for each colour", "[colourplugin]")
{
    ColourPluginProcessor processor;

    CHECK (processor.getName() == "UpBeat Colours");
    CHECK (processor.getParameters().processor.getParameters().size() == LaneColours::numColours * LaneColours::numChannels);

    std::set<juce::String> names;
    for (int colour = 0; colour < LaneColours::numColours; ++colour)
    {
        for (int channel = 0; channel < LaneColours::numChannels; ++channel)
        {
            auto& info = LaneColours::channels[(size_t) channel];
            auto& param = parameter (processor, colour, channel);
            CHECK (param.getNormalisableRange().start == info.minValue);
            CHECK (param.getNormalisableRange().end == info.maxValue);
            names.insert (param.getName (100));
        }
    }
    CHECK (names.size() == (size_t) (LaneColours::numColours * LaneColours::numChannels));
}

TEST_CASE ("UpBeat Colours starts on the game's default colours", "[colourplugin]")
{
    ColourPluginProcessor processor;
    for (int colour = 0; colour < LaneColours::numColours; ++colour)
    {
        INFO (LaneColours::infos[(size_t) colour].id);
        CHECK (toColour (processor.getOklab (colour)).getARGB() == LaneColours::infos[(size_t) colour].defaultArgb);
    }
}

TEST_CASE ("UpBeat Colours parameters map linearly from normalised values", "[colourplugin]")
{
    // parse_als.py converts automation with this same mapping, so linear automation in
    // Ableton is linear in Oklab. test_parse_als.py checks these same values.
    ColourPluginProcessor processor;

    parameter (processor, LaneColours::playhead, 1).setValueNotifyingHost (0.75f);
    CHECK_THAT (processor.getOklab (LaneColours::playhead).a, WithinAbs (0.2, 1e-6));

    parameter (processor, LaneColours::playhead, 1).setValueNotifyingHost (0.0f);
    CHECK_THAT (processor.getOklab (LaneColours::playhead).a, WithinAbs (-0.4, 1e-6));

    parameter (processor, LaneColours::playhead, 0).setValueNotifyingHost (0.25f);
    CHECK_THAT (processor.getOklab (LaneColours::playhead).L, WithinAbs (0.25, 1e-6));
}

TEST_CASE ("UpBeat Colours parameter ids hash the way parse_als.py expects", "[colourplugin]")
{
    // The VST3 id Ableton saves for a JUCE parameter, which parse_als.py's juce_param_hash
    // computes. test_parse_als.py checks this same value.
    auto vstId = static_cast<juce::uint32> (LaneColours::parameterId (LaneColours::playhead, 0).hashCode()) & 0x7fffffffu;
    CHECK (vstId == 22623713u);
}

TEST_CASE ("UpBeat Colours saves its state in the form parse_als.py reads", "[colourplugin]")
{
    ColourPluginProcessor processor;
    setOklab (processor, LaneColours::lane0, { 0.5, 0.2, -0.1 });

    juce::MemoryBlock state;
    processor.getStateInformation (state);

    SECTION ("a JUCE binary XML block")
    {
        REQUIRE (state.getSize() > 8);
        CHECK (juce::String::fromUTF8 (static_cast<const char*> (state.getData()), 4) == "VC2!");
    }

    SECTION ("PARAM elements holding each parameter's id and real value")
    {
        auto xml = juce::AudioProcessor::getXmlFromBinary (state.getData(), (int) state.getSize());
        REQUIRE (xml != nullptr);

        std::map<juce::String, double> values;
        for (auto* param : xml->getChildWithTagNameIterator ("PARAM"))
            values[param->getStringAttribute ("id")] = param->getDoubleAttribute ("value");

        CHECK (values.size() == (size_t) (LaneColours::numColours * LaneColours::numChannels));
        CHECK_THAT (values["lane_0_L"], WithinAbs (0.5, 1e-6));
        CHECK_THAT (values["lane_0_a"], WithinAbs (0.2, 1e-6));
        CHECK_THAT (values["lane_0_b"], WithinAbs (-0.1, 1e-6));
    }

    SECTION ("which restores the colours")
    {
        ColourPluginProcessor restored;
        restored.setStateInformation (state.getData(), (int) state.getSize());

        auto lab = restored.getOklab (LaneColours::lane0);
        CHECK_THAT (lab.L, WithinAbs (0.5, 1e-6));
        CHECK_THAT (lab.a, WithinAbs (0.2, 1e-6));
        CHECK_THAT (lab.b, WithinAbs (-0.1, 1e-6));
        CHECK (toColour (restored.getOklab (LaneColours::lane1)) == juce::Colours::deepskyblue);
    }

    SECTION ("and ignores data that isn't its own")
    {
        ColourPluginProcessor restored;
        restored.setStateInformation ("nonsense", 8);
        CHECK (toColour (restored.getOklab (LaneColours::lane0)) == juce::Colours::hotpink);
    }
}

TEST_CASE ("UpBeat Colours makes no sound", "[colourplugin]")
{
    ColourPluginProcessor processor;
    CHECK (processor.acceptsMidi());
    CHECK (processor.getTotalNumOutputChannels() == 2);
    CHECK (processor.getTotalNumInputChannels() == 0);

    juce::AudioBuffer<float> buffer (2, 64);
    for (int channel = 0; channel < 2; ++channel)
        juce::FloatVectorOperations::fill (buffer.getWritePointer (channel), 0.5f, 64);
    juce::MidiBuffer midi;
    midi.addEvent (juce::MidiMessage::noteOn (1, 60, (juce::uint8) 100), 0);

    processor.processBlock (buffer, midi);

    CHECK (buffer.getMagnitude (0, 64) == 0.0f);
    CHECK (midi.isEmpty());
}

TEST_CASE ("UpBeat Colours editor is a grid of colour squares", "[colourplugin][ui]")
{
    ColourPluginProcessor processor;
    std::unique_ptr<juce::AudioProcessorEditor> baseEditor (processor.createEditorAndMakeActive());
    auto* editor = dynamic_cast<ColourPluginEditor*> (baseEditor.get());
    REQUIRE (editor != nullptr);

    SECTION ("each square has three vertical sliders and fits in the window without overlapping")
    {
        std::vector<juce::Rectangle<int>> bounds;
        for (int colour = 0; colour < LaneColours::numColours; ++colour)
        {
            auto& square = editor->getSquare (colour);
            CHECK (editor->getLocalBounds().contains (square.getBounds()));
            CHECK_FALSE (square.getBounds().isEmpty());

            for (auto& other : bounds)
                CHECK_FALSE (other.intersects (square.getBounds()));
            bounds.push_back (square.getBounds());

            for (int channel = 0; channel < LaneColours::numChannels; ++channel)
            {
                auto& slider = square.getSlider (channel);
                CHECK (slider.getSliderStyle() == juce::Slider::LinearVertical);
                CHECK (slider.isVisible());
                CHECK (square.getLocalBounds().contains (slider.getBounds()));
                CHECK (slider.getHeight() > slider.getWidth());
            }
        }
    }

    SECTION ("sliders set the parameters of their own colour and channel")
    {
        auto& slider = editor->getSquare (LaneColours::barline).getSlider (2);
        CHECK_THAT (slider.getMinimum(), WithinAbs (-0.4, 1e-6));
        CHECK_THAT (slider.getMaximum(), WithinAbs (0.4, 1e-6));

        slider.setValue (0.3, juce::sendNotificationSync);
        CHECK_THAT (processor.getOklab (LaneColours::barline).b, WithinAbs (0.3, 1e-6));
        CHECK (toColour (processor.getOklab (LaneColours::playhead)) == juce::Colours::white);
    }

    SECTION ("sliders follow the parameters, as they do under the host's automation")
    {
        parameter (processor, LaneColours::lane1, 0).setValueNotifyingHost (0.25f);
        CHECK_THAT (editor->getSquare (LaneColours::lane1).getSlider (0).getValue(), WithinAbs (0.25, 1e-6));
    }

    SECTION ("each square's background is its current colour")
    {
        auto backgroundOf = [&editor] (int colour) {
            auto& square = editor->getSquare (colour);
            return square.createComponentSnapshot (square.getLocalBounds()).getPixelAt (1, 1);
        };

        CHECK (backgroundOf (LaneColours::lane0) == juce::Colours::hotpink);
        CHECK (backgroundOf (LaneColours::lane1) == juce::Colours::deepskyblue);

        Oklab newColour { 0.4, -0.1, 0.1 };
        setOklab (processor, LaneColours::lane0, newColour);
        CHECK (backgroundOf (LaneColours::lane0) == toColour (newColour));
        CHECK (editor->getSquare (LaneColours::lane0).getCurrentColour() == toColour (newColour));
        CHECK (backgroundOf (LaneColours::lane1) == juce::Colours::deepskyblue);
    }
}
