//
// Created by Arden on 7/8/2026.
//

#ifndef UPBEAT_CHARTPERFORMANCESCENE_H
#define UPBEAT_CHARTPERFORMANCESCENE_H

#include "../Audio/ChartSynthLoader.h"
#include "../Audio/MetronomeSynth.h"
#include "../Chart/BarAtom.h"
#include "../Chart/PlaybackClock.h"
#include "../UI/NoteLaneDisplay.h"
#include "../UI/PlaybackControls.h"
#include "../UI/TimingHistogram.h"
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
    PlaybackControls playbackControls;

    // Player hits and background notes both play on the chart synth the note belongs to.
    ChartSynthLoader synthLoader;
    MetronomeSynth metronomeSynth;
    // Built before playback starts and only read afterwards, so the audio thread can read it freely.
    CountIn countIn;

    NoteLaneDisplay noteLaneDisplay;
    TimingHistogram timingHistogram;

    void buttonClicked (juce::Button*) override;
    void update() override;
    void paint (juce::Graphics& g) override;
    void paintOverChildren (juce::Graphics& g) override;
    void resized() override;
    void startGame();
    bool keyPressed (const juce::KeyPress& key) override;
    // How long a synth should hold event's note, in real (tempo-scaled) seconds.
    double noteDurationSeconds (const ChartEvent& event) const;
    bool playing;
    long long timeMs;

    // Message-thread chart time (playhead, key presses). Re-anchors on tempo changes.
    PlaybackClock clock;
    // Audio-thread chart time, advanced each block by the block length times the
    // clock's current tempo scale.
    double audioChartTimeMs;
    // When the chart's last note (playable or background) stops sounding.
    long long lastNoteEndMs = 0;
    SceneIDs::SceneID desiredSceneId;

    std::queue<ChartEvent*> playbackQueue;

    juce::CriticalSection playbackLock;

    // Pointer view of gameState->currentChart->events, built once at game start, that
    // NoteLaneDisplay uses so its display/hit-testing code stays chart-agnostic.
    std::multimap<long long, ChartEvent*> displayEvents;

    double sampleRate;
};

#endif //UPBEAT_CHARTPERFORMANCESCENE_H
