//
// Created by Arden on 7/7/2026.
//

#ifndef UPBEAT_TITLESCENE_H
#define UPBEAT_TITLESCENE_H
#include "Scene.h"

class TitleScene : public Scene
{
public:
    TitleScene(GameState* gs);
    ~TitleScene() override;
    void paint(juce::Graphics& g) override;
    void update() override;
    SceneIDs::SceneID getSceneID() const override;
    SceneIDs::SceneID getDesiredSceneID() override;
    void mouseUp(const juce::MouseEvent& event) override;
private:
    SceneIDs::SceneID sceneID;

};

#endif //UPBEAT_TITLESCENE_H
