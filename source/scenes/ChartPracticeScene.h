//
// Created by Arden on 9/7/2026.
//

#ifndef UPBEAT_CHARTPRACTICESCENE_H
#define UPBEAT_CHARTPRACTICESCENE_H

#include "../Audio/SineWaveSynth.h"
#include "../Audio/SquareWaveSynth.h"
#include "../Audio/MetronomeSynth.h"
#include "../Chart/BarAtom.h"
#include "../UI/BarProgressDisplay.h"
#include "../UI/NoteLaneDisplay.h"
#include "Scene.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include <deque>

// The practice-mode counterpart to ChartPerformanceScene: the chart is split into bars
// (BarAtoms), and a rolling queue of a few bars is repeated until each is played
// consistently well before the next new bar is introduced. See Practice Mode.md for the
// full BarQueue algorithm.
class ChartPracticeScene : public Scene, public juce::Button::Listener
{
public:
    explicit ChartPracticeScene (GameState* gs);
    ~ChartPracticeScene() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    SceneIDs::SceneID getDesiredSceneID() override;
    SceneIDs::SceneID getSceneID() const override;

private:
    // How many bars are practiced concurrently, and how many clean reps in a row a bar
    // needs before it's considered learned.
    static constexpr int queueSize = 4;
    static constexpr int repsToLearn = 3;

    void buttonClicked (juce::Button*) override;
    void update() override;
    void paint (juce::Graphics& g) override;
    void resized() override;
    void startGame();
    bool keyPressed (const juce::KeyPress& key) override;

    // Moves barIndex to the back of the playQueue and seeds a fresh performance-timing
    // slot on each of its notes for the upcoming rep.
    void enqueueBar (int barIndex);

    // Called when the playhead passes the end of the front bar in the playQueue: scores
    // that rep, then either re-queues the bar or brings in the next unlearned one.
    void advanceBarQueue();

    // Rebuilds displayEvents from the bars currently in playQueue, laid out back-to-back
    // starting at currentBarStartMs with no gaps, for NoteLaneDisplay and processBlock.
    void rebuildDisplayEvents();

    juce::TextButton startButton;

    SquareWaveSynth synth;
    SineWaveSynth backgroundSynth;
    MetronomeSynth metronomeSynth;

    NoteLaneDisplay noteLaneDisplay;
    std::vector<BarAtom> bars;
    BarProgressDisplay barProgressDisplay;

    std::deque<int> unlearnedBarQueue;
    std::deque<int> playQueue;
    long long currentBarStartMs;

    std::multimap<long long, ChartEvent*> displayEvents;
    juce::CriticalSection displayEventsLock;

    bool playing;
    long long timeMs;
    long long elapsedSamples;
    SceneIDs::SceneID desiredSceneId;

    std::queue<ChartEvent*> playbackQueue;
    juce::CriticalSection playbackLock;

    long long gameStartTime;
    long long countInTime;
    double tempoScale;

    double sampleRate;
};

#endif //UPBEAT_CHARTPRACTICESCENE_H
