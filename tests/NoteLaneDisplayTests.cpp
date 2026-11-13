#include <GameState.h>
#include <UI/NoteLaneDisplay.h>
#include <UI/ToleranceLabel.h>

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

        void markPlayed (long long timeMs)
        {
            for (auto& note : notes)
                if (note.timeMs == timeMs)
                    note.performanceTimings.push_back (0);
        }

        juce::Image paint (std::vector<NoteLaneDisplay::BarSpan> bars = { { 0, 1000 } }, int firstBarNumber = 0, long long playheadMs = 0)
        {
            NoteLaneDisplay display (&state, lanes);
            display.setBounds (0, 0, 400, 300);
            display.setEvents (&events);
            display.setBars (std::move (bars), firstBarNumber);
            display.setPlayheadTimeMs (playheadMs);

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

    // The top grid line spans x 5 to 395 and y 5 to 134, split between its two rows at x 200.
    // Its inner area spans y 14 to 125, and x 24 to 376 (kept a note's width from the sides);
    // the top-left row's x 24 to 200 is the bar's 1000 ms. The bottom grid line is 135 px down.
    constexpr int topRowTop = 5, topRowBottom = 134;
    constexpr int innerTop = 14, innerBottom = 125;
    // Left of every note the tests place (the earliest at 250 ms, reaching back to x 54), so
    // only the lines are drawn there.
    constexpr int emptyColumnX = 40;

    int xAtTime (long long timeMs)
    {
        return juce::roundToInt (24.0f + 176.0f * static_cast<float> (timeMs) / 1000.0f);
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

    // The middle of each line of this colour crossing the column at x in the top row, as
    // (first + last) / 2 of the pixel rows it covers. Lines are several pixels thick, so a
    // line drawn centred between two pixel rows is found half a pixel above the lower one.
    std::vector<float> lineYs (const juce::Image& image, int x, juce::Colour colour)
    {
        std::vector<float> middles;
        int runStart = -1;
        for (int y = topRowTop + 2; y <= topRowBottom - 2; ++y)
        {
            auto onLine = y < topRowBottom - 2 && isLinePixel (image.getPixelAt (x, y), colour);
            if (onLine && runStart < 0)
                runStart = y;
            else if (!onLine && runStart >= 0)
            {
                middles.push_back (static_cast<float> (runStart + y - 1) / 2.0f);
                runStart = -1;
            }
        }
        return middles;
    }

    // A pixel row covered by a line found by lineYs.
    int onLine (float lineMiddle) { return (int) lineMiddle; }

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
        auto top = lane0.front() - (float) innerTop, middle = lane1.front() - lane0.front(), bottom = (float) innerBottom - lane1.front();
        CHECK (std::abs (top - middle) <= 1.5f);
        CHECK (std::abs (middle - bottom) <= 1.5f);
    }

    SECTION ("in every row, not just the one being played")
    {
        for (auto [dx, dy] : { std::pair { 195, 0 }, { 0, 135 }, { 195, 135 } })
        {
            CHECK (isLinePixel (image.getPixelAt (emptyColumnX + dx, onLine (lane0.front()) + dy), display.laneColour (0)));
            CHECK (isLinePixel (image.getPixelAt (emptyColumnX + dx, onLine (lane1.front()) + dy), display.laneColour (1)));
        }
    }
}

TEST_CASE ("A one-lane chart has a single line across the middle of each row", "[ui][lanes]")
{
    OneBarDisplay display (1);
    auto image = display.paint();

    auto lane0 = lineYs (image, emptyColumnX, display.laneColour (0));
    REQUIRE (lane0.size() == 1);
    CHECK (std::abs (lane0.front() - (float) (innerTop + innerBottom) / 2.0f) <= 1.5f);
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

    auto lane0Lines = lineYs (image, emptyColumnX, display.laneColour (0));
    auto lane1Lines = lineYs (image, emptyColumnX, display.laneColour (1));
    REQUIRE (lane0Lines.size() == 1);
    REQUIRE (lane1Lines.size() == 1);
    auto lane0Line = lane0Lines.front();
    auto lane1Line = lane1Lines.front();

    CHECK (noteCentreY (image, xAtTime (250), display.laneColour (0)) == lane0Line);
    CHECK (noteCentreY (image, xAtTime (500), display.laneColour (0)) == lane0Line);
    CHECK (noteCentreY (image, xAtTime (750), display.laneColour (1)) == lane1Line);
    CHECK (noteCentreY (image, xAtTime (875), display.laneColour (1)) == lane1Line);
}

