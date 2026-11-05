#include <GameState.h>
#include <UI/NoteLaneDisplay.h>

#include <catch2/catch_test_macros.hpp>

#include <deque>

namespace
{
    // A 400x300 display showing one 1000 ms bar in its top row, with the playhead at its start.
    struct OneBarDisplay
    {
        explicit OneBarDisplay (int numLanes)
        {
            auto chart = Chart::fromYaml ("title: lanes\nlanes: 2\nevents: []\n");
            REQUIRE (chart.has_value());
            state.charts.push_back (std::move (*chart));
            state.currentChart = &state.charts.front();
            lanes = numLanes;
        }

        void addNote (long long timeMs, int lane, int midiNote)
        {
            auto& note = notes.emplace_back (timeMs, ChartEvent::NOTE);
            note.inputButton = lane;
            note.midiNote = midiNote;
            events.insert ({ timeMs, &note });
        }

        juce::Image paint()
        {
            NoteLaneDisplay display (&state, lanes);
            display.setBounds (0, 0, 400, 300);
            display.setEvents (&events);
            display.setBars ({ { 0, 1000 } }, 0);
            display.setPlayheadTimeMs (0);

            juce::Image image (juce::Image::ARGB, 400, 300, true, juce::SoftwareImageType());
            juce::Graphics g (image);
            display.paint (g);
            return image;
        }

        juce::Colour laneColour (int lane) const
        {
            return state.currentChart->colours.colourAt (LaneColours::lane0 + lane, 0.0);
        }

        GameState state;
        int lanes;
        std::deque<ChartEvent> notes;
        std::multimap<long long, ChartEvent*> events;
    };

    // The top row's inner area spans y 14 to 57, and x 14 to 386 for the bar's 1000 ms.
    constexpr int topRowTop = 5, topRowBottom = 66;
    constexpr int emptyColumnX = 50;

    int xAtTime (long long timeMs)
    {
        return juce::roundToInt (14.0f + 372.0f * static_cast<float> (timeMs) / 1000.0f);
    }

    bool sameRgb (juce::Colour a, juce::Colour b)
    {
        return std::abs (a.getRed() - b.getRed()) <= 3 && std::abs (a.getGreen() - b.getGreen()) <= 3
               && std::abs (a.getBlue() - b.getBlue()) <= 3;
    }

    // Whether a pixel shows a line of this colour: half see-through, over the default row
    // background (a quarter-opaque black).
    bool isLinePixel (juce::Colour pixel, juce::Colour colour)
    {
        auto expected = juce::Colours::black.withAlpha (0.25f).overlaidWith (colour.withAlpha (0.5f));
        return std::abs (pixel.getAlpha() - expected.getAlpha()) <= 3 && sameRgb (pixel, expected);
    }

    // The ys in the top row where the column at x shows a line of this colour.
    std::vector<int> lineYs (const juce::Image& image, int x, juce::Colour colour)
    {
        std::vector<int> ys;
        for (int y = topRowTop + 2; y < topRowBottom - 2; ++y)
            if (isLinePixel (image.getPixelAt (x, y), colour))
                ys.push_back (y);
        return ys;
    }

    // The middle of the opaque run of this colour in the column at x, or -1 if there's none.
    float noteCentreY (const juce::Image& image, int x, juce::Colour colour)
    {
        std::vector<int> ys;
        for (int y = topRowTop + 2; y < topRowBottom - 2; ++y)
            if (image.getPixelAt (x, y) == colour)
                ys.push_back (y);
        return ys.empty() ? -1.0f : static_cast<float> (ys.front() + ys.back()) / 2.0f;
    }
}

TEST_CASE ("NoteLaneDisplay draws a horizontal line in each row for each lane", "[ui][lanes]")
{
    OneBarDisplay display (2);
    auto image = display.paint();

    auto lane0 = lineYs (image, emptyColumnX, display.laneColour (0));
    auto lane1 = lineYs (image, emptyColumnX, display.laneColour (1));
    REQUIRE (lane0.size() == 1);
    REQUIRE (lane1.size() == 1);
    CHECK (lane0.front() < lane1.front());

    SECTION ("spaced evenly down the row")
    {
        auto top = lane0.front() - 14, middle = lane1.front() - lane0.front(), bottom = 57 - lane1.front();
        CHECK (std::abs (top - middle) <= 1);
        CHECK (std::abs (middle - bottom) <= 1);
    }

    SECTION ("in every row, not just the one being played")
    {
        for (auto rowOffset : { 67, 134, 201 })
        {
            CHECK (isLinePixel (image.getPixelAt (emptyColumnX, lane0.front() + rowOffset), display.laneColour (0)));
            CHECK (isLinePixel (image.getPixelAt (emptyColumnX, lane1.front() + rowOffset), display.laneColour (1)));
        }
    }
}

TEST_CASE ("A one-lane chart has a single line across the middle of each row", "[ui][lanes]")
{
    OneBarDisplay display (1);
    auto image = display.paint();

    auto lane0 = lineYs (image, emptyColumnX, display.laneColour (0));
    REQUIRE (lane0.size() == 1);
    CHECK (std::abs (lane0.front() - (14 + 57) / 2) <= 1);
    CHECK (lineYs (image, emptyColumnX, display.laneColour (1)).empty());
}

TEST_CASE ("Notes sit on their lane's line whatever their pitch", "[ui][lanes]")
{
    OneBarDisplay display (2);
    display.addNote (250, 0, 100);
    display.addNote (500, 0, 30);
    display.addNote (750, 1, 64);
    display.addNote (875, 1, 20);
    auto image = display.paint();

    auto lane0Line = (float) lineYs (image, emptyColumnX, display.laneColour (0)).front();
    auto lane1Line = (float) lineYs (image, emptyColumnX, display.laneColour (1)).front();

    CHECK (noteCentreY (image, xAtTime (250), display.laneColour (0)) == lane0Line);
    CHECK (noteCentreY (image, xAtTime (500), display.laneColour (0)) == lane0Line);
    CHECK (noteCentreY (image, xAtTime (750), display.laneColour (1)) == lane1Line);
    CHECK (noteCentreY (image, xAtTime (875), display.laneColour (1)) == lane1Line);
}
