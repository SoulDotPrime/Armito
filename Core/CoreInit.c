#include "Includes/CoreInit.h"
#include "Includes/CoreRegistry.h"
#include "Includes/CoreDependency.h"
#include "Includes/CoreConfig.h"

static ArmitoModuleID initialization_order[ARMITO_CORE_MAX_MODULES];

static uint32_t initialized_count = 0;
static uint32_t started_count = 0;
static uint32_t module_count = 0;

static uint8_t prepared = 0;
static uint8_t executed = 0;
static uint8_t started = 0;

static void RollbackModules(
    uint32_t count,
    ArmitoModuleState state)
{
    while (count > 0) {
        count--;

        ArmitoModuleID id = initialization_order[count];

        const ArmitoModuleDescriptor *module =
            ArmitoCoreRegistryGet(id);

        if (module != NULL && module->shutdown != NULL)
            module->shutdown();

        ArmitoCoreRegistrySetState(id, state);
    }
}

static ArmitoCoreStatus AbortStart(ArmitoCoreStatus status)
{
    RollbackModules(initialized_count, ARMITO_MODULE_STOPPED);

    started_count = 0;
    initialized_count = 0;
    started = 0;
    executed = 0;

    return status;
}

ArmitoCoreStatus ArmitoCoreInitPrepare(void)
{
    if (prepared)
        return ARMITO_CORE_ALREADY_INITIALIZED;

    ArmitoCoreStatus status;

    status = ArmitoCoreRegistryInitialize();

    if (status != ARMITO_CORE_SUCCESS)
        return status;

    status = ArmitoCoreDependencyInitialize();

    if (status != ARMITO_CORE_SUCCESS) {
        ArmitoCoreRegistryReset();
        return status;
    }

    initialized_count = 0;
    started_count = 0;
    module_count = 0;

    prepared = 1;

    return ARMITO_CORE_SUCCESS;
}

ArmitoCoreStatus ArmitoCoreInitExecute(void)
{
    if (!prepared)
        return ARMITO_CORE_NOT_INITIALIZED;

    if (executed)
        return ARMITO_CORE_ALREADY_INITIALIZED;

    module_count = ArmitoCoreRegistryCount();

    if (module_count > ARMITO_CORE_MAX_MODULES)
        return ARMITO_CORE_OUT_OF_RESOURCES;

    if (module_count == 0) {
        executed = 1;
        return ARMITO_CORE_SUCCESS;
    }

    uint32_t resolved_count = 0;

    ArmitoCoreStatus status =
        ArmitoCoreDependencyResolve(
            initialization_order,
            ARMITO_CORE_MAX_MODULES,
            &resolved_count
        );

    if (status != ARMITO_CORE_SUCCESS)
        return status;

    if (resolved_count != module_count)
        return ARMITO_CORE_DEPENDENCY_ERROR;

    for (uint32_t i = 0; i < resolved_count; i++) {

        ArmitoModuleID id = initialization_order[i];

        const ArmitoModuleDescriptor *module =
            ArmitoCoreRegistryGet(id);

        if (module == NULL)
            return AbortStart(ARMITO_CORE_ERROR);

        status = ArmitoCoreRegistrySetState(
            id,
            ARMITO_MODULE_INITIALIZING
        );

        if (status != ARMITO_CORE_SUCCESS)
            return AbortStart(status);

        status = module->initialize();

        if (status != ARMITO_CORE_SUCCESS) {
            ArmitoCoreRegistrySetState(
                id,
                ARMITO_MODULE_INITIALIZATION_FAILED
            );

            RollbackModules(initialized_count, ARMITO_MODULE_STOPPED);
            initialized_count = 0;
            executed = 0;

            return ARMITO_CORE_INITIALIZATION_FAILED;
        }

        status = ArmitoCoreRegistrySetState(
            id,
            ARMITO_MODULE_INITIALIZED
        );

        if (status != ARMITO_CORE_SUCCESS)
            return AbortStart(status);

        initialized_count++;
    }

    executed = 1;

    return ARMITO_CORE_SUCCESS;
}

ArmitoCoreStatus ArmitoCoreInitStart(void)
{
    if (!executed)
        return ARMITO_CORE_NOT_INITIALIZED;

    if (started)
        return ARMITO_CORE_ALREADY_INITIALIZED;

    for (uint32_t i = 0; i < module_count; i++) {

        ArmitoModuleID id = initialization_order[i];

        const ArmitoModuleDescriptor *module =
            ArmitoCoreRegistryGet(id);

        if (module == NULL)
            return AbortStart(ARMITO_CORE_ERROR);

        ArmitoCoreStatus status = module->start();

        if (status != ARMITO_CORE_SUCCESS) {
            ArmitoCoreRegistrySetState(
                id,
                ARMITO_MODULE_EXECUTION_FAILED
            );

            return AbortStart(ARMITO_CORE_EXECUTION_FAILED);
        }

        status = ArmitoCoreRegistrySetState(
            id,
            ARMITO_MODULE_RUNNING
        );

        if (status != ARMITO_CORE_SUCCESS)
            return AbortStart(status);

        started_count++;
    }

    started = 1;

    return ARMITO_CORE_SUCCESS;
}

void ArmitoCoreInitShutdown(void)
{
    RollbackModules(initialized_count, ARMITO_MODULE_STOPPED);

    initialized_count = 0;
    started_count = 0;
    module_count = 0;

    started = 0;
    executed = 0;
    prepared = 0;
}