TEST_CASE ("NoteLaneDisplay lays its four rows out in a 2x2 grid", "[ui][lanes]")
{
    OneBarDisplay display (2);
    auto image = display.paint();
    auto lane0Lines = lineYs (image, emptyColumnX, display.laneColour (0));
    REQUIRE (lane0Lines.size() == 1);
    auto lane0 = onLine (lane0Lines.front());

    // The playing bar's grid line (the top one) has the thicker current-row outline, around
    // both of its bars; the bottom one has the plain outline.
    auto pixel = [&image] (int x, int y) { return image.getPixelAt (x, y); };
    for (auto x : { 5, 6, 7, 392, 393, 394 })
        CHECK (pixel (x, 20) == juce::Colours::white);
    CHECK (pixel (8, 20) != juce::Colours::white);
    for (auto x : { 5, 6, 393, 394 })
        CHECK (pixel (x, 155) == juce::Colours::grey);
    CHECK (pixel (7, 155) != juce::Colours::grey);

    // There's no gap or border where the two bars side by side meet, just the row background.
    for (auto x : { 198, 199, 200, 201, 202 })
    {
        CHECK (std::abs (pixel (x, 20).getAlpha() - 64) <= 1);
        CHECK (pixel (x, 20).getRed() == 0);
    }

    // Their lane lines run straight on from one bar into the next.
    for (auto x : { 190, 199, 200, 201, 210 })
        CHECK (isLinePixel (pixel (x, lane0), display.laneColour (0)));

    // The grid lines above and below still have a gap between them.
    CHECK (pixel (emptyColumnX, 137).getAlpha() == 0);
}

TEST_CASE ("Notes keep their place in the bar on either side of a grid line", "[ui][lanes]")
{
    OneBarDisplay display (2);
    display.addNote (0, 0, 60);
    auto image = display.paint();

    // A note at the very start of the first bar sits inside the border, not on it. (The
    // playhead runs through its middle, so it's checked a little to the right.) It covers
    // the usual empty column, so its lane's line is found in the empty second bar instead.
    auto lane0Lines = lineYs (image, 300, display.laneColour (0));
    REQUIRE (lane0Lines.size() == 1);
    CHECK (noteCentreY (image, xAtTime (0) + 4, display.laneColour (0)) == lane0Lines.front());
    CHECK (image.getPixelAt (5, onLine (lane0Lines.front())) == juce::Colours::white);
}

namespace
{
    // Six 1000 ms bars with one lane-0 note in the middle of each. Bars 1,2 and 5,6 fill the
    // top grid line, bars 3,4 the bottom one.
    std::vector<NoteLaneDisplay::BarSpan> sixBars()
    {
        std::vector<NoteLaneDisplay::BarSpan> bars;
        for (long long i = 0; i < 6; ++i)
            bars.push_back ({ i * 1000, 1000 });
        return bars;
    }

    // Where the note in the middle of a bar in each row is drawn: left or right half, top
    // or bottom grid line, on lane 0's line.
    juce::Point<int> noteSpot (int row)
    {
        return { row % 2 == 0 ? 112 : 288, row / 2 == 0 ? 51 : 186 };
    }

    // How opaque the note drawn at a spot is, from 0 (only the lane line shows) to 1. The
    // lane line under it is 159/255 opaque, and the note covers the rest.
    float noteOpacity (const juce::Image& image, juce::Point<int> spot)
    {
        return static_cast<float> (image.getPixelAt (spot.x, spot.y).getAlpha() - 159) / 96.0f;
    }

    struct SixBarDisplay : OneBarDisplay
    {
        SixBarDisplay() : OneBarDisplay (2)
        {
            for (long long bar = 0; bar < 6; ++bar)
                addNote (bar * 1000 + 500, 0, 60);
        }

