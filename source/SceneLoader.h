//
// Created by Arden on 7/7/2026.
//

#ifndef UPBEAT_SCENELOADER_H
#define UPBEAT_SCENELOADER_H

#include "juce_gui_basics/juce_gui_basics.h"
#include "scenes/Scene.h"
#include <memory>

class SceneLoader : public juce::Timer, public juce::Component
{
public:
    SceneLoader();
    ~SceneLoader() override;

    std::unique_ptr<Scene> currentScene;
private:
    void loadInitialScene();
    void loadScene();
    void timerCallback() override;
};

#endif //UPBEAT_SCENELOADER_H
