#include <BundledResources.h>
#include <scenes/ChartPerformanceScene.h>
#include <scenes/ChartPracticeScene.h>
#include <UI/TimingHistogram.h>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using Catch::Matchers::WithinAbs;

namespace
{
    // The widest tolerance: the hit window either side of a note.
    long long hitWindow (const GameState& state)
    {
        return state.tolerances.back();
    }

    GameState makeTestChartGameState()
    {
        auto chart = Chart::fromYaml (BundledResources::loadFile ("default_charts/yaml/test.yaml").toString());
        REQUIRE (chart.has_value());

        GameState gameState;
        gameState.charts.push_back (std::move (*chart));
        gameState.currentChart = &gameState.charts.front();
        return gameState;
    }

    template <typename ComponentType>
    ComponentType* findChild (juce::Component& parent)
    {
        for (auto* child : parent.getChildren())
            if (auto* found = dynamic_cast<ComponentType*> (child))
                return found;
        return nullptr;
    }

    // Starts a scene's game the way its Start button does.
    void startGame (juce::Component& scene)
    {
        for (auto* child : scene.getChildren())
        {
            if (auto* button = dynamic_cast<juce::TextButton*> (child); button != nullptr && button->getButtonText() == "Start")
            {
                dynamic_cast<juce::Button::Listener&> (scene).buttonClicked (button);
                return;
            }
        }
        FAIL ("The scene has no Start button");
    }

    // Presses lane 0's first key at a range of times until it hits a note, as a player
    // would. The step is well under the hit window, so no note is skipped over.
    void hitANote (NoteLaneDisplay& lanes, TimingHistogram& histogram)
    {
        for (long long time = -2000; time < 60000 && histogram.getCount() == 0; time += 37)
            lanes.registerKeyPress (juce::KeyPress ('A'), time);
    }

    // The histogram and lanes of a scene, checked to be laid out side by side.
    template <typename SceneType>
    void checkSceneHistogram()
    {
        auto gameState = makeTestChartGameState();
        SceneType scene (&gameState);
        scene.setSize (1200, 800);

        auto* histogram = findChild<TimingHistogram> (scene);
        auto* lanes = findChild<NoteLaneDisplay> (scene);
        REQUIRE (histogram != nullptr);
        REQUIRE (lanes != nullptr);

        SECTION ("to the right of the lanes, as tall as them")
        {
            CHECK (histogram->isVisible());
            CHECK (histogram->getX() >= lanes->getRight());
            CHECK (histogram->getX() - lanes->getRight() <= 20);
            CHECK (histogram->getY() == lanes->getY());
            CHECK (histogram->getHeight() == lanes->getHeight());
            CHECK (histogram->getWidth() > 0);
            CHECK (scene.getLocalBounds().contains (histogram->getBounds()));
        }

        SECTION ("recording each hit as it happens, and starting afresh each run")
        {
            startGame (scene);
            hitANote (*lanes, *histogram);
            REQUIRE (histogram->getCount() == 1);
            CHECK (std::abs (histogram->getMeanErrorMs()) < (double) hitWindow (gameState));

            startGame (scene);
            CHECK (histogram->getCount() == 0);
        }
    }
}

TEST_CASE ("TimingHistogram sorts errors into bins across the hit window", "[histogram]")
{
    GameState state;
    TimingHistogram histogram (state);
    auto range = hitWindow (state);
    constexpr auto width = TimingHistogram::binWidthMs;

    CHECK (histogram.getNumBins() == 2 * range / width);
    CHECK (histogram.binForError (-range) == 0);
    CHECK (histogram.binForError (-range + width - 1) == 0);
    CHECK (histogram.binForError (-range + width) == 1);
    CHECK (histogram.binForError (-1) == histogram.getNumBins() / 2 - 1);
    CHECK (histogram.binForError (0) == histogram.getNumBins() / 2);
    CHECK (histogram.binForError (width - 1) == histogram.getNumBins() / 2);
    CHECK (histogram.binForError (range - 1) == histogram.getNumBins() - 1);

    // Anything outside the window counts at its edge.
    CHECK (histogram.binForError (-range - 1) == 0);
    CHECK (histogram.binForError (-5000) == 0);
    CHECK (histogram.binForError (range) == histogram.getNumBins() - 1);
    CHECK (histogram.binForError (5000) == histogram.getNumBins() - 1);
}

TEST_CASE ("TimingHistogram bins follow the game's tolerances", "[histogram]")
{
    GameState state;
    state.tolerances = { 10, 20, 50, 400 };
    TimingHistogram histogram (state);
    CHECK (histogram.getNumBins() == 2 * 400 / TimingHistogram::binWidthMs);
}

