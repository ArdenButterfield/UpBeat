//
// Created by Arden on 7/7/2026.
//

#include "ChartSelectionScene.h"

ChartSelectionScene::ChartSelectionScene()
{
}
ChartSelectionScene::~ChartSelectionScene()
{
}
void ChartSelectionScene::update()
{
}
void ChartSelectionScene::paint (juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);
    g.setColour (juce::Colours::white);
    g.setFont (24);
    g.drawText ("Chart Selection", getLocalBounds(), juce::Justification::centred, 1);

}