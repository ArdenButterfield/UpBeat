#include <BundledResources.h>
#include <Chart/BarAtom.h>
#include <Chart/Chart.h>

#include <catch2/catch_test_macros.hpp>
#include <yaml-cpp/yaml.h>

namespace
{
    // One bar per time signature, each with a BeatEvent on every beat, as parse_als.py writes them.
    juce::String chartWithBars (std::initializer_list<std::pair<int, int>> timeSignatures)
    {
        juce::String yaml = "title: metronome\nevents:\n";
        auto time = 0.0;
        auto bar = 1;
        for (auto [numerator, denominator] : timeSignatures)
        {
            yaml << "- { type: BarEvent, time: " << time << ", bar: " << bar
                 << ", numerator: " << numerator << ", denominator: " << denominator << " }\n";
            for (int beat = 1; beat <= numerator; ++beat)
                yaml << "- { type: BeatEvent, time: " << time + (beat - 1) * 0.25 << ", bar: " << bar << ", beat: " << beat << " }\n";
            time += numerator * 0.25;
            bar += 1;
        }
        return yaml;
    }

    // For each beat after the downbeat, in order, whether the metronome clicks on it.
    std::vector<bool> beatClicks (const Chart& chart)
    {
        std::vector<bool> clicks;
        for (auto& [time, event] : chart.events)
            if (event.type == ChartEvent::BEAT)
                clicks.push_back (event.metronomeClick);
        return clicks;
    }
}

TEST_CASE ("The metronome clicks on every beat in simple time", "[metronome]")
{
    for (auto [numerator, denominator] : { std::pair { 2, 4 }, { 3, 4 }, { 4, 4 }, { 5, 4 }, { 6, 4 }, { 3, 8 }, { 5, 8 }, { 7, 8 }, { 2, 2 } })
    {
        INFO (numerator << "/" << denominator);
        for (int beat = 1; beat <= numerator; ++beat)
            CHECK (Chart::metronomeClicksOnBeat (numerator, denominator, beat));
    }
}

TEST_CASE ("The metronome clicks every three beats in compound time", "[metronome]")
{
    for (auto numerator : { 6, 9, 12 })
    {
        INFO (numerator << "/8");
        for (int beat = 1; beat <= numerator; ++beat)
            CHECK (Chart::metronomeClicksOnBeat (numerator, 8, beat) == (beat % 3 == 1));
    }
}

TEST_CASE ("Chart::fromYaml marks beats with the time signature of their bar", "[metronome][chart]")
{
    auto chart = Chart::fromYaml (chartWithBars ({ { 6, 8 }, { 4, 4 }, { 12, 8 }, { 3, 4 } }));
    REQUIRE (chart.has_value());

    // Beat 1 of each bar is its BarEvent, so beats start at 2.
    std::vector<bool> expected {
        false, false, true, false, false,                            // 6/8: beat 4
        true, true, true,                                            // 4/4
        false, false, true, false, false, true, false, false, true, false, false, // 12/8: beats 4, 7, 10
        true, true,                                                  // 3/4
    };
    CHECK (beatClicks (*chart) == expected);
}

TEST_CASE ("Beats default to clicking when a bar's time signature is missing", "[metronome][chart]")
{
    auto chart = Chart::fromYaml (R"(
title: no signature
events:
- { type: BarEvent, time: 0.0, bar: 1 }
- { type: BeatEvent, time: 0.5, bar: 1, beat: 2 }
- { type: BeatEvent, time: 1.0, bar: 1, beat: 3 }
)");
    REQUIRE (chart.has_value());
    CHECK (beatClicks (*chart) == std::vector<bool> { true, true });
}

TEST_CASE ("The bundled 6/8 jig clicks only on beat 4 between barlines", "[metronome][chart]")
{
    auto yamlText = BundledResources::loadFile ("default_charts/yaml/jig.yaml").toString();
    auto chart = Chart::fromYaml (yamlText);
    REQUIRE (chart.has_value());

    // Read each beat's number from the file itself, since its last bar is cut short.
    std::vector<bool> expected;
    for (const auto& event : YAML::Load (yamlText.toStdString())["events"])
    {
        if (event["type"].as<std::string>() == "BarEvent")
            REQUIRE (event["numerator"].as<int>() == 6);
        if (event["type"].as<std::string>() == "BeatEvent" && event["beat"].as<int>() != 1)
            expected.push_back (event["beat"].as<int>() == 4);
    }

    REQUIRE (expected.size() > 0);
    CHECK (beatClicks (*chart) == expected);
}

namespace
{
    // The count-in's clicks as (time, is barline) pairs, in order.
    std::vector<std::pair<long long, bool>> countInClicks (const Chart& chart)
    {
        CountIn countIn (BarAtom::splitIntoBars (chart).front());
        std::vector<std::pair<long long, bool>> clicks;
        for (auto& [time, event] : countIn.clicks)
            clicks.emplace_back (time, event.type == ChartEvent::BARLINE);
        return clicks;
    }
}

TEST_CASE ("The count-in lasts two of the chart's first measures", "[metronome][countin]")
{
    // Beats are 250 ms apart.
    auto fourFour = Chart::fromYaml (chartWithBars ({ { 4, 4 }, { 3, 4 } }));
    auto threeFour = Chart::fromYaml (chartWithBars ({ { 3, 4 }, { 4, 4 } }));
    REQUIRE (fourFour.has_value());
    REQUIRE (threeFour.has_value());

    CHECK (CountIn (BarAtom::splitIntoBars (*fourFour).front()).lengthMs == 2000);
    CHECK (CountIn (BarAtom::splitIntoBars (*threeFour).front()).lengthMs == 1500);
}

TEST_CASE ("The count-in clicks the first measure's metronome twice, ending as the chart starts", "[metronome][countin]")
{
    SECTION ("simple time clicks every beat")
    {
        auto chart = Chart::fromYaml (chartWithBars ({ { 4, 4 }, { 4, 4 } }));
        REQUIRE (chart.has_value());
        std::vector<std::pair<long long, bool>> expected {
            { -2000, true }, { -1750, false }, { -1500, false }, { -1250, false },
            { -1000, true }, { -750, false }, { -500, false }, { -250, false },
        };
        CHECK (countInClicks (*chart) == expected);
    }

    SECTION ("compound time clicks every dotted beat")
    {
        auto chart = Chart::fromYaml (chartWithBars ({ { 6, 8 }, { 4, 4 } }));
        REQUIRE (chart.has_value());
        std::vector<std::pair<long long, bool>> expected { { -3000, true }, { -2250, false }, { -1500, true }, { -750, false } };
        CHECK (countInClicks (*chart) == expected);
    }

    SECTION ("the chart's own clicks don't repeat in it")
    {
        auto chart = Chart::fromYaml (chartWithBars ({ { 2, 4 }, { 2, 4 } }));
        REQUIRE (chart.has_value());
        for (auto [time, barline] : countInClicks (*chart))
            CHECK (time < 0);
    }
}

TEST_CASE ("A chart without barlines has no count-in", "[metronome][countin]")
{
    auto chart = Chart::fromYaml (R"(
title: no barlines
events:
- { type: BeatEvent, time: 0.0, bar: 1, beat: 1 }
- { type: BeatEvent, time: 0.5, bar: 1, beat: 2 }
- { type: BeatEvent, time: 1.0, bar: 1, beat: 3 }
)");
    REQUIRE (chart.has_value());

    CountIn countIn (BarAtom::splitIntoBars (*chart).front());
    CHECK (countIn.lengthMs == 0);
    CHECK (countIn.clicks.empty());
}
