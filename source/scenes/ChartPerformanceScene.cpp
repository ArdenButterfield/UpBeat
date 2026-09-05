//
// Created by Arden on 7/8/2026.
//

#include "ChartPerformanceScene.h"

namespace
{
    // Central row of the keyboard: lane 0 maps to 'a', lane 1 to 's', and so on.
    constexpr std::array<int, 10> laneKeyCodes = { 65, 83, 68, 70, 71, 72, 74, 75, 76, 59 };
}

ChartPerformanceScene::ChartPerformanceScene(GameState* gs) : Scene(gs), startButton("Start"), playing(false), desiredSceneId(SceneIDs::CHART_PERFORMANCE_SCENE)
{
    auto numLanes = gameState->currentChart->numLanes;
    lanes.resize ((size_t) numLanes);
    buttonIndicators.resize ((size_t) numLanes);
    indicatorLighting.assign ((size_t) numLanes, 0.f);
    keys.resize ((size_t) numLanes);
    for (int i = 0; i < numLanes; ++i)
        keys[(size_t) i] = (i < (int) laneKeyCodes.size()) ? laneKeyCodes[(size_t) i] : -1;

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
    noteVelocitySlider.onValueChange = [this] { gameState->currentChart->noteOnScreenVelocity = noteVelocitySlider.getValue(); };
    addAndMakeVisible (noteVelocitySlider);
    noteVelocityLabel.setText ("Note velocity", juce::dontSendNotification);
    noteVelocityLabel.attachToComponent (&noteVelocitySlider, true);
    addAndMakeVisible (noteVelocityLabel);

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
    playbackIterator = gameState->currentChart->events.lower_bound (timeMs);

    lastNoteTimeMs = 0;
    for (auto& event : gameState->currentChart->events)
    {
        if (event.second.type == ChartEvent::NOTE)
        {
            event.second.performanceTimings.emplace_back(UNPLAYED_NOTE);
            lastNoteTimeMs = std::max (lastNoteTimeMs, event.first);
        }
    }
}

void ChartPerformanceScene::update()
{
    auto elapsed = getMillisecondsSinceLastUpdate();

    if (playing)
    {
        auto* chart = gameState->currentChart;
        timeMs = Chart::chartTimeForRealElapsedMs (juce::Time::currentTimeMillis() - gameStartTime, chart->tempoScale, chart->countInTime);
        for (auto& indicator : indicatorLighting)
        {
            indicator = std::max(0.f, indicator - elapsed * 0.001f);
        }

        if (timeMs > lastNoteTimeMs + 500)
        {
            desiredSceneId = SceneIDs::CHART_FEEDBACK_SCENE;
        }
    }

    for (int i = toleranceLabels.size() - 1; i >= 0; --i)
    {
        if (toleranceLabels[i]->advance (static_cast<double> (elapsed)))
        {
            toleranceLabels.remove (i);
        }
    }

    for (size_t i = 0; i < keys.size(); ++i)
    {
        if (juce::KeyPress::isKeyCurrentlyDown(keys[i]))
        {
            indicatorLighting[i] = 1;
        } else if (indicatorLighting[i] > 0.5f)
        {
            indicatorLighting[i] = 0.5f;
        }
    }
}

void ChartPerformanceScene::paint (juce::Graphics& g)
{
    g.fillAll(juce::Colours::darkgrey);
    g.setColour (juce::Colours::lightgrey);
    g.drawRect (laneOutline);
    for (auto& lane : lanes)
    {
        g.drawRect (lane);
    }

    for (size_t i = 0; i < buttonIndicators.size(); ++i)
    {
        g.setColour (juce::Colours::white.withAlpha (indicatorLighting[i]));
        g.fillRect (buttonIndicators[i]);
    }

    for (auto& event : gameState->currentChart->events)
    {
        auto eventYPosition = static_cast<long>((timeMs - event.first) * gameState->currentChart->noteOnScreenVelocity) + lanes[0].getBottom();

        if (eventYPosition > lanes[0].getBottom() || eventYPosition < lanes[0].getY())
        {
            continue;
        }

        if (event.second.type == ChartEvent::NOTE)
        {
            g.setColour (juce::Colours::pink);
            g.drawRect (lanes[event.second.inputButton].withY (eventYPosition - 3L).withHeight (6));
        } else if (event.second.type == ChartEvent::BEAT)
        {
            g.setColour(juce::Colours::grey);
            g.drawHorizontalLine (eventYPosition, lanes[0].getX(), lanes.back().getRight());
        } else if (event.second.type == ChartEvent::BARLINE)
        {
            g.setColour (juce::Colours::white);
            g.drawHorizontalLine (eventYPosition, lanes[0].getX(), lanes.back().getRight());
        }
    }
}

