The chart is a yaml file consisting of:

title: the title of the als file, without the extension

List of synths to initialize:
Each track in the PLAYABLE and BACKGROUND group in the ableton set should have an odin2 synth. This list has the plugin
state of each of these synthesizers.

List of events, in order of time:
There are several types of events, each with its own parameters. In addition, event has a time in seconds since the start parameter.

- PlayableNoteEvent. This corresponds to the start of a midi note in the PLAYABLE group. This event has:
  length, both in seconds and in number of beats
  - Velocity
  - which synth it corresponds to.
- BackgroundNoteEvent. This corresponds to the start of a midi note in the BACKGROUND group. This event has the same parameters as the PlayableNoteEvent.
- GUIChangeEvent. This corresponds to the start of a midi clip in a track in the GUI group. Parameters:
  - Event type (name of the track)
  - Event value (name of the midi clip)
- AutomationEvent. This corresponds to an automation event in an Odin parameter on any lane. Parameters:
  - Which synth
  - What plugin parameter
  - new value
- BeatEvent
  - Occurs at every beat, calculated using tempo and time signature automation in the als file.
- BarEvent.
  - Same to beat event but on every barline.