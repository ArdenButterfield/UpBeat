Within the GameState, we store a vector of *Charts*, which represent a single playable song in our rhythm game. 
A Chart is made from a MIDI file and a choice of which tracks and channels should be playable. For instance, consider a midi file with 
notes on track 1 channel 1 and track 1 channel 2: we could create a chart where only track 1 channel 2 is playable, in which case
the Chart would be constructed with the notes in that channel in lane 0 and the other channel's notes as background notes.
Or, we could create a chart where both channels are playable, in which case track 1 channel 1 notes would be assigned
to lane 0, and track 1 channel 2 notes would be assigned to lane 1.

The creation of charts happens in the ChartSelectionScene. Within this scene, there should be a panel with a list of "Your Charts":
the first time the user sees this scene, this panel should be empty, as there are no existing charts. Below that panel is a 
"New Chart" button; the user clicks on this button to open a chart creation panel.

In the chart creation panel, the left half of the panel will be taken up with a list, called "MIDI Files" of all the default chart midis
in BundledResources. Below that is an "Upload your own MIDI" button, which opens a file browser window for the user to select 
a midi file to add to this list. If the user clicks on a midi file in the list, the right side of the panel populates with a list
of all (track, channel) pairs in that file, and how many notes are in each. Besides each pair is a checkbox: if the user
checks the box for a (track, channel) pair, those will become playable notes in the chart, as described in the first paragraph.

Once the user is satisfied, they can click a "Create" or "Cancel" button, which close the creation panel and return us to the ChartSelectionScene.
From there, the user clicks on a chart in the existing chart list, which plays that chart in the ChartPerformanceScene.
Previously, the number of lanes of a chart was set as 4, controlled by asdf keys. With this new paradigm, the number of lanes is flexible,
depending on the number of (track, channel) pairs checked in the previous step. The first lane should be mapped to the a key, then s, d, f, g, h, and 
so on across the central row of the keyboard.