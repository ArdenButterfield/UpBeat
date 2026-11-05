"""Tests for how parse_als.py reads the UpBeat Colours plugin, on generated .als files.

tests/fixtures/colour_chart.yaml is this parser's output for a generated set. The C++ tests
parse that same file, which keeps the two sides agreeing on the chart format. To regenerate
it after an intended format change, run with UPDATE_FIXTURES=1 set.

Run with: python test_parse_als.py
"""

import contextlib
import gzip
import io
import os
import sys
import tempfile
import unittest
from pathlib import Path
from xml.sax.saxutils import quoteattr

import yaml

sys.dont_write_bytecode = True
sys.path.insert(0, str(Path(__file__).parent))

import parse_als  # noqa: E402

FIXTURE = Path(__file__).parent.parent / "tests" / "fixtures" / "colour_chart.yaml"

INITIAL = parse_als.INITIAL_EVENT_TIME


def fixture_colours() -> dict:
    """A different Oklab value for every colour and channel, none of them the game's default."""
    return {
        colour: {"L": round(0.1 + 0.1 * i, 6), "a": round(-0.1 + 0.02 * i, 6), "b": round(0.1 - 0.02 * i, 6)}
        for i, colour in enumerate(parse_als.LANE_COLOUR_IDS)
    }


def plugin_state_hex(values: dict[str, float]) -> str:
    """A state as Ableton saves it: the plugin's JUCE binary XML block, then the VST3 wrapper's data."""
    params = "".join(f'<PARAM id="{k}" value="{v}"/>' for k, v in values.items())
    xml = f"<UpBeatColours>{params}</UpBeatColours>".encode() + b"\0"
    return (b"VC2!" + len(xml).to_bytes(4, "little") + xml + b"wrapper data").hex()


def flatten(colours: dict) -> dict[str, float]:
    return {f"{colour}_{channel}": value for colour, lab in colours.items() for channel, value in lab.items()}


def plugin_device(state_hex: str, name: str = parse_als.COLOUR_PLUGIN_NAME) -> str:
    """A plugin device whose parameters' automation target ids are their parameter ids."""
    params = "".join(
        f"""<PluginFloatParameter>
              <ParameterName Value="{param_id}"/>
              <ParameterId Value="{parse_als.juce_param_hash(param_id)}"/>
              <ParameterValue><AutomationTarget Id="{param_id}"/></ParameterValue>
            </PluginFloatParameter>"""
        for param_id in parse_als.colour_parameter_ids()
    )
    return f"""<PluginDevice>
        <PluginDesc><Vst3PluginInfo>
          <Name Value="{name}"/>
          <Preset><Vst3Preset><ProcessorState>{state_hex}</ProcessorState></Vst3Preset></Preset>
        </Vst3PluginInfo></PluginDesc>
        <ParameterList>{params}</ParameterList>
      </PluginDevice>"""


def envelope(target: str, points: list[tuple]) -> str:
    """An automation envelope from (time, value) or (time, value, curve) points."""
    events = ""
    for point in points:
        curve = ""
        if len(point) == 3:
            c1x, c1y, c2x, c2y = point[2]
            curve = f'CurveControl1X="{c1x}" CurveControl1Y="{c1y}" CurveControl2X="{c2x}" CurveControl2Y="{c2y}"'
        events += f'<FloatEvent Time="{point[0]}" Value="{point[1]}" {curve}/>'
    return f"""<AutomationEnvelope>
        <EnvelopeTarget><PointeeId Value="{target}"/></EnvelopeTarget>
        <Automation><Events>{events}</Events></Automation>
      </AutomationEnvelope>"""


def midi_track(track_id: int, name: str, group_id: int, devices: str = "", envelopes: str = "", clips: str = "") -> str:
    return f"""<MidiTrack Id="{track_id}">
      <Name><EffectiveName Value={quoteattr(name)}/></Name>
      <TrackGroupId Value="{group_id}"/>
      <AutomationEnvelopes><Envelopes>{envelopes}</Envelopes></AutomationEnvelopes>
      <DeviceChain>
        <MainSequencer><ClipTimeable><ArrangerAutomation><Events>{clips}</Events></ArrangerAutomation></ClipTimeable></MainSequencer>
        <DeviceChain><Devices>{devices}</Devices></DeviceChain>
      </DeviceChain>
    </MidiTrack>"""


def group_track(track_id: int, name: str) -> str:
    return f"""<GroupTrack Id="{track_id}">
      <Name><EffectiveName Value="{name}"/></Name>
      <TrackGroupId Value="-1"/>
    </GroupTrack>"""


