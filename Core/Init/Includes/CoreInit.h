#ifndef ARMITO_CORE_INIT_H
#define ARMITO_CORE_INIT_H

#include "../../Includes/Core.h"

ArmitoCoreStatus ArmitoCoreInitPrepare(void);

ArmitoCoreStatus ArmitoCoreInitExecute(void);

ArmitoCoreStatus ArmitoCoreInitStart(void);

void ArmitoCoreInitShutdown(void);

#endif