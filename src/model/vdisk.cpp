#include "vdisk.h"
#include "group.h"

namespace arctic {

TVDisk::TVDisk(TVDiskId id, TPDiskId pdiskId, TDCId dcId)
    : Id(id), PDiskId(pdiskId), DCId(dcId), State(VDiskState::Active) {

}

void TVDisk::AssignToGroup(TGroupId groupId) {
    GroupId = groupId;
}

void TVDisk::SetState(VDiskState state) {
    State = state;
    if (State != VDiskState::Replicating) {


    }
}

TVDisk::VDiskState TVDisk::GetState() const {
    return State;
}

TVDiskId TVDisk::GetId() const {
    return Id;
}

TPDiskId TVDisk::GetPDiskId() const {
    return PDiskId;
}

TGroupId TVDisk::GetGroupId() const {
    return GroupId;
}

TDCId TVDisk::GetDCId() const {
    return DCId;
}

void TVDisk::MarkReplicationTriggered(double completeTime) {
    if (State == VDiskState::Faulty) {
         ReplicationTriggered = true;
         if (completeTime > 0) {
            State = VDiskState::Replicating;
            ReplicationCompleteTime = completeTime;
         } else {
            ReplicationCompleteTime = 0;
         }
    } else {


    }
}

} 
