#ifndef UPBEAT_CHARTSYNTHLOADER_H
#define UPBEAT_CHARTSYNTHLOADER_H

#include "ChartSynthBank.h"
#include "juce_events/juce_events.h"
#include <atomic>
#include <functional>
#include <memory>

// Loads a chart's ChartSynthBank on a background thread, since building every Odin synth
// takes a while, and hands it to the audio thread once it's ready.
//
// Construct, prepareToPlay, isLoaded and onLoaded are message-thread only; getSynths is for
// the audio thread.
class ChartSynthLoader : private juce::Thread, private juce::AsyncUpdater
{
public:
    // Starts loading straight away. chart must outlive the loader, and its synths and
    // automation mustn't change while loading.
    explicit ChartSynthLoader (const Chart& chart);
    // Stops loading early if it's still going.
    ~ChartSynthLoader() override;

    // Prepares the synths now if they've loaded, or as soon as they do otherwise.
    void prepareToPlay (double sampleRate, int maximumBlockSize);

    bool isLoaded() const { return loadedSynths != nullptr; }

    // Call each time the screen showing the loading progress is painted. Loading pauses before
    // each synth until it has been, so the screen can be painted while the synths load (see
    // ChartSynthBank's loading-thread constructor); if it's never called, loading still goes
    // ahead, just with a short pause before each synth.
    void framePainted() { notify(); }

    // Called on the message thread once the synths are loaded (and, if prepareToPlay has been
    // called, handed to the audio thread).
    std::function<void()> onLoaded;

    // The loaded synths, or nullptr until they're ready to play. Once set, they stay for the
    // loader's lifetime.
    ChartSynthBank* getSynths() const noexcept { return publishedSynths.load (std::memory_order_acquire); }

private:
    void run() override;
    void handleAsyncUpdate() override;

    const Chart& chart;

    // Written by the loading thread before it triggers the async update, which hands it to
    // the message thread.
    std::unique_ptr<ChartSynthBank> builtSynths;
    // Owned on the message thread once loading has finished.
    std::unique_ptr<ChartSynthBank> loadedSynths;
    std::atomic<ChartSynthBank*> publishedSynths { nullptr };

    bool prepared = false;
    double sampleRate = 44100.0;
    int maximumBlockSize = 512;
};

#endif //UPBEAT_CHARTSYNTHLOADER_H
