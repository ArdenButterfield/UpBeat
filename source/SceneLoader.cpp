//
// Created by Arden on 7/7/2026.
//

#include "SceneLoader.h"

#include "BundledResources.h"
#include "scenes/ChartPerformanceScene.h"
#include "scenes/ChartSelectionScene.h"
#include "scenes/TitleScene.h"

SceneLoader::SceneLoader (GameState* gs)
{
    gameState = gs;
    prepared = false;

    loadInitialScene();
    startTimerHz (60);
}


SceneLoader::~SceneLoader()
{
}

void SceneLoader::prepareToPlay (double _sampleRate, int _samplesPerBlock)
{
    sampleRate = _sampleRate;
    samplesPerBlock = _samplesPerBlock;

    if (currentScene != nullptr)
    {
        currentScene->prepareToPlay (sampleRate, samplesPerBlock);
    }
}

void SceneLoader::processBlock (juce::AudioBuffer<float>& audioBuffer, juce::MidiBuffer& midiBuffer)
{
    const juce::ScopedTryLock sceneTransitionLock(sceneTransitionCriticalSection);
    if (sceneTransitionLock.isLocked() && currentScene != nullptr)
    {
        currentScene->processBlock (audioBuffer, midiBuffer);
    }
}

void SceneLoader::releaseResources()
{
    if (currentScene != nullptr)
    {
        currentScene->releaseResources();
    }
}

void SceneLoader::loadInitialScene()
{
    currentScene = std::make_unique<TitleScene>(gameState);
    loadScene();
}

void SceneLoader::loadScene()
{
    if (currentScene != nullptr)
    {
        addAndMakeVisible (currentScene.get());
        currentScene->setBounds (getLocalBounds());
        if (prepared)
        {
            currentScene->prepareToPlay (sampleRate, samplesPerBlock);
        }
    }
}

void SceneLoader::timerCallback()
{
    if (currentScene->getSceneID() != currentScene->getDesiredSceneID())
    {
        const juce::ScopedLock sceneLock(sceneTransitionCriticalSection);
        removeChildComponent (currentScene.get());
        currentScene->releaseResources();
        switch (currentScene->getDesiredSceneID())
        {
            case SceneIDs::NO_SCENE:
                currentScene = nullptr;
            case SceneIDs::TITLE_SCENE:
                currentScene = std::make_unique<TitleScene>(gameState);
                break;
            case SceneIDs::CHART_SELECT_SCENE:
                currentScene = std::make_unique<ChartSelectionScene>(gameState);
                break;
            case SceneIDs::CHART_PERFORMANCE_SCENE:
                currentScene = std::make_unique<ChartPerformanceScene>(gameState);
                break;
            default:;
        }
    }
    loadScene();
}
