//
// Created by Arden on 9/5/2026.
//

#include "ChartSelectionButton.h"
#include "juce_audio_basics/juce_audio_basics.h"

#include <map>

namespace
{
    juce::StringArray summariseChannels (const juce::MemoryBlock& midiData)
    {
        juce::MidiFile midiFile;
        auto inputStream = juce::MemoryInputStream (midiData, false);
        midiFile.readFrom (inputStream);

        std::map<int, int> noteCountByChannel;
        for (int trackIndex = 0; trackIndex < midiFile.getNumTracks(); ++trackIndex)
        {
            auto* track = midiFile.getTrack (trackIndex);
            for (int i = 0; i < track->getNumEvents(); ++i)
            {
                auto& message = track->getEventPointer (i)->message;
                if (message.isNoteOn())
                    noteCountByChannel[message.getChannel()] += 1;
            }
        }

        juce::StringArray lines;
        for (auto& [channel, count] : noteCountByChannel)
            lines.add ("Ch " + juce::String (channel) + ": " + juce::String (count) + " notes");

        return lines;
    }
}

ChartSelectionButton::ChartSelectionButton (const juce::String& chartName, int chartIndex, const juce::MemoryBlock& midiData)
    : juce::TextButton (chartName), chartIndex (chartIndex), channelLines (summariseChannels (midiData))
{
}

void ChartSelectionButton::paintButton (juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    getLookAndFeel().drawButtonBackground (g, *this,
        findColour (getToggleState() ? buttonOnColourId : buttonColourId),
        shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);

    auto bounds = getLocalBounds().reduced (4);
    auto nameBounds = bounds.removeFromTop (bounds.getHeight() / 2);

    g.setColour (findColour (textColourOffId));
    g.setFont (juce::Font (16.0f, juce::Font::bold));
    g.drawFittedText (getButtonText(), nameBounds, juce::Justification::centred, 1);

    if (! channelLines.isEmpty())
    {
        g.setFont (juce::Font (12.0f));
        auto rowHeight = bounds.getHeight() / channelLines.size();
        for (auto& line : channelLines)
        {
            auto row = bounds.removeFromTop (rowHeight);
            g.drawFittedText (line, row, juce::Justification::centred, 1);
        }
    }
}
