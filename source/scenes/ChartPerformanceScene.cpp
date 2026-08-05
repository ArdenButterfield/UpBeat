//
// Created by Arden on 7/8/2026.
//

#include "ChartPerformanceScene.h"

ChartPerformanceScene::ChartPerformanceScene(GameState* gs) : Scene(gs), startButton("Start"), playing(false)
{
    addAndMakeVisible (startButton);
    startButton.addListener (this);
    indicatorLighting.fill(0);
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
        auto bufferStartTime = elapsedSamples * 1000 / static_cast<long long>(sampleRate) - gameState->currentChart->countInTime;
        auto bufferEndTime = (elapsedSamples + audio_buffer.getNumSamples()) * 1000 / static_cast<long long>(sampleRate) - gameState->currentChart->countInTime;

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
    return SceneIDs::CHART_PERFORMANCE_SCENE;
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
    gameStartTime = juce::Time::currentTimeMillis() + gameState->currentChart->countInTime;
    grabKeyboardFocus();
    playbackIterator = gameState->currentChart->events.lower_bound (timeMs);

    for (auto& event : gameState->currentChart->events)
    {
        if (event.second.type == ChartEvent::NOTE)
        {
            event.second.performanceTimings.emplace_back(UNPLAYED_NOTE);
        }
    }
}

void ChartPerformanceScene::update()
{
    auto elapsed = getMillisecondsSinceLastUpdate();

    if (playing)
    {
        timeMs += elapsed;
        for (auto& indicator : indicatorLighting)
        {
            indicator = std::max(0.f, indicator - elapsed * 0.001f);
        }
    }

    for (int i = toleranceLabels.size() - 1; i >= 0; --i)
    {
        if (toleranceLabels[i]->advance (static_cast<double> (elapsed)))
        {
            toleranceLabels.remove (i);
        }
    }

    for (int i = 0; i < GameState::numberOfInputLanes; ++i)
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

    for (int i = 0; i < GameState::numberOfInputLanes; ++i)
    {
        g.setColour (juce::Colours::white.withAlpha (indicatorLighting[i]));
        g.fillRect (buttonIndicators[i]);
    }

    for (auto& event : gameState->currentChart->events)
    {
        auto eventYPosition = static_cast<long>((timeMs - event.first) * pixelsPerMillisecond) + lanes[0].getBottom();

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
    laneOutline = getLocalBounds().withWidth (std::min(300, getWidth() - 40)).withTrimmedTop (20).withTrimmedBottom (20).withCentre ({getWidth() / 2, getHeight() / 2});
    auto lanesInner = laneOutline.reduced(5).withTrimmedBottom (30);
    auto indicatorsInner = laneOutline.reduced(5).withTop (lanesInner.getBottom() + 5);
    auto laneW = lanesInner.getWidth() / 4;
    for (int i = 0; i < lanes.size(); ++i)
    {
        lanes[i] = lanesInner.withWidth (laneW).withX(lanesInner.getX() + i * laneW);
        buttonIndicators[i] = indicatorsInner.withWidth (laneW).withX(lanesInner.getX() + i * laneW);
    }
}

bool ChartPerformanceScene::keyPressed (const juce::KeyPress& key)
{
    auto hitTime = juce::Time::currentTimeMillis() - gameStartTime;
    for (int i = 0; i < GameState::numberOfInputLanes; ++i)
    {
        if (keys[i] == key.getKeyCode())
        {
            std::cout << "hit at time " << hitTime << " lane " << i << std::endl;
            indicatorLighting[i] = 1;
            auto closestEvent = findClosestNoteForHit (i, hitTime);
            if (closestEvent != nullptr)
            {
                closestEvent->performanceTimings.back() = hitTime;
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
