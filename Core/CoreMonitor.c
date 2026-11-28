#include "Includes/CoreMonitor.h"
#include "../Registry/Includes/CoreRegistry.h"
#include "../Config/Includes/CoreConfig.h"

static uint8_t initialized = 0;

static ArmitoCoreStatus ArmitoCoreMonitorCheckEnabled(void)
{
    uint8_t enabled = 0;

    ArmitoCoreStatus status =
        ArmitoCoreConfigIsFeatureEnabled(
            ARMITO_CORE_FEATURE_MONITOR,
            &enabled
        );

    if (status != ARMITO_CORE_SUCCESS)
        return status;

    return enabled ? ARMITO_CORE_SUCCESS : ARMITO_CORE_NOT_SUPPORTED;
}

ArmitoCoreStatus ArmitoCoreMonitorInitialize(void)
{
    if (initialized)
        return ARMITO_CORE_ALREADY_INITIALIZED;

    ArmitoCoreStatus status = ArmitoCoreMonitorCheckEnabled();

    if (status != ARMITO_CORE_SUCCESS)
        return status;

    initialized = 1;

    return ARMITO_CORE_SUCCESS;
}

ArmitoCoreStatus ArmitoCoreMonitorGetModuleState(
    ArmitoModuleID module_id,
    ArmitoModuleState *state)
{
    if (!initialized)
        return ARMITO_CORE_NOT_INITIALIZED;

    ArmitoCoreStatus status = ArmitoCoreMonitorCheckEnabled();

    if (status != ARMITO_CORE_SUCCESS)
        return status;

    if (state == NULL || module_id == ARMITO_MODULE_ID_INVALID)
        return ARMITO_CORE_ERROR;

    if (ArmitoCoreRegistryGet(module_id) == NULL)
        return ARMITO_CORE_ERROR;

    *state = ArmitoCoreRegistryGetState(module_id);

    return ARMITO_CORE_SUCCESS;
}

ArmitoCoreStatus ArmitoCoreMonitorGetReport(
    ArmitoCoreMonitorReport *report)
{
    if (!initialized)
        return ARMITO_CORE_NOT_INITIALIZED;

    ArmitoCoreStatus status = ArmitoCoreMonitorCheckEnabled();

    if (status != ARMITO_CORE_SUCCESS)
        return status;

    if (report == NULL)
        return ARMITO_CORE_ERROR;

    report->total = 0;
    report->registered = 0;
    report->initializing = 0;
    report->initialized = 0;
    report->running = 0;
    report->stopped = 0;
    report->unregistered = 0;

    uint32_t count = ArmitoCoreRegistryCount();

    for (ArmitoModuleID id = 1; id <= count; id++) {
        if (ArmitoCoreRegistryGet(id) == NULL)
            continue;

        ArmitoModuleState state =
            ArmitoCoreRegistryGetState(id);

        report->total++;

        switch (state) {
            case ARMITO_MODULE_REGISTERED:
                report->registered++;
                break;

            case ARMITO_MODULE_INITIALIZING:
                report->initializing++;
                break;

            case ARMITO_MODULE_INITIALIZED:
                report->initialized++;
                break;

            case ARMITO_MODULE_RUNNING:
                report->running++;
                break;

            case ARMITO_MODULE_STOPPED:
                report->stopped++;
                break;

            default:
                report->unregistered++;
                break;
        }
    }

    return ARMITO_CORE_SUCCESS;
}

ArmitoCoreStatus ArmitoCoreMonitorIsHealthy(
    uint8_t *healthy)
{
    if (!initialized)
        return ARMITO_CORE_NOT_INITIALIZED;

    ArmitoCoreStatus status = ArmitoCoreMonitorCheckEnabled();

    if (status != ARMITO_CORE_SUCCESS)
        return status;

    if (healthy == NULL)
        return ARMITO_CORE_ERROR;

    ArmitoCoreMonitorReport report;

    status = ArmitoCoreMonitorGetReport(&report);

    if (status != ARMITO_CORE_SUCCESS)
        return status;

    *healthy = (
        report.total > 0 &&
        report.initializing == 0 &&
        report.registered == 0 &&
        report.stopped == 0 &&
        report.unregistered == 0 &&
        report.running == report.total
    );

    return ARMITO_CORE_SUCCESS;
}

void ArmitoCoreMonitorReset(void)
{
    initialized = 0;
}