TEST_CASE ("TimingHistogram counts hits and their mean error", "[histogram]")
{
    GameState state;
    TimingHistogram histogram (state);
    CHECK (histogram.getCount() == 0);
    CHECK (histogram.getMeanErrorMs() == 0.0);

    for (auto error : { -30LL, -10LL, 5LL, 7LL, 100LL })
        histogram.addTiming (error);

    CHECK (histogram.getCount() == 5);
    CHECK_THAT (histogram.getMeanErrorMs(), WithinAbs (14.4, 1e-9));
    CHECK (histogram.getBinCount (histogram.binForError (5)) == 2);
    CHECK (histogram.getBinCount (histogram.binForError (-30)) == 1);
    CHECK (histogram.getBinCount (histogram.binForError (100)) == 1);

    histogram.reset();
    CHECK (histogram.getCount() == 0);
    CHECK (histogram.getMeanErrorMs() == 0.0);
    for (int bin = 0; bin < histogram.getNumBins(); ++bin)
        CHECK (histogram.getBinCount (bin) == 0);
}

TEST_CASE ("TimingHistogram finds the median and quartiles of its hits", "[histogram]")
{
    GameState state;
    TimingHistogram histogram (state);
    CHECK (histogram.getPercentileErrorMs (0.5) == 0.0);

    // Added out of order: sorted they are -30, -10, 5, 7, 100.
    for (auto error : { 7LL, -30LL, 100LL, 5LL, -10LL })
        histogram.addTiming (error);

    CHECK_THAT (histogram.getPercentileErrorMs (0.5), WithinAbs (5.0, 1e-9));
    CHECK_THAT (histogram.getPercentileErrorMs (0.25), WithinAbs (-10.0, 1e-9));
    CHECK_THAT (histogram.getPercentileErrorMs (0.75), WithinAbs (7.0, 1e-9));
    CHECK_THAT (histogram.getPercentileErrorMs (0.0), WithinAbs (-30.0, 1e-9));
    CHECK_THAT (histogram.getPercentileErrorMs (1.0), WithinAbs (100.0, 1e-9));

    SECTION ("an even count interpolates between the middle hits")
    {
        histogram.addTiming (9);
        // -30, -10, 5, 7, 9, 100
        CHECK_THAT (histogram.getPercentileErrorMs (0.5), WithinAbs (6.0, 1e-9));
        CHECK_THAT (histogram.getPercentileErrorMs (0.25), WithinAbs (-6.25, 1e-9));
        CHECK_THAT (histogram.getPercentileErrorMs (0.75), WithinAbs (8.5, 1e-9));
    }

    SECTION ("resetting forgets them")
    {
        histogram.reset();
        CHECK (histogram.getPercentileErrorMs (0.5) == 0.0);
    }
}

TEST_CASE ("TimingHistogram draws dotted lines at the median and quartiles", "[histogram]")
{
    GameState state;
    TimingHistogram histogram (state);
    histogram.setSize (120, 600);

    // The quartiles and median (-105, -40 and 35) fall between these hits' bars, so the
    // lines are drawn over the background.
    for (auto error : { -120LL, -100LL, 20LL, 80LL })
        histogram.addTiming (error);
    REQUIRE_THAT (histogram.getPercentileErrorMs (0.25), WithinAbs (-105.0, 1e-9));
    REQUIRE_THAT (histogram.getPercentileErrorMs (0.5), WithinAbs (-40.0, 1e-9));
    REQUIRE_THAT (histogram.getPercentileErrorMs (0.75), WithinAbs (35.0, 1e-9));

    juce::Image image (juce::Image::RGB, 120, 600, true, juce::SoftwareImageType());
    {
        juce::Graphics g (image);
        g.fillAll (juce::Colours::black);
        histogram.paint (g);
    }

    auto plot = histogram.getPlotArea();
    auto row = [&] (double errorMs) { return juce::roundToInt (std::floor (histogram.yForError (errorMs))); };
    auto litPixels = [&] (int y) {
        int lit = 0;
        for (int x = (int) plot.getX(); x < (int) plot.getRight(); ++x)
            lit += image.getPixelAt (x, y).getBrightness() > 0.3f ? 1 : 0;
        return lit;
    };

    for (auto error : { -105.0, -40.0, 35.0 })
    {
        INFO ("marker at " << error << " ms");
        auto lit = litPixels (row (error));
        // Dotted: some of the row is drawn, but not all of it.
        CHECK (lit > 0);
        CHECK (lit < (int) plot.getWidth());
    }

    // The median's line is drawn stronger than the quartiles'.
    auto brightest = [&] (int y) {
        float best = 0.f;
        for (int x = (int) plot.getX(); x < (int) plot.getRight(); ++x)
            best = std::max (best, image.getPixelAt (x, y).getBrightness());
        return best;
    };
    CHECK (brightest (row (-40.0)) > brightest (row (-105.0)));
    CHECK (brightest (row (-40.0)) > brightest (row (35.0)));

    // Each line's label sits just above it, towards the right.
    auto labelRow = row (-40.0) - 6;
    int litRight = 0, litLeft = 0;
    for (int x = (int) plot.getX(); x < (int) plot.getRight(); ++x)
        (x < (int) plot.getCentreX() ? litLeft : litRight) += image.getPixelAt (x, labelRow).getBrightness() > 0.3f ? 1 : 0;
    CHECK (litRight > 0);
    CHECK (litLeft == 0);
}

