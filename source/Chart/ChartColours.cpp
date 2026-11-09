#include "ChartColours.h"

ChartColours::ChartColours()
{
    for (int colour = 0; colour < LaneColours::numColours; ++colour)
        base[static_cast<size_t> (colour)] = LaneColours::defaultOklab (colour);
}

Oklab ChartColours::oklabAt (int colour, double chartTimeMs) const
{
    auto lab = base[static_cast<size_t> (colour)];
    auto& lanes = automation[static_cast<size_t> (colour)];
    for (int channel = 0; channel < LaneColours::numChannels; ++channel)
    {
        auto& lane = lanes[static_cast<size_t> (channel)];
        if (! lane.points.empty())
            lab[channel] = lane.valueAt (chartTimeMs);
    }
    return lab;
}

juce::Colour ChartColours::colourAt (int colour, double chartTimeMs) const
{
    return toColour (oklabAt (colour, chartTimeMs));
}

std::array<juce::Colour, LaneColours::numColours> ChartColours::paletteAt (double chartTimeMs) const
{
    std::array<juce::Colour, LaneColours::numColours> palette;
    for (int colour = 0; colour < LaneColours::numColours; ++colour)
        palette[static_cast<size_t> (colour)] = colourAt (colour, chartTimeMs);
    return palette;
}
