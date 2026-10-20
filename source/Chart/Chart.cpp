//
// Created by Arden on 7/9/2026.
//

#include "Chart.h"

#include <algorithm>
#include <cmath>
#include <yaml-cpp/yaml.h>

namespace
{
    long long secondsToMs (double seconds)
    {
        return std::llround (seconds * 1000.0);
    }

    juce::String toJuceString (const YAML::Node& node)
    {
        return juce::String::fromUTF8 (node.as<std::string> (std::string()).c_str());
    }

    juce::MemoryBlock decodeBase64 (const juce::String& base64)
    {
        juce::MemoryOutputStream decoded;
        juce::Base64::convertFromBase64 (decoded, base64);
        return decoded.getMemoryBlock();
    }
}

std::optional<Chart> Chart::fromYaml (const juce::String& yamlText)
{
    try
    {
        auto root = YAML::Load (yamlText.toStdString());
        if (! root.IsMap() || ! root["events"].IsSequence())
            return std::nullopt;

        Chart chart;
        chart.name = toJuceString (root["title"]);

        std::map<int, int> laneBySynthId;
        for (const auto& synthNode : root["synths"])
        {
            auto group = synthNode["group"].as<std::string> (std::string()) == "PLAYABLE" ? ChartSynth::PLAYABLE : ChartSynth::BACKGROUND;
            auto& synth = chart.synths.emplace_back (ChartSynth {
                synthNode["id"].as<int>(),
                toJuceString (synthNode["name"]),
                group,
                decodeBase64 (toJuceString (synthNode["state"])) });

            if (group == ChartSynth::PLAYABLE)
                laneBySynthId[synth.id] = chart.numLanes++;
        }

        for (const auto& eventNode : root["events"])
        {
            auto type = eventNode["type"].as<std::string> (std::string());
            auto timeMs = secondsToMs (eventNode["time"].as<double> (0.0));

            if (type == "PlayableNoteEvent" || type == "BackgroundNoteEvent")
            {
                auto synthId = eventNode["synth"].as<int> (-1);
                auto laneIt = laneBySynthId.find (synthId);
                auto playable = type == "PlayableNoteEvent" && laneIt != laneBySynthId.end();

                ChartEvent event (timeMs, playable ? ChartEvent::NOTE : ChartEvent::BACKGROUND_NOTE);
                event.lengthMs = secondsToMs (eventNode["length_seconds"].as<double> (0.0));
                event.midiNote = eventNode["pitch"].as<int> (0);
                event.velocity = eventNode["velocity"].as<int> (0);
                event.synthId = synthId;
                event.inputButton = playable ? laneIt->second : -1;
                chart.events.insert ({ timeMs, std::move (event) });
            }
            else if (type == "BarEvent")
            {
                chart.events.insert ({ timeMs, ChartEvent (timeMs, ChartEvent::BARLINE) });
            }
            else if (type == "BeatEvent")
            {
                // The first beat of each bar is already represented by its BarEvent.
                if (eventNode["beat"].as<int> (0) != 1)
                    chart.events.insert ({ timeMs, ChartEvent (timeMs, ChartEvent::BEAT) });
            }
            else if (type == "AutomationEvent")
            {
                auto synthId = eventNode["synth"].as<int> (-1);
                auto parameterId = toJuceString (eventNode["parameter"]);

                auto laneIt = std::find_if (chart.automation.begin(), chart.automation.end(), [&] (const AutomationLane& lane) {
                    return lane.synthId == synthId && lane.parameterId == parameterId;
                });
                if (laneIt == chart.automation.end())
                    laneIt = chart.automation.insert (chart.automation.end(), AutomationLane { synthId, parameterId, {} });

                AutomationPoint point { eventNode["time"].as<double> (0.0) * 1000.0, eventNode["value"].as<double> (0.0) };
                if (auto curve = eventNode["curve"]; curve && curve.IsSequence() && curve.size() == 4)
                {
                    point.hasCurve = true;
                    for (size_t i = 0; i < 4; ++i)
                        point.curve[i] = curve[i].as<double>();
                }
                laneIt->points.push_back (point);
            }
            else if (type == "GUIChangeEvent")
            {
                ChartEvent event (timeMs, ChartEvent::GUI_CHANGE);
                event.guiEventType = toJuceString (eventNode["event_type"]);
                event.guiValue = toJuceString (eventNode["value"]);
                chart.events.insert ({ timeMs, std::move (event) });
            }
        }

        // Events should already be in time order; stable, so jumps keep their order regardless.
        for (auto& lane : chart.automation)
            std::stable_sort (lane.points.begin(), lane.points.end(), [] (const AutomationPoint& a, const AutomationPoint& b) {
                return a.timeMs < b.timeMs;
            });

        return chart;
    }
    catch (const YAML::Exception& e)
    {
        DBG ("Failed to parse chart YAML: " << e.what());
        return std::nullopt;
    }
}
