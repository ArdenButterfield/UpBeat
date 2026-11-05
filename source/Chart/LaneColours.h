#ifndef UPBEAT_LANECOLOURS_H
#define UPBEAT_LANECOLOURS_H

#include "juce_graphics/juce_graphics.h"
#include <array>
#include <cmath>
#include <optional>
#include <string_view>

// A colour in Oklab, where a straight line between two colours looks like a smooth fade.
struct Oklab
{
    double L = 0.0;
    double a = 0.0;
    double b = 0.0;

    double& operator[] (int channel) { return channel == 0 ? L : (channel == 1 ? a : b); }
    double operator[] (int channel) const { return channel == 0 ? L : (channel == 1 ? a : b); }
};

inline Oklab toOklab (juce::Colour colour)
{
    auto toLinear = [] (juce::uint8 channel) {
        auto c = channel / 255.0;
        return c <= 0.04045 ? c / 12.92 : std::pow ((c + 0.055) / 1.055, 2.4);
    };
    auto r = toLinear (colour.getRed());
    auto g = toLinear (colour.getGreen());
    auto b = toLinear (colour.getBlue());

    auto l = std::cbrt (0.4122214708 * r + 0.5363325363 * g + 0.0514459929 * b);
    auto m = std::cbrt (0.2119034982 * r + 0.6806995451 * g + 0.1073969566 * b);
    auto s = std::cbrt (0.0883024619 * r + 0.2817188376 * g + 0.6299787005 * b);

    return { 0.2104542553 * l + 0.7936177850 * m - 0.0040720468 * s,
        1.9779984951 * l - 2.4285922050 * m + 0.4505937099 * s,
        0.0259040371 * l + 0.7827717662 * m - 0.8086757660 * s };
}

// Oklab can describe colours the screen can't show; those are clipped channel by channel.
inline juce::Colour toColour (Oklab lab)
{
    auto l = lab.L + 0.3963377774 * lab.a + 0.2158037573 * lab.b;
    auto m = lab.L - 0.1055613458 * lab.a - 0.0638541728 * lab.b;
    auto s = lab.L - 0.0894841775 * lab.a - 1.2914855480 * lab.b;
    l = l * l * l;
    m = m * m * m;
    s = s * s * s;

    auto toSrgb = [] (double linear) {
        auto c = linear <= 0.0031308 ? 12.92 * linear : 1.055 * std::pow (linear, 1.0 / 2.4) - 0.055;
        return static_cast<juce::uint8> (juce::roundToInt (juce::jlimit (0.0, 1.0, c) * 255.0));
    };

    return juce::Colour (toSrgb (4.0767416621 * l - 3.3077115913 * m + 0.2309699292 * s),
        toSrgb (-1.2684380046 * l + 2.6097574011 * m - 0.3413193965 * s),
        toSrgb (-0.0041960863 * l - 0.7034186147 * m + 1.7076147010 * s));
}

// The NoteLaneDisplay colours a chart can set and automate, shared by the game and the
// UpBeat Colours plugin. The ids, channel names and ranges are also used by
// Ableton Charts/parse_als.py, and must be changed there too.
namespace LaneColours
{
    enum Id
    {
        rowBackground,
        rowOutline,
        rowActiveOutline,
        barline,
        beatLine,
        playhead,
        lane0,
        lane1,
        numColours
    };

    struct Info
    {
        const char* id;
        const char* title;
        juce::uint32 defaultArgb;
    };

    inline constexpr std::array<Info, numColours> infos { {
        { "row_background", "Row background", 0xff000000 },
        { "row_outline", "Row outline", 0xff808080 },
        { "row_active_outline", "Current row outline", 0xffffffff },
        { "barline", "Barline", 0xffffffff },
        { "beat_line", "Beat line", 0xffffffff },
        { "playhead", "Playhead", 0xffffffff },
        { "lane_0", "Lane 1 notes", 0xffff69b4 },
        { "lane_1", "Lane 2 notes", 0xff00bfff },
    } };

    inline constexpr int numChannels = 3;

    struct ChannelInfo
    {
        const char* name;
        float minValue;
        float maxValue;
    };

    // Wide enough for every colour a screen can show.
    inline constexpr std::array<ChannelInfo, numChannels> channels { {
        { "L", 0.0f, 1.0f },
        { "a", -0.4f, 0.4f },
        { "b", -0.4f, 0.4f },
    } };

    inline std::optional<int> findColour (std::string_view id)
    {
        for (size_t i = 0; i < infos.size(); ++i)
            if (id == infos[i].id)
                return static_cast<int> (i);
        return std::nullopt;
    }

    inline std::optional<int> findChannel (std::string_view name)
    {
        for (size_t i = 0; i < channels.size(); ++i)
            if (name == channels[i].name)
                return static_cast<int> (i);
        return std::nullopt;
    }

    inline Oklab defaultOklab (int colour)
    {
        return toOklab (juce::Colour (infos[static_cast<size_t> (colour)].defaultArgb));
    }

    // The id of a channel's parameter in the UpBeat Colours plugin, e.g. "playhead_L".
    inline juce::String parameterId (int colour, int channel)
    {
        return juce::String (infos[static_cast<size_t> (colour)].id) + "_" + channels[static_cast<size_t> (channel)].name;
    }
}

#endif //UPBEAT_LANECOLOURS_H
