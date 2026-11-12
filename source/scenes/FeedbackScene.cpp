//
// Created by Arden on 7/8/2026.
//

#include "FeedbackScene.h"
#include "../UI/GuiStyle.h"

namespace
{
    constexpr int titleHeight = 48;
}

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
    g.setFont (GuiStyle::fontSize (24.0f));

    auto titleSection = getLocalBounds().withHeight (titleHeight);
    g.drawText ("Feedback", titleSection, juce::Justification::centred, 1);
}

void FeedbackScene::resized()
{
    constexpr int retryButtonHeight = 52;
    constexpr int histogramHeight = 140;

    auto titleSection = getLocalBounds().withHeight (titleHeight);
    auto bodySection = getLocalBounds().withTrimmedTop (titleSection.getHeight());
    auto retryButtonSection = bodySection.removeFromBottom (retryButtonHeight);

    timingHistogram.setBounds (bodySection.removeFromBottom (histogramHeight));
    performanceGraph.setBounds (bodySection);
    retryButton.setBounds (retryButtonSection.withSizeKeepingCentre (150, 40));
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
