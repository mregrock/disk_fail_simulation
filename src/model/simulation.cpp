#include "simulation.h"
#include <cmath>
#include <map>
#include <vector>
#include <unordered_set>
#include <algorithm>
#include "../utils/logger.h"
#include "group.h"
#include "vdisk.h"
#include "pdisk.h"
#include <random>

namespace arctic {

void Simulation::Reset() {
    CurrentTime = 0;

    LostGroupInfo.clear();

    PDiskMap.clear();
    VDiskMap.clear();
    GroupMap.clear();

    for (int dc = 0; dc < 3; ++dc) {
        sparePDiskIdsByDC[dc].clear();
    }

    InitializePDisks();
    InitializeGroups();
}

void Simulation::InitializePDisks() {
    Ui32 pdiskIdCounter = 0;
    Ui32 vdiskIdCounter = 0;

    for (int dcId = 0; dcId < 3; ++dcId) {
        for (int pdiskIndex = 0; pdiskIndex < GDisksPerDc; ++pdiskIndex) {
            TPDiskId pdiskId = TPDiskId::FromValue(pdiskIdCounter);
            PDiskMap[pdiskId] = std::make_shared<TPDisk>(pdiskId, TDCId(dcId), TPDisk::Active);
            for (int vdiskIndex = 0; vdiskIndex < 9; ++vdiskIndex) {
                TVDiskId vdiskId = TVDiskId::FromValue(vdiskIdCounter);
                const auto vdisk = std::make_shared<TVDisk>(vdiskId, pdiskId, TDCId(dcId));
                VDiskMap[vdiskId] = vdisk;
                PDiskMap[pdiskId]->AddVDisk(vdisk);
                vdiskIdCounter++;
            }
            pdiskIdCounter++;
        }
    }

    for (int dcId = 0; dcId < 3; ++dcId) {
        for (int spareIndex = 0; spareIndex < GSpareDisksPerDc; ++spareIndex) {
            TPDiskId pdiskId = TPDiskId::FromValue(pdiskIdCounter);
            PDiskMap[pdiskId] = std::make_shared<TPDisk>(pdiskId, TDCId(dcId), TPDisk::Spare);
            for (int vdiskIndex = 0; vdiskIndex < 9; ++vdiskIndex) {
                TVDiskId vdiskId = TVDiskId::FromValue(vdiskIdCounter);
                const auto vdisk = std::make_shared<TVDisk>(vdiskId, pdiskId, TDCId(dcId));
                VDiskMap[vdiskId] = vdisk;
                PDiskMap[pdiskId]->AddVDisk(vdisk);
                vdiskIdCounter++;
            }
            pdiskIdCounter++;
        }
    }

    LOG_DEBUG("Initialized " + std::to_string(PDiskMap.size()) + " PDisks (" +
             std::to_string(GDisksPerDc * 3) + " Active, " +
             std::to_string(GSpareDisksPerDc * 3) + " Spare) and " +
             std::to_string(VDiskMap.size()) + " VDisks.");
}

void Simulation::InitializeGroups() {
    const int kNumDCs = 3;
    const int kVDisksPerDCInGroup = 3;
    TGroupId currentGroupId = TGroupId::FromValue(0);
    std::vector<std::vector<TVDiskId>> availableVDisksByDC(kNumDCs);
    for (const auto& [vdiskId, vdiskPtr] : VDiskMap) {
        if (vdiskPtr) {
            availableVDisksByDC[vdiskPtr->GetDCId()].push_back(vdiskId);
        } else {
             LOG_WARNING("Found nullptr TVDisk in VDiskMap with ID " + std::to_string(vdiskId.GetRawId()));
        }
    }
    while (true) {
        std::vector<std::vector<TVDiskId>> selectedVDisksPerDC(kNumDCs);
        bool possibleToCreateGroup = true;

        for (int dc = 0; dc < kNumDCs; ++dc) {
            std::unordered_set<TPDiskId> pdisks_in_group_for_dc;
            for (const TVDiskId& vdiskId : availableVDisksByDC[dc]) {
                auto vdiskIt = VDiskMap.find(vdiskId);
                if (vdiskIt == VDiskMap.end() || !vdiskIt->second) continue;

                TPDiskId pdiskId = vdiskIt->second->GetPDiskId();

                if (pdisks_in_group_for_dc.find(pdiskId) == pdisks_in_group_for_dc.end()) {
                    selectedVDisksPerDC[dc].push_back(vdiskId);
                    pdisks_in_group_for_dc.insert(pdiskId);
                    if (selectedVDisksPerDC[dc].size() == kVDisksPerDCInGroup) {
                        break;
                    }
                }
            }
            if (selectedVDisksPerDC[dc].size() < kVDisksPerDCInGroup) {
                possibleToCreateGroup = false;
                break;
            }
        }
        if (!possibleToCreateGroup) {
            break;
        }

        auto group = std::make_shared<TGroup>(currentGroupId);
        GroupMap[currentGroupId] = group;
        for (int dc = 0; dc < kNumDCs; ++dc) {
            for (const TVDiskId& vdiskId : selectedVDisksPerDC[dc]) {
                auto vdisk = VDiskMap[vdiskId];
                vdisk->AssignToGroup(currentGroupId);
                group->AddVDisk(vdisk, TDCId(dc));
            }
        }

        for (int dc = 0; dc < kNumDCs; ++dc) {
            std::unordered_set<TVDiskId> ids_to_remove(selectedVDisksPerDC[dc].begin(), selectedVDisksPerDC[dc].end());
            availableVDisksByDC[dc].erase(
                std::remove_if(availableVDisksByDC[dc].begin(), availableVDisksByDC[dc].end(),
                               [&](const TVDiskId& id){ return ids_to_remove.count(id); }),
                availableVDisksByDC[dc].end()
            );
        }

        currentGroupId = TGroupId::FromValue(currentGroupId.GetRawId() + 1);
    }

    LOG_DEBUG("Initialized " + std::to_string(currentGroupId.GetRawId()) + " groups with unique PDisks per DC.");
}

void Simulation::SimulateHour(std::mt19937& rng) {
    double failuresThisHour = GFailureRate / 24.0;
    Si32 failures = static_cast<Si32>(failuresThisHour);

    std::uniform_real_distribution<double> dist(0.0, 1.0);
    if (dist(rng) < (failuresThisHour - failures)) {
        failures++;
    }

    ProcessFailures(failures, rng);
    ProcessGroups();
    CompleteReplications();

    CurrentTime += 1.0;
}

void Simulation::ProcessFailures(Si32 failures, std::mt19937& rng) {
    Si32 successful_failures = 0;
    const int max_attempts_for_one_failure = PDiskMap.size() > 0 ? PDiskMap.size() : 1;
    int attempts_since_last_success = 0;

    std::vector<TPDiskId> pdiskIds;
    pdiskIds.reserve(PDiskMap.size());
    for(const auto& pair : PDiskMap) {
        pdiskIds.push_back(pair.first);
    }

    if (pdiskIds.empty()) {
        LOG_WARNING("ProcessFailures: No PDisks available to fail.");
        return;
    }

    while (successful_failures < failures) {
        if (attempts_since_last_success >= max_attempts_for_one_failure) {
            LOG_WARNING("ProcessFailures: Exceeded attempt limit (" +
                         std::to_string(max_attempts_for_one_failure) +
                         ") to find a non-broken disk for failure " + std::to_string(successful_failures + 1) +
                         " of " + std::to_string(failures) + ". Stopping failure processing for this hour.");
            break;
        }

        std::uniform_int_distribution<Si32> pdisk_dist(0, pdiskIds.size() - 1);
        Si32 randomIndex = pdisk_dist(rng);
        TPDiskId randomPDiskId = pdiskIds[randomIndex];
        auto pdiskIt = PDiskMap.find(randomPDiskId);

        if (pdiskIt == PDiskMap.end() || !pdiskIt->second) {
             LOG_WARNING("ProcessFailures: Randomly selected PDisk ID " + randomPDiskId.ToString() + " not found or is null.");
             attempts_since_last_success++;
             continue;
        }

        auto& pdisk = pdiskIt->second;

        if (pdisk->GetState() != TPDisk::Broken) {
            pdisk->Fail();
            successful_failures++;
            attempts_since_last_success = 0;
            LOG_DEBUG("PDisk failed: ID=" + randomPDiskId.ToString() + ", DC=" + std::to_string(pdisk->GetDCId()));
        } else {
            attempts_since_last_success++;
            LOG_DEBUG("ProcessFailures: Attempted to fail already broken PDisk ID " + randomPDiskId.ToString() + " (Attempt " + std::to_string(attempts_since_last_success) + ")");
        }
    }
}

void Simulation::ProcessGroups() {
    for (auto const& [groupId, groupPtr] : GroupMap) {
        if (!groupPtr) continue;

        if (LostGroupInfo.count(groupId)) {
            continue;
        }

        if (groupPtr->CheckDataLoss()) {
            LostGroupInfo[groupId] = CurrentTime;
            LOG_DEBUG("Data loss detected in group " + std::to_string(groupId.GetRawId()) + " at time " + std::to_string(CurrentTime));
            continue;
        }

        std::vector<TVDiskId> faultyVDisksToReplicate;
        for (const auto& vdiskId : groupPtr->GetAllVDiskIds()) {
            auto vdiskIt = VDiskMap.find(vdiskId);
            if (vdiskIt != VDiskMap.end() && vdiskIt->second) {
                const auto& vdisk = vdiskIt->second;
                if (vdisk->GetState() == TVDisk::Faulty && !vdisk->IsReplicationTriggered()) {
                    faultyVDisksToReplicate.push_back(vdiskId);
                }
            }
        }

        for (const auto& faultyVDiskId : faultyVDisksToReplicate) {
            auto faultyVDiskIt = VDiskMap.find(faultyVDiskId);
            if (faultyVDiskIt == VDiskMap.end() || !faultyVDiskIt->second) continue;
            auto faultyVDisk = faultyVDiskIt->second;
            TDCId dcId = faultyVDisk->GetDCId();

            std::shared_ptr<TPDisk> bestSparePDisk = nullptr;
            int maxSlots = -1;

            for (const auto& [pdiskId, pdiskPtr] : PDiskMap) {
                if (pdiskPtr && pdiskPtr->GetDCId() == dcId && pdiskPtr->GetState() == TPDisk::Spare) {
                    int currentSlots = pdiskPtr->GetAvailableVDiskSlots();
                    if (currentSlots > 0 && currentSlots > maxSlots) {
                        maxSlots = currentSlots;
                        bestSparePDisk = pdiskPtr;
                    }
                }
            }

            if (bestSparePDisk) {
                bestSparePDisk->DecrementAvailableVDiskSlots();

                double replicationDurationHours = (GWriteSpeed > 0) ? (static_cast<double>(GDiskSize) * 1024.0 / GWriteSpeed) / 3600.0 : std::numeric_limits<double>::infinity();
                double completeTime = CurrentTime + replicationDurationHours;

                faultyVDisk->MarkReplicationTriggered(completeTime);

                LOG_DEBUG("Started replication for VDisk " + std::to_string(faultyVDiskId.GetRawId()) +
                          " (Group " + std::to_string(groupId.GetRawId()) +
                          ") onto Spare PDisk " + std::to_string(bestSparePDisk->GetId().GetRawId()) +
                          " (DC " + std::to_string(dcId) +
                          "). Slots left: " + std::to_string(bestSparePDisk->GetAvailableVDiskSlots()) +
                          ". Completion at T+" + std::to_string(replicationDurationHours));
            } else {
                 faultyVDisk->MarkReplicationTriggered(0);
                 faultyVDisk->SetState(TVDisk::Faulty);

                LOG_DEBUG("No suitable Spare PDisk found in DC " + std::to_string(dcId) +
                          " for VDisk " + std::to_string(faultyVDiskId.GetRawId()) +
                          " (Group " + std::to_string(groupId.GetRawId()) + "). Replication cannot start.");
            }
        }
    }
}

void Simulation::CompleteReplications() {
    for (auto& [vdiskId, vdiskPtr] : VDiskMap) {
        if (vdiskPtr && vdiskPtr->GetState() == TVDisk::Replicating) {
             if (CurrentTime >= vdiskPtr->GetReplicationCompleteTime()) {
                 vdiskPtr->SetState(TVDisk::Replicated);
                 LOG_DEBUG("Replication complete for VDisk " + std::to_string(vdiskId.GetRawId()) +
                           " (Group " + std::to_string(vdiskPtr->GetGroupId().GetRawId()) +
                           ") at time " + std::to_string(CurrentTime));
             }
        }
    }
}

}