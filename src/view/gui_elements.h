#pragma once
#include <arctic/engine/easy.h>
#include <map>
#include "model/simulation_params.h"

namespace arctic {

extern Font GFont;

struct GuiElements {
    std::shared_ptr<GuiTheme> Theme;
    std::shared_ptr<Panel> Gui;
    Font Font;

    std::shared_ptr<Text> TextDisksPerDc;
    std::shared_ptr<Text> TextDiskSize;
    std::shared_ptr<Text> TextFailureRate;
    std::shared_ptr<Text> TextDataLoss;
    std::shared_ptr<Text> TextStats;
    std::shared_ptr<Text> TextSpareDisks;
    std::shared_ptr<Text> TextWriteSpeed;

    std::shared_ptr<Scrollbar> ScrollDisksPerDc;
    std::shared_ptr<Scrollbar> ScrollDiskSize;
    std::shared_ptr<Scrollbar> ScrollFailureRate;
    std::shared_ptr<Scrollbar> ScrollSpareDisks;
    std::shared_ptr<Scrollbar> ScrollWriteSpeed;
};

void InitializeGui(GuiElements& gui);
void UpdateGuiText(GuiElements& gui);
void DrawSimulation(const std::map<Si64, Si64>& probabilityByDay);
void DrawCumulative(const std::map<Si64, Si64>& valuesByCount, Rgba color, 
                   Vec2Si32 position, Vec2Si32 size, const char* title);

void HandleMouseInteraction(const std::vector<Vec2D>& points, 
                          Vec2Si32 position, Vec2Si32 size, 
                          Vec2D scale);

void DrawLabelsAndMouseInteraction(const std::vector<Vec2D>& points, 
                                 Vec2Si32 position, Vec2Si32 size, 
                                 Vec2D scale, const char* title);

} // namespace arctic 