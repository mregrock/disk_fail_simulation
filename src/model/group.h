#pragma once
#include <arctic/engine/easy.h>
#include <unordered_set>
#include "id_wrapper.h"

namespace arctic {

using TGroupId = TIdWrapper<Ui32, TGroupIdTag>;

class TVDisk;

class TGroup {
public:
    TGroup(TGroupId id);
    void AddVDisk(std::shared_ptr<TVDisk> vdisk, TDCId dcId);
    bool MakeVDiskFaulty(TVDiskId vdiskId);
    bool CheckDataLoss() const;
    std::vector<TVDiskId> GetAllVDiskIds() const;
private:
    TGroupId Id;
    std::unordered_map<TDCId, std::vector<TVDiskId>> VDisksByDC;
    std::unordered_map<TVDiskId, std::shared_ptr<TVDisk>> VDisks;
    std::unordered_set<TVDiskId> AllVDiskIds;
};
}