#include "Includes/CoreShutdown.h"
#include "Includes/CoreInit.h"
#include "Includes/CoreIPC.h"
#include "Includes/CoreScheduler.h"
#include "Includes/CoreDispatch.h"
#include "Includes/CoreMonitor.h"
#include "Includes/CoreDiagnostics.h"
#include "Includes/CoreTopology.h"
#include "Includes/CoreDependency.h"
#include "Includes/CoreRegistry.h"
#include "Includes/CoreConfig.h"
#include "../Decoder/Includes/Decoder.h"

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

    if (ArmitoDecoderShutdown() != ARMITO_DECODER_SUCCESS)
        return ARMITO_CORE_ERROR;
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
