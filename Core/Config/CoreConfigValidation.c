#include "Includes/CoreConfigValidation.h"

ArmitoCoreStatus ArmitoCoreConfigValidate(
    const ArmitoCoreConfig *config
)
{
    if (config == NULL)
        return ARMITO_CORE_CONFIGURATION_ERROR;

    if (config->max_modules == 0 ||
        config->max_modules > 64)
        return ARMITO_CORE_CONFIGURATION_ERROR;

    if (config->max_dependencies == 0 ||
        config->max_dependencies > 16)
        return ARMITO_CORE_CONFIGURATION_ERROR;

    if (config->max_tasks == 0 ||
        config->max_tasks > 64)
        return ARMITO_CORE_CONFIGURATION_ERROR;

    if (config->max_ipc_messages == 0 ||
        config->max_ipc_messages > 64)
        return ARMITO_CORE_CONFIGURATION_ERROR;

    if (config->max_ipc_payload == 0 ||
        config->max_ipc_payload > 256)
        return ARMITO_CORE_CONFIGURATION_ERROR;

    if (config->enable_scheduler > 1 ||
        config->enable_ipc > 1 ||
        config->enable_monitor > 1 ||
        config->enable_diagnostics > 1)
        return ARMITO_CORE_CONFIGURATION_ERROR;

    return ARMITO_CORE_SUCCESS;
}