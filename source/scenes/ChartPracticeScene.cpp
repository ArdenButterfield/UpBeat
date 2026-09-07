//
// Created by Arden on 9/7/2026.
//

#include "ChartPracticeScene.h"

#include <cstdlib>

ChartPracticeScene::ChartPracticeScene (GameState* gs)
    : Scene (gs),
      startButton ("Start"),
      noteLaneDisplay (gs, gs->currentChart->numLanes),
      bars (BarAtom::splitIntoBars (*gs->currentChart)),
      barProgressDisplay (static_cast<int> (bars.size())),
      playing (false),
      desiredSceneId (SceneIDs::CHART_PRACTICE_SCENE),
      countInTime (gs->currentChart->countInTime),
      tempoScale (gs->currentChart->tempoScale)
{
    addAndMakeVisible (startButton);
    startButton.addListener (this);

    noteLaneDisplay.setNoteOnScreenVelocity (gameState->currentChart->noteOnScreenVelocity);
    addAndMakeVisible (noteLaneDisplay);

    addAndMakeVisible (barProgressDisplay);

    setWantsKeyboardFocus (true);
    elapsedSamples = 0;
}

ChartPracticeScene::~ChartPracticeScene()
{
}

void ChartPracticeScene::prepareToPlay (double _sampleRate, int samplesPerBlock)
{
    sampleRate = _sampleRate;
    juce::ignoreUnused (samplesPerBlock);
    synth.prepareToPlay (sampleRate);
    backgroundSynth.prepareToPlay (sampleRate);
    metronomeSynth.prepareToPlay (sampleRate);
}

void ChartPracticeScene::processBlock (juce::AudioBuffer<float>& audio_buffer, juce::MidiBuffer&)
{
    auto lock = juce::ScopedTryLock (playbackLock);
    if (lock.isLocked())
    {
        while (!playbackQueue.empty())
        {
            auto e = playbackQueue.back();
            playbackQueue.pop();
            synth.noteOn (e->midiNote);
        }
    }

    if (playing)
    {
        auto bufferStartRealMs = static_cast<long long> (elapsedSamples * 1000.0 / sampleRate);
        auto bufferEndRealMs = static_cast<long long> ((elapsedSamples + audio_buffer.getNumSamples()) * 1000.0 / sampleRate);
        auto bufferStartTime = Chart::chartTimeForRealElapsedMs (bufferStartRealMs, tempoScale, countInTime);
        auto bufferEndTime = Chart::chartTimeForRealElapsedMs (bufferEndRealMs, tempoScale, countInTime);

        auto displayLock = juce::ScopedTryLock (displayEventsLock);
        if (displayLock.isLocked())
        {
            for (auto event = displayEvents.lower_bound (bufferStartTime); event != displayEvents.end() && event->first < bufferEndTime; ++event)
            {
                if (event->second->type == ChartEvent::NOTE)
                {
                    backgroundSynth.noteOn (event->second->midiNote);
                }
                else if (event->second->type == ChartEvent::BARLINE)
                {
                    metronomeSynth.noteOn (MetronomeSynth::BARLINE);
                }
                else if (event->second->type == ChartEvent::BEAT)
                {
                    metronomeSynth.noteOn (MetronomeSynth::BEAT);
                }
            }
        }
        elapsedSamples += audio_buffer.getNumSamples();
    }
    else
    {
        elapsedSamples = 0;
    }

    synth.renderNextBlock (audio_buffer, 0, audio_buffer.getNumSamples());
    backgroundSynth.renderNextBlock (audio_buffer, 0, audio_buffer.getNumSamples());
    metronomeSynth.renderNextBlock (audio_buffer, 0, audio_buffer.getNumSamples());
}

SceneIDs::SceneID ChartPracticeScene::getDesiredSceneID()
{
    return desiredSceneId;
}

SceneIDs::SceneID ChartPracticeScene::getSceneID() const
{
    return SceneIDs::CHART_PRACTICE_SCENE;
}

void ChartPracticeScene::buttonClicked (juce::Button* b)
{
    if (b == &startButton)
    {
        startGame();
    }
}

void ChartPracticeScene::enqueueBar (int barIndex)
{
    playQueue.push_back (barIndex);
    barProgressDisplay.setBarState (barIndex, BarProgressDisplay::BarState::InProgress);
    for (auto& [t, event] : bars[(size_t) barIndex].events)
        if (event.type == ChartEvent::NOTE)
            event.performanceTimings.emplace_back (UNPLAYED_NOTE);
}

