"""Convert an Ableton Live Set (.als) file into an UpBeat chart (.yaml).

An .als file is a gzip-compressed XML document. See chart_format.md for the
structure of the generated chart.

Ableton stores all arrangement positions in beats (quarter notes). Those are
converted to seconds using the tempo automation on the main track.
"""

import argparse
import base64
import bisect
import gzip
import math
import re
import sys
import xml.etree.ElementTree as ET
from dataclasses import dataclass, field
from pathlib import Path

import yaml

TRACK_TAGS = ("AudioTrack", "MidiTrack", "GroupTrack")

PLAYABLE_GROUP = "PLAYABLE"
BACKGROUND_GROUP = "BACKGROUND"
GUI_GROUP = "GUI"

ODIN_PLUGIN_NAME = "Odin2"

# Ableton's Utility device. On a PLAYABLE track, the panning (Balance) of the
# first Utility after the Odin synth picks the chart lane of each note.
UTILITY_DEVICE_TAG = "StereoGain"
LEFT_LANE = 0
RIGHT_LANE = 1

# The UpBeat Colours plugin, found on a track in the GUI group, sets the chart's
# NoteLaneDisplay colours. Its parameters are "<colour>_<channel>". These must match
# LaneColours in source/Chart/LaneColours.h.
COLOUR_PLUGIN_NAME = "UpBeat Colours"
LANE_COLOUR_IDS = (
    "row_background",
    "row_outline",
    "row_active_outline",
    "barline",
    "beat_line",
    "playhead",
    "lane_0",
    "lane_1",
)
# Each Oklab channel's (minimum, maximum). Ableton stores automation normalised to 0 to 1.
OKLAB_CHANNELS = {"L": (0.0, 1.0), "a": (-0.4, 0.4), "b": (-0.4, 0.4)}

# Automation envelopes start with an event at this time, holding the value
# that applies before the first real breakpoint.
INITIAL_EVENT_TIME = -63072000

# Events that share a timestamp are ordered like this, so that e.g. a synth
# parameter change is applied before a note that starts at the same moment.
EVENT_ORDER = {
    "BarEvent": 0,
    "BeatEvent": 1,
    "AutomationEvent": 2,
    "ColourAutomationEvent": 3,
    "GUIChangeEvent": 4,
    "BackgroundNoteEvent": 5,
    "PlayableNoteEvent": 6,
}

DECIMALS = 6


def warn(message: str) -> None:
    print(f"warning: {message}", file=sys.stderr)


def value_of(elem: ET.Element | None, path: str, default: str | None = None) -> str | None:
    """The Value attribute of the child element at `path`."""
    if elem is None:
        return default
    child = elem.find(path)
    return child.get("Value", default) if child is not None else default


# --------------------------------------------------------------------------
# Tracks


@dataclass
class Track:
    id: str
    name: str
    kind: str  # "AudioTrack", "MidiTrack" or "GroupTrack"
    group_id: str | None  # Id of the containing GroupTrack, or None if top level
    elem: ET.Element = field(repr=False)


def load_live_set(path: Path) -> ET.Element:
    with gzip.open(path, "rb") as f:
        root = ET.parse(f).getroot()
    live_set = root.find("LiveSet")
    if live_set is None:
        raise ValueError(f"{path} does not contain a LiveSet element")
    return live_set


def parse_tracks(live_set: ET.Element) -> list[Track]:
    tracks_elem = live_set.find("Tracks")
    if tracks_elem is None:
        return []

    tracks = []
    for elem in tracks_elem:
        if elem.tag not in TRACK_TAGS:
            continue
        group_id = value_of(elem, "TrackGroupId", "-1")
        tracks.append(
            Track(
                id=elem.get("Id", ""),
                name=value_of(elem, "Name/EffectiveName", ""),
                kind=elem.tag,
                group_id=None if group_id == "-1" else group_id,
                elem=elem,
            )
        )
    return tracks


