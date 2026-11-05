#ifndef UPBEAT_CHARTCOLOURS_H
#define UPBEAT_CHARTCOLOURS_H

#include "Automation.h"
#include "LaneColours.h"

// A chart's NoteLaneDisplay colours: a base value for each Oklab channel, which automation
// overrides on any channel that has it. Starts out as LaneColours' defaults.
struct ChartColours
{
    ChartColours();

    Oklab oklabAt (int colour, double chartTimeMs) const;
    juce::Colour colourAt (int colour, double chartTimeMs) const;
    std::array<juce::Colour, LaneColours::numColours> paletteAt (double chartTimeMs) const;

    std::array<Oklab, LaneColours::numColours> base;
    // Indexed by colour, then channel. Point values are Oklab channel values.
    std::array<std::array<AutomationLane, LaneColours::numChannels>, LaneColours::numColours> automation;
};

#endif //UPBEAT_CHARTCOLOURS_H
