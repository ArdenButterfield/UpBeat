#include "TimingHistogram.h"

namespace
{
    constexpr int headerHeight = 40;
    constexpr int labelHeight = 16;
    constexpr float sideMargin = 4.f;
    constexpr int markerLabelHeight = 12;

    juce::String formatMs (long long ms)
    {
        return juce::String (ms > 0 ? "+" : "") + juce::String (ms) + " ms";
    }
}

TimingHistogram::TimingHistogram (const GameState& gameState_) : gameState (gameState_)
{
    setInterceptsMouseClicks (false, false);
    bins.assign ((size_t) (2 * rangeMs() / binWidthMs), 0);
}

long long TimingHistogram::rangeMs() const
{
    return gameState.tolerances.back();
}

void TimingHistogram::addTiming (long long errorMs)
{
    bins[(size_t) binForError (errorMs)] += 1;
    count += 1;
    totalErrorMs += errorMs;
    sortedErrors.insert (std::upper_bound (sortedErrors.begin(), sortedErrors.end(), errorMs), errorMs);
    repaint();
}

void TimingHistogram::reset()
{
    std::fill (bins.begin(), bins.end(), 0);
    sortedErrors.clear();
    count = 0;
    totalErrorMs = 0;
    repaint();
}

double TimingHistogram::getMeanErrorMs() const
{
    return count == 0 ? 0.0 : static_cast<double> (totalErrorMs) / count;
}

double TimingHistogram::getPercentileErrorMs (double fraction) const
{
    if (sortedErrors.empty())
        return 0.0;

    auto position = juce::jlimit (0.0, 1.0, fraction) * static_cast<double> (sortedErrors.size() - 1);
    auto below = static_cast<size_t> (position);
    auto above = std::min (below + 1, sortedErrors.size() - 1);
    auto t = position - static_cast<double> (below);
    return static_cast<double> (sortedErrors[below]) * (1.0 - t) + static_cast<double> (sortedErrors[above]) * t;
}

int TimingHistogram::binForError (long long errorMs) const
{
    auto bin = (errorMs + rangeMs()) / binWidthMs;
    // Integer division rounds towards zero, so an error just past the early edge still
    // lands in bin 0; anything further out is clamped there too.
    return (int) juce::jlimit (0LL, (long long) bins.size() - 1, errorMs < -rangeMs() ? 0LL : bin);
}

juce::Rectangle<float> TimingHistogram::getPlotArea() const
{
    auto area = getLocalBounds().toFloat();
    area.removeFromTop ((float) headerHeight + labelHeight);
    area.removeFromBottom ((float) labelHeight);
    return area.reduced (sideMargin, 0.f);
}

float TimingHistogram::yForError (double errorMs) const
{
    auto plot = getPlotArea();
    auto range = static_cast<double> (rangeMs());
    auto clamped = juce::jlimit (-range, range, errorMs);
    return plot.getY() + static_cast<float> ((clamped + range) / (2.0 * range)) * plot.getHeight();
}

void TimingHistogram::drawMarker (juce::Graphics& g, double errorMs, juce::Colour colour) const
{
    auto plot = getPlotArea();
    // Through the middle of a row of pixels, so the dots are crisp.
    auto y = std::floor (yForError (errorMs)) + 0.5f;
    const float dashes[] = { 2.f, 2.f };

    g.setColour (colour);
    g.drawDashedLine ({ plot.getX(), y, plot.getRight(), y }, dashes, 2);
    g.setFont (10.0f);
    g.drawText (formatMs (juce::roundToInt (errorMs)),
        juce::Rectangle<float> (plot.getX(), y - (float) markerLabelHeight, plot.getWidth(), (float) markerLabelHeight),
        juce::Justification::centredRight);
}

int TimingHistogram::categoryForError (double errorMs) const
{
    for (int i = 0; i < GameState::NUM_TOLERANCE_CATEGORIES; ++i)
        if (std::abs (errorMs) < static_cast<double> (gameState.tolerances[(size_t) i]))
            return i;
    return GameState::NUM_TOLERANCE_CATEGORIES - 1;
}

juce::Rectangle<float> TimingHistogram::getBarBounds (int bin) const
{
    auto largest = *std::max_element (bins.begin(), bins.end());
    if (largest == 0 || bins[(size_t) bin] == 0)
        return {};

    auto plot = getPlotArea();
    auto top = yForError ((double) (bin * binWidthMs - rangeMs()));
    auto bottom = yForError ((double) ((bin + 1) * binWidthMs - rangeMs()));
    auto width = plot.getWidth() * static_cast<float> (bins[(size_t) bin]) / static_cast<float> (largest);
    // A pixel's gap between bars, when they're tall enough to spare one.
    auto gap = bottom - top > 3.f ? 1.f : 0.f;
    return { plot.getX(), top, width, bottom - top - gap };
}

void TimingHistogram::paint (juce::Graphics& g)
{
    auto& colours = gameState.colours;
    g.fillAll (colours.histogramBackground);

    auto bounds = getLocalBounds();
    auto header = bounds.removeFromTop (headerHeight);
    g.setColour (colours.text);
    g.setFont (14.0f);
    g.drawText (juce::String (count) + (count == 1 ? " hit" : " hits"), header.removeFromTop (headerHeight / 2), juce::Justification::centred);
    auto mean = juce::roundToInt (getMeanErrorMs());
    g.drawText (count == 0 ? juce::String ("mean -") : "mean " + formatMs (mean),
        header,
        juce::Justification::centred);

    g.setFont (12.0f);
    g.drawText ("Early", bounds.removeFromTop (labelHeight), juce::Justification::centred);
    g.drawText ("Late", bounds.removeFromBottom (labelHeight), juce::Justification::centred);

    auto plot = getPlotArea();

    // The tolerance categories' edges, either side of a perfectly timed hit.
    g.setColour (colours.histogramAxis.withAlpha (0.2f));
    for (int i = 0; i < GameState::NUM_TOLERANCE_CATEGORIES - 1; ++i)
        for (auto sign : { -1.0, 1.0 })
            g.drawHorizontalLine (juce::roundToInt (yForError (sign * (double) gameState.tolerances[(size_t) i])), plot.getX(), plot.getRight());

    for (int bin = 0; bin < getNumBins(); ++bin)
    {
        auto bar = getBarBounds (bin);
        if (bar.isEmpty())
            continue;
        auto centre = (bin + 0.5) * binWidthMs - (double) rangeMs();
        g.setColour (colours.histogramBars[(size_t) categoryForError (centre)]);
        g.fillRect (bar);
    }

    g.setColour (colours.histogramAxis);
    g.drawHorizontalLine (juce::roundToInt (yForError (0.0)), plot.getX(), plot.getRight());

    if (count > 0)
    {
        drawMarker (g, getPercentileErrorMs (0.25), colours.histogramQuartile);
        drawMarker (g, getPercentileErrorMs (0.75), colours.histogramQuartile);
        drawMarker (g, getPercentileErrorMs (0.5), colours.histogramMedian);
    }
}