def group_path(track: Track, by_id: dict[str, Track]) -> list[str]:
    """Names of the groups containing this track, outermost first."""
    path = []
    group_id = track.group_id
    while group_id is not None and group_id in by_id:
        group = by_id[group_id]
        path.insert(0, group.name)
        group_id = group.group_id
    return path


def print_tracks(path: Path) -> None:
    tracks = parse_tracks(load_live_set(path))
    by_id = {t.id: t for t in tracks}

    print(f"== {path.name} ==")
    for track in tracks:
        groups = group_path(track, by_id)
        label = " (group)" if track.kind == "GroupTrack" else ""
        location = f"  in: {' > '.join(groups)}" if groups else ""
        print(f"  {track.name}{label}{location}")
    print()


# --------------------------------------------------------------------------
# Automation envelopes


@dataclass
class Breakpoint:
    time: float  # beats
    value: float
    curve: tuple[float, float, float, float] | None  # bezier controls of the segment starting here


def parse_breakpoints(envelope: ET.Element) -> list[Breakpoint]:
    """Breakpoints of an automation envelope, in file order.

    Two breakpoints at the same time form a jump. The initial event is moved
    to time 0.
    """
    points = []
    for event in envelope.findall("Automation/Events/*"):
        time = float(event.get("Time", "0"))
        curve = None
        if event.get("CurveControl1X") is not None:
            curve = tuple(float(event.get(f"CurveControl{n}{axis}")) for n in (1, 2) for axis in "XY")
        points.append(Breakpoint(max(time, 0.0), float(event.get("Value", "0")), curve))
    points.sort(key=lambda p: p.time)  # stable, so jumps keep their order
    return points


def find_envelopes(track_elem: ET.Element) -> dict[str, ET.Element]:
    """The track's automation envelopes, keyed by automation target id."""
    return {
        value_of(env, "EnvelopeTarget/PointeeId"): env
        for env in track_elem.findall("AutomationEnvelopes/Envelopes/AutomationEnvelope")
    }


def bezier_fraction(curve: tuple[float, float, float, float], x: float) -> float:
    """Evaluate an Ableton automation curve.

    The curve is a cubic bezier from (0, 0) to (1, 1) through the two control
    points, in coordinates normalised to the segment. Returns y for the given x.
    """
    c1x, c1y, c2x, c2y = curve

    def bezier(p1: float, p2: float, u: float) -> float:
        return 3 * (1 - u) ** 2 * u * p1 + 3 * (1 - u) * u**2 * p2 + u**3

    lo, hi = 0.0, 1.0
    for _ in range(50):
        mid = (lo + hi) / 2
        if bezier(c1x, c2x, mid) < x:
            lo = mid
        else:
            hi = mid
    return bezier(c1y, c2y, (lo + hi) / 2)


# --------------------------------------------------------------------------
# Tempo map


