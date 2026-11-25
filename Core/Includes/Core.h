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

ArmitoCoreStatus ArmitoCoreRegisterDependency(
    ArmitoModuleID module_id,
    ArmitoModuleID dependency_id
);

ArmitoCoreStatus ArmitoCoreInitialize(void);
ArmitoCoreStatus ArmitoCoreStart(void);
void ArmitoCoreShutdown(void);

ArmitoCoreState ArmitoCoreGetStatus(void);

ArmitoModuleState ArmitoCoreGetModuleState(
    ArmitoModuleID id
);

#endif
