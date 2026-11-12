//
// Created by Arden on 9/7/2026.
//

#include "ChartPracticeScene.h"

#include <cstdlib>

ChartPracticeScene::ChartPracticeScene (GameState* gs)
    : Scene (gs),
      startButton ("Start"),
      playbackControls (gs->currentChart->tempoScale),
      synthLoader (*gs->currentChart),
      noteLaneDisplay (gs, gs->currentChart->numLanes),
      bars (BarAtom::splitIntoBars (*gs->currentChart)),
      countIn (bars.front()),
      barProgressDisplay (static_cast<int> (bars.size()), gs->colours),
      timingHistogram (*gs),
      playing (false),
      desiredSceneId (SceneIDs::CHART_PRACTICE_SCENE)
{
    // Hidden, with "Loading..." in its place, until the synths have loaded.
    addChildComponent (startButton);
    startButton.addListener (this);
    synthLoader.onLoaded = [this]
    {
        startButton.setVisible (true);
        repaint();
    };

    clock.setTempoScale (gameState->currentChart->tempoScale);
    playbackControls.onTempoScaleChange = [this] (double tempoScale)
    {
        gameState->currentChart->tempoScale = tempoScale;
        clock.setTempoScale (tempoScale);
    };
    addAndMakeVisible (playbackControls);

    addAndMakeVisible (noteLaneDisplay);

    addAndMakeVisible (barProgressDisplay);

    noteLaneDisplay.onNoteTimed = [this] (long long errorMs) { timingHistogram.addTiming (errorMs); };
    addAndMakeVisible (timingHistogram);

    setWantsKeyboardFocus (true);
    audioChartTimeMs = static_cast<double> (-countIn.lengthMs);
}

ChartPracticeScene::~ChartPracticeScene()
{
}

void ChartPracticeScene::prepareToPlay (double _sampleRate, int samplesPerBlock)
{
    sampleRate = _sampleRate;
    synthLoader.prepareToPlay (sampleRate, samplesPerBlock);
    metronomeSynth.prepareToPlay (sampleRate);
}

void ChartPracticeScene::processBlock (juce::AudioBuffer<float>& audio_buffer, juce::MidiBuffer&)
{
    // nullptr until the synths have loaded; the game can't start before then.
    auto* synths = synthLoader.getSynths();

    auto lock = juce::ScopedTryLock (playbackLock);
    if (lock.isLocked())
    {
        while (!playbackQueue.empty())
        {
            auto e = playbackQueue.front();
            playbackQueue.pop();
            if (synths != nullptr)
                synths->noteOn (e->synthId, e->midiNote, e->velocity, noteDurationSeconds (*e));
        }
    }

    if (playing)
    {
        auto bufferChartMs = audio_buffer.getNumSamples() * 1000.0 / sampleRate * clock.getTempoScale();
        auto bufferStartTime = static_cast<long long> (std::floor (audioChartTimeMs));
        auto bufferEndTime = static_cast<long long> (std::floor (audioChartTimeMs + bufferChartMs));

        for (auto click = countIn.clicks.lower_bound (bufferStartTime); click != countIn.clicks.end() && click->first < bufferEndTime; ++click)
            metronomeSynth.noteOn (click->second.type == ChartEvent::BARLINE ? MetronomeSynth::BARLINE : MetronomeSynth::BEAT);

        auto displayLock = juce::ScopedTryLock (displayEventsLock);
        if (displayLock.isLocked())
        {
            if (synths != nullptr)
                synths->applyAutomation (chartTimeAt (audioChartTimeMs));

            for (auto event = displayEvents.lower_bound (bufferStartTime); event != displayEvents.end() && event->first < bufferEndTime; ++event)
            {
                if (event->second->type == ChartEvent::BACKGROUND_NOTE)
                {
                    if (synths != nullptr)
                        synths->noteOn (event->second->synthId, event->second->midiNote, event->second->velocity, noteDurationSeconds (*event->second));
                }
                else if (event->second->type == ChartEvent::BARLINE)
                {
                    metronomeSynth.noteOn (MetronomeSynth::BARLINE);
                }
                else if (event->second->type == ChartEvent::BEAT && event->second->metronomeClick)
                {
                    metronomeSynth.noteOn (MetronomeSynth::BEAT);
                }
            }
        }
        audioChartTimeMs += bufferChartMs;
    }
    else
    {
        if (synths != nullptr)
            synths->applyAutomation (static_cast<double> (-countIn.lengthMs));
        audioChartTimeMs = static_cast<double> (-countIn.lengthMs);
    }

    if (synths != nullptr)
        synths->renderNextBlock (audio_buffer, 0, audio_buffer.getNumSamples());
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
    // The bar's notes leave the display (and can no longer be hit) once it's rebuilt below,
    // even those whose hit window is still open, so they're labelled as missed now.
    noteLaneDisplay.labelMissedNotesBefore (currentBarStartMs + bar.lengthMs);
    for (auto& [t, event] : bar.events)
        if (event.type == ChartEvent::NOTE || event.type == ChartEvent::BACKGROUND_NOTE)
            lastNoteEndMs = std::max (lastNoteEndMs, currentBarStartMs + t + event.lengthMs);
    currentBarStartMs += bar.lengthMs;
    ++completedBarCount;

    bool allFreshOr100 = true;
    for (auto& [t, event] : bar.events)
    {
        if (event.type != ChartEvent::NOTE)
            continue;

        auto& timings = event.performanceTimings;
        if (timings.empty() || timings.back() == UNPLAYED_NOTE || std::abs (timings.back()) >= gameState->tolerances[1])
        {
            allFreshOr100 = false;
            break;
        }
    }

    bar.successHistory.push_back (allFreshOr100);
    barProgressDisplay.setSuccessHistory (barIndex, bar.successHistory);

    if (allFreshOr100)
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
    timeMs = -countIn.lengthMs;
    currentBarStartMs = 0;
    lastNoteEndMs = 0;
    completedBarCount = 0;
    timingHistogram.reset();
    clock.start (countIn.lengthMs);
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

        if (playQueue.empty() && timeMs >= lastNoteEndMs + resultsDelayChartMs (clock.getTempoScale()))
        {
            desiredSceneId = SceneIDs::CHART_FEEDBACK_SCENE;
        }
    }

    noteLaneDisplay.setPlayheadTimeMs (timeMs);
    noteLaneDisplay.setChartTimeMs (chartTimeAt (static_cast<double> (timeMs)));
    noteLaneDisplay.advance (elapsed);
}

void ChartPracticeScene::paint (juce::Graphics& g)
{
    g.fillAll (gameState->colours.playBackground);
}

void ChartPracticeScene::paintOverChildren (juce::Graphics& g)
{
    if (! synthLoader.isLoaded())
    {
        paintLoadingMessage (g);
        // Lets the loader take the message thread for its next synth, now the scene is on screen.
        synthLoader.framePainted();
    }
}

void ChartPracticeScene::resized()
{
    grabKeyboardFocus();
    startButton.setBounds (getLocalBounds().withSizeKeepingCentre (200, 40));

    constexpr int controlsWidth = 70;
    constexpr int progressWidth = 100;
    constexpr int histogramHeight = 110;
    auto bounds = getLocalBounds().reduced (10, 20);
    playbackControls.setBounds (bounds.removeFromLeft (controlsWidth));
    bounds.removeFromLeft (10);
    barProgressDisplay.setBounds (bounds.removeFromRight (progressWidth));
    bounds.removeFromRight (10);
    timingHistogram.setBounds (bounds.removeFromBottom (histogramHeight));
    bounds.removeFromBottom (10);
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
