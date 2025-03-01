#include "simulation.h"
#include <cmath>
#include <map>
#include "../utils/logger.h"
namespace arctic {

void Simulation::Reset() {
    Dcs.resize(3);
    CurrentTime = 0;
    LostGroupInfo.clear();

    for (Si32 dc = 0; dc < 3; ++dc) {
        Dcs[dc].Disks.resize(GDisksPerDc + GSpareDisksPerDc);
        Dcs[dc].SpareDisks.clear();

        for (Si32 i = 0; i < GDisksPerDc; ++i) {
            Dcs[dc].Disks[i].State = Disk::Active;
            Dcs[dc].Disks[i].DcId = dc;
            Dcs[dc].Disks[i].RebuildTriggered = false;
            Dcs[dc].Disks[i].GroupId = -1;
        }

        for (Si32 i = GDisksPerDc; i < GDisksPerDc + GSpareDisksPerDc; ++i) {
            Dcs[dc].SpareDisks.push_back(i);
            Dcs[dc].Disks[i].State = Disk::Spare;
            Dcs[dc].Disks[i].DcId = dc;
            Dcs[dc].Disks[i].RebuildTriggered = false;
            Dcs[dc].Disks[i].GroupId = -1;
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
            disk.State = Disk::Faulty;
            disk.RebuildTriggered = false;
            LOG_DEBUG("Disk failed: DC=" + std::to_string(dcIndex) + ", Index=" + std::to_string(diskIndex) + ", Group=" + std::to_string(disk.GroupId));
        }
    }
}

void Simulation::ProcessGroups() {
    std::map<Si32, std::vector<Disk*>> disksByGroup;
    for (auto& dc : Dcs) {
        for (auto& disk : dc.Disks) {
            if (disk.GroupId >= 0 && (disk.State == Disk::Active || disk.State == Disk::Faulty || disk.State == Disk::Rebuilding)) {
                disksByGroup[disk.GroupId].push_back(&disk);
            }
        }
    }

    // LOG_DEBUG("Processing " + std::to_string(disksByGroup.size()) + " groups");

    for (auto const& [groupId, groupDisks] : disksByGroup) {
        Si32 faultyCount = 0;
        bool rebuildJustTriggeredInGroup = false;

        for (auto* disk : groupDisks) {
            if (disk->State == Disk::Faulty && !disk->RebuildTriggered) {
                auto& dc = Dcs[disk->DcId];
                if (!dc.SpareDisks.empty()) {
                    Ui32 spareDiskIndex = dc.SpareDisks.back();
                    dc.SpareDisks.pop_back();

                    Disk& spareDisk = dc.Disks[spareDiskIndex];
                    spareDisk.State = Disk::Rebuilding;
                    spareDisk.GroupId = groupId;
                    double rebuildDurationHours = (GWriteSpeed > 0) ? (static_cast<double>(GDiskSize) * 1024.0 / GWriteSpeed) / 3600.0 : std::numeric_limits<double>::infinity();
                    spareDisk.RebuildCompleteTime = CurrentTime + rebuildDurationHours;

                    disk->RebuildTriggered = true;
                    rebuildJustTriggeredInGroup = true;
                    LOG_DEBUG("Started rebuild for Group " + std::to_string(groupId) + 
                             ". Faulty disk (DC=" + std::to_string(disk->DcId) + ") replaced by Spare index " + 
                             std::to_string(spareDiskIndex) + ". Rebuild completion at T+" + std::to_string(rebuildDurationHours));
                } else {
                    disk->RebuildTriggered = true;
                    LOG_DEBUG("No spare disks in DC " + std::to_string(disk->DcId) + " for Group " + std::to_string(groupId) + ". Rebuild cannot start.");
                }
            }
        }

        if (LostGroupInfo.find(groupId) == LostGroupInfo.end()) {
             Si32 currentFaultyCount = 0;
             for (const auto* disk : groupDisks) {
                 if (disk->State == Disk::Faulty) {
                     currentFaultyCount++;
                 }
             }

             if (currentFaultyCount == groupDisks.size()) {
                 LostGroupInfo[groupId] = CurrentTime;
                 LOG_DEBUG("Data loss detected in group " + std::to_string(groupId) + " at time " + std::to_string(CurrentTime) + ". All disks are Faulty.");
             }
        }
    }
}

void Simulation::CompleteRebuilds() {
    for (auto& dc : Dcs) {
        for (auto& disk : dc.Disks) {
            if (disk.State == Disk::Rebuilding && CurrentTime >= disk.RebuildCompleteTime) {
                disk.State = Disk::Active;
                disk.RebuildCompleteTime = 0;
                LOG_DEBUG("Rebuild complete for disk in Group " + std::to_string(disk.GroupId) + " at DC " + std::to_string(disk.DcId) + " at time " + std::to_string(CurrentTime));
            }
        }
    }
}

}