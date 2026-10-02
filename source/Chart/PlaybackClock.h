//
// Created by Arden on 10/2/2026.
//

#ifndef UPBEAT_PLAYBACKCLOCK_H
#define UPBEAT_PLAYBACKCLOCK_H

#include "juce_core/juce_core.h"
#include <atomic>

// Maps wall-clock time to chart time for a playing chart, allowing the tempo scale to
// change mid-playback without the playhead jumping: each tempo change re-anchors the
// clock at the current chart time, so only time elapsed after the change is scaled by
// the new tempo.
//
// start(), now() and setTempoScale() are message-thread only. getTempoScale() is safe
// to call from the audio thread, which advances its own chart-time accumulator per block.
//
// Neither clock may round as it goes: now() recomputes from the anchor each call, and the
// audio accumulator must stay a double, since accumulating rounded per-block/per-frame
// deltas loses a fraction of a millisecond each time and makes the two clocks drift apart.
class PlaybackClock
{
public:
    void start (long long countInTime)
    {
        anchorChartMs = static_cast<double> (-countInTime);
        anchorWallMs = juce::Time::currentTimeMillis();
    }

    long long now() const
    {
        auto elapsedWallMs = juce::Time::currentTimeMillis() - anchorWallMs;
        return static_cast<long long> (anchorChartMs + static_cast<double> (elapsedWallMs) * tempoScale.load());
    }

    void setTempoScale (double newTempoScale)
    {
        auto nowWallMs = juce::Time::currentTimeMillis();
        anchorChartMs += static_cast<double> (nowWallMs - anchorWallMs) * tempoScale.load();
        anchorWallMs = nowWallMs;
        tempoScale.store (newTempoScale);
    }

    double getTempoScale() const { return tempoScale.load(); }

private:
    std::atomic<double> tempoScale { 1.0 };
    double anchorChartMs = 0.0;
    long long anchorWallMs = 0;
};

#endif //UPBEAT_PLAYBACKCLOCK_H
