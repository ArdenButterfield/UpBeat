//
// Created by Arden on 9/5/2026.
//

#include "ChartCreationPanel.h"

#include "../BundledResources.h"

namespace
{
    constexpr int rowHeight = 40;
    constexpr int rowSpacing = 6;
    constexpr int margin = 10;
}

ChartCreationPanel::ChartCreationPanel()
{
    for (auto& entry : BundledResources::listEntries ("default_charts/midi"))
        addMidiSource (entry, BundledResources::loadFile (entry));

    addAndMakeVisible (uploadButton);
    uploadButton.addListener (this);

    addAndMakeVisible (createButton);
    createButton.addListener (this);

    addAndMakeVisible (cancelButton);
    cancelButton.addListener (this);
}

ChartCreationPanel::~ChartCreationPanel()
{
}

void ChartCreationPanel::addMidiSource (const juce::String& name, const juce::MemoryBlock& data)
{
    auto index = (int) midiSources.size();
    midiSources.push_back ({ name, data });

    auto button = std::make_unique<ChartSelectionButton> (name, index);
    addAndMakeVisible (button.get());
    button->addListener (this);
    midiFileButtons.push_back (std::move (button));
}

void ChartCreationPanel::openFileBrowser()
{
    fileChooser = std::make_unique<juce::FileChooser> ("Select a MIDI file to upload", juce::File(), "*.mid;*.midi");

    fileChooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this] (const juce::FileChooser& chooser)
        {
            auto file = chooser.getResult();
            if (! file.existsAsFile())
                return;

            juce::MemoryBlock data;
            if (file.loadFileAsData (data))
            {
                addMidiSource (file.getFileNameWithoutExtension(), data);
                resized();
            }
        });
}

void ChartCreationPanel::selectMidiSource (int index)
{
    if (index < 0 || index >= (int) midiSources.size())
        return;

    selectedMidiIndex = index;
    currentTrackChannels = Chart::analyzeTrackChannels (midiSources[(size_t) index].data);

    trackChannelToggles.clear();
    for (auto& info : currentTrackChannels)
    {
        auto label = "Trk " + juce::String (info.track) + " Ch " + juce::String (info.channel) + ": " + juce::String (info.noteCount) + " notes";
        auto* toggle = trackChannelToggles.add (new juce::ToggleButton (label));
        addAndMakeVisible (toggle);
    }

    resized();
}

void ChartCreationPanel::buttonClicked (juce::Button* b)
{
    if (b == &uploadButton)
    {
        openFileBrowser();
        return;
    }

    if (b == &createButton)
    {
        if (selectedMidiIndex < 0 || ! onChartCreated)
            return;

        std::vector<std::pair<int, int>> selectedTrackChannels;
        for (size_t i = 0; i < currentTrackChannels.size(); ++i)
        {
            if (trackChannelToggles[(int) i]->getToggleState())
            {
                auto& info = currentTrackChannels[i];
                selectedTrackChannels.push_back ({ info.track, info.channel });
            }
        }

        if (selectedTrackChannels.empty())
            return;

        auto& source = midiSources[(size_t) selectedMidiIndex];
        onChartCreated (Chart (source.data, source.name, selectedTrackChannels));
        return;
    }

    if (b == &cancelButton)
    {
        if (onCancel)
            onCancel();
        return;
    }

    if (auto* midiButton = dynamic_cast<ChartSelectionButton*> (b))
    {
        selectMidiSource (midiButton->index);
    }
}

void ChartCreationPanel::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black.withAlpha (0.9f));
    g.setColour (juce::Colours::white);
    g.setFont (20);

    auto bounds = getLocalBounds().reduced (margin);
    auto halfWidth = bounds.getWidth() / 2;

    g.drawText ("MIDI Files", bounds.removeFromLeft (halfWidth).withHeight (30), juce::Justification::centred, 1);

    auto rightBounds = getLocalBounds().reduced (margin);
    rightBounds.removeFromLeft (halfWidth);
    g.drawText ("Tracks / Channels", rightBounds.withHeight (30), juce::Justification::centred, 1);
}

void ChartCreationPanel::resized()
{
    auto bounds = getLocalBounds().reduced (margin);
    auto bottomButtons = bounds.removeFromBottom (rowHeight);
    cancelButton.setBounds (bottomButtons.removeFromRight (120));
    bottomButtons.removeFromRight (rowSpacing);
    createButton.setBounds (bottomButtons.removeFromRight (120));

    auto leftBounds = bounds.removeFromLeft (bounds.getWidth() / 2).withTrimmedTop (30);
    bounds.removeFromLeft (margin);
    auto rightBounds = bounds.withTrimmedTop (30);

    auto uploadBounds = leftBounds.removeFromBottom (rowHeight);
    uploadButton.setBounds (uploadBounds);
    leftBounds.removeFromBottom (rowSpacing);

    for (auto& button : midiFileButtons)
    {
        button->setBounds (leftBounds.removeFromTop (rowHeight));
        leftBounds.removeFromTop (rowSpacing);
    }

    for (auto* toggle : trackChannelToggles)
    {
        toggle->setBounds (rightBounds.removeFromTop (rowHeight));
        rightBounds.removeFromTop (rowSpacing);
    }
}
