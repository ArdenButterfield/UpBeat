//
// Created by Arden on 7/8/2026.
//

#ifndef UPBEAT_FEEDBACKSCENE_H
#define UPBEAT_FEEDBACKSCENE_H
#include "Scene.h"
#include "../UI/LineGraph.h"

class FeedbackScene : public Scene, public juce::Button::Listener
{
public:
    explicit FeedbackScene(GameState* gs);
    ~FeedbackScene() override;
    void paint(juce::Graphics& g) override;
    void update() override;
    void resized() override;
    SceneIDs::SceneID getSceneID() const override;
    SceneIDs::SceneID getDesiredSceneID() override;
private:
    SceneIDs::SceneID desiredSceneId;
    LineGraph performanceGraph;
    juce::TextButton retryButton;

    void buildPerformanceGraph();
    void buttonClicked(juce::Button*) override;
};

#endif //UPBEAT_FEEDBACKSCENE_H
