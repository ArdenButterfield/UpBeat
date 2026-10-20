Rather than having the notes fall and be hit when they reach the fixed point of the bottom of the screen, the chart
performance and chart practice interfaces will present the notes horizontally in bar units, with a moving playhead. Accomplishing
this will require a complete rewrite of the NoteLaneDisplay class.

The display will show 4 wide rectangles, stacked vertically, showing the notes in the current bar and the bars that follow.
So performance mode will start out displaying bars 1,2,3,4 and after the completion of bar 1 it will show bars 5,2,3,4, then
5,6,3,4, and so on: in both modes, after completing a bar, we replace that one with the next bar in the queue.

Within each bar, display the notes as circles where the x value corresponds to how far through the bar the note occurs,
and the y value corresponds to the pitch relative to the other notes in the bar (higher pitch=higher on the screen). Also
draw faint vertical lines for each beat within the bar. Color code the notes based on which lane they are in.

Draw a moving vertical playhead that travels through the current bar. When a note is played, remove the note from the
screen and replace it with the text label for how well it is played in the same spot. Fade out the text label, as before.
When the playhead passes a note that has not yet been played, have the note travel with the playhead until the end of the
timing window, then disappear.