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
    virtual void prepareToPlay(double sampleRate, int samplesPerBlock);
    virtual void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&);
    void releaseResources();

    void paint(juce::Graphics& g) override;
    void resized() override;

    virtual SceneIDs::SceneID getSceneID() const;
    virtual SceneIDs::SceneID getDesiredSceneID();
protected:
    // "Loading..." across the middle of the scene, for while it isn't ready to play.
    void paintLoadingMessage (juce::Graphics& g) const;

    // How long chart scenes keep going after their last note ends before showing the results, in
    // real (not tempo-scaled) time, and that delay in chart time at the given tempo scale.
    static constexpr double resultsDelayMs = 1500.0;
    static long long resultsDelayChartMs (double tempoScale) { return static_cast<long long> (resultsDelayMs * tempoScale); }

    GameState* gameState;
};

#endif //UPBEAT_SCENE_H