def clip(name: str, start: float, end: float) -> str:
    return f'<MidiClip><Name Value="{name}"/><CurrentStart Value="{start}"/><CurrentEnd Value="{end}"/></MidiClip>'


GUI_GROUP_ID = 10


def build(path: Path, *tracks: str) -> dict:
    """The chart for a 120 bpm, 4/4 set with a GUI group (id 10) and these tracks."""
    xml = f"""<Ableton><LiveSet>
      <Tracks>{group_track(GUI_GROUP_ID, parse_als.GUI_GROUP)}{"".join(tracks)}</Tracks>
      <MainTrack>
        <AutomationEnvelopes><Envelopes/></AutomationEnvelopes>
        <DeviceChain><Mixer>
          <Tempo><Manual Value="120"/><AutomationTarget Id="main-tempo"/></Tempo>
          <TimeSignature><Manual Value="201"/><AutomationTarget Id="main-signature"/></TimeSignature>
        </Mixer></DeviceChain>
      </MainTrack>
    </LiveSet></Ableton>"""
    with gzip.open(path, "wb") as f:
        f.write(xml.encode())
    return parse_als.build_chart(path)


# Normalised values: playhead L ramps 0.25 -> 0.75 over the first two beats, holds, then
# jumps. The repeated last point should be dropped. Lane 1's a ramps along a bezier curve.
FIXTURE_ENVELOPES = (
    envelope("playhead_L", [(INITIAL, 0.25), (2, 0.75), (4, 0.75), (4, 0.1), (4, 0.1)])
    + envelope("lane_0_a", [(INITIAL, 0.25), (1, 0.25, (0.25, 0.0, 1.0, 0.0)), (3, 0.75)])
)


