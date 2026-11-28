#include "Includes/CoreTopology.h"
#include "Includes/CoreRegistry.h"
#include "Includes/CoreDependency.h"

static ArmitoModuleID topology_order[ARMITO_TOPOLOGY_MAX_MODULES];
static uint32_t topology_count = 0;
static uint8_t initialized = 0;
static uint8_t built = 0;

ArmitoCoreStatus ArmitoCoreTopologyInitialize(void)
{
    if (initialized)
        return ARMITO_CORE_ALREADY_INITIALIZED;

    topology_count = 0;
    built = 0;

    for (uint32_t i = 0; i < ARMITO_TOPOLOGY_MAX_MODULES; i++)
        topology_order[i] = ARMITO_MODULE_ID_INVALID;

    initialized = 1;

    return ARMITO_CORE_SUCCESS;
}

ArmitoCoreStatus ArmitoCoreTopologyBuild(void)
{
    if (!initialized)
        return ARMITO_CORE_NOT_INITIALIZED;

    topology_count = 0;
    built = 0;

    uint32_t registered_count = ArmitoCoreRegistryCount();

    if (registered_count > ARMITO_TOPOLOGY_MAX_MODULES)
        return ARMITO_CORE_OUT_OF_RESOURCES;

    if (registered_count == 0) {
        built = 1;
        return ARMITO_CORE_SUCCESS;
    }

    ArmitoCoreStatus status =
        ArmitoCoreDependencyResolve(
            topology_order,
            ARMITO_TOPOLOGY_MAX_MODULES,
            &topology_count
        );

    if (status != ARMITO_CORE_SUCCESS) {
        topology_count = 0;
        return status;
    }

    if (topology_count != registered_count) {
        topology_count = 0;
        return ARMITO_CORE_DEPENDENCY_ERROR;
    }

    built = 1;

    return ARMITO_CORE_SUCCESS;
}

ArmitoCoreStatus ArmitoCoreTopologyGetOrder(
    ArmitoModuleID *ordered_modules,
    uint32_t capacity,
    uint32_t *module_count)
{
    if (!initialized)
        return ARMITO_CORE_NOT_INITIALIZED;

    if (!built)
        return ARMITO_CORE_INVALID_STATE;

    if (ordered_modules == NULL || module_count == NULL)
        return ARMITO_CORE_ERROR;

    if (capacity < topology_count)
        return ARMITO_CORE_OUT_OF_RESOURCES;

    for (uint32_t i = 0; i < topology_count; i++)
        ordered_modules[i] = topology_order[i];

    *module_count = topology_count;

    return ARMITO_CORE_SUCCESS;
}

ArmitoCoreStatus ArmitoCoreTopologyGetPosition(
    ArmitoModuleID module_id,
    uint32_t *position)
{
    if (!initialized)
        return ARMITO_CORE_NOT_INITIALIZED;

    if (!built)
        return ARMITO_CORE_INVALID_STATE;

    if (module_id == ARMITO_MODULE_ID_INVALID || position == NULL)
        return ARMITO_CORE_ERROR;

    for (uint32_t i = 0; i < topology_count; i++) {
        if (topology_order[i] == module_id) {
            *position = i;
            return ARMITO_CORE_SUCCESS;
        }
    }

    return ARMITO_CORE_ERROR;
}

void ArmitoCoreTopologyReset(void)
{
    for (uint32_t i = 0; i < ARMITO_TOPOLOGY_MAX_MODULES; i++)
        topology_order[i] = ARMITO_MODULE_ID_INVALID;

    topology_count = 0;
    built = 0;
    initialized = 0;
}
