#ifndef ARMITO_CORE_MONITOR_H
#define ARMITO_CORE_MONITOR_H

#include "Includes/Core.h"

typedef struct {
    uint32_t total;
    uint32_t registered;
    uint32_t initializing;
    uint32_t initialized;
    uint32_t running;
    uint32_t stopped;
    uint32_t unregistered;
} ArmitoCoreMonitorReport;

ArmitoCoreStatus ArmitoCoreMonitorInitialize(void);

ArmitoCoreStatus ArmitoCoreMonitorGetModuleState(
    ArmitoModuleID module_id,
    ArmitoModuleState *state
);

ArmitoCoreStatus ArmitoCoreMonitorGetReport(
    ArmitoCoreMonitorReport *report
);

ArmitoCoreStatus ArmitoCoreMonitorIsHealthy(
    uint8_t *healthy
);

void ArmitoCoreMonitorReset(void);

#endif