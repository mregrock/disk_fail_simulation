#pragma once
#include <arctic/engine/easy.h>

namespace arctic {

struct Disk {
    enum DiskState { Active, Faulty, Spare, Rebuilding };
    DiskState State = Spare;
    double RebuildCompleteTime = 0;
    bool RebuildTriggered = false;
    Ui32 DcId = 0;
    Si32 GroupId = -1;
};

} // namespace arctic 