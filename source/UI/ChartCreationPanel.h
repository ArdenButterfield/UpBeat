//
// Created by Arden on 9/5/2026.
//

#ifndef UPBEAT_CHARTCREATIONPANEL_H
#define UPBEAT_CHARTCREATIONPANEL_H

#include "../Chart/Chart.h"
#include "ChartSelectionButton.h"
#include "juce_gui_basics/juce_gui_basics.h"

#include <functional>
#include <memory>
#include <vector>

// The chart creation panel shown when the user clicks "New Chart" in the ChartSelectionScene.
// Left half: a list of MIDI files (bundled defaults plus any the user has uploaded) to choose from.
// Right half, once a MIDI file is chosen: a checkbox per (track, channel) pair found in it, so the
// user can pick which ones become playable lanes (in the order checked lanes appear in the list).
class ChartCreationPanel : public juce::Component, public juce::Button::Listener
{
public:
    ChartCreationPanel();
    ~ChartCreationPanel() override;

    void paint (juce::Graphics& g) override;
    void resized() override;
    void buttonClicked (juce::Button*) override;

    // Called with the newly built chart when the user clicks "Create".
    std::function<void (Chart)> onChartCreated;
    // Called when the user clicks "Cancel".
    std::function<void()> onCancel;

private:
    struct MidiSource
    {
        juce::String name;
        juce::MemoryBlock data;
    };

    void addMidiSource (const juce::String& name, const juce::MemoryBlock& data);
    void selectMidiSource (int index);
    void openFileBrowser();

    std::vector<MidiSource> midiSources;
    std::vector<std::unique_ptr<ChartSelectionButton>> midiFileButtons;
    juce::TextButton uploadButton { "Upload your own MIDI" };

    int selectedMidiIndex = -1;
    std::vector<TrackChannelInfo> currentTrackChannels;
    juce::OwnedArray<juce::ToggleButton> trackChannelToggles;

    juce::TextButton createButton { "Create" };
    juce::TextButton cancelButton { "Cancel" };

    std::unique_ptr<juce::FileChooser> fileChooser;
};

#endif //UPBEAT_CHARTCREATIONPANEL_H
