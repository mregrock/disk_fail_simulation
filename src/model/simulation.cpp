#include "simulation.h"
#include <cmath>
#include <map>
#include "../utils/logger.h"
namespace arctic {

void Simulation::Reset() {
    Dcs.resize(3);
    CurrentTime = 0;
    LostData = 0;
    
    for (Si32 dc = 0; dc < 3; ++dc) {
        Dcs[dc].Disks.resize(GDisksPerDc + GSpareDisksPerDc);
        Dcs[dc].SpareDisks.clear();
        
        for (Si32 i = 0; i < GDisksPerDc; ++i) {
            Dcs[dc].Disks[i].State = Disk::Active;
            Dcs[dc].Disks[i].DcId = dc;
        }
        
        for (Si32 i = GDisksPerDc; i < GDisksPerDc + GSpareDisksPerDc; ++i) {
            Dcs[dc].SpareDisks.push_back(i);
            Dcs[dc].Disks[i].State = Disk::Inactive;
            Dcs[dc].Disks[i].DcId = dc;
        }
    }
    
    InitializeGroups();
}

void Simulation::InitializeGroups() {
    Si32 groupId = 0;
    std::vector<bool> usedDisks[3];
    for (Si32 dc = 0; dc < 3; ++dc) {
        usedDisks[dc].resize(GDisksPerDc, false);
    }
    
    while (true) {
        bool found = false;
        Si32 diskIndices[3];
        
        for (Si32 dc = 0; dc < 3; ++dc) {
            for (Si32 i = 0; i < GDisksPerDc; ++i) {
                if (!usedDisks[dc][i]) {
                    diskIndices[dc] = i;
                    found = true;
                    break;
                }
            }
            if (!found) break;
        }
        
        if (!found) break;
        
        for (Si32 dc = 0; dc < 3; ++dc) {
            usedDisks[dc][diskIndices[dc]] = true;
            Dcs[dc].Disks[diskIndices[dc]].GroupId = groupId;
        }
        groupId++;
    }
}

void Simulation::SimulateHour() {
    
    double failuresThisHour = GFailureRate / 24.0;
    Si32 failures = static_cast<Si32>(failuresThisHour);
    
    if (Random32() % 1000000 < (failuresThisHour - failures) * 1000000) {
        failures++;
    }
    
    ProcessFailures(failures);
    ProcessGroups();
    CompleteRebuilds();
    
    CurrentTime += 1.0;
}

void Simulation::ProcessFailures(Si32 failures) {
    for (Si32 i = 0; i < failures; ++i) {
        Si32 dcIndex = Random32() % 3;
        Si32 diskIndex = Random32() % GDisksPerDc;
        
        Disk& disk = Dcs[dcIndex].Disks[diskIndex];
        if (disk.State == Disk::Active && disk.GroupId >= 0) {
            disk.State = Disk::Inactive;
            disk.InactiveTime = CurrentTime;
        }
    }
}

void Simulation::ProcessGroups() {
    std::map<Si32, std::vector<Disk*>> disksByGroup;
    for (auto& dc : Dcs) {
        for (auto& disk : dc.Disks) {
            if (disk.GroupId >= 0) {
                disksByGroup[disk.GroupId].push_back(&disk);
            }
        }
    }

    LOG_DEBUG("Processing " + std::to_string(disksByGroup.size()) + " groups");

    for (const auto& group : disksByGroup) {
        const auto& groupDisks = group.second;
        
        Si32 activeDisks = 0;
        Si32 inactiveDisks = 0;
        Si32 faultyDisks = 0;
        
        for (const auto* disk : groupDisks) {
            if (disk->State == Disk::Active) activeDisks++;
            else if (disk->State == Disk::Inactive) inactiveDisks++;
            else if (disk->State == Disk::Faulty) faultyDisks++;
        }

        if (inactiveDisks > 0) {
            for (auto* disk : groupDisks) {
                if (disk->State == Disk::Inactive && CurrentTime >= disk->InactiveTime + 1.0) {
                    LOG_DEBUG("Found inactive disk in group " + std::to_string(group.first) + 
                             " at time " + std::to_string(CurrentTime) + 
                             ", inactive since " + std::to_string(disk->InactiveTime));
                    
                    if (CurrentTime >= disk->InactiveTime + 1.0) {
                        auto& dc = Dcs[disk->DcId];
                        if (!dc.SpareDisks.empty()) {
                            Si32 spareIndex = dc.SpareDisks.back();
                            dc.SpareDisks.pop_back();
                            
                            Disk& newDisk = dc.Disks[spareIndex];
                            newDisk.State = Disk::Inactive;
                            newDisk.GroupId = group.first;
                            newDisk.RebuildTime = CurrentTime + (GDiskSize * 1024.0 / GWriteSpeed) / 3600.0;
                            
                            disk->State = Disk::Faulty;
                            LOG_DEBUG("Started rebuild for disk in group " + std::to_string(group.first));
                        } else {
                            disk->State = Disk::Faulty;
                            LOG_DEBUG("No spare disks available for group " + std::to_string(group.first));
                        }
                    }
                }
            }
        }

        activeDisks = 0;
        inactiveDisks = 0;
        faultyDisks = 0;
        
        for (const auto* disk : groupDisks) {
            if (disk->State == Disk::Active) activeDisks++;
            else if (disk->State == Disk::Inactive) inactiveDisks++;
            else if (disk->State == Disk::Faulty) faultyDisks++;
        }

        if (activeDisks == 0 && (faultyDisks + inactiveDisks) == groupDisks.size()) {
            LOG_DEBUG("Data loss detected in group " + std::to_string(group.first));
            LostData++;
        }
    }
}

void Simulation::CompleteRebuilds() {
    for (auto& dc : Dcs) {
        for (auto& disk : dc.Disks) {
            if (disk.State == Disk::Inactive && CurrentTime >= disk.RebuildTime) {
                disk.State = Disk::Active;
            }
        }
    }
}

}