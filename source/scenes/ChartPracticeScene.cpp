//
// Created by Arden on 9/7/2026.
//

#include "ChartPracticeScene.h"

#include <cstdlib>

ChartPracticeScene::ChartPracticeScene (GameState* gs)
    : Scene (gs),
      startButton ("Start"),
      playbackControls (gs->currentChart->tempoScale),
      synths (*gs->currentChart),
      noteLaneDisplay (gs, gs->currentChart->numLanes),
      bars (BarAtom::splitIntoBars (*gs->currentChart)),
      barProgressDisplay (static_cast<int> (bars.size())),
      playing (false),
      desiredSceneId (SceneIDs::CHART_PRACTICE_SCENE),
      countInTime (gs->currentChart->countInTime)
{
    addAndMakeVisible (startButton);
    startButton.addListener (this);

    clock.setTempoScale (gameState->currentChart->tempoScale);
    playbackControls.onTempoScaleChange = [this] (double tempoScale)
    {
        gameState->currentChart->tempoScale = tempoScale;
        clock.setTempoScale (tempoScale);
    };
    addAndMakeVisible (playbackControls);

    addAndMakeVisible (noteLaneDisplay);

    addAndMakeVisible (barProgressDisplay);

    setWantsKeyboardFocus (true);
    audioChartTimeMs = static_cast<double> (-countInTime);
}

ChartPracticeScene::~ChartPracticeScene()
{
}

void ChartPracticeScene::prepareToPlay (double _sampleRate, int samplesPerBlock)
{
    sampleRate = _sampleRate;
    synths.prepareToPlay (sampleRate, samplesPerBlock);
    metronomeSynth.prepareToPlay (sampleRate);
}

void ChartPracticeScene::processBlock (juce::AudioBuffer<float>& audio_buffer, juce::MidiBuffer&)
{
    auto lock = juce::ScopedTryLock (playbackLock);
    if (lock.isLocked())
    {
        while (!playbackQueue.empty())
        {
            auto e = playbackQueue.front();
            playbackQueue.pop();
            synths.noteOn (e->synthId, e->midiNote, e->velocity, noteDurationSeconds (*e));
        }
    }

    if (playing)
    {
        auto bufferChartMs = audio_buffer.getNumSamples() * 1000.0 / sampleRate * clock.getTempoScale();
        auto bufferStartTime = static_cast<long long> (std::floor (audioChartTimeMs));
        auto bufferEndTime = static_cast<long long> (std::floor (audioChartTimeMs + bufferChartMs));

        auto displayLock = juce::ScopedTryLock (displayEventsLock);
        if (displayLock.isLocked())
        {
            synths.applyAutomation (chartTimeAt (audioChartTimeMs));

            for (auto event = displayEvents.lower_bound (bufferStartTime); event != displayEvents.end() && event->first < bufferEndTime; ++event)
            {
                if (event->second->type == ChartEvent::BACKGROUND_NOTE)
                {
                    synths.noteOn (event->second->synthId, event->second->midiNote, event->second->velocity, noteDurationSeconds (*event->second));
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
        audioChartTimeMs += bufferChartMs;
    }
    else
    {
        synths.applyAutomation (static_cast<double> (-countInTime));
        audioChartTimeMs = static_cast<double> (-countInTime);
    }

    synths.renderNextBlock (audio_buffer, 0, audio_buffer.getNumSamples());
    metronomeSynth.renderNextBlock (audio_buffer, 0, audio_buffer.getNumSamples());
}

double ChartPracticeScene::chartTimeAt (double practiceTimeMs) const
{
    if (queuedBarSpans.empty())
        return practiceTimeMs;

    // Times before the first queued bar (the count-in) or after the last run on from that bar.
    auto span = std::find_if (queuedBarSpans.begin(), queuedBarSpans.end() - 1, [practiceTimeMs] (const QueuedBarSpan& s) {
        return practiceTimeMs < static_cast<double> (s.startMs + s.lengthMs);
    });
    return static_cast<double> (span->chartStartMs) + practiceTimeMs - static_cast<double> (span->startMs);
}

double ChartPracticeScene::noteDurationSeconds (const ChartEvent& event) const
{
    return static_cast<double> (event.lengthMs) / 1000.0 / clock.getTempoScale();
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
    ++completedBarCount;

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

    bar.successHistory.push_back (allGreatOrPerfect);
    barProgressDisplay.setSuccessHistory (barIndex, bar.successHistory);

    if (allGreatOrPerfect)
    {
        bar.learnedScore += 1;
    } else
    {
        bar.learnedScore = 0;
    }

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
    std::vector<QueuedBarSpan> newSpans;
    std::vector<NoteLaneDisplay::BarSpan> displayBars;
    auto offset = currentBarStartMs;
    for (auto barIndex : playQueue)
    {
        auto& bar = bars[(size_t) barIndex];
        for (auto& [t, event] : bar.events)
            newEvents.insert ({ t + offset, &event });
        newSpans.push_back ({ offset, bar.chartStartMs, bar.lengthMs });
        displayBars.push_back ({ offset, bar.lengthMs });
        offset += bar.lengthMs;
    }

    {
        juce::ScopedLock lock (displayEventsLock);
        displayEvents = std::move (newEvents);
        queuedBarSpans = std::move (newSpans);
    }
    noteLaneDisplay.setEvents (&displayEvents);
    noteLaneDisplay.setBars (std::move (displayBars), completedBarCount);
    barProgressDisplay.setCurrentBar (playQueue.empty() ? -1 : playQueue.front());
}

void ChartPracticeScene::startGame()
{
    startButton.setVisible (false);
    playing = true;
    timeMs = -countInTime;
    currentBarStartMs = 0;
    completedBarCount = 0;
    clock.start (countInTime);
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
        timeMs = clock.now();

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

    constexpr int controlsWidth = 70;
    constexpr int progressWidth = 100;
    auto bounds = getLocalBounds().reduced (10, 20);
    playbackControls.setBounds (bounds.removeFromLeft (controlsWidth));
    bounds.removeFromLeft (10);
    barProgressDisplay.setBounds (bounds.removeFromRight (progressWidth));
    bounds.removeFromRight (10);
    noteLaneDisplay.setBounds (bounds);
}

bool ChartPracticeScene::keyPressed (const juce::KeyPress& key)
{
    auto hitTime = clock.now();
    auto* closestEvent = noteLaneDisplay.registerKeyPress (key, hitTime);
    if (closestEvent != nullptr)
    {
        auto scopedLock = juce::ScopedLock (playbackLock);
        playbackQueue.push (closestEvent);
    }
    return true;
}
