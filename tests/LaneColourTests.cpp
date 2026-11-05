#include <Chart/Chart.h>
#include <GameState.h>
#include <UI/NoteLaneDisplay.h>

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <set>

using Catch::Matchers::WithinAbs;

namespace
{
    Chart parseChart (const juce::String& yaml)
    {
        auto chart = Chart::fromYaml (yaml);
        REQUIRE (chart.has_value());
        return std::move (*chart);
    }

    // Written by Ableton Charts/parse_als.py; see test_parse_als.py there.
    juce::String loadParserFixture()
    {
        auto file = juce::File (UPBEAT_TEST_FIXTURES_DIR).getChildFile ("colour_chart.yaml");
        REQUIRE (file.existsAsFile());
        return file.loadFileAsString();
    }

    // Paints a NoteLaneDisplay with no bars, leaving just its rows and key indicators.
    struct PaintedDisplay
    {
        explicit PaintedDisplay (Chart chart)
        {
            state.charts.push_back (std::move (chart));
            state.currentChart = &state.charts.front();
        }

        juce::Image paint (double chartTimeMs)
        {
            NoteLaneDisplay display (&state, 2);
            display.setBounds (0, 0, 400, 300);
            display.setChartTimeMs (chartTimeMs);

            juce::Image image (juce::Image::ARGB, 400, 300, true, juce::SoftwareImageType());
            juce::Graphics g (image);
            display.paint (g);
            return image;
        }

        GameState state;
    };

    // Places inside the 400x300 display where only one colour is drawn.
    constexpr int rowOutlineX = 5, rowOutlineY = 20;
    constexpr int rowInsideX = 100, rowInsideY = 20;
    constexpr int lane0IndicatorX = 7, lane1IndicatorX = 202, indicatorY = 285;
}

TEST_CASE ("toOklab matches known Oklab values", "[colours]")
{
    auto white = toOklab (juce::Colours::white);
    CHECK_THAT (white.L, WithinAbs (1.0, 1e-4));
    CHECK_THAT (white.a, WithinAbs (0.0, 1e-4));
    CHECK_THAT (white.b, WithinAbs (0.0, 1e-4));

    auto black = toOklab (juce::Colours::black);
    CHECK_THAT (black.L, WithinAbs (0.0, 1e-6));

    // Reference values from Bjorn Ottosson's Oklab post and its sRGB transfer function.
    auto red = toOklab (juce::Colour (0xffff0000));
    CHECK_THAT (red.L, WithinAbs (0.62796, 1e-3));
    CHECK_THAT (red.a, WithinAbs (0.22486, 1e-3));
    CHECK_THAT (red.b, WithinAbs (0.12585, 1e-3));

    auto blue = toOklab (juce::Colour (0xff0000ff));
    CHECK_THAT (blue.L, WithinAbs (0.45201, 1e-3));
    CHECK_THAT (blue.a, WithinAbs (-0.03246, 1e-3));
    CHECK_THAT (blue.b, WithinAbs (-0.31153, 1e-3));
}

TEST_CASE ("Colours survive a round trip through Oklab", "[colours]")
{
    auto argb = GENERATE (0xff000000u, 0xffffffffu, 0xff808080u, 0xffff69b4u, 0xff00bfffu, 0xff123456u, 0xfffe0102u, 0xff01fe80u);
    CHECK (toColour (toOklab (juce::Colour (argb))).getARGB() == argb);
}

TEST_CASE ("Oklab colours a screen can't show are clipped", "[colours]")
{
    auto tooBright = toColour ({ 1.0, 0.4, 0.4 });
    CHECK (tooBright.getAlpha() == 255);
    CHECK (tooBright.getRed() == 255);

    CHECK (toColour ({ 0.0, 0.0, 0.0 }).getARGB() == 0xff000000);
    CHECK (toColour ({ 1.0, 0.0, 0.0 }).getARGB() == 0xffffffff);
}

TEST_CASE ("Every Oklab channel range holds every screen colour", "[colours]")
{
    for (int r = 0; r <= 255; r += 51)
        for (int g = 0; g <= 255; g += 51)
            for (int b = 0; b <= 255; b += 51)
            {
                auto lab = toOklab (juce::Colour ((juce::uint8) r, (juce::uint8) g, (juce::uint8) b));
                for (int channel = 0; channel < LaneColours::numChannels; ++channel)
                {
                    auto& info = LaneColours::channels[(size_t) channel];
                    // Pure white's L can land a hair above 1 from rounding in the matrix.
                    CHECK (lab[channel] >= info.minValue - 1e-4);
                    CHECK (lab[channel] <= info.maxValue + 1e-4);
                }
            }
}

