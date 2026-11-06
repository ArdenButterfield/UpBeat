#ifndef UPBEAT_TIMINGHISTOGRAM_H
#define UPBEAT_TIMINGHISTOGRAM_H

#include "../GameState.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include <vector>

// A vertical histogram of how early or late the player's hits were, shown beside the lanes
// and updated as each note is hit. Timing error runs down the component, from the earliest
// a hit can be at the top to the latest at the bottom, and each bin's bar grows to the right
// with its count, in the colour of the tolerance category it falls in.
class TimingHistogram : public juce::Component
{
public:
    static constexpr int binWidthMs = 5;

    explicit TimingHistogram (const GameState& gameState);

    // Records a hit's timing error: negative for early, positive for late. Errors beyond the
    // hit window count towards the bin at its edge.
    void addTiming (long long errorMs);
    void reset();

    int getCount() const { return count; }
    // The average error of every hit so far, or 0 before the first.
    double getMeanErrorMs() const;
    // The error a given fraction of the way through every hit so far, ordered from earliest to
    // latest, interpolating between neighbouring hits: 0.5 is the median, 0.25 and 0.75 the
    // quartiles. 0 before the first hit.
    double getPercentileErrorMs (double fraction) const;
    // The height in the plot that an error is drawn at, clamped to the hit window.
    float yForError (double errorMs) const;

    int getNumBins() const { return (int) bins.size(); }
    int getBinCount (int bin) const { return bins[(size_t) bin]; }
    int binForError (long long errorMs) const;
    // Where a bin's bar is drawn: empty while the bin has no hits.
    juce::Rectangle<float> getBarBounds (int bin) const;
    // Where the bars are drawn, between the header and the "Late" label.
    juce::Rectangle<float> getPlotArea() const;

    void paint (juce::Graphics& g) override;

private:
    // The largest error a hit can have: the widest tolerance.
    long long rangeMs() const;
    int categoryForError (double errorMs) const;
    // A dotted line across the plot at an error, labelled with it just above, at the right.
    void drawMarker (juce::Graphics& g, double errorMs, juce::Colour colour) const;

    const GameState& gameState;
    std::vector<int> bins;
    // Every hit's error, kept sorted for the median and quartiles.
    std::vector<long long> sortedErrors;
    int count = 0;
    long long totalErrorMs = 0;
};

#endif //UPBEAT_TIMINGHISTOGRAM_H
