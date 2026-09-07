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
    chartTitleLabels.clear();
    performanceButtons.clear();
    practiceButtons.clear();

    for (int i = 0; i < (int) gameState->charts.size(); ++i)
    {
        auto title = std::make_unique<juce::Label> (juce::String(), gameState->charts[(size_t) i].name);
        title->setColour (juce::Label::textColourId, juce::Colours::white);
        title->setFont (juce::Font (juce::FontOptions (18.0f)));
        addAndMakeVisible (title.get());
        chartTitleLabels.push_back (std::move (title));

        auto performanceButton = std::make_unique<ChartSelectionButton> ("Performance", i);
        addAndMakeVisible (performanceButton.get());
        performanceButton->addListener (this);
        performanceButtons.push_back (std::move (performanceButton));

        auto practiceButton = std::make_unique<ChartSelectionButton> ("Practice", i);
        addAndMakeVisible (practiceButton.get());
        practiceButton->addListener (this);
        practiceButtons.push_back (std::move (practiceButton));
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

    for (auto& button : performanceButtons)
    {
        if (b == button.get())
        {
            gameState->currentChart = &gameState->charts[(size_t) button->index];
            desiredSceneId = SceneIDs::CHART_PERFORMANCE_SCENE;
            return;
        }
    }

    for (auto& button : practiceButtons)
    {
        if (b == button.get())
        {
            gameState->currentChart = &gameState->charts[(size_t) button->index];
            desiredSceneId = SceneIDs::CHART_PRACTICE_SCENE;
            return;
        }
    }
}
void ChartSelectionScene::resized()
{
    constexpr int titleHeight = 40;
    constexpr int rowHeight = 70;
    constexpr int rowSpacing = 20;
    constexpr int buttonWidth = 110;
    constexpr int buttonSpacing = 10;

    auto bounds = getLocalBounds().withTrimmedLeft (10).withTrimmedRight (10).withTrimmedTop (titleHeight);

    for (size_t i = 0; i < chartTitleLabels.size(); ++i)
    {
        auto row = bounds.removeFromTop (rowHeight);
        auto buttonsArea = row.removeFromRight (buttonWidth * 2 + buttonSpacing);
        practiceButtons[i]->setBounds (buttonsArea.removeFromRight (buttonWidth));
        buttonsArea.removeFromRight (buttonSpacing);
        performanceButtons[i]->setBounds (buttonsArea.removeFromRight (buttonWidth));
        chartTitleLabels[i]->setBounds (row);
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
