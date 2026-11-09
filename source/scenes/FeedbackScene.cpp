//
// Created by Arden on 7/8/2026.
//

#include "FeedbackScene.h"

FeedbackScene::FeedbackScene (GameState* gs) : Scene (gs), desiredSceneId (SceneIDs::CHART_FEEDBACK_SCENE), performanceGraph (gs->colours), timingHistogram (*gs), retryButton ("Retry")
{
    addAndMakeVisible (performanceGraph);
    addAndMakeVisible (timingHistogram);
    buildPerformanceGraph();

    addAndMakeVisible (retryButton);
    retryButton.addListener (this);
}

FeedbackScene::~FeedbackScene()
{
}

void FeedbackScene::buildPerformanceGraph()
{
    size_t performanceCount = 0;
    for (auto& event : gameState->currentChart->events)
    {
        if (event.second.type == ChartEvent::NOTE)
        {
            performanceCount = std::max (performanceCount, event.second.performanceTimings.size());
        }
    }

    for (size_t performance = 0; performance < performanceCount; ++performance)
    {
        std::vector<float> xValues, yValues;
        for (auto& event : gameState->currentChart->events)
        {
            if (event.second.type != ChartEvent::NOTE)
                continue;

            auto& timings = event.second.performanceTimings;
            if (performance >= timings.size() || timings[performance] == UNPLAYED_NOTE)
                continue;

            xValues.push_back (static_cast<float> (event.first));
            yValues.push_back (static_cast<float> (timings[performance]));

            if (performance == performanceCount - 1)
                timingHistogram.addTiming (timings[performance]);
        }

        performanceGraph.addData (xValues, yValues);
    }
}

void FeedbackScene::paint (juce::Graphics& g)
{
    g.fillAll (gameState->colours.menuBackground);
    g.setColour (gameState->colours.text);
    g.setFont (24);

    auto titleSection = getLocalBounds().withHeight (40);
    g.drawText ("Feedback", titleSection, juce::Justification::centred, 1);
}

void FeedbackScene::resized()
{
    constexpr int retryButtonHeight = 40;
    constexpr int histogramHeight = 110;

    auto titleSection = getLocalBounds().withHeight (40);
    auto bodySection = getLocalBounds().withTrimmedTop (titleSection.getHeight());
    auto retryButtonSection = bodySection.removeFromBottom (retryButtonHeight);

    timingHistogram.setBounds (bodySection.removeFromBottom (histogramHeight));
    performanceGraph.setBounds (bodySection);
    retryButton.setBounds (retryButtonSection.withSizeKeepingCentre (150, 30));
}

void FeedbackScene::buttonClicked (juce::Button* b)
{
    if (b == &retryButton)
    {
        desiredSceneId = SceneIDs::CHART_PERFORMANCE_SCENE;
    }
}

void FeedbackScene::update()
{
}

SceneIDs::SceneID FeedbackScene::getSceneID() const
{
    return SceneIDs::CHART_FEEDBACK_SCENE;
}

SceneIDs::SceneID FeedbackScene::getDesiredSceneID()
{
    return desiredSceneId;
}
