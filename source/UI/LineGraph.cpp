//
// Created by Arden on 7/8/2026.
//

#include "LineGraph.h"
#include "GuiStyle.h"

namespace
{
    constexpr float pointRadius = 3.5f;
    constexpr float leftMargin = 72.0f;
    constexpr float bottomMargin = 38.0f;
    constexpr float topMargin = 18.0f;
    constexpr float rightMargin = 18.0f;
    constexpr float tickLength = 6.0f;
    constexpr float tickLabelWidth = 64.0f;
    constexpr float tickLabelHeight = 22.0f;
    constexpr int desiredTickCount = 6;

    // Rounds a range to a "nice" value (1, 2, 5, or 10 times a power of ten).
    double niceNum (double range, bool round)
    {
        if (range <= 0.0)
            return 1.0;

        auto exponent = std::floor (std::log10 (range));
        auto fraction = range / std::pow (10.0, exponent);
        double niceFraction;

        if (round)
        {
            if (fraction < 1.5) niceFraction = 1.0;
            else if (fraction < 3.0) niceFraction = 2.0;
            else if (fraction < 7.0) niceFraction = 5.0;
            else niceFraction = 10.0;
        }
        else
        {
            if (fraction <= 1.0) niceFraction = 1.0;
            else if (fraction <= 2.0) niceFraction = 2.0;
            else if (fraction <= 5.0) niceFraction = 5.0;
            else niceFraction = 10.0;
        }

        return niceFraction * std::pow (10.0, exponent);
    }

    // Computes nice axis bounds and tick values that cover [dataMin, dataMax].
    void calculateTicks (float dataMin, float dataMax, int maxTicks, std::vector<float>& ticksOut, float& niceMinOut, float& niceMaxOut)
    {
        if (dataMin == dataMax)
        {
            dataMin -= 1.0f;
            dataMax += 1.0f;
        }

        auto range = niceNum (static_cast<double> (dataMax - dataMin), false);
        auto tickSpacing = niceNum (range / (maxTicks - 1), true);
        auto niceMin = std::floor (dataMin / tickSpacing) * tickSpacing;
        auto niceMax = std::ceil (dataMax / tickSpacing) * tickSpacing;

        niceMinOut = static_cast<float> (niceMin);
        niceMaxOut = static_cast<float> (niceMax);

        ticksOut.clear();
        for (auto v = niceMin; v <= niceMax + tickSpacing * 0.5; v += tickSpacing)
            ticksOut.push_back (static_cast<float> (v));
    }

    juce::String formatTickLabel (float value)
    {
        if (std::abs (value - std::round (value)) < 0.0001f)
            return juce::String (static_cast<int> (std::round (value)));
        return juce::String (value, 2);
    }
}

LineGraph::LineGraph (const GameColours& colours_) : colours (colours_)
{
}

LineGraph::~LineGraph()
{
}

void LineGraph::addData (const std::vector<float>& xValues, const std::vector<float>& yValues, std::optional<juce::Colour> colour)
{
    jassert (xValues.size() == yValues.size());

    Series newSeries;
    newSeries.xValues = xValues;
    newSeries.yValues = yValues;
    newSeries.colour = colour.has_value() ? *colour : colours.graphSeriesPalette[series.size() % colours.graphSeriesPalette.size()];
    series.push_back (std::move (newSeries));

    repaint();
}

void LineGraph::paint (juce::Graphics& g)
{
    g.fillAll (colours.graphBackground);

    auto plotArea = getLocalBounds().toFloat().withTrimmedLeft (leftMargin).withTrimmedBottom (bottomMargin).withTrimmedTop (topMargin).withTrimmedRight (rightMargin);

    g.setColour (colours.graphForeground);
    g.drawRect (plotArea, (float) GuiStyle::thinLine);

    if (series.empty())
        return;

    auto dataMinX = std::numeric_limits<float>::max();
    auto dataMaxX = std::numeric_limits<float>::lowest();
    auto dataMinY = std::numeric_limits<float>::max();
    auto dataMaxY = std::numeric_limits<float>::lowest();

    for (auto& s : series)
    {
        for (auto x : s.xValues)
        {
            dataMinX = std::min (dataMinX, x);
            dataMaxX = std::max (dataMaxX, x);
        }
        for (auto y : s.yValues)
        {
            dataMinY = std::min (dataMinY, y);
            dataMaxY = std::max (dataMaxY, y);
        }
    }

    if (dataMinX > dataMaxX || dataMinY > dataMaxY)
        return;

    std::vector<float> xTicks, yTicks;
    float axisMinX, axisMaxX, axisMinY, axisMaxY;
    calculateTicks (dataMinX, dataMaxX, desiredTickCount, xTicks, axisMinX, axisMaxX);
    calculateTicks (dataMinY, dataMaxY, desiredTickCount, yTicks, axisMinY, axisMaxY);

    auto toScreenX = [&] (float x)
    {
        return plotArea.getX() + (x - axisMinX) / (axisMaxX - axisMinX) * plotArea.getWidth();
    };
    auto toScreenY = [&] (float y)
    {
        return plotArea.getBottom() - (y - axisMinY) / (axisMaxY - axisMinY) * plotArea.getHeight();
    };

    g.setFont (GuiStyle::fontSize (12.0f));
    g.setColour (colours.graphForeground);

    for (auto x : xTicks)
    {
        auto sx = toScreenX (x);
        g.drawLine (sx, plotArea.getBottom(), sx, plotArea.getBottom() + tickLength, (float) GuiStyle::thinLine);
        g.drawText (formatTickLabel (x), juce::Rectangle<float> (sx - tickLabelWidth / 2.0f, plotArea.getBottom() + tickLength + 1.0f, tickLabelWidth, tickLabelHeight), juce::Justification::centred, false);
    }

    for (auto y : yTicks)
    {
        auto sy = toScreenY (y);
        g.drawLine (plotArea.getX() - tickLength, sy, plotArea.getX(), sy, (float) GuiStyle::thinLine);
        g.drawText (formatTickLabel (y), juce::Rectangle<float> (0.0f, sy - tickLabelHeight / 2.0f, plotArea.getX() - tickLength - 2.0f, tickLabelHeight), juce::Justification::centredRight, false);
    }

    for (auto& s : series)
    {
        auto pointCount = std::min (s.xValues.size(), s.yValues.size());
        if (pointCount == 0)
            continue;

        g.setColour (s.colour);

        juce::Path path;
        for (size_t i = 0; i < pointCount; ++i)
        {
            auto px = toScreenX (s.xValues[i]);
            auto py = toScreenY (s.yValues[i]);
            if (i == 0)
                path.startNewSubPath (px, py);
            else
                path.lineTo (px, py);
        }
        g.strokePath (path, juce::PathStrokeType ((float) GuiStyle::mediumLine));

        for (size_t i = 0; i < pointCount; ++i)
        {
            auto px = toScreenX (s.xValues[i]);
            auto py = toScreenY (s.yValues[i]);
            g.fillEllipse (px - pointRadius, py - pointRadius, pointRadius * 2.0f, pointRadius * 2.0f);
        }
    }
}
