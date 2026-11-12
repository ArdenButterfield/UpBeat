#ifndef UPBEAT_METRONOMECLICKS_H
#define UPBEAT_METRONOMECLICKS_H

#include "ChartEvent.h"
#include <map>
#include <optional>

namespace MetronomeClicks
{
    inline const ChartEvent& eventOf (const ChartEvent& event) { return event; }
    inline const ChartEvent& eventOf (const ChartEvent* event) { return *event; }

    // Whether the metronome clicks on this event: every BARLINE, and the BEATs marked to click.
    inline bool clicksOn (const ChartEvent& event)
    {
        return event.type == ChartEvent::BARLINE || (event.type == ChartEvent::BEAT && event.metronomeClick);
    }

    // The time of the latest metronome click at or before time in a time-keyed map of events
    // (held by value or by pointer), or nullopt if there's none.
    template <typename Value>
    std::optional<long long> latestAtOrBefore (const std::multimap<long long, Value>& events, long long time)
    {
        for (auto it = events.upper_bound (time); it != events.begin();)
        {
            --it;
            if (clicksOn (eventOf (it->second)))
                return it->first;
        }
        return std::nullopt;
    }

    // The latest click at or before time across a count-in's clicks and the events after it.
    template <typename Value>
    std::optional<long long> latestAtOrBefore (const std::multimap<long long, ChartEvent>& countInClicks,
        const std::multimap<long long, Value>& events,
        long long time)
    {
        auto fromCountIn = latestAtOrBefore (countInClicks, time);
        auto fromEvents = latestAtOrBefore (events, time);
        if (fromCountIn && fromEvents)
            return std::max (*fromCountIn, *fromEvents);
        return fromEvents ? fromEvents : fromCountIn;
    }
}

#endif //UPBEAT_METRONOMECLICKS_H
