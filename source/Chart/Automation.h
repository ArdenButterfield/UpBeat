//
// Created by Arden on 10/20/2026.
//

#ifndef UPBEAT_AUTOMATION_H
#define UPBEAT_AUTOMATION_H

#include "juce_core/juce_core.h"
#include <array>
#include <vector>

// One breakpoint of a parameter's automation. The value ramps from here to the next point:
// linearly, or along curve if hasCurve is set.
struct AutomationPoint
{
    double timeMs;
    double value; // normalised, 0 to 1

    bool hasCurve = false;
    // Ableton's bezier control points (x1, y1, x2, y2), in coordinates normalised to the
    // segment, for a curve from (0, 0) to (1, 1).
    std::array<double, 4> curve {};
};

// The automation of one Odin parameter on one chart synth.
struct AutomationLane
{
    int synthId;
    juce::String parameterId;
    // In time order. Two points at the same time form a jump.
    std::vector<AutomationPoint> points;

    // The parameter's value at timeMs: held at the first point's value before it and the last
    // point's value after it. Doesn't allocate, so it's safe on the audio thread.
    double valueAt (double timeMs) const;
};

#endif //UPBEAT_AUTOMATION_H