        // The opacity of the note in each row, playing the bars from firstBar at playheadMs.
        // Notes the playhead has passed count as played, so they aren't drawn.
        std::array<float, 4> rowOpacities (long long playheadMs, int firstBar = 0)
        {
            for (auto& note : notes)
                if (note.timeMs < playheadMs && note.performanceTimings.empty())
                    note.performanceTimings.push_back (0);

            auto bars = sixBars();
            bars.erase (bars.begin(), bars.begin() + firstBar);
            auto image = paint (bars, firstBar, playheadMs);

            std::array<float, 4> opacities {};
            for (int row = 0; row < 4; ++row)
                opacities[(size_t) row] = noteOpacity (image, noteSpot (row));
            return opacities;
        }
    };

    bool near (float a, float b) { return std::abs (a - b) <= 0.05f; }
}

TEST_CASE ("NoteLaneDisplay shows both grid lines in full at the start", "[ui][lanes][fade]")
{
    SixBarDisplay display;
    auto rows = display.rowOpacities (0);
    for (auto opacity : rows)
        CHECK (near (opacity, 1.0f));
}

TEST_CASE ("The next bars wait until the grid line they replace is finished", "[ui][lanes][fade]")
{
    SixBarDisplay display;

    // Playing bar 2: bar 1's note is played, and bar 5 doesn't take its place yet.
    auto rows = display.rowOpacities (1200);
    CHECK (near (rows[0], 0.0f));
    CHECK (near (rows[1], 1.0f));
    CHECK (near (rows[2], 1.0f));
    CHECK (near (rows[3], 1.0f));
}

TEST_CASE ("The next grid line fades in over the first bar of the current one", "[ui][lanes][fade]")
{
    SixBarDisplay display;

    SECTION ("bars 5,6 start invisible as bar 3 starts")
    {
        auto rows = display.rowOpacities (2000);
        CHECK (near (rows[0], 0.0f));
        CHECK (near (rows[1], 0.0f));
        CHECK (near (rows[2], 1.0f));
        CHECK (near (rows[3], 1.0f));
    }

    SECTION ("are half faded in halfway through bar 3")
    {
        auto rows = display.rowOpacities (2500);
        CHECK (near (rows[0], 0.5f));
        CHECK (near (rows[1], 0.5f));
        CHECK (near (rows[3], 1.0f));
    }

    SECTION ("and fully there once bar 4 starts")
    {
        auto rows = display.rowOpacities (3200);
        CHECK (near (rows[0], 1.0f));
        CHECK (near (rows[1], 1.0f));
        CHECK (near (rows[3], 1.0f));
    }
}

TEST_CASE ("Practice's bar numbering fades in the same way", "[ui][lanes][fade]")
{
    // Practice hands in only the bars from the one being played, numbered from how many
    // came before it.
    SixBarDisplay display;

    auto rows = display.rowOpacities (2500, 2);
    CHECK (near (rows[0], 0.5f));
    CHECK (near (rows[1], 0.5f));
    CHECK (near (rows[3], 1.0f));

    // Playing bar 4, bar 3 has already been dropped, so its fade-in is long over.
    rows = display.rowOpacities (3500, 3);
    CHECK (near (rows[0], 1.0f));
    CHECK (near (rows[1], 1.0f));
}

TEST_CASE ("Bars already played in the current grid line keep their beat lines", "[ui][lanes][fade]")
{
    SixBarDisplay display;
    // At 320 ms the beat line falls well inside a pixel column (x = 80.3). A time landing on
    // a half pixel, like 125 ms (x = 46.5), can round either way under Release's /fp:fast,
    // so the line could be drawn a column off from the one checked.
    display.notes.emplace_back (320, ChartEvent::BEAT);
    display.events.insert ({ 320, &display.notes.back() });

    // Checked below lane 0's notes and clear of lane 1's line.
    auto image = display.paint (sixBars(), 0, 1200);
    auto background = image.getPixelAt (xAtTime (320) + 10, 110);
    CHECK (image.getPixelAt (xAtTime (320), 110) != background);
}

