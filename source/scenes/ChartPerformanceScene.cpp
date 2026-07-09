//
// Created by Arden on 7/8/2026.
//

#include "ChartPerformanceScene.h"

ChartPerformanceScene::ChartPerformanceScene(GameState* gs) : Scene(gs), startButton("Start"), playing(false)
{
    addAndMakeVisible (startButton);
    startButton.addListener (this);
}

ChartPerformanceScene::~ChartPerformanceScene()
{
}

void ChartPerformanceScene::buttonClicked (juce::Button* b)
{
    if (b == &startButton)
    {
        startButton.setVisible (false);
        playing = true;
        timeMs = -2000;
    }
}
void ChartPerformanceScene::update()
{
    if (playing)
    {
        timeMs += getMillisecondsSinceLastUpdate();
    }
}

void ChartPerformanceScene::paint (juce::Graphics& g)
{

    g.fillAll(juce::Colours::darkgrey);
    g.setColour (juce::Colours::lightgrey);
    g.drawRect (laneOutline);
    for (auto& lane : lanes)
    {
        g.drawRect (lane);
    }

    g.setColour (juce::Colours::pink);
    for (auto& event : gameState->currentChart->events)
    {
        if (event.type == ChartEvent::NOTE)
        {
            auto notePosition = static_cast<int>((timeMs - event.timeMs) * pixelsPerMillisecond) + lanes[0].getBottom();
            g.drawRect (lanes[event.inputButton - 1].withY (notePosition - 3).withHeight (6));
        }
    }
}
void ChartPerformanceScene::resized()
{
    startButton.setBounds (getLocalBounds().withSizeKeepingCentre (200, 40));
    laneOutline = getLocalBounds().withWidth (std::min(300, getWidth() - 40)).withTrimmedTop (20).withTrimmedBottom (20).withCentre ({getWidth() / 2, getHeight() / 2});
    auto lanesInner = laneOutline.reduced(5);
    auto laneW = lanesInner.getWidth() / 4;
    for (int i = 0; i < lanes.size(); ++i)
    {
        lanes[i] = lanesInner.withWidth (laneW).withX(lanesInner.getX() + i * laneW);
    }
}