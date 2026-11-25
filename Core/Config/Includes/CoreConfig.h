#ifndef ARMITO_CORE_CONFIG_H
#define ARMITO_CORE_CONFIG_H

#include "../../Includes/Core.h"

typedef enum {
    ARMITO_CORE_FEATURE_SCHEDULER = 0,
    ARMITO_CORE_FEATURE_IPC,
    ARMITO_CORE_FEATURE_MONITOR,
    ARMITO_CORE_FEATURE_DIAGNOSTICS
} ArmitoCoreFeature;

typedef struct {
    uint32_t max_modules;
    uint32_t max_dependencies;
    uint32_t max_tasks;
    uint32_t max_ipc_messages;
    uint32_t max_ipc_payload;
    uint8_t enable_scheduler;
    uint8_t enable_ipc;
    uint8_t enable_monitor;
    uint8_t enable_diagnostics;
} ArmitoCoreConfig;

ArmitoCoreStatus ArmitoCoreConfigInitialize(void);

ArmitoCoreStatus ArmitoCoreConfigSet(
    const ArmitoCoreConfig *config
);

ArmitoCoreStatus ArmitoCoreConfigGet(
    ArmitoCoreConfig *config
);

ArmitoCoreStatus ArmitoCoreConfigIsFeatureEnabled(
    ArmitoCoreFeature feature,
    uint8_t *enabled
);

ArmitoCoreStatus ArmitoCoreConfigReset(void);
ArmitoCoreStatus ArmitoCoreConfigShutdown(void);

ArmitoCoreStatus ArmitoCoreConfigLock(void);
ArmitoCoreStatus ArmitoCoreConfigUnlock(void);

uint8_t ArmitoCoreConfigIsLocked(void);
uint8_t ArmitoCoreConfigIsInitialized(void);

#endif