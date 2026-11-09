#include "ColourPluginProcessor.h"

// Kept apart from the rest of the plugin's code, which the Tests target also links: this
// symbol would clash there with the game plugin's own createPluginFilter.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ColourPluginProcessor();
}
