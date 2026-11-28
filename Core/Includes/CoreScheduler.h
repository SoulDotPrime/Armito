#ifndef ARMITO_CORE_SCHEDULER_H
#define ARMITO_CORE_SCHEDULER_H

#include "../Includes/Core.h"

#define ARMITO_SCHEDULER_MAX_TASKS 64

typedef uint32_t ArmitoTaskID;

#define ARMITO_TASK_ID_INVALID 0

typedef enum {
    ARMITO_TASK_PENDING = 0,
    ARMITO_TASK_EXECUTING,
    ARMITO_TASK_COMPLETED,
    ARMITO_TASK_FAILED
} ArmitoTaskState;

typedef ArmitoCoreStatus (*ArmitoTaskFunction)(void *context);

typedef struct {
    ArmitoTaskID id;
    ArmitoModuleID module_id;
    ArmitoModulePriority priority;
    ArmitoTaskFunction execute;
    void *context;
} ArmitoTaskDescriptor;

ArmitoCoreStatus ArmitoCoreSchedulerInitialize(void);

ArmitoCoreStatus ArmitoCoreSchedulerSubmit(
    const ArmitoTaskDescriptor *task,
    ArmitoTaskID *assigned_id
);

ArmitoCoreStatus ArmitoCoreSchedulerExecuteNext(
    ArmitoTaskID *executed_task
);

ArmitoCoreStatus ArmitoCoreSchedulerGetTaskState(
    ArmitoTaskID task_id,
    ArmitoTaskState *state
);

void ArmitoCoreSchedulerReset(void);

#endif