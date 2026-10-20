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
// Lists chart YAML files (bundled defaults plus any the user has uploaded); clicking one creates
// a chart from it. Which notes are playable is defined by the YAML file itself.
class ChartCreationPanel : public juce::Component, public juce::Button::Listener
{
public:
    ChartCreationPanel();
    ~ChartCreationPanel() override;

    void paint (juce::Graphics& g) override;
    void resized() override;
    void buttonClicked (juce::Button*) override;

    // Called with the newly built chart when the user clicks a chart file.
    std::function<void (Chart)> onChartCreated;
    // Called when the user clicks "Cancel".
    std::function<void()> onCancel;

private:
    struct ChartSource
    {
        juce::String name;
        juce::String yamlText;
    };

    void addChartSource (const juce::String& name, const juce::String& yamlText);
    void createChartFromSource (int index);
    void openFileBrowser();

    std::vector<ChartSource> chartSources;
    std::vector<std::unique_ptr<ChartSelectionButton>> chartFileButtons;
    juce::TextButton uploadButton { "Upload your own chart" };
    juce::Label errorLabel;

    juce::TextButton cancelButton { "Cancel" };

    std::unique_ptr<juce::FileChooser> fileChooser;
};

#endif //UPBEAT_CHARTCREATIONPANEL_H
