//
// Created by Arden on 7/8/2026.
//

#ifndef UPBEAT_CHARTPERFORMANCESCENE_H
#define UPBEAT_CHARTPERFORMANCESCENE_H

#include "../Audio/SineWaveSynth.h"
#include "../Audio/SquareWaveSynth.h"
#include "../Audio/MetronomeSynth.h"
#include "../UI/NoteLaneDisplay.h"
#include "Scene.h"
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
    juce::Slider tempoScaleSlider;
    juce::Label tempoScaleLabel;
    juce::Slider noteVelocitySlider;
    juce::Label noteVelocityLabel;

    SquareWaveSynth synth;
    SineWaveSynth backgroundSynth;
    MetronomeSynth metronomeSynth;

    NoteLaneDisplay noteLaneDisplay;

    void buttonClicked (juce::Button*) override;
    void update() override;
    void paint (juce::Graphics& g) override;
    void resized() override;
    void startGame();
    bool keyPressed (const juce::KeyPress& key) override;
    bool playing;
    long long timeMs;
    long long elapsedSamples;
    long long lastNoteTimeMs;
    SceneIDs::SceneID desiredSceneId;

    std::queue<ChartEvent*> playbackQueue;

    juce::CriticalSection playbackLock;

    long long gameStartTime;

    // Pointer view of gameState->currentChart->events, built once at game start, that
    // NoteLaneDisplay uses so its display/hit-testing code stays chart-agnostic.
    std::multimap<long long, ChartEvent*> displayEvents;

    double sampleRate;
};

#endif //UPBEAT_CHARTPERFORMANCESCENE_H
