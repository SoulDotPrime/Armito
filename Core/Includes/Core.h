
#ifndef ARMITO_CORE_H
#define ARMITO_CORE_H

#include "CoreTypes.h"

typedef struct {
    ArmitoModuleID id;
    const char *name;
    ArmitoModulePriority priority;

    ArmitoCoreStatus (*initialize)(void);
    ArmitoCoreStatus (*start)(void);
    void (*shutdown)(void);

} ArmitoModuleDescriptor;

ArmitoCoreStatus ArmitoCoreRegisterModule(
    const ArmitoModuleDescriptor *module,
    ArmitoModuleID *assigned_id
);

ArmitoCoreStatus ArmitoCoreInitialize(void);
ArmitoCoreStatus ArmitoCoreStart(void);
void ArmitoCoreShutdown(void);

ArmitoModuleState ArmitoCoreGetModuleState(
    ArmitoModuleID id
);

#endif
