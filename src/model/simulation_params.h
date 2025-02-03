#pragma once
#include <arctic/engine/easy.h>
#include <memory>

namespace arctic {

extern Ui32 GDisksPerDc;
extern Ui32 GDiskSize;
extern Ui32 GFailureRate;
extern Ui32 GSpareDisksPerDc;
extern Ui32 GWriteSpeed;
extern Ui64 GSims;
extern double GDataLossProb;
extern bool GDoRestart;

extern std::shared_ptr<GuiTheme> GTheme;

} // namespace arctic 