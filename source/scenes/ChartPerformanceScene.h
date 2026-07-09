//
// Created by Arden on 7/8/2026.
//

#ifndef UPBEAT_CHARTPERFORMANCESCENE_H
#define UPBEAT_CHARTPERFORMANCESCENE_H

#include "Scene.h"

class ChartPerformanceScene : public Scene
{
public:
    ChartPerformanceScene();
    ~ChartPerformanceScene();
private:
    void update() override;
    void paint(juce::Graphics& g) override;
};

#endif //UPBEAT_CHARTPERFORMANCESCENE_H
