//
// Created by Arden on 9/7/2026.
//

#include "BarAtom.h"

#include <algorithm>

BarAtom::BarAtom (long long _lengthMs, std::multimap<long long, ChartEvent> _events)
    : lengthMs (_lengthMs), events (std::move (_events))
{
}

std::vector<BarAtom> BarAtom::splitIntoBars (const Chart& chart)
{
    std::vector<long long> barlineTimes;
    for (auto& [time, event] : chart.events)
        if (event.type == ChartEvent::BARLINE)
            barlineTimes.push_back (time);

    if (barlineTimes.empty())
        barlineTimes.push_back (0);

    long long chartEndTime = 0;
    for (auto& [time, event] : chart.events)
        if (event.type == ChartEvent::NOTE)
            chartEndTime = std::max (chartEndTime, time);
    chartEndTime = std::max (chartEndTime, barlineTimes.back());

    std::vector<BarAtom> bars;
    for (size_t i = 0; i < barlineTimes.size(); ++i)
    {
        auto barStart = barlineTimes[i];
        auto barEnd = (i + 1 < barlineTimes.size()) ? barlineTimes[i + 1] : chartEndTime;
        barEnd = std::max (barEnd, barStart + 1);

        std::multimap<long long, ChartEvent> barEvents;
        for (auto it = chart.events.lower_bound (barStart); it != chart.events.end() && it->first < barEnd; ++it)
        {
            auto localTime = it->first - barStart;
            auto event = it->second;
            event.timeMs = localTime;
            event.performanceTimings.clear();
            barEvents.insert ({ localTime, std::move (event) });
        }

        bars.emplace_back (barEnd - barStart, std::move (barEvents));
    }

    return bars;
}
