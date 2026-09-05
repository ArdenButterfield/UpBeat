//
// Created by Arden on 9/5/2026.
//

#ifndef UPBEAT_CHARTSELECTIONBUTTON_H
#define UPBEAT_CHARTSELECTIONBUTTON_H

#include "juce_gui_basics/juce_gui_basics.h"

// A simple named, indexed button used for the selectable lists in the chart selection
// and chart creation UI (e.g. "Your Charts" and "MIDI Files").
class ChartSelectionButton : public juce::TextButton
{
public:
    ChartSelectionButton (const juce::String& label, int index);

    int index;
};

#endif //UPBEAT_CHARTSELECTIONBUTTON_H
