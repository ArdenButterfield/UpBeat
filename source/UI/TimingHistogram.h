#ifndef UPBEAT_TIMINGHISTOGRAM_H
#define UPBEAT_TIMINGHISTOGRAM_H

#include "../GameState.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include <optional>
#include <vector>

// A horizontal histogram of how early or late the player's hits were, shown under the lanes
// and updated as each note is hit. Timing error runs across the component, from the earliest
// a hit can be at the left to the latest at the right, and each bin's bar grows upwards with
// its count, in the colour of the tolerance category it falls in. The hit count and mean error
// sit in a strip down the left-hand side. A smoothed curve of the counts runs over the bars,
// coloured like them and filled beneath at half opacity, and its height pulses with each
// metronome click.
class TimingHistogram : public juce::Component
{
public:
    static constexpr int binWidthMs = 5;

    explicit TimingHistogram (const GameState& gameState);

    // Records a hit's timing error: negative for early, positive for late. Errors beyond the
    // hit window count towards the bin at its edge.
    void addTiming (long long errorMs);
    void reset();

    // How long ago, in real milliseconds, the metronome last clicked, or nullopt when it isn't
    // playing. The curve swells on each click and settles back as the click fades.
    void setTimeSinceBeatMs (std::optional<double> realMs);
    // How far the curve is swollen by the latest click: 1 on the click, falling towards 0.
    float getBeatPulse() const { return beatPulse; }
    // The smoothed count at an error: each bin's count spread across its neighbours.
    double smoothedCountAt (double errorMs) const;

    int getCount() const { return count; }
    // The average error of every hit so far, or 0 before the first.
    double getMeanErrorMs() const;
    // The error a given fraction of the way through every hit so far, ordered from earliest to
    // latest, interpolating between neighbouring hits: 0.5 is the median, 0.25 and 0.75 the
    // quartiles. 0 before the first hit.
    double getPercentileErrorMs (double fraction) const;
    // Where across the plot an error is drawn, clamped to the hit window.
    float xForError (double errorMs) const;

    int getNumBins() const { return (int) bins.size(); }
    int getBinCount (int bin) const { return bins[(size_t) bin]; }
    int binForError (long long errorMs) const;
    // Where a bin's bar is drawn: empty while the bin has no hits.
    juce::Rectangle<float> getBarBounds (int bin) const;
    // Where the bars are drawn: right of the stats, between the marker labels above and the
    // "Early"/"Late" labels below.
    juce::Rectangle<float> getPlotArea() const;

    void paint (juce::Graphics& g) override;

private:
    // The largest error a hit can have: the widest tolerance.
    long long rangeMs() const;
    int categoryForError (double errorMs) const;
    // The smoothed curve across the plot, scaled like the bars and swollen by the beat pulse,
    // closed along the bottom of the plot when filled.
    juce::Path curvePath (bool closed) const;
    void drawCurve (juce::Graphics& g, bool fill) const;
    // A dotted line up the plot at an error, labelled with it above the plot, the text ending at
    // the line. labelRow 0 is the top row of labels, 1 the row just above the plot.
    void drawMarker (juce::Graphics& g, double errorMs, juce::Colour colour, int labelRow) const;

    const GameState& gameState;
    std::vector<int> bins;
    // Every hit's error, kept sorted for the median and quartiles.
    std::vector<long long> sortedErrors;
    int count = 0;
    long long totalErrorMs = 0;
    float beatPulse = 0.f;
};

#endif //UPBEAT_TIMINGHISTOGRAM_H
