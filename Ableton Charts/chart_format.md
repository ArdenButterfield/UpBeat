The chart is a yaml file consisting of:

title: the title of the als file, without the extension

lanes: the number of chart lanes the player plays on. 2 if any PLAYABLE track has a Utility device after its Odin synth,
otherwise 1.

colours: only present if a track in the GUI group has the UpBeat Colours plugin (the first one found is used). The
NoteLaneDisplay colours of this chart, each with its L, a and b values in Oklab, taken from the plugin's parameter values.
The colours are row_background, row_outline, row_active_outline, barline, beat_line, playhead, lane_0 and lane_1. The game
uses its default for any colour that is left out.

List of synths to initialize:
Each track in the PLAYABLE and BACKGROUND group in the ableton set should have an odin2 synth. This list has the plugin
state of each of these synthesizers.

List of events, in order of time:
There are several types of events, each with its own parameters. In addition, event has a time in seconds since the start parameter.

- PlayableNoteEvent. This corresponds to the start of a midi note in the PLAYABLE group. This event has:
  length, both in seconds and in number of beats
  - Velocity
  - which synth it corresponds to (the synth that plays its sound).
  - which chart lane it belongs to (the input the player hits it with). This is independent of the synth: it is read
    from the panning (Balance) of the first Utility device after the Odin synth on the note's track, at the time the
    note starts. Lane 0 if the Utility is panned left (or centered), lane 1 if it is panned right. If the track has no
    Utility after its Odin synth, every note is in lane 0.
- BackgroundNoteEvent. This corresponds to the start of a midi note in the BACKGROUND group. This event has the same
  parameters as the PlayableNoteEvent, except for the lane.
- GUIChangeEvent. This corresponds to the start of a midi clip in a track in the GUI group. Parameters:
  - Event type (name of the track)
  - Event value (name of the midi clip)
- AutomationEvent. This corresponds to an automation event in an Odin parameter on any lane. Parameters:
  - Which synth
  - What plugin parameter
  - new value
- ColourAutomationEvent. This corresponds to an automation event in a parameter of the UpBeat Colours plugin. Between two
  events of the same colour channel the value ramps linearly in Oklab, or along the bezier curve if the first event has one.
  Parameters:
  - Which colour
  - Which channel (L, a or b)
  - new value, in Oklab
  - curve (optional)
- BeatEvent
  - Occurs at every beat, calculated using tempo and time signature automation in the als file.
- BarEvent.
  - Same to beat event but on every barline.