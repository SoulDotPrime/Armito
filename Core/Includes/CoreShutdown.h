#ifndef ARMITO_CORE_SHUTDOWN_H
#define ARMITO_CORE_SHUTDOWN_H

#include "../../Includes/Core.h"

ArmitoCoreStatus ArmitoCoreShutdownManagerInitialize(void);
ArmitoCoreStatus ArmitoCoreShutdownManagerRequest(void);
ArmitoCoreStatus ArmitoCoreShutdownManagerExecute(void);

uint8_t ArmitoCoreShutdownManagerIsRequested(void);
uint8_t ArmitoCoreShutdownManagerIsComplete(void);

void ArmitoCoreShutdownManagerReset(void);

#endif