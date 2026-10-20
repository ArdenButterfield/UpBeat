//
// Created by Arden on 10/20/2026.
//

#include "Automation.h"

#include <algorithm>

namespace
{
    // Evaluates an Ableton automation curve (see AutomationPoint::curve) at x, by bisecting
    // for the bezier parameter whose x matches, as Ableton Charts/parse_als.py does.
    double bezierFraction (const std::array<double, 4>& curve, double x)
    {
        auto [c1x, c1y, c2x, c2y] = curve;
        auto bezier = [] (double p1, double p2, double u) {
            return 3 * (1 - u) * (1 - u) * u * p1 + 3 * (1 - u) * u * u * p2 + u * u * u;
        };

        double lo = 0.0, hi = 1.0;
        for (int i = 0; i < 50; ++i)
        {
            auto mid = (lo + hi) / 2;
            if (bezier (c1x, c2x, mid) < x)
                lo = mid;
            else
                hi = mid;
        }
        return bezier (c1y, c2y, (lo + hi) / 2);
    }
}

double AutomationLane::valueAt (double timeMs) const
{
    if (points.empty())
        return 0.0;

    // The first point strictly after timeMs, so that at a jump the later of the two points wins.
    auto next = std::upper_bound (points.begin(), points.end(), timeMs, [] (double t, const AutomationPoint& p) {
        return t < p.timeMs;
    });

    if (next == points.begin())
        return points.front().value;
    if (next == points.end())
        return points.back().value;

    auto& previous = *(next - 1);
    auto fraction = (timeMs - previous.timeMs) / (next->timeMs - previous.timeMs);
    if (previous.hasCurve)
        fraction = bezierFraction (previous.curve, fraction);

    return previous.value + (next->value - previous.value) * fraction;
}
