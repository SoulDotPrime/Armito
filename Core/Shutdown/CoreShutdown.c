#include "Includes/CoreShutdown.h"
#include "../Init/Includes/CoreInit.h"
#include "../IPC/Includes/CoreIPC.h"
#include "../Scheduler/Includes/CoreScheduler.h"
#include "../Dispatch/Includes/CoreDispatch.h"
#include "../Monitor/Includes/CoreMonitor.h"
#include "../Diagnostics/Includes/CoreDiagnostics.h"
#include "../Topology/Includes/CoreTopology.h"
#include "../Dependency/Includes/CoreDependency.h"
#include "../Registry/Includes/CoreRegistry.h"
#include "../Config/Includes/CoreConfig.h"

static uint8_t initialized = 0;
static uint8_t requested = 0;
static uint8_t completed = 0;

ArmitoCoreStatus ArmitoCoreShutdownManagerInitialize(void)
{
    if (initialized)
        return ARMITO_CORE_ALREADY_INITIALIZED;

    requested = 0;
    completed = 0;
    initialized = 1;

    return ARMITO_CORE_SUCCESS;
}

ArmitoCoreStatus ArmitoCoreShutdownManagerRequest(void)
{
    if (!initialized)
        return ARMITO_CORE_NOT_INITIALIZED;

    if (completed)
        return ARMITO_CORE_INVALID_STATE;

    requested = 1;

    return ARMITO_CORE_SUCCESS;
}

ArmitoCoreStatus ArmitoCoreShutdownManagerExecute(void)
{
    if (!initialized)
        return ARMITO_CORE_NOT_INITIALIZED;

    if (!requested)
        return ARMITO_CORE_INVALID_STATE;

    if (completed)
        return ARMITO_CORE_ALREADY_INITIALIZED;

    ArmitoCoreInitShutdown();

    ArmitoCoreSchedulerReset();
    ArmitoCoreIPCReset();
    ArmitoCoreDispatchReset();
    ArmitoCoreMonitorReset();
    ArmitoCoreDiagnosticsReset();
    ArmitoCoreTopologyReset();
    ArmitoCoreDependencyReset();
    ArmitoCoreRegistryReset();
    ArmitoCoreConfigShutdown();

    completed = 1;

    return ARMITO_CORE_SUCCESS;
}

uint8_t ArmitoCoreShutdownManagerIsRequested(void)
{
    return requested;
}

uint8_t ArmitoCoreShutdownManagerIsComplete(void)
{
    return completed;
}

void ArmitoCoreShutdownManagerReset(void)
{
    requested = 0;
    completed = 0;
    initialized = 0;
}
