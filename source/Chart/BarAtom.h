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
    BarAtom (long long chartStartMs, long long lengthMs, std::multimap<long long, ChartEvent> events);

    long long chartStartMs; // where the bar starts in the original chart
    long long lengthMs;
    int learnedScore = 0;
    std::vector<bool> successHistory; // one entry per practice rep: true if every note was Fresh or 100
    std::multimap<long long, ChartEvent> events;

    // Splits a chart into consecutive bars at its BARLINE events, with each bar's event
    // times made relative to that bar's own start. The final bar runs to the end of the
    // chart's last playable note.
    static std::vector<BarAtom> splitIntoBars (const Chart& chart);
};

// The count-in before a chart (or a practice run) starts: the chart's first measure played
// numBars times, clicking the metronome as that measure does, timed to end at time 0 where
// the chart begins. A chart without barlines has no measures to count in with, so its
// count-in is empty: no length and no clicks.
struct CountIn
{
    static constexpr int numBars = 2;

    explicit CountIn (const BarAtom& firstBar);

    long long lengthMs = 0;
    // Each repeat of the first measure's BARLINE and clicking BEAT events, at times from
    // -lengthMs up to 0.
    std::multimap<long long, ChartEvent> clicks;
};

#endif //UPBEAT_BARATOM_H
