#include "Includes/CoreDependency.h"
#include "Includes/CoreRegistry.h"
#include "Includes/CoreConfig.h"

typedef struct {
    ArmitoModuleID module_id;
    ArmitoModuleID dependencies[ARMITO_CORE_MAX_DEPENDENCIES];
    uint32_t dependency_count;
} ArmitoDependencyEntry;

static ArmitoDependencyEntry dependency_table[ARMITO_CORE_MAX_MODULES];
static uint32_t entry_count = 0;
static uint8_t initialized = 0;

static uint8_t visited[ARMITO_CORE_MAX_MODULES];
static uint8_t active[ARMITO_CORE_MAX_MODULES];
static ArmitoModuleID module_at[ARMITO_CORE_MAX_MODULES];
static uint32_t module_total = 0;

ArmitoCoreStatus ArmitoCoreDependencyInitialize(void)
{
    if (initialized)
        return ARMITO_CORE_ALREADY_INITIALIZED;

    for (uint32_t i = 0; i < ARMITO_CORE_MAX_MODULES; i++) {
        dependency_table[i].module_id = ARMITO_MODULE_ID_INVALID;
        dependency_table[i].dependency_count = 0;

        for (uint32_t j = 0; j < ARMITO_CORE_MAX_DEPENDENCIES; j++)
            dependency_table[i].dependencies[j] = ARMITO_MODULE_ID_INVALID;
    }

    entry_count = 0;
    initialized = 1;

    return ARMITO_CORE_SUCCESS;
}

static ArmitoDependencyEntry *FindEntry(ArmitoModuleID module_id)
{
    for (uint32_t i = 0; i < entry_count; i++) {
        if (dependency_table[i].module_id == module_id)
            return &dependency_table[i];
    }

    return NULL;
}

static int IndexOf(ArmitoModuleID id)
{
    for (uint32_t i = 0; i < module_total; i++) {
        if (module_at[i] == id)
            return (int)i;
    }

    return -1;
}

ArmitoCoreStatus ArmitoCoreDependencyAdd(
    ArmitoModuleID module_id,
    ArmitoModuleID dependency_id)
{
    if (!initialized)
        return ARMITO_CORE_NOT_INITIALIZED;

    if (module_id == ARMITO_MODULE_ID_INVALID ||
        dependency_id == ARMITO_MODULE_ID_INVALID ||
        module_id == dependency_id)
        return ARMITO_CORE_ERROR;

    if (ArmitoCoreRegistryGet(module_id) == NULL ||
        ArmitoCoreRegistryGet(dependency_id) == NULL)
        return ARMITO_CORE_ERROR;

    ArmitoCoreConfig config;

    ArmitoCoreStatus status = ArmitoCoreConfigGet(&config);

    if (status != ARMITO_CORE_SUCCESS)
        return status;

    ArmitoDependencyEntry *entry = FindEntry(module_id);

    if (entry == NULL) {
        if (entry_count >= ARMITO_CORE_MAX_MODULES)
            return ARMITO_CORE_OUT_OF_RESOURCES;

        entry = &dependency_table[entry_count++];
        entry->module_id = module_id;
        entry->dependency_count = 0;
    }

    for (uint32_t i = 0; i < entry->dependency_count; i++) {
        if (entry->dependencies[i] == dependency_id)
            return ARMITO_CORE_ERROR;
    }

    if (entry->dependency_count >= config.max_dependencies ||
        entry->dependency_count >= ARMITO_CORE_MAX_DEPENDENCIES)
        return ARMITO_CORE_OUT_OF_RESOURCES;

    entry->dependencies[entry->dependency_count++] = dependency_id;

    return ARMITO_CORE_SUCCESS;
}

ArmitoCoreStatus ArmitoCoreDependencyValidate(void)
{
    if (!initialized)
        return ARMITO_CORE_NOT_INITIALIZED;

    for (uint32_t i = 0; i < entry_count; i++) {
        ArmitoDependencyEntry *entry = &dependency_table[i];

        if (ArmitoCoreRegistryGet(entry->module_id) == NULL)
            return ARMITO_CORE_DEPENDENCY_ERROR;

        for (uint32_t j = 0; j < entry->dependency_count; j++) {
            if (ArmitoCoreRegistryGet(entry->dependencies[j]) == NULL)
                return ARMITO_CORE_DEPENDENCY_ERROR;
        }
    }

    return ARMITO_CORE_SUCCESS;
}

static int Visit(
    uint32_t index,
    ArmitoModuleID *ordered,
    uint32_t *count,
    uint32_t capacity)
{
    if (active[index])
        return 0;

    if (visited[index])
        return 1;

    active[index] = 1;

    ArmitoDependencyEntry *entry = FindEntry(module_at[index]);

    if (entry != NULL) {
        for (uint32_t i = 0; i < entry->dependency_count; i++) {
            int dependency_index = IndexOf(entry->dependencies[i]);

            if (dependency_index < 0)
                return 0;

            if (!Visit((uint32_t)dependency_index, ordered, count, capacity))
                return 0;
        }
    }

    active[index] = 0;
    visited[index] = 1;

    if (*count >= capacity)
        return 0;

    ordered[(*count)++] = module_at[index];

    return 1;
}

ArmitoCoreStatus ArmitoCoreDependencyResolve(
    ArmitoModuleID *ordered_modules,
    uint32_t capacity,
    uint32_t *resolved_count)
{
    if (!initialized)
        return ARMITO_CORE_NOT_INITIALIZED;

    if (ordered_modules == NULL || resolved_count == NULL)
        return ARMITO_CORE_ERROR;

    *resolved_count = 0;

    ArmitoCoreStatus status = ArmitoCoreDependencyValidate();

    if (status != ARMITO_CORE_SUCCESS)
        return status;

    module_total = ArmitoCoreRegistryCount();

    if (module_total > ARMITO_CORE_MAX_MODULES)
        return ARMITO_CORE_OUT_OF_RESOURCES;

    if (capacity < module_total)
        return ARMITO_CORE_OUT_OF_RESOURCES;

    for (uint32_t i = 0; i < ARMITO_CORE_MAX_MODULES; i++) {
        visited[i] = 0;
        active[i] = 0;
        module_at[i] = ARMITO_MODULE_ID_INVALID;
    }

    for (uint32_t i = 0; i < module_total; i++) {
        if (ArmitoCoreRegistryGetAt(i, &module_at[i]) != ARMITO_CORE_SUCCESS)
            return ARMITO_CORE_DEPENDENCY_ERROR;
    }

    for (uint32_t i = 0; i < module_total; i++) {
        if (!Visit(i, ordered_modules, resolved_count, capacity))
            return ARMITO_CORE_DEPENDENCY_ERROR;
    }

    if (*resolved_count != module_total)
        return ARMITO_CORE_DEPENDENCY_ERROR;

    return ARMITO_CORE_SUCCESS;
}

uint32_t ArmitoCoreDependencyCount(ArmitoModuleID module_id)
{
    if (!initialized)
        return 0;

    ArmitoDependencyEntry *entry = FindEntry(module_id);

    return entry != NULL ? entry->dependency_count : 0;
}

void ArmitoCoreDependencyReset(void)
{
    for (uint32_t i = 0; i < ARMITO_CORE_MAX_MODULES; i++) {
        dependency_table[i].module_id = ARMITO_MODULE_ID_INVALID;
        dependency_table[i].dependency_count = 0;
    }

    entry_count = 0;
    initialized = 0;
}
