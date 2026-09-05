//
// Created by Arden on 7/7/2026.
//

#ifndef UPBEAT_CHARTSELECTIONSCENE_H
#define UPBEAT_CHARTSELECTIONSCENE_H

#include "Scene.h"
#include "../UI/ChartCreationPanel.h"
#include "../UI/ChartSelectionButton.h"
#include "juce_gui_basics/juce_gui_basics.h"

class ChartSelectionScene : public Scene, public juce::Button::Listener
{
public:
    ChartSelectionScene(GameState*);
    ~ChartSelectionScene() override;
    void update() override;
    void paint(juce::Graphics& g) override;
    void buttonClicked(juce::Button*) override;
    void resized() override;
    SceneIDs::SceneID getDesiredSceneID() override;
    SceneIDs::SceneID getSceneID() const override;
private:
    void rebuildChartButtons();
    void openCreationPanel();
    void closeCreationPanel();

    SceneIDs::SceneID desiredSceneId;
    std::vector<std::unique_ptr<ChartSelectionButton>> chartSelectionButtons;
    juce::TextButton newChartButton { "New Chart" };
    std::unique_ptr<ChartCreationPanel> creationPanel;
};

#endif //UPBEAT_CHARTSELECTIONSCENE_H
