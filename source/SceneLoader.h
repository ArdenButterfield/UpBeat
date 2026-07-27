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
    SceneLoader(GameState* gs);
    ~SceneLoader() override;

    std::unique_ptr<Scene> currentScene;

    void prepareToPlay(double sampleRate, int samplesPerBlock);
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&);
    void releaseResources();
    void resized() override;
private:
    void loadInitialScene();
    void loadScene();
    void timerCallback() override;

    juce::CriticalSection sceneTransitionCriticalSection;

    double sampleRate;
    int samplesPerBlock;
    bool prepared = false;
    GameState* gameState;
};

#endif //UPBEAT_SCENELOADER_H
