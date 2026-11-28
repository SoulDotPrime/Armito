#ifndef ARMITO_CORE_DISPATCH_H
#define ARMITO_CORE_DISPATCH_H

#include "../Includes/Core.h"

typedef ArmitoCoreStatus (*ArmitoDispatchFunction)(
    void *context
);

ArmitoCoreStatus ArmitoCoreDispatchInitialize(void);

ArmitoCoreStatus ArmitoCoreDispatchExecute(
    ArmitoModuleID module_id,
    ArmitoDispatchFunction function,
    void *context
);

ArmitoCoreStatus ArmitoCoreDispatchIsAvailable(
    ArmitoModuleID module_id
);

void ArmitoCoreDispatchReset(void);

#endif