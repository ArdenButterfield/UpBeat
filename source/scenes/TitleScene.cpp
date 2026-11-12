//
// Created by Arden on 7/7/2026.
//

#include "TitleScene.h"
#include "../UI/GuiStyle.h"
TitleScene::TitleScene(GameState* gs) : Scene(gs)
{
    sceneID = SceneIDs::TITLE_SCENE;
}
TitleScene::~TitleScene()
{
}
void TitleScene::paint (juce::Graphics& g)
{
    g.setColour (gameState->colours.titleBackground);
    g.fillAll();
    g.setColour (gameState->colours.text);
    g.setFont (GuiStyle::fontSize (24.0f));
    g.drawText ("UpBeat", getLocalBounds(), juce::Justification::centred, 1);
}

void TitleScene::update()
{
}
SceneIDs::SceneID TitleScene::getSceneID() const
{
    return SceneIDs::TITLE_SCENE;
}
SceneIDs::SceneID TitleScene::getDesiredSceneID()
{
    return sceneID;
}

void TitleScene::mouseUp (const juce::MouseEvent& event)
{
    if (event.mouseWasClicked())
    {
        sceneID = SceneIDs::CHART_SELECT_SCENE;
    }
}