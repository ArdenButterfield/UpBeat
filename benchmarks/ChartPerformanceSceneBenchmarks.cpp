#include <BundledResources.h>
#include <scenes/ChartPerformanceScene.h>

namespace
{
    GameState makeTestChartGameState()
    {
        auto chart = Chart::fromYaml (BundledResources::loadFile ("default_charts/yaml/test.yaml").toString());
        REQUIRE (chart.has_value());

        GameState gameState;
        gameState.charts.push_back (std::move (*chart));
        gameState.currentChart = &gameState.charts.front();
        return gameState;
    }

    // startGame is private; it's reached the same way the UI reaches it, by the
    // scene hearing its Start button get clicked.
    void startGame (ChartPerformanceScene& scene)
    {
        for (auto* child : scene.getChildren())
        {
            if (auto* button = dynamic_cast<juce::TextButton*> (child); button != nullptr && button->getButtonText() == "Start")
            {
                static_cast<juce::Button::Listener&> (scene).buttonClicked (button);
                return;
            }
        }
        FAIL ("ChartPerformanceScene has no Start button");
    }
}

TEST_CASE ("ChartPerformanceScene performance (test.yaml)")
{
    auto gameState = makeTestChartGameState();
    ChartPerformanceScene scene (&gameState);
    scene.setSize (800, 600);

    BENCHMARK ("ChartPerformanceScene::prepareToPlay")
    {
        scene.prepareToPlay (44100.0, 512);
    };

    scene.prepareToPlay (44100.0, 512);

    // Each start appends one UNPLAYED_NOTE timing to every note, so repeated runs on
    // the same scene grow those vectors slightly; the amortized cost is negligible
    // next to rebuilding the display events and bar spans.
    BENCHMARK ("ChartPerformanceScene::startGame")
    {
        startGame (scene);
    };
}
