#pragma once
#include "pdisk.h"
#include "vdisk.h"
#include "group.h"
#include "simulation_params.h"
#include <map>
#include <vector>
#include <random>
#include <memory>
#include <unordered_map>

namespace arctic {

class Simulation {
public:
    void Reset();
    void SimulateHour(std::mt19937& rng);

    std::unordered_map<TPDiskId, std::shared_ptr<TPDisk>> PDiskMap;
    std::unordered_map<TVDiskId, std::shared_ptr<TVDisk>> VDiskMap;
    std::unordered_map<TGroupId, std::shared_ptr<TGroup>> GroupMap;
    std::vector<TPDiskId> PDiskIdsByDC[3];
    double CurrentTime = 0;
    std::map<TGroupId, double> LostGroupInfo;

    std::vector<TPDiskId> sparePDiskIdsByDC[3];

private:
    void InitializePDisks();
    void InitializeGroups();
    void ProcessFailures(Si32 failures, std::mt19937& rng);
    void ProcessGroups();
    void CompleteReplications();
    void ProcessRecoveries();
};

extern Ui32 GDisksPerDc;
extern Ui32 GDiskSize;
extern Ui32 GFailureRate;
extern Ui32 GSpareDisksPerDc;
extern Ui32 GWriteSpeed;
extern Ui64 GSims;
extern double GDataLossProb;
} // namespace arctic 