#include "simulation_params.h"

namespace arctic {

std::shared_ptr<GuiTheme> GTheme;

Ui32 GDisksPerDc = 100;
Ui32 GDiskSize = 4096;
Ui32 GFailureRate = 3;
Ui32 GSpareDisksPerDc = 10;
Ui32 GWriteSpeed = 100;
Ui64 GSims = 0;

Ui32 GPDiskRecoveryTimeHours = 24;
Ui32 GVDisksPerPDisk = 9;

double GDataLossProb = 0.0;
bool GDoRestart = true;

} // namespace arctic 