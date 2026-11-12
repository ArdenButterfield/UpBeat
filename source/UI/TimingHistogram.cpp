#include "TimingHistogram.h"
#include "GuiStyle.h"

namespace
{
    constexpr int statsWidth = 120;
    constexpr int labelHeight = 18; // The "Early" and "Late" labels under the plot
    constexpr float sideMargin = 4.f;
    // Two rows of marker labels above the plot: the median's on top, the quartiles' below,
    // so the median's label doesn't collide with the quartiles' either side of it.
    constexpr int markerLabelHeight = 16;
    constexpr int numMarkerLabelRows = 2;
    constexpr float markerLabelWidth = 80.f;
    constexpr int statsLineHeight = 26;

    // The smoothing curve spreads each bin's count over a bell curve this wide (its standard
    // deviation), and is sampled this often across the plot.
    constexpr double smoothingWidthMs = 2.5 * TimingHistogram::binWidthMs;
    constexpr double curveStepMs = 1.0;
    // On a click the curve is this much taller, settling back over roughly this long.
    constexpr float beatPulseDepth = 0.3f;
    constexpr double beatPulseDecayMs = 150.0;

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

void TimingHistogram::setTimeSinceBeatMs (std::optional<double> realMs)
{
    auto pulse = realMs ? static_cast<float> (std::exp (-std::max (0.0, *realMs) / beatPulseDecayMs)) : 0.f;
    if (pulse == beatPulse)
        return;

    beatPulse = pulse;
    if (count > 0)
        repaint();
}

double TimingHistogram::smoothedCountAt (double errorMs) const
{
    auto total = 0.0;
    for (size_t bin = 0; bin < bins.size(); ++bin)
    {
        if (bins[bin] == 0)
            continue;
        auto centre = ((double) bin + 0.5) * binWidthMs - (double) rangeMs();
        auto distance = (errorMs - centre) / smoothingWidthMs;
        total += bins[bin] * std::exp (-0.5 * distance * distance);
    }
    // Scaled so a bin's spread-out hits add back up to its count across the bins they cover.
    return total * binWidthMs / (smoothingWidthMs * std::sqrt (2.0 * juce::MathConstants<double>::pi));
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
    area.removeFromLeft ((float) statsWidth);
    area.removeFromTop ((float) (markerLabelHeight * numMarkerLabelRows));
    area.removeFromBottom ((float) labelHeight);
    return area.reduced (sideMargin, 0.f);
}

float TimingHistogram::xForError (double errorMs) const
{
    auto plot = getPlotArea();
    auto range = static_cast<double> (rangeMs());
    auto clamped = juce::jlimit (-range, range, errorMs);
    return plot.getX() + static_cast<float> ((clamped + range) / (2.0 * range)) * plot.getWidth();
}

void TimingHistogram::drawMarker (juce::Graphics& g, double errorMs, juce::Colour colour, int labelRow) const
{
    auto plot = getPlotArea();
    // Through the middle of a column of pixels, so the dots are crisp.
    auto x = std::floor (xForError (errorMs)) + 0.5f;
    auto labelBottom = plot.getY() - (float) (markerLabelHeight * (numMarkerLabelRows - 1 - labelRow));
    const float dashes[] = { 2.f, 2.f };

    g.setColour (colour);
    g.drawDashedLine ({ x, labelBottom, x, plot.getBottom() }, dashes, 2, (float) GuiStyle::thinLine);
    g.setFont (GuiStyle::fontSize (10.0f));
    g.drawText (formatMs (juce::roundToInt (errorMs)),
        juce::Rectangle<float> (x - markerLabelWidth, labelBottom - (float) markerLabelHeight, markerLabelWidth - 2.f, (float) markerLabelHeight),
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
    auto left = xForError ((double) (bin * binWidthMs - rangeMs()));
    auto right = xForError ((double) ((bin + 1) * binWidthMs - rangeMs()));
    auto height = plot.getHeight() * static_cast<float> (bins[(size_t) bin]) / static_cast<float> (largest);
    // A pixel's gap between bars, when they're wide enough to spare one.
    auto gap = right - left > 3.f ? 1.f : 0.f;
    return { left, plot.getBottom() - height, right - left - gap, height };
}

juce::Path TimingHistogram::curvePath (bool closed) const
{
    juce::Path path;
    auto largest = *std::max_element (bins.begin(), bins.end());
    if (largest == 0)
        return path;

    auto plot = getPlotArea();
    auto scale = plot.getHeight() * (1.f + beatPulseDepth * beatPulse) / static_cast<float> (largest);
    auto range = static_cast<double> (rangeMs());

    // (Path::isEmpty can't tell whether the curve has started: it stays true until a line is drawn.)
    auto started = closed;
    if (closed)
        path.startNewSubPath (xForError (-range), plot.getBottom());
    for (auto error = -range; error <= range; error += curveStepMs)
    {
        juce::Point<float> point (xForError (error), plot.getBottom() - static_cast<float> (smoothedCountAt (error)) * scale);
        if (started)
            path.lineTo (point);
        else
            path.startNewSubPath (point);
        started = true;
    }
    if (closed)
    {
        path.lineTo (xForError (range), plot.getBottom());
        path.closeSubPath();
    }
    return path;
}

void TimingHistogram::drawCurve (juce::Graphics& g, bool fill) const
{
    auto path = curvePath (fill);
    if (path.isEmpty())
        return;

    // Each stretch of the curve takes the colour of the tolerance category beneath it. The
    // categories' edges, from the earliest error to the latest.
    std::vector<double> edges { -(double) rangeMs() };
    for (int i = GameState::NUM_TOLERANCE_CATEGORIES - 2; i >= 0; --i)
        edges.push_back (-(double) gameState.tolerances[(size_t) i]);
    for (int i = 0; i < GameState::NUM_TOLERANCE_CATEGORIES - 1; ++i)
        edges.push_back ((double) gameState.tolerances[(size_t) i]);
    edges.push_back ((double) rangeMs());

    auto bounds = getLocalBounds();
    for (size_t i = 0; i + 1 < edges.size(); ++i)
    {
        auto left = i == 0 ? bounds.getX() : juce::roundToInt (xForError (edges[i]));
        auto right = i + 2 == edges.size() ? bounds.getRight() : juce::roundToInt (xForError (edges[i + 1]));
        auto colour = gameState.colours.histogramBars[(size_t) categoryForError ((edges[i] + edges[i + 1]) / 2.0)];

        juce::Graphics::ScopedSaveState state (g);
        // Whole-pixel edges, so neighbouring stretches meet without overlapping.
        g.reduceClipRegion (bounds.withLeft (left).withRight (right));
        if (fill)
        {
            g.setColour (colour.withMultipliedAlpha (0.5f));
            g.fillPath (path);
        }
        else
        {
            g.setColour (colour);
            g.strokePath (path, juce::PathStrokeType ((float) GuiStyle::mediumLine, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
    }
}

void TimingHistogram::paint (juce::Graphics& g)
{
    auto& colours = gameState.colours;
    g.fillAll (colours.histogramBackground);

    auto bounds = getLocalBounds();
    auto stats = bounds.removeFromLeft (statsWidth).withSizeKeepingCentre (statsWidth, 2 * statsLineHeight);
    g.setColour (colours.text);
    g.setFont (GuiStyle::fontSize (14.0f));
    g.drawText (juce::String (count) + (count == 1 ? " hit" : " hits"), stats.removeFromTop (statsLineHeight), juce::Justification::centred);
    auto mean = juce::roundToInt (getMeanErrorMs());
    g.drawText (count == 0 ? juce::String ("mean -") : "mean " + formatMs (mean), stats, juce::Justification::centred);

    auto plot = getPlotArea();

    g.setFont (GuiStyle::fontSize (12.0f));
    auto labels = juce::Rectangle<float> (plot.getX(), plot.getBottom(), plot.getWidth(), (float) labelHeight);
    g.drawText ("Early", labels, juce::Justification::centredLeft);
    g.drawText ("Late", labels, juce::Justification::centredRight);

    // The tolerance categories' edges, either side of a perfectly timed hit.
    g.setColour (colours.histogramAxis.withAlpha (0.2f));
    for (int i = 0; i < GameState::NUM_TOLERANCE_CATEGORIES - 1; ++i)
        for (auto sign : { -1.0, 1.0 })
            GuiStyle::fillVerticalLine (g, (float) juce::roundToInt (xForError (sign * (double) gameState.tolerances[(size_t) i])), plot.getY(), plot.getBottom(), GuiStyle::thinLine);

    // The curve's fill sits behind the bars, and its line over them.
    drawCurve (g, true);

    for (int bin = 0; bin < getNumBins(); ++bin)
    {
        auto bar = getBarBounds (bin);
        if (bar.isEmpty())
            continue;
        auto centre = (bin + 0.5) * binWidthMs - (double) rangeMs();
        g.setColour (colours.histogramBars[(size_t) categoryForError (centre)]);
        g.fillRect (bar);
    }

    drawCurve (g, false);

    g.setColour (colours.histogramAxis);
    GuiStyle::fillVerticalLine (g, (float) juce::roundToInt (xForError (0.0)), plot.getY(), plot.getBottom(), GuiStyle::thinLine);

    if (count > 0)
    {
        drawMarker (g, getPercentileErrorMs (0.25), colours.histogramQuartile, 1);
        drawMarker (g, getPercentileErrorMs (0.75), colours.histogramQuartile, 1);
        drawMarker (g, getPercentileErrorMs (0.5), colours.histogramMedian, 0);
    }
}
