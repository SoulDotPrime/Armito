#include "Includes/CoreRegistry.h"
#include "Config/Includes/CoreConfig.h"

typedef struct {
    ArmitoModuleDescriptor descriptor;
    ArmitoModuleState state;
    uint8_t registered;
} ArmitoRegistryEntry;

static ArmitoRegistryEntry registry[ARMITO_CORE_MAX_MODULES];
static uint32_t module_count = 0;
static ArmitoModuleID next_id = 1;
static uint8_t registry_initialized = 0;

ArmitoCoreStatus ArmitoCoreRegistryInitialize(void)
{
    if (registry_initialized)
        return ARMITO_CORE_ALREADY_INITIALIZED;

    for (uint32_t i = 0; i < ARMITO_CORE_MAX_MODULES; i++) {
        registry[i].registered = 0;
        registry[i].state = ARMITO_MODULE_UNREGISTERED;
    }

    module_count = 0;
    next_id = 1;
    registry_initialized = 1;

    return ARMITO_CORE_SUCCESS;
}

ArmitoCoreStatus ArmitoCoreRegistryRegister(
    const ArmitoModuleDescriptor *module,
    ArmitoModuleID *assigned_id)
{
    if (!registry_initialized)
        return ARMITO_CORE_NOT_INITIALIZED;

    if (module == NULL)
        return ARMITO_CORE_ERROR;

    if (module->name == NULL ||
        module->initialize == NULL ||
        module->start == NULL)
        return ARMITO_CORE_ERROR;

    ArmitoCoreConfig config;

    ArmitoCoreStatus status = ArmitoCoreConfigGet(&config);

    if (status != ARMITO_CORE_SUCCESS)
        return status;

    if (module_count >= config.max_modules ||
        module_count >= ARMITO_CORE_MAX_MODULES)
        return ARMITO_CORE_OUT_OF_RESOURCES;

    for (uint32_t i = 0; i < ARMITO_CORE_MAX_MODULES; i++) {
        if (!registry[i].registered) {
            registry[i].descriptor = *module;
            registry[i].descriptor.id = next_id;
            registry[i].state = ARMITO_MODULE_REGISTERED;
            registry[i].registered = 1;

            if (assigned_id != NULL)
                *assigned_id = next_id;

            next_id++;
            module_count++;

            return ARMITO_CORE_SUCCESS;
        }
    }

    return ARMITO_CORE_OUT_OF_RESOURCES;
}

const ArmitoModuleDescriptor *ArmitoCoreRegistryGet(ArmitoModuleID id)
{
    if (!registry_initialized || id == ARMITO_MODULE_ID_INVALID)
        return NULL;

    for (uint32_t i = 0; i < ARMITO_CORE_MAX_MODULES; i++) {
        if (registry[i].registered &&
            registry[i].descriptor.id == id)
            return &registry[i].descriptor;
    }

    return NULL;
}

ArmitoModuleState ArmitoCoreRegistryGetState(ArmitoModuleID id)
{
    if (!registry_initialized)
        return ARMITO_MODULE_UNREGISTERED;

    for (uint32_t i = 0; i < ARMITO_CORE_MAX_MODULES; i++) {
        if (registry[i].registered &&
            registry[i].descriptor.id == id)
            return registry[i].state;
    }

    return ARMITO_MODULE_UNREGISTERED;
}

ArmitoCoreStatus ArmitoCoreRegistrySetState(
    ArmitoModuleID id,
    ArmitoModuleState state)
{
    if (!registry_initialized)
        return ARMITO_CORE_NOT_INITIALIZED;

    for (uint32_t i = 0; i < ARMITO_CORE_MAX_MODULES; i++) {
        if (registry[i].registered &&
            registry[i].descriptor.id == id) {
            registry[i].state = state;
            return ARMITO_CORE_SUCCESS;
        }
    }

    return ARMITO_CORE_ERROR;
}

uint32_t ArmitoCoreRegistryCount(void)
{
    return module_count;
}

uint32_t ArmitoCoreRegistryCapacity(void)
{
    return ARMITO_CORE_MAX_MODULES;
}

ArmitoCoreStatus ArmitoCoreRegistryGetAt(
    uint32_t index,
    ArmitoModuleID *id)
{
    if (!registry_initialized)
        return ARMITO_CORE_NOT_INITIALIZED;

    if (id == NULL || index >= ARMITO_CORE_MAX_MODULES)
        return ARMITO_CORE_ERROR;

    if (!registry[index].registered)
        return ARMITO_CORE_ERROR;

    *id = registry[index].descriptor.id;

    return ARMITO_CORE_SUCCESS;
}

void ArmitoCoreRegistryReset(void)
{
    for (uint32_t i = 0; i < ARMITO_CORE_MAX_MODULES; i++) {
        registry[i].registered = 0;
        registry[i].state = ARMITO_MODULE_UNREGISTERED;
    }

    module_count = 0;
    next_id = 1;
    registry_initialized = 0;
}
