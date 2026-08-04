//
// Created by Arden on 7/9/2026.
//

#ifndef UPBEAT_CHART_H
#define UPBEAT_CHART_H
#include "ChartEvent.h"
#include "juce_core/juce_core.h"
#include <map>

class Chart
{
public:
    explicit Chart(const juce::MemoryBlock& midiData);
    ~Chart();
    juce::String name;
    juce::MemoryBlock midiData;
    std::multimap<long, ChartEvent> events;

    long countInTime = 2000;
};

#endif //UPBEAT_CHART_H
