//
// Created by Arden on 7/7/2026.
//

#include "SceneLoader.h"

#include "scenes/ChartSelectionScene.h"
#include "scenes/TitleScene.h"
SceneLoader::SceneLoader ()
{
    loadInitialScene();
    startTimerHz (60);
}

SceneLoader::~SceneLoader()
{
}
void SceneLoader::prepareToPlay (double sampleRate, int samplesPerBlock)
{

}
void SceneLoader::processBlock (juce::AudioBuffer<float>& audioBuffer, juce::MidiBuffer& midiBuffer)
{
    const juce::ScopedTryLock sceneTransitionLock(sceneTransitionCriticalSection);
    if (sceneTransitionLock.isLocked())
    {
        if (currentScene != nullptr)
        {
            currentScene->processBlock (audioBuffer, midiBuffer);
        }
    }
}
void SceneLoader::releaseResources()
{
}

void SceneLoader::loadInitialScene()
{
    currentScene = std::make_unique<TitleScene>();
    loadScene();
}

void SceneLoader::loadScene()
{
    if (currentScene != nullptr)
    {
        addAndMakeVisible (currentScene.get());
        currentScene->setBounds (getLocalBounds());
    }
}
void SceneLoader::timerCallback()
{
    if (currentScene->getSceneID() != currentScene->getDesiredSceneID())
    {
        const juce::ScopedLock sceneLock(sceneTransitionCriticalSection);
        removeChildComponent (currentScene.get());
        switch (currentScene->getDesiredSceneID())
        {
            case SceneIDs::NO_SCENE:
                break;
            case SceneIDs::TITLE_SCENE:
                currentScene = std::make_unique<TitleScene>();
                break;
            case SceneIDs::CHART_SELECT_SCENE:
                currentScene = std::make_unique<ChartSelectionScene>();
                break;
            default:;
        }
    }
    loadScene();
}