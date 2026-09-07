## Introduction

Practice mode is a counterpart to performance mode: like performance mode, it is a falling-note rhythm game chart, however
the game tracks your performance on each bar, providing repetitions of it until you get it right consistently.

You can get to practice mode or performance mode (already implemented in ChartPerformanceScene) using two buttons on a chart
title in ChartSelectionScene.

## Architecture

Within the practice mode scene, the chart is stored as a vector of BarAtom objects, corresponding to the individual measures, in order, of the piece.
Each BarAtom is much like a Chart: it has a length in milliseconds, a int learnedScore (initialized to 0), and a std::multimap<long long, ChartEvent>.

There is also the playQueue, which contains the next N (default 4) bars that will be practiced. The bar queue notes are what
fall in the grid, one bar after another with no gap, the way chart notes do in a chart performance.

The BarQueue algorithm is as follows:

at start, fill the unlearnedBarQueue with all BarAtoms. Pop the first unlearned bars into the playQueue.
Set the practice playhead time to the countInTime. At all times, display the playQueue notes on screen as in chart performance.

while the playQueue is not empty:
    advance the playhead, making note of chartevent performance as in ChartPerformanceScene
    when the playhead gets past the length of the first bar in the playQueue, pop that bar from the playQueue
    if all notes were just played in the Great or Perfect tolerance window:
        add 1 to the  bar's learnedScore
    if the learned score is >=3:
        the bar is fully learned. Pop the next unlearnedBarQueue member (unless queue is empty) and append to playQueue
    else:
        re-append the bar that was just popped to the back of the playQueue.

# Display

The central lanes look exactly the same as in the ChartPerformanceScene. Create new classes as needed to avoid repeating
code for this. On the right of the lanes, create a graphic with a small square for each bar, which start out transparent, turn yellow
while the bar is being learned, and finally turn green once the bar is fully learned.