void ChartPerformanceScene::resized()
{
    grabKeyboardFocus();
    startButton.setBounds (getLocalBounds().withSizeKeepingCentre (200, 40));
    tempoScaleSlider.setBounds (getLocalBounds().withSizeKeepingCentre (200, 20).withY (startButton.getBottom() + 30));
    noteVelocitySlider.setBounds (getLocalBounds().withSizeKeepingCentre (200, 20).withY (tempoScaleSlider.getBottom() + 20));
    laneOutline = getLocalBounds().withWidth (std::min(300, getWidth() - 40)).withTrimmedTop (20).withTrimmedBottom (20).withCentre ({getWidth() / 2, getHeight() / 2});
    auto lanesInner = laneOutline.reduced(5).withTrimmedBottom (30);
    auto indicatorsInner = laneOutline.reduced(5).withTop (lanesInner.getBottom() + 5);
    auto laneW = lanes.empty() ? lanesInner.getWidth() : lanesInner.getWidth() / (int) lanes.size();
    for (size_t i = 0; i < lanes.size(); ++i)
    {
        lanes[i] = lanesInner.withWidth (laneW).withX(lanesInner.getX() + (int) i * laneW);
        buttonIndicators[i] = indicatorsInner.withWidth (laneW).withX(lanesInner.getX() + (int) i * laneW);
    }
}

bool ChartPerformanceScene::keyPressed (const juce::KeyPress& key)
{
    auto* chart = gameState->currentChart;
    auto hitTime = Chart::chartTimeForRealElapsedMs (juce::Time::currentTimeMillis() - gameStartTime, chart->tempoScale, chart->countInTime);
    for (size_t i = 0; i < keys.size(); ++i)
    {
        if (keys[i] == key.getKeyCode())
        {
            std::cout << "hit at time " << hitTime << " lane " << i << std::endl;
            indicatorLighting[i] = 1;
            auto closestEvent = findClosestNoteForHit ((int) i, hitTime);
            if (closestEvent != nullptr)
            {
                closestEvent->performanceTimings.back() = hitTime - closestEvent->timeMs;
                auto scopedLock = juce::ScopedLock (playbackLock);
                playbackQueue.push (closestEvent);
            }
        }
    }
    return true;
}

ChartEvent* ChartPerformanceScene::findClosestNoteForHit (int lane, long long time)
{
    std::cout << "searching at lane " << lane << ", time " << time << std::endl;
    auto totalHitWindow = gameState->tolerances[GameState::NUM_TOLERANCE_CATEGORIES-1];
    auto closeness = totalHitWindow+1;
    ChartEvent* closestNote = nullptr;
    for (auto it = gameState->currentChart->events.lower_bound (time - totalHitWindow);
        it != gameState->currentChart->events.end() && it->first < time + totalHitWindow;
        ++it)
    {
        if (it->second.type == ChartEvent::NOTE
            && it->second.inputButton == lane
            && abs(it->first - time) < closeness
            && it->second.performanceTimings.back() == UNPLAYED_NOTE)
        {
            closeness = abs(it->first - time);
            closestNote = &(it->second);
        }
    }
    auto message = gameState->getMessage (closeness);
    std::cout << *message << std::endl;

    constexpr int toleranceLabelHeight = 20;
    auto laneBounds = lanes[lane];
    auto* label = toleranceLabels.add (new ToleranceLabel (*message));
    label->setBounds (laneBounds.getX(), laneBounds.getBottom() - toleranceLabelHeight, laneBounds.getWidth(), toleranceLabelHeight);
    addAndMakeVisible (label);

    return closestNote;
}
