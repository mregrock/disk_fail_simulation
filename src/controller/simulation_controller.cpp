#include "simulation_controller.h"
#include <map>
#include "../utils/logger.h"

namespace arctic {

const Ui32 kScreenWidth = 1920;
const Ui32 kScreenHeight = 1080;

void SimulationController::Initialize() {
    LOG("Initializing SimulationController"); 
    try {
        ResizeScreen(kScreenWidth, kScreenHeight);
        LOG_DEBUG("Screen resized to " + std::to_string(kScreenWidth) + "x" + std::to_string(kScreenHeight));

        GTheme = std::make_shared<GuiTheme>();

        LOG("GUI theme loaded successfully");

        GFont.Load("data/arctic_one_bmf.fnt");

        InitializeGui(Gui);
        Sim.Reset();
        DataLossByDay.clear();
        TotalSimsByDay.clear();
        GSims = 0;
    } catch (const std::exception& e) {
        LOG_ERROR("Exception in Initialize: " + std::string(e.what()));
    } catch (...) {
        LOG_ERROR("Unknown exception in Initialize");
    }
    LOG("SimulationController initialized");
}

void SimulationController::Update() {
    LOG_DEBUG("Update start");
    HandleGuiEvents();

    if (GDoRestart) {
        LOG_DEBUG("Restarting simulation");
        GDoRestart = false;
        Sim.Reset();
        DataLossByDay.clear();
        TotalSimsByDay.clear();
        GSims = 0;
    }

    static bool simulationInProgress = false;
    if (!simulationInProgress) {
        LOG_DEBUG("Starting new simulation frame");
        simulationInProgress = true;
        RunSimulation();
        UpdateStatistics();
        simulationInProgress = false;
        LOG_DEBUG("Simulation frame completed");
    } else {
        LOG_DEBUG("Simulation already in progress, skipping");
    }
    LOG_DEBUG("Update end");
}

void SimulationController::ProcessInput() {
    LOG_DEBUG("Processing input start");
    for (Si32 messageIndex = 0; messageIndex < InputMessageCount(); ++messageIndex) {
        Gui.Gui->ApplyInput(GetInputMessage(messageIndex), nullptr);
    }
    LOG_DEBUG("Processing input end");
}

void SimulationController::RunSimulation() {
    LOG_DEBUG("Starting simulation run");
    Sim.Reset();
    bool hadDataLoss = false;

    const Si32 maxIterations = 30 * 24;
    Si32 iterations = 0;

    for (Si32 day = 0; day < 30; ++day) {
        TotalSimsByDay[day]++;

        for (Si32 hour = 0; hour < 24; ++hour) {
            iterations++;
            if (iterations > maxIterations) {
                LOG_ERROR("Simulation exceeded maximum iterations!");
                return;
            }

            Sim.SimulateHour();

            if (!Sim.LostGroupInfo.empty() && !hadDataLoss) {
                LOG_DEBUG("Data loss occurred on day " + std::to_string(day));
                DataLossByDay[day]++;
                hadDataLoss = true;
                break;
            }
        }

        if (hadDataLoss) {
            for (Si32 remainingDay = day + 1; remainingDay < 30; ++remainingDay) {
                TotalSimsByDay[remainingDay]++;
                DataLossByDay[remainingDay]++;
            }
            break;
        }
    }
    GSims++;
    LOG_DEBUG("Simulation completed. Total sims: " + std::to_string(GSims));
}

void SimulationController::UpdateStatistics() {
    if (!TotalSimsByDay.empty()) {
        GDataLossProb = static_cast<double>(DataLossByDay[29]) / 
                       static_cast<double>(TotalSimsByDay[29]);
        LOG_DEBUG("Updated probability: " + std::to_string(GDataLossProb));
        LOG_DEBUG("Data losses on day 29: " + std::to_string(DataLossByDay[29]));
        LOG_DEBUG("Total sims on day 29: " + std::to_string(TotalSimsByDay[29]));
    }
}

void SimulationController::HandleGuiEvents() {
    if (GDisksPerDc != Gui.ScrollDisksPerDc->GetValue()) {
        GDisksPerDc = Gui.ScrollDisksPerDc->GetValue();
        GDoRestart = true;
        std::stringstream str;
        str << "Disks per DC: " << GDisksPerDc;
        Gui.TextDisksPerDc->SetText(str.str());
    }
    if (GDiskSize != Gui.ScrollDiskSize->GetValue()) {
        GDiskSize = Gui.ScrollDiskSize->GetValue();
        GDoRestart = true;
        std::stringstream str;
        str << "Disk Size: " << GDiskSize << " GB";
        Gui.TextDiskSize->SetText(str.str());
    }
    if (GFailureRate != Gui.ScrollFailureRate->GetValue()) {
        GFailureRate = Gui.ScrollFailureRate->GetValue();
        GDoRestart = true;
        std::stringstream str;
        str << "Failure Rate: " << GFailureRate << " disks/day";
        Gui.TextFailureRate->SetText(str.str());
    }
    if (GSpareDisksPerDc != Gui.ScrollSpareDisks->GetValue()) {
        GSpareDisksPerDc = Gui.ScrollSpareDisks->GetValue();
        GDoRestart = true;
        std::stringstream str;
        str << "Spare Disks per DC: " << GSpareDisksPerDc;
        Gui.TextSpareDisks->SetText(str.str());
    }
    if (GWriteSpeed != Gui.ScrollWriteSpeed->GetValue()) {
        GWriteSpeed = Gui.ScrollWriteSpeed->GetValue();
        GDoRestart = true;
        std::stringstream str;
        str << "Write Speed: " << GWriteSpeed << " MB/s";
        Gui.TextWriteSpeed->SetText(str.str());
    }
}

void SimulationController::Draw() {
    LOG_DEBUG("Draw start");
    Clear();

    std::map<Si64, Si64> probabilityByDay;
    Si32 totalLosses = 0;

    for (Si32 day = 0; day < 30; ++day) {
        if (TotalSimsByDay[day] > 0) {
            totalLosses += DataLossByDay[day];
            double probability = static_cast<double>(totalLosses) / 
                               static_cast<double>(TotalSimsByDay[day]);
            probabilityByDay[day] = static_cast<Si64>(probability * 1000000);
        }
    }

    DrawSimulation(probabilityByDay);
    UpdateGuiText(Gui);
    Gui.Gui->Draw(Vec2Si32(0, 0));
    ShowFrame();
    LOG_DEBUG("Draw end");
}

} 