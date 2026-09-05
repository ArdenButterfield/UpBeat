//
// Created by Arden on 7/7/2026.
//

#include "ChartSelectionScene.h"

#include "../BundledResources.h"

ChartSelectionScene::ChartSelectionScene(GameState* gs) : Scene(gs), desiredSceneId (SceneIDs::CHART_SELECT_SCENE)
{
    for (auto& entry : BundledResources::listEntries ("default_charts/midi"))
    {
        auto midiData = BundledResources::loadFile (entry);
        chartSelectionButtons.push_back (std::make_unique<ChartSelectionButton> (entry, (int) gameState->charts.size(), midiData));
        gameState->charts.push_back (Chart (midiData));
    }
    for (auto& button : chartSelectionButtons)
    {
        addAndMakeVisible (button.get());
        button->addListener (this);
    }

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

void ChartSelectionScene::buttonClicked (juce::Button* b)
{
    if (auto* chartButton = dynamic_cast<ChartSelectionButton*> (b))
    {
        gameState->currentChart = &gameState->charts[chartButton->chartIndex];
        desiredSceneId = SceneIDs::CHART_PERFORMANCE_SCENE;
    }
}
void ChartSelectionScene::resized()
{
    for (int i = 0; i < chartSelectionButtons.size(); ++i)
    {
        chartSelectionButtons[i]->setBounds (getLocalBounds().withTrimmedLeft (10).withTrimmedRight (10).withHeight (70).withY (i * 90 + 20));
    }
}
SceneIDs::SceneID ChartSelectionScene::getDesiredSceneID()
{
    return desiredSceneId;
}
SceneIDs::SceneID ChartSelectionScene::getSceneID() const
{
    return SceneIDs::CHART_SELECT_SCENE;
}