namespace
{
    // A display showing one 1000 ms bar with an unplayed lane-0 note at 500 ms.
    struct HitLabelDisplay
    {
        HitLabelDisplay()
        {
            note.inputButton = 0;
            note.performanceTimings.push_back (UNPLAYED_NOTE);
            events.insert ({ 500, &note });

            display.setBounds (0, 0, 400, 300);
            display.setEvents (&events);
            display.setBars ({ { 0, 1000 } }, 0);
        }

        void advanceTo (long long timeMs)
        {
            display.setPlayheadTimeMs (timeMs);
            display.advance (0.0);
        }

        GameState state;
        ChartEvent note { 500, ChartEvent::NOTE };
        std::multimap<long long, ChartEvent*> events;
        NoteLaneDisplay display { &state, 2 };
        // 'A', mapped to lane 0.
        juce::KeyPress laneZeroKey { 65 };
    };
}

TEST_CASE ("A key press that hits no note isn't labelled", "[ui][lanes][labels]")
{
    HitLabelDisplay d;
    d.advanceTo (100);
    CHECK (d.display.registerKeyPress (d.laneZeroKey, 100) == nullptr);
    CHECK (d.display.getNumChildComponents() == 0);
}

TEST_CASE ("A note never hit is labelled as missed once its hit window closes", "[ui][lanes][labels]")
{
    HitLabelDisplay d;
    auto window = d.state.tolerances.back();

    d.advanceTo (500 + window - 1);
    CHECK (d.display.getNumChildComponents() == 0);

    d.advanceTo (500 + window + 1);
    CHECK (d.display.getNumChildComponents() == 1);

    // Only once.
    d.advanceTo (800);
    CHECK (d.display.getNumChildComponents() == 1);
}

TEST_CASE ("A hit note is labelled once, and never as missed", "[ui][lanes][labels]")
{
    HitLabelDisplay d;
    d.advanceTo (490);
    CHECK (d.display.registerKeyPress (d.laneZeroKey, 490) == &d.note);
    CHECK (d.display.getNumChildComponents() == 1);

    d.advanceTo (900);
    CHECK (d.display.getNumChildComponents() == 1);
}

TEST_CASE ("Misses can be labelled before their hit window closes", "[ui][lanes][labels]")
{
    HitLabelDisplay d;
    d.advanceTo (510);
    d.display.labelMissedNotesBefore (1000);
    CHECK (d.display.getNumChildComponents() == 1);

    d.advanceTo (900);
    CHECK (d.display.getNumChildComponents() == 1);
}

TEST_CASE ("Restarting the playhead checks the notes for misses again", "[ui][lanes][labels]")
{
    HitLabelDisplay d;
    d.advanceTo (900);
    CHECK (d.display.getNumChildComponents() == 1);

    d.note.performanceTimings.push_back (UNPLAYED_NOTE);
    d.advanceTo (-500);
    d.advanceTo (900);
    CHECK (d.display.getNumChildComponents() == 2);
}

TEST_CASE ("A miss is labelled where the note was placed, not where the playhead took it", "[ui][lanes][labels]")
{
    HitLabelDisplay d;
    d.advanceTo (700);
    REQUIRE (d.display.getNumChildComponents() == 1);

    auto centre = d.display.getChildComponent (0)->getBounds().getCentre();
    CHECK (std::abs (centre.x - xAtTime (500)) <= 1);
    CHECK (std::abs (centre.x - xAtTime (700)) > 10);
}

TEST_CASE ("A late hit's label sits over a line back to where the note should have been played", "[ui][lanes][labels]")
{
    HitLabelDisplay d;
    d.advanceTo (560);
    REQUIRE (d.display.registerKeyPress (d.laneZeroKey, 560) == &d.note);
    REQUIRE (d.display.getNumChildComponents() == 1);

    auto* label = dynamic_cast<ToleranceLabel*> (d.display.getChildComponent (0));
    REQUIRE (label != nullptr);
    REQUIRE (label->getHitLine().size() == 1);
    auto line = label->getHitLine().front();
    CHECK (std::abs (line.getStartX() - (float) xAtTime (500)) <= 1.f);
    CHECK (std::abs (line.getEndX() - (float) xAtTime (560)) <= 1.f);
    CHECK (line.getStartY() == line.getEndY());
    CHECK (label->getBounds().toFloat().getCentre().getDistanceFrom (line.getEnd()) <= 1.f);
    CHECK (label->getHitLineColour() == d.state.colours.histogramBars[(size_t) d.state.getCategory (60)]);
}

