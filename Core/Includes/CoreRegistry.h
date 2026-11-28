#ifndef ARMITO_CORE_REGISTRY_H
#define ARMITO_CORE_REGISTRY_H

#include "../../Includes/Core.h"

ArmitoCoreStatus ArmitoCoreRegistryInitialize(void);

ArmitoCoreStatus ArmitoCoreRegistryRegister(
    const ArmitoModuleDescriptor *module,
    ArmitoModuleID *assigned_id
);

const ArmitoModuleDescriptor *ArmitoCoreRegistryGet(
    ArmitoModuleID id
);

ArmitoModuleState ArmitoCoreRegistryGetState(
    ArmitoModuleID id
);

ArmitoCoreStatus ArmitoCoreRegistrySetState(
    ArmitoModuleID id,
    ArmitoModuleState state
);

uint32_t ArmitoCoreRegistryCount(void);

uint32_t ArmitoCoreRegistryCapacity(void);

ArmitoCoreStatus ArmitoCoreRegistryGetAt(
    uint32_t index,
    ArmitoModuleID *id
);

void ArmitoCoreRegistryReset(void);

#endif
