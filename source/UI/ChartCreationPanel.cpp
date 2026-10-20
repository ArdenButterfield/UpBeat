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
    for (auto& entry : BundledResources::listEntries ("default_charts/yaml"))
        if (entry.endsWithIgnoreCase (".yaml") || entry.endsWithIgnoreCase (".yml"))
            addChartSource (entry.fromLastOccurrenceOf ("/", false, false).upToLastOccurrenceOf (".", false, false),
                BundledResources::loadFile (entry).toString());

    addAndMakeVisible (uploadButton);
    uploadButton.addListener (this);

    errorLabel.setColour (juce::Label::textColourId, juce::Colours::red);
    addAndMakeVisible (errorLabel);

    addAndMakeVisible (cancelButton);
    cancelButton.addListener (this);
}

ChartCreationPanel::~ChartCreationPanel()
{
}

void ChartCreationPanel::addChartSource (const juce::String& name, const juce::String& yamlText)
{
    auto index = (int) chartSources.size();
    chartSources.push_back ({ name, yamlText });

    auto button = std::make_unique<ChartSelectionButton> (name, index);
    addAndMakeVisible (button.get());
    button->addListener (this);
    chartFileButtons.push_back (std::move (button));
}

void ChartCreationPanel::openFileBrowser()
{
    fileChooser = std::make_unique<juce::FileChooser> ("Select a chart file to upload", juce::File(), "*.yaml;*.yml");

    fileChooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this] (const juce::FileChooser& chooser)
        {
            auto file = chooser.getResult();
            if (! file.existsAsFile())
                return;

            addChartSource (file.getFileNameWithoutExtension(), file.loadFileAsString());
            resized();
        });
}

void ChartCreationPanel::createChartFromSource (int index)
{
    if (index < 0 || index >= (int) chartSources.size() || ! onChartCreated)
        return;

    auto& source = chartSources[(size_t) index];
    auto chart = Chart::fromYaml (source.yamlText);
    if (! chart.has_value() || chart->numLanes == 0)
    {
        errorLabel.setText ("\"" + source.name + "\" is not a valid chart with playable notes", juce::dontSendNotification);
        return;
    }

    if (chart->name.isEmpty())
        chart->name = source.name;

    onChartCreated (std::move (*chart));
}

void ChartCreationPanel::buttonClicked (juce::Button* b)
{
    if (b == &uploadButton)
    {
        openFileBrowser();
        return;
    }

    if (b == &cancelButton)
    {
        if (onCancel)
            onCancel();
        return;
    }

    if (auto* chartButton = dynamic_cast<ChartSelectionButton*> (b))
    {
        createChartFromSource (chartButton->index);
    }
}

void ChartCreationPanel::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black.withAlpha (0.9f));
    g.setColour (juce::Colours::white);
    g.setFont (20);

    g.drawText ("Chart Files", getLocalBounds().reduced (margin).withHeight (30), juce::Justification::centred, 1);
}

void ChartCreationPanel::resized()
{
    auto bounds = getLocalBounds().reduced (margin);
    auto bottomButtons = bounds.removeFromBottom (rowHeight);
    cancelButton.setBounds (bottomButtons.removeFromRight (120));
    bottomButtons.removeFromRight (rowSpacing);
    errorLabel.setBounds (bottomButtons);

    bounds.removeFromBottom (rowSpacing);
    bounds.removeFromTop (30);

    uploadButton.setBounds (bounds.removeFromBottom (rowHeight));
    bounds.removeFromBottom (rowSpacing);

    for (auto& button : chartFileButtons)
    {
        button->setBounds (bounds.removeFromTop (rowHeight));
        bounds.removeFromTop (rowSpacing);
    }
}
