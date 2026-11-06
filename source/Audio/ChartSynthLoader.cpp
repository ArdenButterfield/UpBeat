#include "ChartSynthLoader.h"

ChartSynthLoader::ChartSynthLoader (const Chart& chart_) : juce::Thread ("Chart synth loader"), chart (chart_)
{
    startThread();
}

ChartSynthLoader::~ChartSynthLoader()
{
    // The loading thread only blocks the message thread while it holds the MessageManagerLock,
    // so it's either waiting for that lock, which gives up when asked to exit, or between synths.
    stopThread (-1);
    cancelPendingUpdate();
}

void ChartSynthLoader::run()
{
    juce::Thread& loadingThread = *this;
    auto synths = std::make_unique<ChartSynthBank> (chart, loadingThread);
    if (threadShouldExit())
        return;

    builtSynths = std::move (synths);
    triggerAsyncUpdate();
}

void ChartSynthLoader::prepareToPlay (double newSampleRate, int newMaximumBlockSize)
{
    prepared = true;
    sampleRate = newSampleRate;
    maximumBlockSize = newMaximumBlockSize;

    if (loadedSynths != nullptr)
    {
        loadedSynths->prepareToPlay (sampleRate, maximumBlockSize);
        publishedSynths.store (loadedSynths.get(), std::memory_order_release);
    }
}

void ChartSynthLoader::handleAsyncUpdate()
{
    // The loading thread has finished with builtSynths: triggering this update is its last act.
    stopThread (-1);
    loadedSynths = std::move (builtSynths);

    // The audio thread only gets the synths once they're prepared; if prepareToPlay hasn't
    // been called yet, it hands them over itself.
    if (prepared)
    {
        loadedSynths->prepareToPlay (sampleRate, maximumBlockSize);
        publishedSynths.store (loadedSynths.get(), std::memory_order_release);
    }

    if (onLoaded)
        onLoaded();
}
