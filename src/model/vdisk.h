#pragma once
#include <arctic/engine/easy.h>
#include "pdisk.h"
#include "id_wrapper.h"

namespace arctic {

using TVDiskId = TIdWrapper<Ui32, TVDiskIdTag>;

class TGroup;

class TVDisk {
public:
    enum VDiskState {
        Active,
        Faulty,
        Replicating,
        Replicated
    };

    TVDisk(TVDiskId id, TPDiskId pdiskId, TDCId dcId);
    void AssignToGroup(TGroupId groupId);
    void SetState(VDiskState state);
    VDiskState GetState() const;
    TVDiskId GetId() const;
    TPDiskId GetPDiskId() const;
    TGroupId GetGroupId() const;
    TDCId GetDCId() const;
    bool IsReplicationTriggered() const { return ReplicationTriggered; }
    double GetReplicationCompleteTime() const { return ReplicationCompleteTime; }
    void MarkReplicationTriggered(double completeTime);

private:
    TVDiskId Id;
    TPDiskId PDiskId;
    TDCId DCId;
    TGroupId GroupId;
    VDiskState State = Active;
    bool ReplicationTriggered = false;
    double ReplicationCompleteTime = 0;
};

} // namespace arctic