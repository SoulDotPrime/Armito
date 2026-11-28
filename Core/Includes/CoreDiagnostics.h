#ifndef ARMITO_CORE_DIAGNOSTICS_H
#define ARMITO_CORE_DIAGNOSTICS_H

#include "../../Includes/Core.h"

typedef enum {
    ARMITO_DIAGNOSTIC_INFO = 0,
    ARMITO_DIAGNOSTIC_WARNING,
    ARMITO_DIAGNOSTIC_ERROR,
    ARMITO_DIAGNOSTIC_FATAL
} ArmitoDiagnosticSeverity;

typedef struct {
    uint32_t error_code;
    ArmitoDiagnosticSeverity severity;
    const char *source;
    const char *message;
} ArmitoFailureInfo;

typedef ArmitoCoreStatus (*ArmitoDiagnosticDisplayHandler)(
    const ArmitoFailureInfo *failure
);

typedef ArmitoCoreStatus (*ArmitoDiagnosticLogHandler)(
    const ArmitoFailureInfo *failure
);

ArmitoCoreStatus ArmitoCoreDiagnosticsInitialize(void);

ArmitoCoreStatus ArmitoCoreDiagnosticsSetDisplayHandler(
    ArmitoDiagnosticDisplayHandler handler
);

ArmitoCoreStatus ArmitoCoreDiagnosticsSetLogHandler(
    ArmitoDiagnosticLogHandler handler
);

ArmitoCoreStatus ArmitoCoreDiagnosticsPresentFailure(
    const ArmitoFailureInfo *failure
);

void ArmitoCoreDiagnosticsReset(void);

#endif