//
// Created by Arden on 9/5/2026.
//

#ifndef UPBEAT_CHARTSELECTIONBUTTON_H
#define UPBEAT_CHARTSELECTIONBUTTON_H

#include "juce_gui_basics/juce_gui_basics.h"

class ChartSelectionButton : public juce::TextButton
{
public:
    ChartSelectionButton (const juce::String& chartName, int chartIndex, const juce::MemoryBlock& midiData);

    void paintButton (juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

    int chartIndex;

private:
    juce::StringArray channelLines;
};

#endif //UPBEAT_CHARTSELECTIONBUTTON_H
