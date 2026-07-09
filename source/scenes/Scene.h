//
// Created by Arden on 7/7/2026.
//

#ifndef UPBEAT_SCENE_H
#define UPBEAT_SCENE_H

#include "../GameState.h"
#include "SceneIds.h"
#include "juce_audio_basics/juce_audio_basics.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include "juce_gui_extra/juce_gui_extra.h"

class Scene : public juce::AnimatedAppComponent
{
public:
    Scene(GameState* gs);
    ~Scene() override;
    void prepareToPlay(double sampleRate, int samplesPerBlock) ;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) ;
    void releaseResources();

    void paint(juce::Graphics& g) override;
    void resized() override;

    virtual SceneIDs::SceneID getSceneID() const;
    virtual SceneIDs::SceneID getDesiredSceneID();
protected:
    GameState* gameState;
};

#endif //UPBEAT_SCENE_H