class TempoMap:
    """Converts positions in beats to seconds using the tempo automation."""

    CURVE_STEPS = 256

    def __init__(self, points: list[Breakpoint]):
        # Each segment ramps from (t0, v0) to (t1, v1). The final segment is
        # open ended and holds the last value.
        self.segments: list[tuple[float, float, float, float, tuple | None]] = []
        for a, b in zip(points, points[1:]):
            if b.time > a.time:
                # The initial value is held until the first real breakpoint.
                v1 = a.value if a is points[0] else b.value
                self.segments.append((a.time, a.value, b.time, v1, a.curve))
        last = points[-1]
        self.segments.append((last.time, last.value, math.inf, last.value, None))
        if self.segments[0][0] > 0:
            first = points[0]
            self.segments.insert(0, (0.0, first.value, self.segments[0][0], first.value, None))

        self.starts = [s[0] for s in self.segments]
        self.start_seconds = []
        seconds = 0.0
        for segment in self.segments:
            self.start_seconds.append(seconds)
            if segment[2] != math.inf:
                seconds += self._segment_seconds(segment, segment[2])

    @staticmethod
    def _segment_seconds(segment: tuple, beat: float) -> float:
        """Seconds from the start of the segment to `beat`."""
        t0, v0, t1, v1, curve = segment
        length = beat - t0
        if length <= 0:
            return 0.0
        if v0 == v1:
            return 60.0 * length / v0
        if curve is None:
            slope = (v1 - v0) / (t1 - t0)
            return 60.0 / slope * math.log((v0 + slope * length) / v0)

        # Integrate 60 / bpm numerically along the curve (Simpson's rule).
        def seconds_per_beat(t: float) -> float:
            return 60.0 / (v0 + (v1 - v0) * bezier_fraction(curve, (t - t0) / (t1 - t0)))

        steps = TempoMap.CURVE_STEPS
        h = length / steps
        total = seconds_per_beat(t0) + seconds_per_beat(beat)
        for i in range(1, steps):
            total += (4 if i % 2 else 2) * seconds_per_beat(t0 + i * h)
        return total * h / 3

    def seconds(self, beat: float) -> float:
        i = max(bisect.bisect_right(self.starts, beat) - 1, 0)
        return self.start_seconds[i] + self._segment_seconds(self.segments[i], beat)


# --------------------------------------------------------------------------
# Time signatures, bars and beats


