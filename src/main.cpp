#include <arctic/engine/easy.h>
#include "controller/simulation_controller.h"
#include <iostream>
#include "../utils/logger.h"
using namespace arctic;

void EasyMain() {
    LOG_DEBUG("Application starting");
    SimulationController controller;
    controller.Initialize();
    LOG_DEBUG("Controller initialized");

    while (!IsKeyDownward(kKeyEscape)) {
        LOG_DEBUG("Main loop iteration start");
        controller.ProcessInput();
        controller.Update();
        controller.Draw();
        LOG_DEBUG("Main loop iteration end");
    }
    LOG_DEBUG("Application closing");
} 