TEST_CASE ("LaneColours has a unique, findable id for each colour", "[colours]")
{
    std::set<std::string> ids;
    for (int colour = 0; colour < LaneColours::numColours; ++colour)
    {
        auto& info = LaneColours::infos[(size_t) colour];
        ids.insert (info.id);
        CHECK (LaneColours::findColour (info.id) == colour);
        CHECK (juce::String (info.title).isNotEmpty());
    }
    CHECK (ids.size() == LaneColours::numColours);
    CHECK_FALSE (LaneColours::findColour ("not a colour").has_value());

    CHECK (LaneColours::findChannel ("a") == 1);
    CHECK_FALSE (LaneColours::findChannel ("A").has_value());
    CHECK (LaneColours::parameterId (LaneColours::playhead, 0) == "playhead_L");
}

TEST_CASE ("ChartColours defaults to the original display colours", "[colours]")
{
    ChartColours colours;
    CHECK (colours.colourAt (LaneColours::rowBackground, 0.0) == juce::Colours::black);
    CHECK (colours.colourAt (LaneColours::rowOutline, 0.0) == juce::Colours::grey);
    CHECK (colours.colourAt (LaneColours::rowActiveOutline, 0.0) == juce::Colours::white);
    CHECK (colours.colourAt (LaneColours::barline, 0.0) == juce::Colours::white);
    CHECK (colours.colourAt (LaneColours::beatLine, 0.0) == juce::Colours::white);
    CHECK (colours.colourAt (LaneColours::playhead, 0.0) == juce::Colours::white);
    CHECK (colours.colourAt (LaneColours::lane0, 0.0) == juce::Colours::hotpink);
    CHECK (colours.colourAt (LaneColours::lane1, 0.0) == juce::Colours::deepskyblue);
}

TEST_CASE ("ChartColours automation overrides only the channels it covers", "[colours]")
{
    ChartColours colours;
    colours.base[LaneColours::playhead] = { 0.5, 0.1, -0.1 };

    SECTION ("without automation the base value holds at every time")
    {
        auto lab = colours.oklabAt (LaneColours::playhead, 12345.0);
        CHECK (lab.L == 0.5);
        CHECK (lab.a == 0.1);
        CHECK (lab.b == -0.1);
    }

    SECTION ("automated channels ramp linearly in Oklab; the others stay at their base")
    {
        colours.automation[LaneColours::playhead][0].points = { { 1000.0, 0.2 }, { 2000.0, 0.8 } };

        auto lab = colours.oklabAt (LaneColours::playhead, 1500.0);
        CHECK_THAT (lab.L, WithinAbs (0.5, 1e-9));
        CHECK (lab.a == 0.1);
        CHECK (lab.b == -0.1);

        CHECK_THAT (colours.oklabAt (LaneColours::playhead, 0.0).L, WithinAbs (0.2, 1e-9));
        CHECK_THAT (colours.oklabAt (LaneColours::playhead, 5000.0).L, WithinAbs (0.8, 1e-9));
    }

    SECTION ("other colours aren't affected")
    {
        colours.automation[LaneColours::playhead][0].points = { { 0.0, 0.2 } };
        CHECK (colours.colourAt (LaneColours::lane0, 0.0) == juce::Colours::hotpink);
    }

    SECTION ("paletteAt gives each colour at that time")
    {
        colours.automation[LaneColours::lane1][0].points = { { 0.0, 0.0 }, { 1000.0, 1.0 } };
        auto palette = colours.paletteAt (500.0);
        CHECK (palette[LaneColours::lane1] == colours.colourAt (LaneColours::lane1, 500.0));
        CHECK (palette[LaneColours::lane0] == juce::Colours::hotpink);
    }
}

TEST_CASE ("Chart::fromYaml keeps the default colours when a chart sets none", "[chart][colours]")
{
    auto chart = parseChart ("title: plain\nevents: []\n");
    for (int colour = 0; colour < LaneColours::numColours; ++colour)
        CHECK (chart.colours.colourAt (colour, 0.0) == juce::Colour (LaneColours::infos[(size_t) colour].defaultArgb));
}