TEST_CASE ("TimingHistogram's bars grow with their counts, early at the top", "[histogram]")
{
    GameState state;
    TimingHistogram histogram (state);
    histogram.setSize (120, 600);
    auto earlyError = -hitWindow (state) / 2;

    histogram.addTiming (earlyError);
    for (int i = 0; i < 4; ++i)
        histogram.addTiming (2);

    auto early = histogram.getBarBounds (histogram.binForError (earlyError));
    auto onTime = histogram.getBarBounds (histogram.binForError (2));
    auto plot = histogram.getPlotArea();

    SECTION ("the busiest bin spans the plot; the others are in proportion")
    {
        CHECK_THAT (onTime.getWidth(), WithinAbs (plot.getWidth(), 1e-3));
        CHECK_THAT (early.getWidth(), WithinAbs (plot.getWidth() / 4.0f, 1e-3));
        CHECK (early.getX() == plot.getX());
    }

    SECTION ("each bin's bar sits at its error, the top of the plot being the earliest")
    {
        CHECK (early.getBottom() < onTime.getY());
        auto zeroY = plot.getY() + plot.getHeight() / 2.0f;
        CHECK_THAT (onTime.getY(), WithinAbs (zeroY, 1e-3));
        CHECK_THAT (early.getY(), WithinAbs (plot.getY() + plot.getHeight() / 4.0f, 1e-3));
        CHECK (plot.contains (early));
        CHECK (plot.contains (onTime));
    }

    SECTION ("empty bins have no bar")
    {
        CHECK (histogram.getBarBounds (0).isEmpty());
        CHECK (histogram.getBarBounds (histogram.getNumBins() - 1).isEmpty());
    }
}

TEST_CASE ("TimingHistogram colours bars by tolerance category", "[histogram]")
{
    GameState state;
    TimingHistogram histogram (state);
    histogram.setSize (120, 600);

    // A hit in the middle of each category - Perfect, Great, Good and Ok - alternately
    // early and late.
    std::vector<long long> errors;
    for (int i = 0; i < GameState::NUM_TOLERANCE_CATEGORIES; ++i)
    {
        auto inner = i == 0 ? 0LL : state.tolerances[(size_t) i - 1];
        auto middle = (inner + state.tolerances[(size_t) i]) / 2;
        errors.push_back (i % 2 == 0 ? middle : -middle);
    }
    for (auto error : errors)
        histogram.addTiming (error);

    juce::Image image (juce::Image::ARGB, 120, 600, true, juce::SoftwareImageType());
    {
        juce::Graphics g (image);
        histogram.paint (g);
    }

    for (int i = 0; i < GameState::NUM_TOLERANCE_CATEGORIES; ++i)
    {
        INFO (state.toleranceLabels[(size_t) i] << " at " << errors[(size_t) i] << " ms");
        auto centre = histogram.getBarBounds (histogram.binForError (errors[(size_t) i])).getCentre().toInt();
        CHECK (image.getPixelAt (centre.x, centre.y) == state.colours.histogramBars[(size_t) i]);
    }
}

TEST_CASE ("NoteLaneDisplay reports each hit's timing error", "[histogram][ui]")
{
    GameState state;
    auto chart = Chart::fromYaml ("title: lanes\nlanes: 1\nevents: []\n");
    REQUIRE (chart.has_value());
    state.charts.push_back (std::move (*chart));
    state.currentChart = &state.charts.front();

    ChartEvent note (1000, ChartEvent::NOTE);
    note.inputButton = 0;
    note.performanceTimings.push_back (UNPLAYED_NOTE);
    std::multimap<long long, ChartEvent*> events { { 1000, &note } };

    NoteLaneDisplay lanes (&state, 1);
    lanes.setBounds (0, 0, 400, 300);
    lanes.setEvents (&events);
    lanes.setBars ({ { 0, 2000 } }, 0);

    std::vector<long long> reported;
    lanes.onNoteTimed = [&reported] (long long errorMs) { reported.push_back (errorMs); };

    SECTION ("early hits are negative")
    {
        lanes.registerKeyPress (juce::KeyPress ('A'), 960);
        CHECK (reported == std::vector<long long> { -40 });
        CHECK (note.performanceTimings.back() == -40);
    }

    SECTION ("late hits are positive")
    {
        lanes.registerKeyPress (juce::KeyPress ('F'), 1025);
        CHECK (reported == std::vector<long long> { 25 });
    }

    SECTION ("presses that hit nothing report nothing")
    {
        lanes.registerKeyPress (juce::KeyPress ('A'), 1000 + hitWindow (state) + 100);
        lanes.registerKeyPress (juce::KeyPress ('Z'), 1000);
        CHECK (reported.empty());
    }
}

TEST_CASE ("The performance scene shows a timing histogram", "[histogram][scene]")
{
    checkSceneHistogram<ChartPerformanceScene>();
}

TEST_CASE ("The practice scene shows a timing histogram", "[histogram][scene]")
{
    checkSceneHistogram<ChartPracticeScene>();
}
