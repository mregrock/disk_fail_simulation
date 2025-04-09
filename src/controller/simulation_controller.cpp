#include "simulation_controller.h"
#include <map>
#include "../utils/logger.h"
#include <functional> 
#include <thread>
#include <future>

namespace arctic {

const Ui32 kScreenWidth = 1920;
const Ui32 kScreenHeight = 1080;


std::mt19937& GetThreadLocalRng() {
    thread_local static std::mt19937 rng(std::random_device{}() + 
                                         std::hash<std::thread::id>{}(std::this_thread::get_id()));
    return rng;
}

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

        DataLossByDay.clear();
        TotalSimsByDay.clear();
        GSims = 0;
    }

    const unsigned int num_threads = std::max(1u, std::thread::hardware_concurrency() - 1);
    LOG_DEBUG("Launching " + std::to_string(num_threads) + " simulation threads.");


    std::vector<std::future<SimulationResult>> futures;


    for (unsigned int i = 0; i < num_threads; ++i) {


        futures.push_back(std::async(std::launch::async, &SimulationController::RunSingleSimulation, this));
    }
    // Идея для оптимизации:
    // поток прерывает работу если получил дата лосс
    // потоки пишут в очередь
    // мы читаем по одному

    for (auto& fut : futures) {
        try {
            SimulationResult result = fut.get(); 


            GSims++;


            for (int day = 0; day < result.daysSimulated; ++day) {
                if (!TotalSimsByDay.count(day)) TotalSimsByDay[day] = 0;
                TotalSimsByDay[day]++;
            }


            if (result.lossDay != -1) {
                for (int d = result.lossDay; d < 30; ++d) {
                    if (!DataLossByDay.count(d)) DataLossByDay[d] = 0;
                    DataLossByDay[d]++;
                }
            }
        } catch (const std::exception& e) {
            LOG_ERROR("Exception caught while getting simulation result: " + std::string(e.what()));
        } catch (...) {
            LOG_ERROR("Unknown exception caught while getting simulation result.");
        }
    }


    UpdateStatistics();

    LOG_DEBUG("Update end, total simulations: " + std::to_string(GSims));
}

void SimulationController::ProcessInput() {
    LOG_DEBUG("Processing input start");
    for (Si32 messageIndex = 0; messageIndex < InputMessageCount(); ++messageIndex) {
        Gui.Gui->ApplyInput(GetInputMessage(messageIndex), nullptr);
    }
    LOG_DEBUG("Processing input end");
}


SimulationResult SimulationController::RunSingleSimulation() {

    auto& rng = GetThreadLocalRng();

    Simulation localSim; 
    localSim.Reset();    

    SimulationResult result;
    bool hadDataLoss = false;

    const Si32 maxIterations = 30 * 24; 
    Si32 iterations = 0;

    for (int day = 0; day < 30; ++day) {
        result.daysSimulated++; 

        for (int hour = 0; hour < 24; ++hour) {
            iterations++;
            if (iterations > maxIterations) {
                LOG_ERROR("Single simulation exceeded maximum iterations!");


                return result;
            }

            localSim.SimulateHour(rng);
            if (!localSim.LostGroupInfo.empty()) { 
                 if (!hadDataLoss) {
                     result.lossDay = day;
                     hadDataLoss = true;
                 }
                 break;
            }
        }
        if (hadDataLoss) {
            break;
        }
    }

    return result;
}

void SimulationController::RunSimulation() {
    LOG_DEBUG("Starting simulation run");
    Sim.Reset();
    bool hadDataLoss = false;
    int lossDay = -1; 

    const Si32 maxIterations = 30 * 24;
    Si32 iterations = 0;
    auto& rng = GetThreadLocalRng();

    for (Si32 day = 0; day < 30; ++day) {


        if (!TotalSimsByDay.count(day)) TotalSimsByDay[day] = 0;
        TotalSimsByDay[day]++;

        for (Si32 hour = 0; hour < 24; ++hour) {
            iterations++;
            if (iterations > maxIterations) {
                LOG_ERROR("Simulation exceeded maximum iterations!");

                 goto end_simulation; 
            }

            Sim.SimulateHour(rng);

            if (!Sim.LostGroupInfo.empty()) { 
                if (!hadDataLoss) { 
                    LOG_DEBUG("Data loss occurred on day " + std::to_string(day));
                    lossDay = day;
                    hadDataLoss = true;

                }

                break;
            }
        }
        if (hadDataLoss) {
            break;
        }
    }

end_simulation: 

    if (hadDataLoss) {
        for (Si32 d = lossDay; d < 30; ++d) {
            if (!DataLossByDay.count(d)) DataLossByDay[d] = 0;
            DataLossByDay[d]++;
        }
    }
    GSims++;
    LOG_DEBUG("Simulation completed. Total sims: " + std::to_string(GSims));
}

void SimulationController::UpdateStatistics() {
    if (GSims > 0) {
        double overallLossCount = DataLossByDay.count(29) ? DataLossByDay[29] : 0.0;
        GDataLossProb = overallLossCount / static_cast<double>(GSims);

        LOG_DEBUG("UpdateStatistics: Overall Stats - Total Losses: " + std::to_string(overallLossCount) +
                   ", Total Sims Run: " + std::to_string(GSims));
        LOG_DEBUG("UpdateStatistics: Calculated GDataLossProb = " + std::to_string(GDataLossProb));
    } else {
        GDataLossProb = 0.0;
        LOG_DEBUG("UpdateStatistics: No simulations run yet.");
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

    if (GPDiskRecoveryTimeHours != Gui.ScrollRecoveryTime->GetValue()) {
        GPDiskRecoveryTimeHours = Gui.ScrollRecoveryTime->GetValue();
        GDoRestart = true;
        std::stringstream str;
        str << "PDisk Recovery Time: " << GPDiskRecoveryTimeHours << " hours";
        Gui.TextRecoveryTime->SetText(str.str());
    }

    // Обработка слайдера VDisks/PDisk
    if (GVDisksPerPDisk != Gui.ScrollVDisksPerPDisk->GetValue()) {
        GVDisksPerPDisk = Gui.ScrollVDisksPerPDisk->GetValue();
        GDoRestart = true;
        std::stringstream str;
        str << "VDisks per PDisk: " << GVDisksPerPDisk;
        Gui.TextVDisksPerPDisk->SetText(str.str());
    }
}

void SimulationController::Draw() {
    LOG_DEBUG("Draw start");
    Clear();

    std::map<Si64, Si64> probabilityByDay;
    for (Si32 day = 0; day < 30; ++day) {
        if (TotalSimsByDay.count(day) && TotalSimsByDay[day] > 0) {
            double probability = static_cast<double>(DataLossByDay[day]) /
                               static_cast<double>(TotalSimsByDay[day]);
            probability = std::min(probability, 1.0);
            probabilityByDay[day] = static_cast<Si64>(probability * 1000000.0);
        } else {
            probabilityByDay[day] = 0;
        }
    }

    DrawSimulation(probabilityByDay);
    UpdateGuiText(Gui);
    Gui.Gui->Draw(Vec2Si32(0, 0));
    ShowFrame();
    LOG_DEBUG("Draw end");
}

} 