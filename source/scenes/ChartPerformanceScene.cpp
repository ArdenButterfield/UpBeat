//
// Created by Arden on 7/8/2026.
//

#include "ChartPerformanceScene.h"
#include "../Chart/MetronomeClicks.h"

#include "../Chart/BarAtom.h"

ChartPerformanceScene::ChartPerformanceScene(GameState* gs) : Scene(gs), startButton("Start"), playbackControls (gs->currentChart->tempoScale), synthLoader (*gs->currentChart), countIn (BarAtom::splitIntoBars (*gs->currentChart).front()), noteLaneDisplay (gs, gs->currentChart->numLanes), timingHistogram (*gs), playing(false), desiredSceneId(SceneIDs::CHART_PERFORMANCE_SCENE)
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

    noteLaneDisplay.onNoteTimed = [this] (long long errorMs) { timingHistogram.addTiming (errorMs); };
    addAndMakeVisible (timingHistogram);

    setWantsKeyboardFocus (true);
    audioChartTimeMs = static_cast<double> (-countIn.lengthMs);
}

ChartPerformanceScene::~ChartPerformanceScene()
{
}
void ChartPerformanceScene::prepareToPlay (double _sampleRate, int samplesPerBlock)
{
    sampleRate = _sampleRate;
    synthLoader.prepareToPlay (sampleRate, samplesPerBlock);
    metronomeSynth.prepareToPlay (sampleRate);
}

void ChartPerformanceScene::processBlock (juce::AudioBuffer<float>& audio_buffer, juce::MidiBuffer& midi_message_metadatas)
{
    juce::ignoreUnused (midi_message_metadatas);
    // nullptr until the synths have loaded; the game can't start before then.
    auto* synths = synthLoader.getSynths();

    auto lock = juce::ScopedTryLock(playbackLock);
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

    if (synths != nullptr)
        synths->applyAutomation (audioChartTimeMs);

    if (playing)
    {
        auto bufferChartMs = audio_buffer.getNumSamples() * 1000.0 / sampleRate * clock.getTempoScale();
        auto bufferStartTime = static_cast<long long> (std::floor (audioChartTimeMs));
        auto bufferEndTime = static_cast<long long> (std::floor (audioChartTimeMs + bufferChartMs));

        for (auto click = countIn.clicks.lower_bound (bufferStartTime); click != countIn.clicks.end() && click->first < bufferEndTime; ++click)
            metronomeSynth.noteOn (click->second.type == ChartEvent::BARLINE ? MetronomeSynth::BARLINE : MetronomeSynth::BEAT);

        for (auto event = gameState->currentChart->events.lower_bound (bufferStartTime); event != gameState->currentChart->events.end() && event->first < bufferEndTime; ++event)
        {
            if (event->second.type == ChartEvent::BACKGROUND_NOTE)
            {
                if (synths != nullptr)
                    synths->noteOn (event->second.synthId, event->second.midiNote, event->second.velocity, noteDurationSeconds (event->second));
            } else if (event->second.type == ChartEvent::BARLINE)
            {
                metronomeSynth.noteOn (MetronomeSynth::BARLINE);
            } else if (event->second.type == ChartEvent::BEAT && event->second.metronomeClick)
            {
                metronomeSynth.noteOn (MetronomeSynth::BEAT);
            }
        }
        audioChartTimeMs += bufferChartMs;
    } else
    {
        audioChartTimeMs = static_cast<double> (-countIn.lengthMs);
    }

    if (synths != nullptr)
        synths->renderNextBlock (audio_buffer, 0, audio_buffer.getNumSamples());
    metronomeSynth.renderNextBlock (audio_buffer, 0, audio_buffer.getNumSamples());

}
double ChartPerformanceScene::noteDurationSeconds (const ChartEvent& event) const
{
    return static_cast<double> (event.lengthMs) / 1000.0 / clock.getTempoScale();
}

SceneIDs::SceneID ChartPerformanceScene::getDesiredSceneID()
{
    return desiredSceneId;
}
SceneIDs::SceneID ChartPerformanceScene::getSceneID() const
{
    return SceneIDs::CHART_PERFORMANCE_SCENE;
}

void ChartPerformanceScene::buttonClicked (juce::Button* b)
{
    if (b == &startButton)
    {
        startGame();
    }
}

void ChartPerformanceScene::startGame()
{
    startButton.setVisible (false);
    playing = true;
    timeMs = -countIn.lengthMs;
    timingHistogram.reset();
    clock.start (countIn.lengthMs);
    grabKeyboardFocus();

    displayEvents.clear();
    lastNoteEndMs = 0;
    for (auto& event : gameState->currentChart->events)
    {
        if (event.second.type == ChartEvent::NOTE)
            event.second.performanceTimings.emplace_back(UNPLAYED_NOTE);
        if (event.second.type == ChartEvent::NOTE || event.second.type == ChartEvent::BACKGROUND_NOTE)
            lastNoteEndMs = std::max (lastNoteEndMs, event.first + event.second.lengthMs);
        displayEvents.insert ({ event.first, &event.second });
    }
    noteLaneDisplay.setEvents (&displayEvents);

    std::vector<NoteLaneDisplay::BarSpan> barSpans;
    for (auto& bar : BarAtom::splitIntoBars (*gameState->currentChart))
        barSpans.push_back ({ bar.chartStartMs, bar.lengthMs });
    noteLaneDisplay.setBars (std::move (barSpans), 0);
}

void ChartPerformanceScene::update()
{
    auto elapsed = getMillisecondsSinceLastUpdate();

    if (playing)
    {
        timeMs = clock.now();

        if (timeMs >= lastNoteEndMs + resultsDelayChartMs (clock.getTempoScale()))
        {
            desiredSceneId = SceneIDs::CHART_FEEDBACK_SCENE;
        }
    }

    noteLaneDisplay.setPlayheadTimeMs (timeMs);
    noteLaneDisplay.setChartTimeMs (static_cast<double> (timeMs));
    noteLaneDisplay.advance (elapsed);

    std::optional<double> sinceBeat;
    if (playing)
        if (auto click = MetronomeClicks::latestAtOrBefore (countIn.clicks, gameState->currentChart->events, timeMs))
            sinceBeat = static_cast<double> (timeMs - *click) / clock.getTempoScale();
    timingHistogram.setTimeSinceBeatMs (sinceBeat);
}

void ChartPerformanceScene::paint (juce::Graphics& g)
{
    g.fillAll (gameState->colours.playBackground);
}

void ChartPerformanceScene::paintOverChildren (juce::Graphics& g)
{
    if (! synthLoader.isLoaded())
    {
        paintLoadingMessage (g);
        // Lets the loader take the message thread for its next synth, now the scene is on screen.
        synthLoader.framePainted();
    }
}

void ChartPerformanceScene::resized()
{
    grabKeyboardFocus();
    startButton.setBounds (getLocalBounds().withSizeKeepingCentre (200, 40));
    constexpr int controlsWidth = 70;
    constexpr int histogramHeight = 110;
    auto bounds = getLocalBounds().reduced (10, 20);
    playbackControls.setBounds (bounds.removeFromLeft (controlsWidth));
    bounds.removeFromLeft (10);
    timingHistogram.setBounds (bounds.removeFromBottom (histogramHeight));
    bounds.removeFromBottom (10);
    noteLaneDisplay.setBounds (bounds);
}

bool ChartPerformanceScene::keyPressed (const juce::KeyPress& key)
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
