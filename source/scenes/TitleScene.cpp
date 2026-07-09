//
// Created by Arden on 7/7/2026.
//

#include "TitleScene.h"
TitleScene::TitleScene()
{
}
TitleScene::~TitleScene()
{
}
void TitleScene::paint (juce::Graphics& g)
{
    g.setColour (juce::Colours::darkblue);
    g.fillAll();
    g.setColour (juce::Colours::white);
    g.setFont (24);
    g.drawText ("UpBeat", getLocalBounds(), juce::Justification::centred, 1);

    sceneID = SceneIDs::TITLE_SCENE;
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