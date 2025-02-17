#pragma once
#include <arctic/engine/easy.h>
#include "model/simulation.h"
#include "view/gui_elements.h"
#include <map>
#include <string>
#include <sstream>
#include <future>
#include <thread>
#include <vector>
#include <numeric>
#include <random>

namespace arctic {

extern const Ui32 kScreenWidth;
extern const Ui32 kScreenHeight;


struct SimulationResult {
    int lossDay = -1;         
    int daysSimulated = 0;    
};

class SimulationController : public Engine {
public:
    void Initialize();
    void Update();
    void ProcessInput();
    void Draw();

private:
    void RunSimulation();
    void UpdateStatistics();
    void HandleGuiEvents();
    SimulationResult RunSingleSimulation();

    Simulation Sim;
    GuiElements Gui;
    std::map<Si32, Si32> DataLossByDay;
    std::map<Si32, Si32> TotalSimsByDay;
    bool DoRestart = false;
};

} 