class ColourPluginTests(unittest.TestCase):
    def setUp(self):
        directory = tempfile.TemporaryDirectory()
        self.addCleanup(directory.cleanup)
        self.path = Path(directory.name) / "colour_chart.als"
        self.warnings = io.StringIO()

    def build(self, *tracks: str) -> dict:
        with contextlib.redirect_stderr(self.warnings):
            return build(self.path, *tracks)

    def colour_track(self, envelopes: str = "", clips: str = "", group_id: int = GUI_GROUP_ID, track_id: int = 11) -> str:
        device = plugin_device(plugin_state_hex(flatten(fixture_colours())))
        return midi_track(track_id, "colours", group_id, device, envelopes, clips)

    def colour_events(self, chart: dict) -> list[dict]:
        return [e for e in chart["events"] if e["type"] == "ColourAutomationEvent"]

    def test_channels_denormalise_linearly_to_oklab(self):
        self.assertEqual(parse_als.denormalise_channel("L", 0.0), 0.0)
        self.assertEqual(parse_als.denormalise_channel("L", 1.0), 1.0)
        self.assertAlmostEqual(parse_als.denormalise_channel("a", 0.0), -0.4)
        self.assertAlmostEqual(parse_als.denormalise_channel("a", 0.5), 0.0)
        self.assertAlmostEqual(parse_als.denormalise_channel("b", 1.0), 0.4)
        # Also checked against the plugin's real parameters in tests/ColourPluginTests.cpp.
        self.assertAlmostEqual(parse_als.denormalise_channel("a", 0.75), 0.2)

    def test_parameter_ids_are_unique_after_hashing(self):
        hashes = {parse_als.juce_param_hash(p) for p in parse_als.colour_parameter_ids()}
        self.assertEqual(len(hashes), len(parse_als.LANE_COLOUR_IDS) * len(parse_als.OKLAB_CHANNELS))
        # The same id and hash are checked against JUCE in tests/ColourPluginTests.cpp.
        self.assertEqual(parse_als.juce_param_hash("playhead_L"), 22623713)

    def test_reads_every_colour_from_the_plugin_state(self):
        chart = self.build(self.colour_track())
        self.assertEqual(chart["colours"], fixture_colours())
        self.assertEqual(self.colour_events(chart), [])
        self.assertEqual(self.warnings.getvalue(), "")

    def test_automation_becomes_oklab_events_at_the_right_times(self):
        chart = self.build(self.colour_track(envelope("playhead_L", [(INITIAL, 0.25), (2, 0.75)])))
        self.assertEqual(self.colour_events(chart), [
            {"type": "ColourAutomationEvent", "time": 0.0, "colour": "playhead", "channel": "L", "value": 0.25},
            {"type": "ColourAutomationEvent", "time": 1.0, "colour": "playhead", "channel": "L", "value": 0.75},
        ])

    def test_a_and_b_automation_is_converted_from_normalised(self):
        chart = self.build(self.colour_track(envelope("lane_1_b", [(INITIAL, 0.75)])))
        event = self.colour_events(chart)[0]
        self.assertEqual((event["colour"], event["channel"], event["value"]), ("lane_1", "b", 0.2))

    def test_keeps_curves_and_jumps_and_drops_repeats(self):
        curve = (0.25, 0.0, 1.0, 0.0)
        chart = self.build(self.colour_track(envelope("lane_0_a", [(INITIAL, 0.5), (1, 0.5, curve), (2, 1.0), (2, 0.0), (2, 0.0)])))
        events = self.colour_events(chart)
        self.assertEqual([(e["time"], e["value"]) for e in events], [(0.0, 0.0), (0.5, 0.0), (1.0, 0.4), (1.0, -0.4)])
        self.assertEqual(events[1]["curve"], list(curve))
        self.assertNotIn("curve", events[0])

    def test_finds_the_plugin_on_any_gui_track(self):
        empty = midi_track(11, "section titles", GUI_GROUP_ID)
        chart = self.build(empty, self.colour_track(track_id=12))
        self.assertEqual(chart["colours"], fixture_colours())

    def test_ignores_the_plugin_outside_the_gui_group(self):
        chart = self.build(self.colour_track(group_id=-1))
        self.assertNotIn("colours", chart)

    def test_a_chart_without_the_plugin_has_no_colours(self):
        chart = self.build(midi_track(11, "section titles", GUI_GROUP_ID))
        self.assertNotIn("colours", chart)
        self.assertEqual(self.colour_events(chart), [])

    def test_only_the_first_plugin_is_used(self):
        other = flatten({colour: {"L": 0.5, "a": 0.0, "b": 0.0} for colour in parse_als.LANE_COLOUR_IDS})
        second = midi_track(12, "more colours", GUI_GROUP_ID, plugin_device(plugin_state_hex(other)),
                            envelope("playhead_L", [(INITIAL, 0.5)]))
        chart = self.build(self.colour_track(), second)
        self.assertEqual(chart["colours"], fixture_colours())
        self.assertEqual(self.colour_events(chart), [])
        self.assertIn("ignoring the extra", self.warnings.getvalue())

    def test_a_colour_missing_from_the_state_is_left_out_with_a_warning(self):
        values = flatten(fixture_colours())
        del values["playhead_a"]
        chart = self.build(midi_track(11, "colours", GUI_GROUP_ID, plugin_device(plugin_state_hex(values))))
        self.assertNotIn("playhead", chart["colours"])
        self.assertIn("barline", chart["colours"])
        self.assertIn("'playhead'", self.warnings.getvalue())

    def test_an_unreadable_state_warns_but_keeps_automation(self):
        device = plugin_device(b"not a juce state".hex())
        envelopes = envelope("playhead_L", [(INITIAL, 0.25)])
        chart = self.build(midi_track(11, "colours", GUI_GROUP_ID, device, envelopes))
        self.assertEqual(chart["colours"], {})
        self.assertEqual(len(self.colour_events(chart)), 1)
        self.assertIn("unexpected", self.warnings.getvalue())

    def test_other_plugins_are_not_mistaken_for_it(self):
        device = plugin_device(plugin_state_hex(flatten(fixture_colours())), name="Some Other Plugin")
        chart = self.build(midi_track(11, "colours", GUI_GROUP_ID, device))
        self.assertNotIn("colours", chart)

    def test_colour_events_sort_between_bars_and_gui_changes(self):
        chart = self.build(self.colour_track(envelope("playhead_L", [(INITIAL, 0.25)]), clip("Verse", 0, 4)))
        types = [e["type"] for e in chart["events"] if e["time"] == 0.0]
        self.assertEqual(types, ["BarEvent", "BeatEvent", "ColourAutomationEvent", "GUIChangeEvent"])

    def test_matches_the_fixture_shared_with_the_cpp_tests(self):
        chart = self.build(self.colour_track(FIXTURE_ENVELOPES, clip("Verse", 0, 4)))
        if os.environ.get("UPDATE_FIXTURES"):
            FIXTURE.parent.mkdir(parents=True, exist_ok=True)
            with open(FIXTURE, "w", encoding="utf-8", newline="\n") as f:
                yaml.safe_dump(chart, f, sort_keys=False, allow_unicode=True, width=1000)
        with open(FIXTURE, encoding="utf-8") as f:
            self.assertEqual(yaml.safe_load(f), yaml.safe_load(yaml.safe_dump(chart, sort_keys=False)))


if __name__ == "__main__":
    unittest.main()
