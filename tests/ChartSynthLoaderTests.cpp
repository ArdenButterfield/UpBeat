#include <Audio/ChartSynthLoader.h>
#include <BundledResources.h>
#include <scenes/ChartPerformanceScene.h>
#include <scenes/ChartPracticeScene.h>

#include <catch2/catch_test_macros.hpp>

namespace
{
    Chart loadTestChart()
    {
        auto chart = Chart::fromYaml (BundledResources::loadFile ("default_charts/yaml/test.yaml").toString());
        REQUIRE (chart.has_value());
        REQUIRE (! chart->synths.empty());
        return std::move (*chart);
    }

    // Runs the message loop, which the loading thread needs to take the MessageManagerLock
    // and to hand over its synths, until done() or the timeout.
    bool runMessageLoopUntil (const std::function<bool()>& done, int timeoutMs = 120000)
    {
        auto end = juce::Time::getMillisecondCounter() + (juce::uint32) timeoutMs;
        while (! done() && juce::Time::getMillisecondCounter() < end)
            juce::MessageManager::getInstance()->runDispatchLoopUntil (10);
        return done();
    }

    juce::TextButton* findStartButton (juce::Component& scene)
    {
        for (auto* child : scene.getChildren())
            if (auto* button = dynamic_cast<juce::TextButton*> (child); button != nullptr && button->getButtonText() == "Start")
                return button;
        return nullptr;
    }

    float renderPeak (ChartSynthBank& synths)
    {
        juce::AudioBuffer<float> buffer (2, 512);
        float peak = 0.0f;
        for (int block = 0; block < 40; ++block)
        {
            buffer.clear();
            synths.renderNextBlock (buffer, 0, buffer.getNumSamples());
            peak = std::max (peak, buffer.getMagnitude (0, buffer.getNumSamples()));
        }
        return peak;
    }
}

TEST_CASE ("ChartSynthLoader loads the synths in the background", "[odin][loading]")
{
    auto chart = loadTestChart();
    ChartSynthLoader loader (chart);
    int loadedCalls = 0;
    loader.onLoaded = [&] { ++loadedCalls; };
    loader.prepareToPlay (44100.0, 512);

    // The constructor returns straight away, leaving the synths to load.
    CHECK_FALSE (loader.isLoaded());
    CHECK (loader.getSynths() == nullptr);

    REQUIRE (runMessageLoopUntil ([&] { return loader.isLoaded(); }));
    CHECK (loadedCalls == 1);
    REQUIRE (loader.getSynths() != nullptr);

    loader.getSynths()->noteOn (chart.synths.front().id, 60, 100, 0.5);
    CHECK (renderPeak (*loader.getSynths()) > 0.0f);
}

TEST_CASE ("ChartSynthLoader only hands over prepared synths", "[odin][loading]")
{
    auto chart = loadTestChart();
    ChartSynthLoader loader (chart);

    REQUIRE (runMessageLoopUntil ([&] { return loader.isLoaded(); }));
    CHECK (loader.getSynths() == nullptr);

    loader.prepareToPlay (44100.0, 512);
    CHECK (loader.getSynths() != nullptr);
}

TEST_CASE ("ChartSynthLoader can be destroyed while still loading", "[odin][loading]")
{
    auto chart = loadTestChart();
    auto start = juce::Time::getMillisecondCounterHiRes();
    {
        // Without the message loop running the loading thread can't take the
        // MessageManagerLock, so it's still waiting for it here.
        ChartSynthLoader loader (chart);
        CHECK_FALSE (loader.isLoaded());
    }
    CHECK (juce::Time::getMillisecondCounterHiRes() - start < 5000.0);
}

TEST_CASE ("Chart scenes hide the Start button until their synths have loaded", "[odin][loading][scene]")
{
    GameState gameState;
    gameState.charts.push_back (loadTestChart());
    gameState.currentChart = &gameState.charts.front();

    auto check = [] (Scene& scene)
    {
        scene.setSize (800, 600);
        scene.prepareToPlay (44100.0, 512);
        auto* startButton = findStartButton (scene);
        REQUIRE (startButton != nullptr);
        CHECK_FALSE (startButton->isVisible());

        CHECK (runMessageLoopUntil ([&] { return startButton->isVisible(); }));
    };

    SECTION ("performance")
    {
        ChartPerformanceScene scene (&gameState);
        check (scene);
    }

    SECTION ("practice")
    {
        ChartPracticeScene scene (&gameState);
        check (scene);
    }
}
