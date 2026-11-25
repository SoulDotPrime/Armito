#include "Includes/CoreDiagnostics.h"
#include "Config/Includes/CoreConfig.h"

static ArmitoDiagnosticDisplayHandler display_handler = NULL;
static ArmitoDiagnosticLogHandler log_handler = NULL;

static uint8_t initialized = 0;

static ArmitoCoreStatus ArmitoCoreDiagnosticsCheckEnabled(void)
{
    uint8_t enabled = 0;

    ArmitoCoreStatus status =
        ArmitoCoreConfigIsFeatureEnabled(
            ARMITO_CORE_FEATURE_DIAGNOSTICS,
            &enabled
        );

    if (status != ARMITO_CORE_SUCCESS)
        return status;

    return enabled ? ARMITO_CORE_SUCCESS : ARMITO_CORE_NOT_SUPPORTED;
}

ArmitoCoreStatus ArmitoCoreDiagnosticsInitialize(void)
{
    if (initialized)
        return ARMITO_CORE_ALREADY_INITIALIZED;

    ArmitoCoreStatus status = ArmitoCoreDiagnosticsCheckEnabled();

    if (status != ARMITO_CORE_SUCCESS)
        return status;

    display_handler = NULL;
    log_handler = NULL;
    initialized = 1;

    return ARMITO_CORE_SUCCESS;
}

ArmitoCoreStatus ArmitoCoreDiagnosticsSetDisplayHandler(
    ArmitoDiagnosticDisplayHandler handler)
{
    if (!initialized)
        return ARMITO_CORE_NOT_INITIALIZED;

    ArmitoCoreStatus status = ArmitoCoreDiagnosticsCheckEnabled();

    if (status != ARMITO_CORE_SUCCESS)
        return status;

    if (handler == NULL)
        return ARMITO_CORE_ERROR;

    display_handler = handler;

    return ARMITO_CORE_SUCCESS;
}

ArmitoCoreStatus ArmitoCoreDiagnosticsSetLogHandler(
    ArmitoDiagnosticLogHandler handler)
{
    if (!initialized)
        return ARMITO_CORE_NOT_INITIALIZED;

    ArmitoCoreStatus status = ArmitoCoreDiagnosticsCheckEnabled();

    if (status != ARMITO_CORE_SUCCESS)
        return status;

    if (handler == NULL)
        return ARMITO_CORE_ERROR;

    log_handler = handler;

    return ARMITO_CORE_SUCCESS;
}

ArmitoCoreStatus ArmitoCoreDiagnosticsPresentFailure(
    const ArmitoFailureInfo *failure)
{
    if (!initialized)
        return ARMITO_CORE_NOT_INITIALIZED;

    ArmitoCoreStatus status = ArmitoCoreDiagnosticsCheckEnabled();

    if (status != ARMITO_CORE_SUCCESS)
        return status;

    if (failure == NULL)
        return ARMITO_CORE_ERROR;

    if (failure->message == NULL || failure->source == NULL)
        return ARMITO_CORE_ERROR;

    ArmitoCoreStatus display_status = ARMITO_CORE_SUCCESS;
    ArmitoCoreStatus log_status = ARMITO_CORE_SUCCESS;

    if (display_handler != NULL)
        display_status = display_handler(failure);

    if (log_handler != NULL)
        log_status = log_handler(failure);

    if (display_status != ARMITO_CORE_SUCCESS)
        return display_status;

    if (log_status != ARMITO_CORE_SUCCESS)
        return log_status;

    if (display_handler == NULL && log_handler == NULL)
        return ARMITO_CORE_INVALID_STATE;

    return ARMITO_CORE_SUCCESS;
}

void ArmitoCoreDiagnosticsReset(void)
{
    display_handler = NULL;
    log_handler = NULL;
    initialized = 0;
}