//
// Created by Arden on 7/8/2026.
//

#include "ChartPerformanceScene.h"

#include "../Chart/BarAtom.h"

ChartPerformanceScene::ChartPerformanceScene(GameState* gs) : Scene(gs), startButton("Start"), playbackControls (gs->currentChart->tempoScale), synths (*gs->currentChart), noteLaneDisplay (gs, gs->currentChart->numLanes), playing(false), desiredSceneId(SceneIDs::CHART_PERFORMANCE_SCENE)
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

    setWantsKeyboardFocus (true);
    audioChartTimeMs = static_cast<double> (-gameState->currentChart->countInTime);
}

ChartPerformanceScene::~ChartPerformanceScene()
{
}
void ChartPerformanceScene::prepareToPlay (double _sampleRate, int samplesPerBlock)
{
    sampleRate = _sampleRate;
    synths.prepareToPlay (sampleRate, samplesPerBlock);
    metronomeSynth.prepareToPlay (sampleRate);
}

void ChartPerformanceScene::processBlock (juce::AudioBuffer<float>& audio_buffer, juce::MidiBuffer& midi_message_metadatas)
{
    juce::ignoreUnused (midi_message_metadatas);
    auto lock = juce::ScopedTryLock(playbackLock);
    if (lock.isLocked())
    {
        while (!playbackQueue.empty())
        {
            auto e = playbackQueue.front();
            playbackQueue.pop();
            synths.noteOn (e->synthId, e->midiNote, e->velocity, noteDurationSeconds (*e));
        }
    }

    synths.applyAutomation (audioChartTimeMs);

    if (playing)
    {
        auto bufferChartMs = audio_buffer.getNumSamples() * 1000.0 / sampleRate * clock.getTempoScale();
        auto bufferStartTime = static_cast<long long> (std::floor (audioChartTimeMs));
        auto bufferEndTime = static_cast<long long> (std::floor (audioChartTimeMs + bufferChartMs));

        for (auto event = gameState->currentChart->events.lower_bound (bufferStartTime); event != gameState->currentChart->events.end() && event->first < bufferEndTime; ++event)
        {
            if (event->second.type == ChartEvent::BACKGROUND_NOTE)
            {
                synths.noteOn (event->second.synthId, event->second.midiNote, event->second.velocity, noteDurationSeconds (event->second));
            } else if (event->second.type == ChartEvent::BARLINE)
            {
                metronomeSynth.noteOn (MetronomeSynth::BARLINE);
            } else if (event->second.type == ChartEvent::BEAT)
            {
                metronomeSynth.noteOn (MetronomeSynth::BEAT);
            }
        }
        audioChartTimeMs += bufferChartMs;
    } else
    {
        audioChartTimeMs = static_cast<double> (-gameState->currentChart->countInTime);
    }

    synths.renderNextBlock (audio_buffer, 0, audio_buffer.getNumSamples());
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
    timeMs = -gameState->currentChart->countInTime;
    clock.start (gameState->currentChart->countInTime);
    grabKeyboardFocus();

    displayEvents.clear();
    lastNoteTimeMs = 0;
    for (auto& event : gameState->currentChart->events)
    {
        if (event.second.type == ChartEvent::NOTE)
        {
            event.second.performanceTimings.emplace_back(UNPLAYED_NOTE);
            lastNoteTimeMs = std::max (lastNoteTimeMs, event.first);
        }
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

        if (timeMs > lastNoteTimeMs + 500)
        {
            desiredSceneId = SceneIDs::CHART_FEEDBACK_SCENE;
        }
    }

    noteLaneDisplay.setPlayheadTimeMs (timeMs);
    noteLaneDisplay.advance (elapsed);
}

void ChartPerformanceScene::paint (juce::Graphics& g)
{
    g.fillAll(juce::Colours::darkgrey);
}

void ChartPerformanceScene::resized()
{
    grabKeyboardFocus();
    startButton.setBounds (getLocalBounds().withSizeKeepingCentre (200, 40));
    constexpr int controlsWidth = 70;
    auto bounds = getLocalBounds().reduced (10, 20);
    playbackControls.setBounds (bounds.removeFromLeft (controlsWidth));
    bounds.removeFromLeft (10);
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
