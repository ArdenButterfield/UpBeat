# A separate note lane display colour palette for each chart

Currently, the colours for the note lane display are set in GameColours.h. Instead, they should be set in the game state,
with options to automate them, just like how synth parameters can be automated in the game state.

Recall that the charts are created in Ableton, and converted to yaml using python. To facilitate the colour selection and
automation, create another vst plugin for setting the colours.

For each NoteLaneDisplay colour, this vst should have 3 float parameters, for the Lab of that colour. The colors should be
stored in Oklab in the yaml as well, so that linear automation between colours is smooth.

The GUI of this helper plugin should be a grid, with a square for each colour. In each square, have the title of that parameter
(that is, what it maps to in the game GUI), and three vertical sliders. The background of the square should be the current
colour of that parameter.

The python parser should look for this plugin on any of the tracks in the GUI group, and add the plugin parameter values
and automation events to the yaml chart file.