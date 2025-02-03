#pragma once
#include "data_center.h"
#include "simulation_params.h"

namespace arctic {

class Simulation {
public:
    void Reset();
    void SimulateHour();
    
    std::vector<DataCenter> Dcs;
    double CurrentTime = 0;
    Ui32 LostData = 0;

private:
    void InitializeGroups();
    void ProcessFailures(Si32 failures);
    void ProcessGroups();
    void CompleteRebuilds();
};

extern Ui32 GDisksPerDc;
extern Ui32 GDiskSize;
extern Ui32 GFailureRate;
extern Ui32 GSpareDisksPerDc;
extern Ui32 GWriteSpeed;
extern Ui64 GSims;
extern double GDataLossProb;
} // namespace arctic 