TEST_CASE ("A miss has no hit line", "[ui][lanes][labels]")
{
    HitLabelDisplay d;
    d.advanceTo (700);
    REQUIRE (d.display.getNumChildComponents() == 1);
    auto* label = dynamic_cast<ToleranceLabel*> (d.display.getChildComponent (0));
    REQUIRE (label != nullptr);
    CHECK (label->getHitLine().empty());
}

TEST_CASE ("The background tile is tiled behind the rows, under their background colour", "[ui][lanes][background]")
{
    OneBarDisplay setup (2);
    NoteLaneDisplay display (&setup.state, 2);
    display.setBounds (0, 0, 400, 300);

    juce::Image tile (juce::Image::ARGB, 8, 8, false, juce::SoftwareImageType());
    tile.clear (tile.getBounds(), juce::Colours::red);
    display.setBackgroundTile (tile);

    juce::Image image (juce::Image::ARGB, 400, 300, true, juce::SoftwareImageType());
    {
        juce::Graphics g (image);
        display.paint (g);
    }

    // Away from the lane lines, in both grid lines: red, darkened by the quarter-opaque black row background.
    auto expected = juce::Colours::red.overlaidWith (juce::Colours::black.withAlpha (0.25f));
    for (auto y : { 20, 155 })
    {
        INFO ("y = " << y);
        CHECK (sameRgb (image.getPixelAt (emptyColumnX, y), expected));
        CHECK (image.getPixelAt (emptyColumnX, y).getAlpha() == 255);
    }
    // Not between the grid lines, or around the rows.
    CHECK (image.getPixelAt (emptyColumnX, 137).getAlpha() == 0);
}

TEST_CASE ("The game's lane background tile is bundled", "[ui][lanes][background]")
{
    auto tile = NoteLaneDisplay::defaultBackgroundTile();
    REQUIRE (tile.isValid());
    CHECK (tile.getWidth() == 152);
    CHECK (tile.getHeight() == 152);
}

TEST_CASE ("A key press starts a ripple on its lane's line at the playhead", "[ui][lanes][ripples]")
{
    HitLabelDisplay d;
    d.advanceTo (100);
    d.display.registerKeyPress (d.laneZeroKey, 100);

    REQUIRE (d.display.getRipples().size() == 1);
    auto& ripple = d.display.getRipples().front();
    CHECK (ripple.lane == 0);
    CHECK (std::abs (ripple.centre.x - (float) xAtTime (100)) <= 1.f);
    CHECK (ripple.diameter() == 0.f);
    CHECK (ripple.alpha() == 0.5f);

    // Keys that aren't mapped to a lane don't ripple.
    d.display.registerKeyPress (juce::KeyPress ('q'), 100);
    CHECK (d.display.getRipples().size() == 1);
}

TEST_CASE ("A ripple grows to twice the hit images' width while fading out over a second", "[ui][lanes][ripples]")
{
    HitLabelDisplay d;
    d.advanceTo (100);
    d.display.registerKeyPress (d.laneZeroKey, 100);

    d.display.advance (500.0);
    REQUIRE (d.display.getRipples().size() == 1);
    auto& halfway = d.display.getRipples().front();
    CHECK (halfway.diameter() == 80.f);
    CHECK (halfway.alpha() == 0.25f);
    // It stays where the key was pressed as the playhead moves on.
    d.advanceTo (600);
    CHECK (std::abs (d.display.getRipples().front().centre.x - (float) xAtTime (100)) <= 1.f);

    d.display.advance (499.0);
    REQUIRE (d.display.getRipples().size() == 1);
    CHECK (d.display.getRipples().front().diameter() > 159.f);

    d.display.advance (1.0);
    CHECK (d.display.getRipples().empty());
}
