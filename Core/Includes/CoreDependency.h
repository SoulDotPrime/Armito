#ifndef ARMITO_CORE_DEPENDENCY_H
#define ARMITO_CORE_DEPENDENCY_H

#include "../Includes/Core.h"

ArmitoCoreStatus ArmitoCoreDependencyInitialize(void);

ArmitoCoreStatus ArmitoCoreDependencyAdd(
    ArmitoModuleID module_id,
    ArmitoModuleID dependency_id
);

ArmitoCoreStatus ArmitoCoreDependencyValidate(void);

ArmitoCoreStatus ArmitoCoreDependencyResolve(
    ArmitoModuleID *ordered_modules,
    uint32_t capacity,
    uint32_t *resolved_count
);

uint32_t ArmitoCoreDependencyCount(
    ArmitoModuleID module_id
);

void ArmitoCoreDependencyReset(void);

#endif
