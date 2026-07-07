//
// Created by Arden on 7/7/2026.
//

#include "Scene.h"
Scene::Scene()
{
}
Scene::~Scene()
{
}
void Scene::prepareToPlay (double sampleRate, int samplesPerBlock)
{
}
void Scene::processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&)
{
}
void Scene::releaseResources()
{
}
void Scene::paint (juce::Graphics& g)
{
    Component::paint (g);
}
void Scene::resized()
{
    Component::resized();
}
SceneIDs::SceneID Scene::getSceneID() const
{
    return SceneIDs::NO_SCENE;
}
SceneIDs::SceneID Scene::getDesiredSceneID()
{
    return SceneIDs::NO_SCENE;
}