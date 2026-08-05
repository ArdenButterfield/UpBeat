//
// Created by Arden on 7/8/2026.
//

#ifndef UPBEAT_CHARTPERFORMANCESCENE_H
#define UPBEAT_CHARTPERFORMANCESCENE_H

#include "../Audio/SineWaveSynth.h"
#include "../Audio/SquareWaveSynth.h"
#include "../Audio/MetronomeSynth.h"
#include "Scene.h"
#include "ToleranceLabel.h"
#include "juce_gui_basics/juce_gui_basics.h"

class ChartPerformanceScene : public Scene, public juce::Button::Listener
{
public:
    explicit ChartPerformanceScene(GameState* gs);
    ~ChartPerformanceScene() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    SceneIDs::SceneID getDesiredSceneID() override;
    SceneIDs::SceneID getSceneID() const override;
private:
    juce::TextButton startButton;

    SquareWaveSynth synth;
    SineWaveSynth backgroundSynth;
    MetronomeSynth metronomeSynth;

    void buttonClicked(juce::Button*) override;
    void update() override;
    void paint(juce::Graphics& g) override;
    void resized() override;
    void startGame();
    bool keyPressed(const juce::KeyPress& key) override;
    bool playing;
    long long timeMs;
    long long elapsedSamples;
    double pixelsPerMillisecond = 0.2;

    juce::Rectangle<int> laneOutline;
    std::array<juce::Rectangle<int>, GameState::numberOfInputLanes> lanes;
    std::array<juce::Rectangle<int>, GameState::numberOfInputLanes> buttonIndicators;
    std::array<float, GameState::numberOfInputLanes> indicatorLighting;
    std::array<int, GameState::numberOfInputLanes> keys = {65, 83, 68, 70};

    std::queue<ChartEvent*> playbackQueue;

    juce::CriticalSection playbackLock;

    long long gameStartTime;

    ChartEvent* findClosestNoteForHit(int lane, long long time);

    juce::OwnedArray<ToleranceLabel> toleranceLabels;

    std::multimap<long long, ChartEvent>::iterator playbackIterator;

    double sampleRate;
};

#endif //UPBEAT_CHARTPERFORMANCESCENE_H