TEST_CASE ("Chart::fromYaml reads a chart's colours", "[chart][colours]")
{
    auto chart = parseChart (R"(
title: colours
colours:
  playhead: { L: 0.5, a: 0.1, b: -0.2 }
  lane_1: { L: 0.7 }
  not_a_colour: { L: 0.1, a: 0.1, b: 0.1 }
events: []
)");

    auto playhead = chart.colours.base[LaneColours::playhead];
    CHECK (playhead.L == 0.5);
    CHECK (playhead.a == 0.1);
    CHECK (playhead.b == -0.2);

    // A colour with only some channels keeps the default of the rest.
    auto lane1 = chart.colours.base[LaneColours::lane1];
    auto lane1Default = LaneColours::defaultOklab (LaneColours::lane1);
    CHECK (lane1.L == 0.7);
    CHECK (lane1.a == lane1Default.a);
    CHECK (lane1.b == lane1Default.b);

    CHECK (chart.colours.colourAt (LaneColours::barline, 0.0) == juce::Colours::white);
}

TEST_CASE ("Chart::fromYaml reads colour automation events", "[chart][colours]")
{
    auto chart = parseChart (R"(
title: automation
events:
- { type: ColourAutomationEvent, time: 2.0, colour: playhead, channel: L, value: 0.8 }
- { type: ColourAutomationEvent, time: 1.0, colour: playhead, channel: L, value: 0.2, curve: [0.25, 0.0, 1.0, 0.0] }
- { type: ColourAutomationEvent, time: 0.0, colour: lane_0, channel: b, value: 0.1 }
- { type: ColourAutomationEvent, time: 0.0, colour: not_a_colour, channel: L, value: 0.1 }
- { type: ColourAutomationEvent, time: 0.0, colour: playhead, channel: not_a_channel, value: 0.1 }
)");

    SECTION ("each goes to its colour and channel, in time order")
    {
        auto& points = chart.colours.automation[LaneColours::playhead][0].points;
        REQUIRE (points.size() == 2);
        CHECK (points[0].timeMs == 1000.0);
        CHECK (points[0].value == 0.2);
        CHECK (points[0].hasCurve);
        CHECK (points[0].curve[2] == 1.0);
        CHECK (points[1].timeMs == 2000.0);
        CHECK_FALSE (points[1].hasCurve);

        CHECK (chart.colours.automation[LaneColours::lane0][2].points.size() == 1);
    }

    SECTION ("invalid colours and channels are ignored")
    {
        CHECK (chart.colours.automation[LaneColours::playhead][1].points.empty());
        CHECK (chart.colours.automation[LaneColours::playhead][2].points.empty());
    }

    SECTION ("they aren't treated as synth automation or chart events")
    {
        CHECK (chart.automation.empty());
        CHECK (chart.events.empty());
    }
}

TEST_CASE ("Each chart has its own colours", "[chart][colours]")
{
    auto first = parseChart ("title: a\ncolours:\n  playhead: { L: 0.1, a: 0.0, b: 0.0 }\nevents: []\n");
    auto second = parseChart ("title: b\ncolours:\n  playhead: { L: 0.9, a: 0.0, b: 0.0 }\nevents: []\n");
    CHECK (first.colours.base[LaneColours::playhead].L == 0.1);
    CHECK (second.colours.base[LaneColours::playhead].L == 0.9);
}

TEST_CASE ("Chart::fromYaml reads what parse_als.py writes", "[chart][colours]")
{
    auto chart = parseChart (loadParserFixture());

    SECTION ("every colour's value, by the parser's ids and channel names")
    {
        for (int colour = 0; colour < LaneColours::numColours; ++colour)
        {
            INFO (LaneColours::infos[(size_t) colour].id);
            auto lab = chart.colours.base[(size_t) colour];
            CHECK_THAT (lab.L, WithinAbs (0.1 + 0.1 * colour, 1e-9));
            CHECK_THAT (lab.a, WithinAbs (-0.1 + 0.02 * colour, 1e-9));
            CHECK_THAT (lab.b, WithinAbs (0.1 - 0.02 * colour, 1e-9));
        }
    }

    SECTION ("automation ramps in Oklab, holds and jumps")
    {
        auto valueAt = [&chart] (double timeMs) { return chart.colours.oklabAt (LaneColours::playhead, timeMs).L; };
        CHECK_THAT (valueAt (0.0), WithinAbs (0.25, 1e-6));
        CHECK_THAT (valueAt (500.0), WithinAbs (0.5, 1e-6));
        CHECK_THAT (valueAt (1000.0), WithinAbs (0.75, 1e-6));
        CHECK_THAT (valueAt (1999.0), WithinAbs (0.75, 1e-6));
        CHECK_THAT (valueAt (2000.0), WithinAbs (0.1, 1e-6));
    }

    SECTION ("curves carry through; this one eases in, staying under the straight line")
    {
        auto valueAt = [&chart] (double timeMs) { return chart.colours.oklabAt (LaneColours::lane0, timeMs).a; };
        CHECK_THAT (valueAt (500.0), WithinAbs (-0.2, 1e-6));
        CHECK (valueAt (1000.0) < -0.05);
        CHECK_THAT (valueAt (1500.0), WithinAbs (0.2, 1e-6));
    }

    SECTION ("channels with no automation stay at their base value")
    {
        CHECK_THAT (chart.colours.oklabAt (LaneColours::playhead, 500.0).a, WithinAbs (-0.1 + 0.02 * static_cast<int> (LaneColours::playhead), 1e-9));
    }
}