def decode_time_signature(value: int) -> tuple[int, int]:
    """Ableton encodes a time signature as (numerator - 1) + 99 * log2(denominator)."""
    return value % 99 + 1, 2 ** (value // 99)


def parse_time_signatures(main_track: ET.Element, envelopes: dict[str, ET.Element]) -> list[tuple[float, int, int]]:
    """(beat, numerator, denominator) for each time signature change."""
    mixer = main_track.find("DeviceChain/Mixer")
    target_id = mixer.find("TimeSignature/AutomationTarget").get("Id")
    if target_id in envelopes:
        points = parse_breakpoints(envelopes[target_id])
    else:
        points = [Breakpoint(0.0, float(value_of(mixer, "TimeSignature/Manual")), None)]

    changes = []
    for point in points:
        changes = [c for c in changes if c[0] < point.time]  # later events at the same time win
        changes.append((point.time, *decode_time_signature(int(point.value))))
    return changes


def bar_and_beat_events(time_signatures: list[tuple[float, int, int]], end_beat: float, tempo: TempoMap) -> list[dict]:
    """A BarEvent at every barline and a BeatEvent on every beat until `end_beat`.

    A beat is one unit of the time signature's denominator, so 6/8 has six
    beats per bar.
    """
    events = []
    bar = 1
    position = 0.0
    index = 0
    while position < end_beat - 1e-9:
        while index + 1 < len(time_signatures) and time_signatures[index + 1][0] <= position + 1e-9:
            index += 1
        _, numerator, denominator = time_signatures[index]
        beat_length = 4.0 / denominator

        bar_end = position + numerator * beat_length
        if index + 1 < len(time_signatures):
            bar_end = min(bar_end, time_signatures[index + 1][0])

        events.append({
            "type": "BarEvent",
            "time": tempo.seconds(position),
            "bar": bar,
            "numerator": numerator,
            "denominator": denominator,
        })
        beat = 0
        while position + beat * beat_length < min(bar_end, end_beat) - 1e-9:
            events.append({
                "type": "BeatEvent",
                "time": tempo.seconds(position + beat * beat_length),
                "bar": bar,
                "beat": beat + 1,
            })
            beat += 1

        position = bar_end
        bar += 1
    return events


# --------------------------------------------------------------------------
# Clips and notes


@dataclass
class Clip:
    name: str
    start: float  # arrangement position in beats
    end: float
    elem: ET.Element = field(repr=False)


@dataclass
class Note:
    start: float  # arrangement position in beats
    end: float
    pitch: int
    velocity: int


def arrangement_clips(track: Track) -> list[Clip]:
    clips = []
    for elem in track.elem.findall("DeviceChain/MainSequencer/ClipTimeable/ArrangerAutomation/Events/MidiClip"):
        if value_of(elem, "Disabled") == "true":
            continue
        if elem.find("Envelopes/Envelopes/*") is not None:
            warn(f"clip envelopes on track '{track.name}' are not supported and were ignored")
        clips.append(Clip(
            name=value_of(elem, "Name", ""),
            start=float(value_of(elem, "CurrentStart")),
            end=float(value_of(elem, "CurrentEnd")),
            elem=elem,
        ))
    return clips


def clip_notes(clip: Clip) -> list[Note]:
    """The notes a clip plays, at their arrangement positions.

    Looped clips are unrolled, and notes are cut off at the loop end and at the
    end of the clip.
    """
    loop = clip.elem.find("Loop")
    loop_start = float(value_of(loop, "LoopStart"))
    loop_end = float(value_of(loop, "LoopEnd"))
    looping = value_of(loop, "LoopOn") == "true"

    content = []  # (time within clip, duration, pitch, velocity)
    for key_track in clip.elem.findall("Notes/KeyTracks/KeyTrack"):
        pitch = int(value_of(key_track, "MidiKey"))
        for event in key_track.findall("Notes/MidiNoteEvent"):
            if event.get("IsEnabled", "true") == "false":
                continue
            content.append((float(event.get("Time")), float(event.get("Duration")), pitch, round(float(event.get("Velocity")))))

    notes = []
    position = clip.start
    content_start = loop_start + float(value_of(loop, "StartRelative", "0"))
    while position < clip.end:
        content_end = loop_end if looping else math.inf
        for time, duration, pitch, velocity in content:
            if not content_start <= time < content_end:
                continue
            start = position + time - content_start
            if start >= clip.end:
                continue
            end = min(position + min(time + duration, content_end) - content_start, clip.end)
            notes.append(Note(start, end, pitch, velocity))
        if not looping or content_end <= content_start:
            break
        position += content_end - content_start
        content_start = loop_start
    return notes


# --------------------------------------------------------------------------
# Odin synths


def juce_param_hash(param_id: str) -> int:
    """The VST3 parameter id JUCE generates for a parameter id string."""
    result = 0
    for c in param_id:
        result = (result * 31 + ord(c)) & 0xFFFFFFFF
    return result & 0x7FFFFFFF


@dataclass
class Synth:
    index: int
    track: Track
    group: str
    state: bytes  # Odin's own state, as passed to setStateInformation
    # Automation target id -> (Odin parameter id, display name)
    parameters: dict[str, tuple[str, str]]


def is_plugin(device: ET.Element, name: str) -> bool:
    return device.tag == "PluginDevice" and value_of(device, "PluginDesc/Vst3PluginInfo/Name") == name


def is_odin(device: ET.Element) -> bool:
    return is_plugin(device, ODIN_PLUGIN_NAME)


def plugin_state(device: ET.Element) -> tuple[bytes, list[ET.Element] | None]:
    """The plugin's own state, as passed to setStateInformation, and its PARAM elements.

    The VST3 wrapper appends its own private data after the block written by
    the plugin's getStateInformation. That block is a JUCE binary XML block:
    a "VC2!" magic number, a 32 bit little endian size, then the XML. The PARAM
    elements are None if the state isn't in that format.
    """
    processor_state = device.find("PluginDesc/Vst3PluginInfo/Preset/Vst3Preset/ProcessorState")
    raw = bytes.fromhex(re.sub(r"\s", "", processor_state.text or ""))
    if raw[:4] != b"VC2!":
        return raw, None
    size = int.from_bytes(raw[4:8], "little")
    state = raw[: 8 + size]
    xml = state[8:].rstrip(b"\0").decode("utf-8")
    return state, list(ET.fromstring(xml).iter("PARAM"))


def find_odin(track: Track) -> ET.Element | None:
    for device in track.elem.findall("DeviceChain/DeviceChain/Devices/*"):
        if is_odin(device):
            return device
    return None


def find_utility_after_odin(track: Track) -> ET.Element | None:
    """The first Utility device after the track's Odin synth, if any."""
    after_odin = False
    for device in track.elem.findall("DeviceChain/DeviceChain/Devices/*"):
        if is_odin(device):
            after_odin = True
        elif after_odin and device.tag == UTILITY_DEVICE_TAG:
            return device
    return None


def parse_synth(index: int, track: Track, group: str) -> Synth | None:
    device = find_odin(track)
    if device is None:
        warn(f"track '{track.name}' in {group} has no {ODIN_PLUGIN_NAME} VST3 device")
        return None

    state, params = plugin_state(device)
    if params is None:
        warn(f"unexpected {ODIN_PLUGIN_NAME} state format on track '{track.name}'")
        params = []
    odin_param_ids = [p.get("id") for p in params]

    by_hash = {juce_param_hash(p): p for p in odin_param_ids}
    parameters = {}
    for param in device.findall("ParameterList/PluginFloatParameter"):
        vst_id = int(value_of(param, "ParameterId", "-1"))
        if vst_id == -1:
            continue
        target = param.find("ParameterValue/AutomationTarget")
        name = value_of(param, "ParameterName", "")
        if vst_id not in by_hash:
            warn(f"could not find the {ODIN_PLUGIN_NAME} parameter for '{name}' on track '{track.name}'")
            continue
        parameters[target.get("Id")] = (by_hash[vst_id], name)

    return Synth(index, track, group, state, parameters)


class LaneMap:
    """Picks the chart lane of a PLAYABLE note from the panning of the Utility
    device after the track's Odin synth: LEFT_LANE when panned left, RIGHT_LANE
    when panned right. Centered counts as left. Without a Utility every note is
    in LEFT_LANE.
    """

    def __init__(self, track: Track):
        self.points: list[Breakpoint] = []
        utility = find_utility_after_odin(track)
        if utility is None:
            return
        balance = utility.find("Balance")
        target_id = balance.find("AutomationTarget").get("Id")
        envelope = find_envelopes(track.elem).get(target_id)
        if envelope is not None:
            self.points = parse_breakpoints(envelope)
        else:
            self.points = [Breakpoint(0.0, float(value_of(balance, "Manual", "0")), None)]

    @property
    def has_utility(self) -> bool:
        return bool(self.points)

    def balance(self, beat: float) -> float:
        """The Utility's balance (-1 left to 1 right) at `beat`.

        At a jump, the value after the jump applies. A small tolerance makes
        notes that start on a jump use the new value despite rounding.
        """
        if not self.points:
            return 0.0
        times = [p.time for p in self.points]
        i = bisect.bisect_right(times, beat + 1e-6) - 1
        if i < 0:
            return self.points[0].value
        a = self.points[i]
        if i + 1 == len(self.points):
            return a.value
        b = self.points[i + 1]
        x = (beat - a.time) / (b.time - a.time)
        if a.curve is not None:
            x = bezier_fraction(a.curve, x)
        return a.value + (b.value - a.value) * min(max(x, 0.0), 1.0)

    def lane(self, beat: float) -> int:
        return RIGHT_LANE if self.balance(beat) > 0 else LEFT_LANE


def distinct_breakpoints(envelope: ET.Element) -> list[Breakpoint]:
    """The envelope's breakpoints, without repeats of the one before."""
    points = []
    for point in parse_breakpoints(envelope):
        if not points or (point.time, point.value) != (points[-1].time, points[-1].value):
            points.append(point)
    return points


def automation_events(synth: Synth, tempo: TempoMap) -> list[dict]:
    """An AutomationEvent for every breakpoint of the synth's parameter automation.

    Values are normalised (0 to 1). Between two events of the same parameter the
    value ramps linearly, or along the bezier `curve` if the first event has one.
    """
    events = []
    for target_id, envelope in find_envelopes(synth.track.elem).items():
        if target_id not in synth.parameters:
            continue
        param_id, name = synth.parameters[target_id]
        for point in distinct_breakpoints(envelope):
            event = {
                "type": "AutomationEvent",
                "time": tempo.seconds(point.time),
                "synth": synth.index,
                "parameter": param_id,
                "parameter_name": name,
                "value": point.value,
            }
            if point.curve is not None:
                event["curve"] = list(point.curve)
            events.append(event)
    return events


# --------------------------------------------------------------------------
# NoteLaneDisplay colours


def find_colour_plugin(track: Track) -> ET.Element | None:
    for device in track.elem.findall("DeviceChain/DeviceChain/Devices/*"):
        if is_plugin(device, COLOUR_PLUGIN_NAME):
            return device
    return None


def colour_parameter_ids() -> dict[str, tuple[str, str]]:
    """Plugin parameter id -> (colour id, channel name)."""
    return {
        f"{colour}_{channel}": (colour, channel)
        for colour in LANE_COLOUR_IDS
        for channel in OKLAB_CHANNELS
    }


def denormalise_channel(channel: str, value: float) -> float:
    """A channel's Oklab value, from Ableton's normalised 0 to 1 parameter value."""
    low, high = OKLAB_CHANNELS[channel]
    return low + (high - low) * value


def parse_colour_plugin(device: ET.Element, track: Track, tempo: TempoMap) -> tuple[dict, list[dict]]:
    """The chart's "colours" section and a ColourAutomationEvent for every breakpoint.

    All values are in Oklab. The state holds a parameter's real values; a colour
    whose state is incomplete is left out, so the game uses its default.
    """
    parameter_ids = colour_parameter_ids()

    _, params = plugin_state(device)
    if params is None:
        warn(f"unexpected {COLOUR_PLUGIN_NAME} state format on track '{track.name}'")
        params = []
    state_values = {p.get("id"): float(p.get("value")) for p in params if p.get("id") in parameter_ids}

    colours = {}
    for colour in LANE_COLOUR_IDS:
        ids = {channel: f"{colour}_{channel}" for channel in OKLAB_CHANNELS}
        if all(param_id in state_values for param_id in ids.values()):
            colours[colour] = {channel: round(state_values[param_id], DECIMALS) for channel, param_id in ids.items()}
        else:
            warn(f"{COLOUR_PLUGIN_NAME} on track '{track.name}' has no saved value for '{colour}'; using the game's default")

    by_hash = {juce_param_hash(param_id): param_id for param_id in parameter_ids}
    targets = {}
    for param in device.findall("ParameterList/PluginFloatParameter"):
        param_id = by_hash.get(int(value_of(param, "ParameterId", "-1")))
        if param_id is not None:
            targets[param.find("ParameterValue/AutomationTarget").get("Id")] = parameter_ids[param_id]

    events = []
    for target_id, envelope in find_envelopes(track.elem).items():
        if target_id not in targets:
            continue
        colour, channel = targets[target_id]
        for point in distinct_breakpoints(envelope):
            event = {
                "type": "ColourAutomationEvent",
                "time": tempo.seconds(point.time),
                "colour": colour,
                "channel": channel,
                "value": round(denormalise_channel(channel, point.value), DECIMALS),
            }
            if point.curve is not None:
                event["curve"] = list(point.curve)
            events.append(event)
    return colours, events


# --------------------------------------------------------------------------
# Chart


def build_chart(path: Path) -> dict:
    live_set = load_live_set(path)
    tracks = parse_tracks(live_set)
    by_id = {t.id: t for t in tracks}

    main_track = live_set.find("MainTrack")
    if main_track is None:
        main_track = live_set.find("MasterTrack")
    main_envelopes = find_envelopes(main_track)

    mixer = main_track.find("DeviceChain/Mixer")
    tempo_target = mixer.find("Tempo/AutomationTarget").get("Id")
    if tempo_target in main_envelopes:
        tempo = TempoMap(parse_breakpoints(main_envelopes[tempo_target]))
    else:
        tempo = TempoMap([Breakpoint(0.0, float(value_of(mixer, "Tempo/Manual")), None)])

    synths: list[Synth] = []
    events: list[dict] = []
    end_beat = 0.0
    num_lanes = 1
    colours = None

    for track in tracks:
        if track.kind == "GroupTrack":
            continue
        groups = group_path(track, by_id)
        group = groups[0] if groups else None
        clips = arrangement_clips(track)
        end_beat = max([end_beat] + [c.end for c in clips])

        if group in (PLAYABLE_GROUP, BACKGROUND_GROUP):
            synth = parse_synth(len(synths), track, group)
            if synth is None:
                continue
            synths.append(synth)
            playable = group == PLAYABLE_GROUP
            lanes = LaneMap(track) if playable else None
            if lanes is not None and lanes.has_utility:
                num_lanes = 2
            for clip in clips:
                for note in clip_notes(clip):
                    start = tempo.seconds(note.start)
                    event = {
                        "type": "PlayableNoteEvent" if playable else "BackgroundNoteEvent",
                        "time": start,
                        "synth": synth.index,
                    }
                    if lanes is not None:
                        event["lane"] = lanes.lane(note.start)
                    event.update({
                        "pitch": note.pitch,
                        "velocity": note.velocity,
                        "length_seconds": tempo.seconds(note.end) - start,
                        "length_beats": note.end - note.start,
                    })
                    events.append(event)
            events.extend(automation_events(synth, tempo))

        elif group == GUI_GROUP:
            colour_plugin = find_colour_plugin(track)
            if colour_plugin is not None:
                if colours is None:
                    colours, colour_events = parse_colour_plugin(colour_plugin, track, tempo)
                    events.extend(colour_events)
                else:
                    warn(f"ignoring the extra {COLOUR_PLUGIN_NAME} plugin on track '{track.name}'")
            for clip in clips:
                events.append({
                    "type": "GUIChangeEvent",
                    "time": tempo.seconds(clip.start),
                    "event_type": track.name,
                    "value": clip.name,
                })

    time_signatures = parse_time_signatures(main_track, main_envelopes)
    events.extend(bar_and_beat_events(time_signatures, end_beat, tempo))

    for event in events:
        for key in ("time", "length_seconds", "length_beats"):
            if key in event:
                event[key] = round(event[key], DECIMALS)
    events.sort(key=lambda e: (e["time"], EVENT_ORDER[e["type"]]))

    chart = {
        "title": path.stem,
        "lanes": num_lanes,
    }
    if colours is not None:
        chart["colours"] = colours
    chart.update({
        "synths": [
            {
                "id": s.index,
                "name": s.track.name,
                "group": s.group,
                "state": base64.b64encode(s.state).decode("ascii"),
            }
            for s in synths
        ],
        "events": events,
    })
    return chart


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("files", nargs="+", type=Path, help=".als files to convert")
    parser.add_argument("-o", "--output-dir", type=Path, help="where to write the charts (default: next to each .als file)")
    parser.add_argument("--list-tracks", action="store_true", help="only print the tracks and their groups")
    args = parser.parse_args()

    for path in args.files:
        if args.list_tracks:
            print_tracks(path)
            continue
        chart = build_chart(path)
        out_dir = args.output_dir or path.parent
        out_dir.mkdir(parents=True, exist_ok=True)
        out_path = out_dir / f"{path.stem}.yaml"
        with open(out_path, "w", encoding="utf-8") as f:
            yaml.safe_dump(chart, f, sort_keys=False, allow_unicode=True, width=1000)
        print(f"wrote {out_path}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
