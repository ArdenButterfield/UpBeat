//
// Created by Arden on 7/8/2026.
//

#include "ChartPerformanceScene.h"

ChartPerformanceScene::ChartPerformanceScene(GameState* gs) : Scene(gs), startButton("Start"), playing(false)
{
    addAndMakeVisible (startButton);
    startButton.addListener (this);
    indicatorLighting.fill(0);
    setWantsKeyboardFocus (true);
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
            grabKeyboardFocus();
    }
}
void ChartPerformanceScene::update()
{
    if (playing)
    {
        auto elapsed = getMillisecondsSinceLastUpdate();
        timeMs += elapsed;
        for (auto& indicator : indicatorLighting)
        {
            indicator = std::max(0.f, indicator - elapsed * 0.001f);
        }
    }

    for (int i = 0; i < GameState::numberOfInputLanes; ++i)
    {
        if (juce::KeyPress::isKeyCurrentlyDown(keys[i]))
        {
            indicatorLighting[i] = 1;
        } else if (indicatorLighting[i] > 0.5f)
        {
            indicatorLighting[i] = 0.5f;
        }
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

    for (int i = 0; i < GameState::numberOfInputLanes; ++i)
    {
        g.setColour (juce::Colours::white.withAlpha (indicatorLighting[i]));
        g.fillRect (buttonIndicators[i]);
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
    grabKeyboardFocus();
    startButton.setBounds (getLocalBounds().withSizeKeepingCentre (200, 40));
    laneOutline = getLocalBounds().withWidth (std::min(300, getWidth() - 40)).withTrimmedTop (20).withTrimmedBottom (20).withCentre ({getWidth() / 2, getHeight() / 2});
    auto lanesInner = laneOutline.reduced(5).withTrimmedBottom (30);
    auto indicatorsInner = laneOutline.reduced(5).withTop (lanesInner.getBottom() + 5);
    auto laneW = lanesInner.getWidth() / 4;
    for (int i = 0; i < lanes.size(); ++i)
    {
        lanes[i] = lanesInner.withWidth (laneW).withX(lanesInner.getX() + i * laneW);
        buttonIndicators[i] = indicatorsInner.withWidth (laneW).withX(lanesInner.getX() + i * laneW);
    }
}
bool ChartPerformanceScene::keyPressed (const juce::KeyPress& key)
{
    for (int i = 0; i < GameState::numberOfInputLanes; ++i)
    {
        if (keys[i] == key.getKeyCode())
        {
            // handle kepress
            indicatorLighting[i] = 1;
        }
    }
    return true;
}