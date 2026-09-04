//
// Created by Arden on 7/8/2026.
//

#ifndef UPBEAT_LINEGRAPH_H
#define UPBEAT_LINEGRAPH_H

#include "juce_gui_basics/juce_gui_basics.h"
#include <optional>
#include <vector>

class LineGraph : public juce::Component
{
public:
    LineGraph();
    ~LineGraph() override;

    // Adds a new line series to the plot. xValues and yValues must be the same size.
    // If colour is not specified, one is chosen automatically from a default palette.
    void addData (const std::vector<float>& xValues, const std::vector<float>& yValues, std::optional<juce::Colour> colour = std::nullopt);

    void paint (juce::Graphics& g) override;

private:
    struct Series
    {
        std::vector<float> xValues;
        std::vector<float> yValues;
        juce::Colour colour;
    };

    std::vector<Series> series;
};

#endif //UPBEAT_LINEGRAPH_H
