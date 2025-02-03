#pragma once
#include <arctic/engine/easy.h>

namespace arctic {

struct Disk {
    enum DiskState { Active, Inactive, Faulty };
    DiskState State = Active;
    double InactiveTime = 0;
    double RebuildTime = 0;
    Ui32 DcId = 0;
    Si32 GroupId = -1;
};

} // namespace arctic 