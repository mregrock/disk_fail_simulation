#pragma once
#include <arctic/engine/easy.h>
#include "vdisk.h"
#include "group.h"
#include "id_wrapper.h"
#include <unordered_map>
#include <memory>

namespace arctic {

using TPDiskId = TIdWrapper<Ui32, TPDiskIdTag>;

class TVDisk;

class TPDisk {
public:
    enum DiskState {
        Active,
        Broken,
        Spare
    };

    TPDisk(TPDiskId id, TDCId dcId, DiskState initialState = DiskState::Active);
    void AddVDisk(std::shared_ptr<TVDisk> vdisk);
    void SetState(DiskState state);
    DiskState GetState() const;
    TPDiskId GetId() const;
    TDCId GetDCId() const;
    int GetAvailableVDiskSlots() const { return AvailableVDiskSlots; }
    void DecrementAvailableVDiskSlots();
    void Fail(double currentTime);
    double GetBrokenTime() const;
    void Recover();

private:
    TPDiskId Id;
    TDCId DCId;
    std::unordered_map<TVDiskId, std::shared_ptr<TVDisk>> VDisks;
    int AvailableVDiskSlots = 9;
    DiskState State = Active;
    double BrokenTime = 0.0;
};

} // namespace arctic