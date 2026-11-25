#include "Includes/CoreDispatch.h"
#include "Registry/Includes/CoreRegistry.h"

static uint8_t initialized = 0;

ArmitoCoreStatus ArmitoCoreDispatchInitialize(void)
{
    if (initialized)
        return ARMITO_CORE_ALREADY_INITIALIZED;

    initialized = 1;

    return ARMITO_CORE_SUCCESS;
}

ArmitoCoreStatus ArmitoCoreDispatchIsAvailable(
    ArmitoModuleID module_id)
{
    if (!initialized)
        return ARMITO_CORE_NOT_INITIALIZED;

    if (module_id == ARMITO_MODULE_ID_INVALID)
        return ARMITO_CORE_ERROR;

    if (ArmitoCoreRegistryGet(module_id) == NULL)
        return ARMITO_CORE_ERROR;

    ArmitoModuleState state =
        ArmitoCoreRegistryGetState(module_id);

    if (state != ARMITO_MODULE_RUNNING)
        return ARMITO_CORE_INVALID_STATE;

    return ARMITO_CORE_SUCCESS;
}

ArmitoCoreStatus ArmitoCoreDispatchExecute(
    ArmitoModuleID module_id,
    ArmitoDispatchFunction function,
    void *context)
{
    if (!initialized)
        return ARMITO_CORE_NOT_INITIALIZED;

    if (function == NULL)
        return ARMITO_CORE_ERROR;

    ArmitoCoreStatus status =
        ArmitoCoreDispatchIsAvailable(module_id);

    if (status != ARMITO_CORE_SUCCESS)
        return status;

    return function(context);
}

void ArmitoCoreDispatchReset(void)
{
    initialized = 0;
}