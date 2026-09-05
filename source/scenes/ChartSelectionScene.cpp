//
// Created by Arden on 7/7/2026.
//

#include "ChartSelectionScene.h"

ChartSelectionScene::ChartSelectionScene(GameState* gs) : Scene(gs), desiredSceneId (SceneIDs::CHART_SELECT_SCENE)
{
    rebuildChartButtons();

    addAndMakeVisible (newChartButton);
    newChartButton.addListener (this);
}

ChartSelectionScene::~ChartSelectionScene()
{
}

void ChartSelectionScene::rebuildChartButtons()
{
    chartSelectionButtons.clear();
    for (int i = 0; i < (int) gameState->charts.size(); ++i)
    {
        auto button = std::make_unique<ChartSelectionButton> (gameState->charts[(size_t) i].name, i);
        addAndMakeVisible (button.get());
        button->addListener (this);
        chartSelectionButtons.push_back (std::move (button));
    }
}

void ChartSelectionScene::openCreationPanel()
{
    creationPanel = std::make_unique<ChartCreationPanel>();

    juce::Component::SafePointer<ChartSelectionScene> safeThis (this);

    creationPanel->onChartCreated = [safeThis] (Chart chart)
    {
        juce::MessageManager::callAsync ([safeThis, chart = std::move (chart)]() mutable
        {
            if (safeThis == nullptr)
                return;
            safeThis->gameState->charts.push_back (std::move (chart));
            safeThis->rebuildChartButtons();
            safeThis->closeCreationPanel();
            safeThis->resized();
        });
    };

    creationPanel->onCancel = [safeThis]()
    {
        juce::MessageManager::callAsync ([safeThis]()
        {
            if (safeThis == nullptr)
                return;
            safeThis->closeCreationPanel();
        });
    };

    addAndMakeVisible (creationPanel.get());
    resized();
}

void ChartSelectionScene::closeCreationPanel()
{
    creationPanel.reset();
}

void ChartSelectionScene::update()
{
}

void ChartSelectionScene::paint (juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);
    g.setColour (juce::Colours::white);
    g.setFont (24);
    g.drawText ("Your Charts", getLocalBounds().withHeight (40), juce::Justification::centred, 1);
}

void ChartSelectionScene::buttonClicked (juce::Button* b)
{
    if (b == &newChartButton)
    {
        openCreationPanel();
        return;
    }

    if (auto* chartButton = dynamic_cast<ChartSelectionButton*> (b))
    {
        gameState->currentChart = &gameState->charts[(size_t) chartButton->index];
        desiredSceneId = SceneIDs::CHART_PERFORMANCE_SCENE;
    }
}
void ChartSelectionScene::resized()
{
    constexpr int titleHeight = 40;
    constexpr int rowHeight = 70;
    constexpr int rowSpacing = 20;

    auto bounds = getLocalBounds().withTrimmedLeft (10).withTrimmedRight (10).withTrimmedTop (titleHeight);

    for (auto& button : chartSelectionButtons)
    {
        button->setBounds (bounds.removeFromTop (rowHeight));
        bounds.removeFromTop (rowSpacing);
    }

    newChartButton.setBounds (bounds.removeFromTop (rowHeight));

    if (creationPanel != nullptr)
        creationPanel->setBounds (getLocalBounds());
}
SceneIDs::SceneID ChartSelectionScene::getDesiredSceneID()
{
    return desiredSceneId;
}
SceneIDs::SceneID ChartSelectionScene::getSceneID() const
{
    return SceneIDs::CHART_SELECT_SCENE;
}
