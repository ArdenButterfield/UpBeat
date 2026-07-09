//
// Created by Arden on 7/7/2026.
//

#ifndef UPBEAT_CHARTSELECTIONSCENE_H
#define UPBEAT_CHARTSELECTIONSCENE_H

#include "Scene.h"

class ChartSelectionScene : public Scene
{
public:
    ChartSelectionScene();
    ~ChartSelectionScene();
    void update() override;
    void paint(juce::Graphics& g) override;

};

#endif //UPBEAT_CHARTSELECTIONSCENE_H
