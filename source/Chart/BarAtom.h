//
// Created by Arden on 9/7/2026.
//

#ifndef UPBEAT_BARATOM_H
#define UPBEAT_BARATOM_H

#include "Chart.h"
#include "ChartEvent.h"
#include <map>
#include <vector>

// One measure of a Chart: a self-contained slice of chart events timed relative to the
// start of the bar, plus practice-mode progress state.
class BarAtom
{
public:
    BarAtom (long long lengthMs, std::multimap<long long, ChartEvent> events);

    long long lengthMs;
    int learnedScore = 0;
    std::multimap<long long, ChartEvent> events;

    // Splits a chart into consecutive bars at its BARLINE events, with each bar's event
    // times made relative to that bar's own start. The final bar runs to the end of the
    // chart's last playable note.
    static std::vector<BarAtom> splitIntoBars (const Chart& chart);
};

#endif //UPBEAT_BARATOM_H
