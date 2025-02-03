#pragma once
#include <arctic/engine/easy.h>
#include "model/simulation.h"
#include "view/gui_elements.h"
#include <map>

namespace arctic {

extern const Ui32 kScreenWidth;
extern const Ui32 kScreenHeight;

class SimulationController {
public:
    void Initialize();
    void Update();
    void ProcessInput();
    void Draw();

private:
    void RunSimulation();
    void UpdateStatistics();
    void HandleGuiEvents();

    Simulation Sim;
    GuiElements Gui;
    std::map<Si32, Si32> DataLossByDay;
    std::map<Si32, Si32> TotalSimsByDay;
    bool DoRestart = false;
};

} // namespace arctic 