//
// Created by Arden on 7/8/2026.
//

#include "ChartPerformanceScene.h"

ChartPerformanceScene::ChartPerformanceScene(GameState* gs) : Scene(gs), startButton("Start"), noteLaneDisplay (gs, gs->currentChart->numLanes), playing(false), desiredSceneId(SceneIDs::CHART_PERFORMANCE_SCENE)
{
    addAndMakeVisible (startButton);
    startButton.addListener (this);

    tempoScaleSlider.setRange (0.25, 2.0);
    tempoScaleSlider.setValue (gameState->currentChart->tempoScale, juce::dontSendNotification);
    tempoScaleSlider.setTextValueSuffix ("x tempo");
    tempoScaleSlider.onValueChange = [this] { gameState->currentChart->tempoScale = tempoScaleSlider.getValue(); };
    addAndMakeVisible (tempoScaleSlider);
    tempoScaleLabel.setText ("Tempo scale", juce::dontSendNotification);
    tempoScaleLabel.attachToComponent (&tempoScaleSlider, true);
    addAndMakeVisible (tempoScaleLabel);

    noteVelocitySlider.setRange (0.05, 0.5);
    noteVelocitySlider.setValue (gameState->currentChart->noteOnScreenVelocity, juce::dontSendNotification);
    noteVelocitySlider.onValueChange = [this]
    {
        gameState->currentChart->noteOnScreenVelocity = noteVelocitySlider.getValue();
        noteLaneDisplay.setNoteOnScreenVelocity (noteVelocitySlider.getValue());
    };
    addAndMakeVisible (noteVelocitySlider);
    noteVelocityLabel.setText ("Note velocity", juce::dontSendNotification);
    noteVelocityLabel.attachToComponent (&noteVelocitySlider, true);
    addAndMakeVisible (noteVelocityLabel);

    noteLaneDisplay.setNoteOnScreenVelocity (gameState->currentChart->noteOnScreenVelocity);
    addAndMakeVisible (noteLaneDisplay);

    setWantsKeyboardFocus (true);
    elapsedSamples = 0;
}

ChartPerformanceScene::~ChartPerformanceScene()
{
}
void ChartPerformanceScene::prepareToPlay (double _sampleRate, int samplesPerBlock)
{
    sampleRate = _sampleRate;
    juce::ignoreUnused (samplesPerBlock);
    synth.prepareToPlay (sampleRate);
    backgroundSynth.prepareToPlay (sampleRate);
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
            auto e = playbackQueue.back();
            playbackQueue.pop();
            auto midiNote = e->midiNote;
            synth.noteOn (midiNote);
        }
    }

    if (playing)
    {
        auto* chart = gameState->currentChart;
        auto bufferStartRealMs = static_cast<long long>(elapsedSamples * 1000.0 / sampleRate);
        auto bufferEndRealMs = static_cast<long long>((elapsedSamples + audio_buffer.getNumSamples()) * 1000.0 / sampleRate);
        auto bufferStartTime = Chart::chartTimeForRealElapsedMs (bufferStartRealMs, chart->tempoScale, chart->countInTime);
        auto bufferEndTime = Chart::chartTimeForRealElapsedMs (bufferEndRealMs, chart->tempoScale, chart->countInTime);

        for (auto event = gameState->currentChart->events.lower_bound (bufferStartTime); event != gameState->currentChart->events.end() && event->first < bufferEndTime; ++event)
        {
            if (event->second.type == ChartEvent::NOTE)
            {
                std::cout << "background note at note " << event->second.midiNote << "time " << bufferStartTime << std::endl;
                backgroundSynth.noteOn (event->second.midiNote);
            } else if (event->second.type == ChartEvent::BARLINE)
            {
                metronomeSynth.noteOn (MetronomeSynth::BARLINE);
            } else if (event->second.type == ChartEvent::BEAT)
            {
                metronomeSynth.noteOn (MetronomeSynth::BEAT);
            }
        }
        elapsedSamples += audio_buffer.getNumSamples();
    } else
    {
        elapsedSamples = 0;
    }

    synth.renderNextBlock (audio_buffer, 0, audio_buffer.getNumSamples());
    backgroundSynth.renderNextBlock (audio_buffer, 0, audio_buffer.getNumSamples());
    metronomeSynth.renderNextBlock (audio_buffer, 0, audio_buffer.getNumSamples());

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
    tempoScaleSlider.setVisible (false);
    tempoScaleLabel.setVisible (false);
    noteVelocitySlider.setVisible (false);
    noteVelocityLabel.setVisible (false);
    playing = true;
    timeMs = -gameState->currentChart->countInTime;
    gameStartTime = juce::Time::currentTimeMillis();
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
}

void ChartPerformanceScene::update()
{
    auto elapsed = getMillisecondsSinceLastUpdate();

    if (playing)
    {
        auto* chart = gameState->currentChart;
        timeMs = Chart::chartTimeForRealElapsedMs (juce::Time::currentTimeMillis() - gameStartTime, chart->tempoScale, chart->countInTime);

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
    tempoScaleSlider.setBounds (getLocalBounds().withSizeKeepingCentre (200, 20).withY (startButton.getBottom() + 30));
    noteVelocitySlider.setBounds (getLocalBounds().withSizeKeepingCentre (200, 20).withY (tempoScaleSlider.getBottom() + 20));
    auto laneOutline = getLocalBounds().withWidth (std::min(300, getWidth() - 40)).withTrimmedTop (20).withTrimmedBottom (20).withCentre ({getWidth() / 2, getHeight() / 2});
    noteLaneDisplay.setBounds (laneOutline);
}

bool ChartPerformanceScene::keyPressed (const juce::KeyPress& key)
{
    auto* chart = gameState->currentChart;
    auto hitTime = Chart::chartTimeForRealElapsedMs (juce::Time::currentTimeMillis() - gameStartTime, chart->tempoScale, chart->countInTime);
    auto* closestEvent = noteLaneDisplay.registerKeyPress (key, hitTime);
    if (closestEvent != nullptr)
    {
        auto scopedLock = juce::ScopedLock (playbackLock);
        playbackQueue.push (closestEvent);
    }
    return true;
}
