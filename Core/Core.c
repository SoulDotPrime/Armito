#include "Includes/Core.h"
#include "Config/Includes/CoreConfig.h"
#include "Registry/Includes/CoreRegistry.h"
#include "Init/Includes/CoreInit.h"
#include "Diagnostics/Includes/CoreDiagnostics.h"
#include "Dispatch/Includes/CoreDispatch.h"
#include "Monitor/Includes/CoreMonitor.h"
#include "Topology/Includes/CoreTopology.h"
#include "Scheduler/Includes/CoreScheduler.h"
#include "IPC/Includes/CoreIPC.h"
#include "Shutdown/Includes/CoreShutdown.h"
#include "Dependency/Includes/CoreDependency.h"

static volatile uint32_t core_state = ARMITO_CORE_UNINITIALIZED;
static ArmitoCoreConfig active_config;

static ArmitoCoreStatus InitializeOptionalSubsystems(void)
{
    ArmitoCoreStatus status;

    if (active_config.enable_diagnostics) {
        status = ArmitoCoreDiagnosticsInitialize();
        if (status != ARMITO_CORE_SUCCESS)
            return status;
    }

    if (active_config.enable_monitor) {
        status = ArmitoCoreMonitorInitialize();
        if (status != ARMITO_CORE_SUCCESS)
            return status;
    }

    if (active_config.enable_scheduler) {
        status = ArmitoCoreSchedulerInitialize();
        if (status != ARMITO_CORE_SUCCESS)
            return status;
    }

    if (active_config.enable_ipc) {
        status = ArmitoCoreIPCInitialize();
        if (status != ARMITO_CORE_SUCCESS)
            return status;
    }

    return ARMITO_CORE_SUCCESS;
}

static void ResetOptionalSubsystems(void)
{
    ArmitoCoreSchedulerReset();
    ArmitoCoreIPCReset();
    ArmitoCoreMonitorReset();
    ArmitoCoreDiagnosticsReset();
}

ArmitoCoreStatus ArmitoCoreInitialize(void)
{
    if (core_state == ARMITO_CORE_READY ||
        core_state == ARMITO_CORE_RUNNING)
        return ARMITO_CORE_ALREADY_INITIALIZED;

    ArmitoCoreStatus status;

    status = ArmitoCoreConfigInitialize();

    if (status != ARMITO_CORE_SUCCESS)
        return status;

    status = ArmitoCoreInitPrepare();

    if (status != ARMITO_CORE_SUCCESS) {
        ArmitoCoreConfigShutdown();
        return status;
    }

    status = ArmitoCoreDispatchInitialize();

    if (status != ARMITO_CORE_SUCCESS) {
        ArmitoCoreDependencyReset();
        ArmitoCoreRegistryReset();
        ArmitoCoreConfigShutdown();
        return status;
    }

    status = ArmitoCoreTopologyInitialize();

    if (status != ARMITO_CORE_SUCCESS) {
        ArmitoCoreDispatchReset();
        ArmitoCoreDependencyReset();
        ArmitoCoreRegistryReset();
        ArmitoCoreConfigShutdown();
        return status;
    }

    status = ArmitoCoreShutdownManagerInitialize();

    if (status != ARMITO_CORE_SUCCESS) {
        ArmitoCoreTopologyReset();
        ArmitoCoreDispatchReset();
        ArmitoCoreDependencyReset();
        ArmitoCoreRegistryReset();
        ArmitoCoreConfigShutdown();
        return status;
    }

    core_state = ARMITO_CORE_READY;

    return ARMITO_CORE_SUCCESS;
}

ArmitoCoreStatus ArmitoCoreRegisterModule(
    const ArmitoModuleDescriptor *module,
    ArmitoModuleID *assigned_id
)
{
    if (core_state != ARMITO_CORE_READY)
        return ARMITO_CORE_INVALID_STATE;

    return ArmitoCoreRegistryRegister(module, assigned_id);
}

ArmitoCoreStatus ArmitoCoreRegisterDependency(
    ArmitoModuleID module_id,
    ArmitoModuleID dependency_id
)
{
    if (core_state != ARMITO_CORE_READY)
        return ARMITO_CORE_INVALID_STATE;

    return ArmitoCoreDependencyAdd(module_id, dependency_id);
}

ArmitoCoreStatus ArmitoCoreStart(void)
{
    if (core_state == ARMITO_CORE_UNINITIALIZED ||
        core_state == ARMITO_CORE_STOPPED)
        return ARMITO_CORE_NOT_INITIALIZED;

    if (core_state == ARMITO_CORE_RUNNING)
        return ARMITO_CORE_ALREADY_INITIALIZED;

    ArmitoCoreStatus status;

    status = ArmitoCoreConfigGet(&active_config);

    if (status != ARMITO_CORE_SUCCESS)
        return status;

    status = ArmitoCoreTopologyBuild();

    if (status != ARMITO_CORE_SUCCESS) {
        core_state = ARMITO_CORE_FAILED;
        return status;
    }

    status = InitializeOptionalSubsystems();

    if (status != ARMITO_CORE_SUCCESS) {
        ResetOptionalSubsystems();
        core_state = ARMITO_CORE_FAILED;
        return status;
    }

    status = ArmitoCoreConfigLock();

    if (status != ARMITO_CORE_SUCCESS) {
        ResetOptionalSubsystems();
        core_state = ARMITO_CORE_FAILED;
        return status;
    }

    status = ArmitoCoreInitExecute();

    if (status != ARMITO_CORE_SUCCESS) {
        ArmitoCoreConfigUnlock();
        ResetOptionalSubsystems();
        core_state = ARMITO_CORE_FAILED;
        return status;
    }

    status = ArmitoCoreInitStart();

    if (status != ARMITO_CORE_SUCCESS) {
        core_state = ARMITO_CORE_FAILED;
        ArmitoCoreInitShutdown();
        ArmitoCoreConfigUnlock();
        ResetOptionalSubsystems();
        return status;
    }

    core_state = ARMITO_CORE_RUNNING;

    return ARMITO_CORE_SUCCESS;
}

void ArmitoCoreShutdown(void)
{
    if (core_state == ARMITO_CORE_UNINITIALIZED ||
        core_state == ARMITO_CORE_STOPPED)
        return;

    if (ArmitoCoreShutdownManagerRequest() != ARMITO_CORE_SUCCESS)
        return;

    if (ArmitoCoreShutdownManagerExecute() != ARMITO_CORE_SUCCESS)
        return;

    ArmitoCoreShutdownManagerReset();

    active_config.max_modules = 0;
    core_state = ARMITO_CORE_STOPPED;
}

ArmitoCoreState ArmitoCoreGetStatus(void)
{
    return core_state;
}

ArmitoModuleState ArmitoCoreGetModuleState(ArmitoModuleID id)
{
    if (core_state == ARMITO_CORE_UNINITIALIZED)
        return ARMITO_MODULE_UNREGISTERED;

    return ArmitoCoreRegistryGetState(id);
}
