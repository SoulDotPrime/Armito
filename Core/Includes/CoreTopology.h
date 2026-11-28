#ifndef ARMITO_CORE_TOPOLOGY_H
#define ARMITO_CORE_TOPOLOGY_H

#include "../Includes/Core.h"

#define ARMITO_TOPOLOGY_MAX_MODULES 64

ArmitoCoreStatus ArmitoCoreTopologyInitialize(void);

ArmitoCoreStatus ArmitoCoreTopologyBuild(void);

ArmitoCoreStatus ArmitoCoreTopologyGetOrder(
    ArmitoModuleID *ordered_modules,
    uint32_t capacity,
    uint32_t *module_count
);

ArmitoCoreStatus ArmitoCoreTopologyGetPosition(
    ArmitoModuleID module_id,
    uint32_t *position
);

void ArmitoCoreTopologyReset(void);

#endif