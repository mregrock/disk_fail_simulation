#pragma once
#include "disk.h"
#include <vector>
#include <unordered_set>

namespace arctic {

struct DataCenter {
    std::vector<Disk> Disks;
    std::vector<Ui32> SpareDisks;
    std::unordered_set<Ui32> InactiveDisks;
    std::unordered_set<Ui32> FaultyDisks;
};

} // namespace arctic 