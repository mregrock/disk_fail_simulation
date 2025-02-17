#include "group.h"
#include "vdisk.h"
#include "../utils/logger.h"
#include <vector>      
#include <numeric>     
#include <algorithm>   

namespace arctic {

TGroup::TGroup(TGroupId id) {
    Id = id;
}

void TGroup::AddVDisk(std::shared_ptr<TVDisk> vdisk, TDCId dcId) {
    VDisksByDC[dcId].push_back(vdisk->GetId());
    VDisks[vdisk->GetId()] = vdisk;
    AllVDiskIds.insert(vdisk->GetId());
}

bool TGroup::CheckDataLoss() const {
    std::vector<int> failedVDiskPerDc(3, 0); 
    for (const auto& vdiskId : AllVDiskIds) {

        auto vdiskIt = VDisks.find(vdiskId);
        if (vdiskIt == VDisks.end() || !vdiskIt->second) {
             LOG_ERROR("VDisk ID " + vdiskId.ToString() + " not found in group " + Id.ToString() + " during CheckDataLoss.");
             continue;
        }
        const auto& vdisk = vdiskIt->second;


        if (vdisk->GetState() == TVDisk::Faulty || vdisk->GetState() == TVDisk::Replicating) {
            TDCId dcId = vdisk->GetDCId();

            if (dcId < failedVDiskPerDc.size()) {
                 failedVDiskPerDc[dcId]++;
            } else {
                 LOG_ERROR("Invalid DC ID " + std::to_string(dcId) + " encountered for VDisk " + vdiskId.ToString() + " in group " + Id.ToString());
            }
        }
    }


    int failedDCsCount = 0;
    for (int count : failedVDiskPerDc) {
        if (count >= 2) { 
            return true;
        }
        if (count == 1) {
            failedDCsCount++;
        }
    }
    if (failedDCsCount >= 2) { 
        return true;
    }

    return false; 
}

std::vector<TVDiskId> TGroup::GetAllVDiskIds() const {
    return std::vector<TVDiskId>(AllVDiskIds.begin(), AllVDiskIds.end());
}

} 