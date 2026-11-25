#include "Includes/CoreConfig.h"
#include "Includes/CoreConfigValidation.h"

static ArmitoCoreConfig core_config;
static uint8_t initialized = 0;
static uint8_t locked = 0;

static ArmitoCoreConfig DefaultConfig(void)
{
    ArmitoCoreConfig config = {
        ARMITO_CORE_MAX_MODULES,
        ARMITO_CORE_MAX_DEPENDENCIES,
        ARMITO_CORE_MAX_TASKS,
        ARMITO_CORE_MAX_IPC_MESSAGES,
        ARMITO_CORE_MAX_IPC_PAYLOAD,
        1,
        1,
        1,
        1
    };

    return config;
}

ArmitoCoreStatus ArmitoCoreConfigInitialize(void)
{
    if (initialized)
        return ARMITO_CORE_ALREADY_INITIALIZED;

    core_config = DefaultConfig();
    locked = 0;
    initialized = 1;

    return ARMITO_CORE_SUCCESS;
}

ArmitoCoreStatus ArmitoCoreConfigSet(
    const ArmitoCoreConfig *config
)
{
    if (!initialized)
        return ARMITO_CORE_NOT_INITIALIZED;

    if (locked)
        return ARMITO_CORE_INVALID_STATE;

    ArmitoCoreStatus status = ArmitoCoreConfigValidate(config);

    if (status != ARMITO_CORE_SUCCESS)
        return status;

    core_config = *config;

    return ARMITO_CORE_SUCCESS;
}

ArmitoCoreStatus ArmitoCoreConfigGet(
    ArmitoCoreConfig *config
)
{
    if (!initialized)
        return ARMITO_CORE_NOT_INITIALIZED;

    if (config == NULL)
        return ARMITO_CORE_ERROR;

    *config = core_config;

    return ARMITO_CORE_SUCCESS;
}

ArmitoCoreStatus ArmitoCoreConfigIsFeatureEnabled(
    ArmitoCoreFeature feature,
    uint8_t *enabled
)
{
    if (!initialized)
        return ARMITO_CORE_NOT_INITIALIZED;

    if (enabled == NULL)
        return ARMITO_CORE_ERROR;

    switch (feature) {
        case ARMITO_CORE_FEATURE_SCHEDULER:
            *enabled = core_config.enable_scheduler;
            break;

        case ARMITO_CORE_FEATURE_IPC:
            *enabled = core_config.enable_ipc;
            break;

        case ARMITO_CORE_FEATURE_MONITOR:
            *enabled = core_config.enable_monitor;
            break;

        case ARMITO_CORE_FEATURE_DIAGNOSTICS:
            *enabled = core_config.enable_diagnostics;
            break;

        default:
            return ARMITO_CORE_CONFIGURATION_ERROR;
    }

    return ARMITO_CORE_SUCCESS;
}

ArmitoCoreStatus ArmitoCoreConfigReset(void)
{
    if (!initialized)
        return ARMITO_CORE_NOT_INITIALIZED;

    if (locked)
        return ARMITO_CORE_INVALID_STATE;

    core_config = DefaultConfig();

    return ARMITO_CORE_SUCCESS;
}

ArmitoCoreStatus ArmitoCoreConfigShutdown(void)
{
    if (!initialized)
        return ARMITO_CORE_NOT_INITIALIZED;

    core_config = DefaultConfig();
    locked = 0;
    initialized = 0;

    return ARMITO_CORE_SUCCESS;
}

ArmitoCoreStatus ArmitoCoreConfigLock(void)
{
    if (!initialized)
        return ARMITO_CORE_NOT_INITIALIZED;

    locked = 1;

    return ARMITO_CORE_SUCCESS;
}

ArmitoCoreStatus ArmitoCoreConfigUnlock(void)
{
    if (!initialized)
        return ARMITO_CORE_NOT_INITIALIZED;

    locked = 0;

    return ARMITO_CORE_SUCCESS;
}

uint8_t ArmitoCoreConfigIsLocked(void)
{
    return locked;
}

uint8_t ArmitoCoreConfigIsInitialized(void)
{
    return initialized;
}