void ChartPracticeScene::advanceBarQueue()
{
    auto barIndex = playQueue.front();
    playQueue.pop_front();
    auto& bar = bars[(size_t) barIndex];
    currentBarStartMs += bar.lengthMs;

    bool allGreatOrPerfect = true;
    for (auto& [t, event] : bar.events)
    {
        if (event.type != ChartEvent::NOTE)
            continue;

        auto& timings = event.performanceTimings;
        if (timings.empty() || timings.back() == UNPLAYED_NOTE || std::abs (timings.back()) >= gameState->tolerances[1])
        {
            allGreatOrPerfect = false;
            break;
        }
    }

    if (allGreatOrPerfect)
        bar.learnedScore += 1;

    if (bar.learnedScore >= repsToLearn)
    {
        barProgressDisplay.setBarState (barIndex, BarProgressDisplay::BarState::Learned);
        if (!unlearnedBarQueue.empty())
        {
            auto nextBarIndex = unlearnedBarQueue.front();
            unlearnedBarQueue.pop_front();
            enqueueBar (nextBarIndex);
        }
    }
    else
    {
        enqueueBar (barIndex);
    }

    rebuildDisplayEvents();
}

void ChartPracticeScene::rebuildDisplayEvents()
{
    std::multimap<long long, ChartEvent*> newEvents;
    auto offset = currentBarStartMs;
    for (auto barIndex : playQueue)
    {
        auto& bar = bars[(size_t) barIndex];
        for (auto& [t, event] : bar.events)
            newEvents.insert ({ t + offset, &event });
        offset += bar.lengthMs;
    }

    {
        juce::ScopedLock lock (displayEventsLock);
        displayEvents = std::move (newEvents);
    }
    noteLaneDisplay.setEvents (&displayEvents);
}

void ChartPracticeScene::startGame()
{
    startButton.setVisible (false);
    playing = true;
    timeMs = -countInTime;
    currentBarStartMs = 0;
    gameStartTime = juce::Time::currentTimeMillis();
    grabKeyboardFocus();

    unlearnedBarQueue.clear();
    playQueue.clear();
    for (int i = 0; i < (int) bars.size(); ++i)
        unlearnedBarQueue.push_back (i);

    for (int i = 0; i < queueSize && !unlearnedBarQueue.empty(); ++i)
    {
        auto barIndex = unlearnedBarQueue.front();
        unlearnedBarQueue.pop_front();
        enqueueBar (barIndex);
    }

    rebuildDisplayEvents();
}

void ChartPracticeScene::update()
{
    auto elapsed = getMillisecondsSinceLastUpdate();

    if (playing)
    {
        timeMs = Chart::chartTimeForRealElapsedMs (juce::Time::currentTimeMillis() - gameStartTime, tempoScale, countInTime);

        while (!playQueue.empty() && timeMs - currentBarStartMs >= bars[(size_t) playQueue.front()].lengthMs)
        {
            advanceBarQueue();
        }

        if (playQueue.empty())
        {
            desiredSceneId = SceneIDs::CHART_FEEDBACK_SCENE;
        }
    }

    noteLaneDisplay.setPlayheadTimeMs (timeMs);
    noteLaneDisplay.advance (elapsed);
}

void ChartPracticeScene::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::darkgrey);
}

void ChartPracticeScene::resized()
{
    grabKeyboardFocus();
    startButton.setBounds (getLocalBounds().withSizeKeepingCentre (200, 40));

    auto laneOutline = getLocalBounds().withWidth (std::min (300, getWidth() - 80)).withTrimmedTop (20).withTrimmedBottom (20).withCentre ({ getWidth() / 2, getHeight() / 2 });
    noteLaneDisplay.setBounds (laneOutline);

    constexpr int progressWidth = 60;
    barProgressDisplay.setBounds (laneOutline.getRight() + 10, laneOutline.getY(), progressWidth, laneOutline.getHeight());
}

bool ChartPracticeScene::keyPressed (const juce::KeyPress& key)
{
    auto hitTime = Chart::chartTimeForRealElapsedMs (juce::Time::currentTimeMillis() - gameStartTime, tempoScale, countInTime);
    auto* closestEvent = noteLaneDisplay.registerKeyPress (key, hitTime);
    if (closestEvent != nullptr)
    {
        auto scopedLock = juce::ScopedLock (playbackLock);
        playbackQueue.push (closestEvent);
    }
    return true;
}