TEST_CASE ("NoteLaneDisplay draws with the current chart's colours", "[colours][ui]")
{
    auto chart = parseChart (R"(
title: display
lanes: 2
colours:
  row_background: { L: 0.3, a: 0.1, b: 0.0 }
  row_outline: { L: 0.6, a: -0.1, b: 0.1 }
  lane_0: { L: 0.7, a: 0.1, b: 0.1 }
  lane_1: { L: 0.5, a: -0.1, b: -0.1 }
events: []
)");
    auto expected = chart.colours;
    PaintedDisplay display (std::move (chart));
    auto image = display.paint (0.0);

    CHECK (image.getPixelAt (lane0IndicatorX, indicatorY) == expected.colourAt (LaneColours::lane0, 0.0));
    CHECK (image.getPixelAt (lane1IndicatorX, indicatorY) == expected.colourAt (LaneColours::lane1, 0.0));
    CHECK (image.getPixelAt (rowOutlineX, rowOutlineY) == expected.colourAt (LaneColours::rowOutline, 0.0));

    // The row background keeps its fixed transparency.
    auto inside = image.getPixelAt (rowInsideX, rowInsideY);
    CHECK (std::abs (inside.getAlpha() - 64) <= 1);
    auto backgroundColour = expected.colourAt (LaneColours::rowBackground, 0.0);
    CHECK (std::abs (inside.getRed() - backgroundColour.getRed()) <= 3);
    CHECK (std::abs (inside.getGreen() - backgroundColour.getGreen()) <= 3);
    CHECK (std::abs (inside.getBlue() - backgroundColour.getBlue()) <= 3);
}

TEST_CASE ("NoteLaneDisplay's default colours match the original look", "[colours][ui]")
{
    PaintedDisplay display (parseChart ("title: plain\nlanes: 2\nevents: []\n"));
    auto image = display.paint (0.0);

    CHECK (image.getPixelAt (lane0IndicatorX, indicatorY) == juce::Colours::hotpink);
    CHECK (image.getPixelAt (lane1IndicatorX, indicatorY) == juce::Colours::deepskyblue);
    CHECK (image.getPixelAt (rowOutlineX, rowOutlineY) == juce::Colours::grey);
}

TEST_CASE ("NoteLaneDisplay's colours follow automation at the chart time", "[colours][ui]")
{
    auto chart = parseChart (R"(
title: automated
lanes: 2
events:
- { type: ColourAutomationEvent, time: 1.0, colour: lane_0, channel: L, value: 0.2 }
- { type: ColourAutomationEvent, time: 2.0, colour: lane_0, channel: L, value: 0.8 }
)");
    auto expected = chart.colours;
    PaintedDisplay display (std::move (chart));

    auto at = [&] (double timeMs) { return display.paint (timeMs).getPixelAt (lane0IndicatorX, indicatorY); };

    CHECK (at (0.0) == expected.colourAt (LaneColours::lane0, 1000.0));
    CHECK (at (1500.0) == expected.colourAt (LaneColours::lane0, 1500.0));
    CHECK (at (2000.0) == expected.colourAt (LaneColours::lane0, 2000.0));
    CHECK (at (1000.0) != at (2000.0));
    CHECK (at (1500.0).getPerceivedBrightness() > at (1000.0).getPerceivedBrightness());
    CHECK (at (2000.0).getPerceivedBrightness() > at (1500.0).getPerceivedBrightness());
}
