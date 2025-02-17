#include "pdisk.h"
#include "vdisk.h"
#include <stdexcept>
#include "../utils/logger.h"

namespace arctic {

extern Ui32 GVDisksPerPDisk;

TPDisk::TPDisk(TPDiskId id, TDCId dcId, DiskState initialState)
    : Id(id), DCId(dcId), State(initialState) {
    AvailableVDiskSlots = GVDisksPerPDisk;
}

void TPDisk::AddVDisk(std::shared_ptr<TVDisk> vdisk) {
    if (vdisk) {
        VDisks[vdisk->GetId()] = vdisk;
    } else {

    }
}

void TPDisk::SetState(DiskState state) {
    State = state;
    if (State == Broken) {
        AvailableVDiskSlots = 0;
    }
}

TPDisk::DiskState TPDisk::GetState() const {
    return State;
}

TPDiskId TPDisk::GetId() const {
    return Id;
}

TDCId TPDisk::GetDCId() const {
    return DCId;
}

void TPDisk::DecrementAvailableVDiskSlots() {
    if (State != Broken && AvailableVDiskSlots > 0) {
        --AvailableVDiskSlots;
    } else if (State == Broken) {


    } else {


    }
}

void TPDisk::Fail(double currentTime) {
    if (State != Broken) {
        LOG_DEBUG("PDisk Failing: ID=" + Id.ToString() + ", Current State=" + std::to_string(State) + " at Time=" + std::to_string(currentTime));
        State = Broken;
        BrokenTime = currentTime;
        AvailableVDiskSlots = 0;

        for (auto& [vdiskId, vdiskPtr] : VDisks) {
            if (vdiskPtr && vdiskPtr->GetState() != TVDisk::Faulty) {
                vdiskPtr->SetState(TVDisk::Faulty);
                LOG_DEBUG("  VDisk marked Faulty: ID=" + vdiskId.ToString());
            }
        }
    } else {

    }
}

double TPDisk::GetBrokenTime() const {
    return BrokenTime;
}

void TPDisk::Recover() {
    if (State == Broken) {
        State = Spare;
        AvailableVDiskSlots = GVDisksPerPDisk;
        BrokenTime = 0.0;
        LOG_DEBUG("PDisk Recovered as Spare: ID=" + Id.ToString());
    } else {

    }
}

} 