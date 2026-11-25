#include "Includes/CoreScheduler.h"
#include "Registry/Includes/CoreRegistry.h"
#include "Config/Includes/CoreConfig.h"

typedef struct {
    ArmitoTaskDescriptor descriptor;
    ArmitoTaskState state;
    uint8_t registered;
} ArmitoSchedulerEntry;

static ArmitoSchedulerEntry task_table[ARMITO_SCHEDULER_MAX_TASKS];
static uint32_t task_count = 0;
static ArmitoTaskID next_task_id = 1;
static uint8_t initialized = 0;

static ArmitoCoreStatus ArmitoCoreSchedulerCheckEnabled(void)
{
    uint8_t enabled = 0;

    ArmitoCoreStatus status =
        ArmitoCoreConfigIsFeatureEnabled(
            ARMITO_CORE_FEATURE_SCHEDULER,
            &enabled
        );

    if (status != ARMITO_CORE_SUCCESS)
        return status;

    return enabled ? ARMITO_CORE_SUCCESS : ARMITO_CORE_NOT_SUPPORTED;
}

ArmitoCoreStatus ArmitoCoreSchedulerInitialize(void)
{
    if (initialized)
        return ARMITO_CORE_ALREADY_INITIALIZED;

    ArmitoCoreStatus status = ArmitoCoreSchedulerCheckEnabled();

    if (status != ARMITO_CORE_SUCCESS)
        return status;

    for (uint32_t i = 0; i < ARMITO_SCHEDULER_MAX_TASKS; i++) {
        task_table[i].registered = 0;
        task_table[i].state = ARMITO_TASK_PENDING;
    }

    task_count = 0;
    next_task_id = 1;
    initialized = 1;

    return ARMITO_CORE_SUCCESS;
}

ArmitoCoreStatus ArmitoCoreSchedulerSubmit(
    const ArmitoTaskDescriptor *task,
    ArmitoTaskID *assigned_id)
{
    if (!initialized)
        return ARMITO_CORE_NOT_INITIALIZED;

    ArmitoCoreStatus status = ArmitoCoreSchedulerCheckEnabled();

    if (status != ARMITO_CORE_SUCCESS)
        return status;

    if (task == NULL || task->execute == NULL)
        return ARMITO_CORE_ERROR;

    if (task->module_id == ARMITO_MODULE_ID_INVALID)
        return ARMITO_CORE_ERROR;

    if (ArmitoCoreRegistryGet(task->module_id) == NULL)
        return ARMITO_CORE_ERROR;

    if (task->priority < ARMITO_PRIORITY_CRITICAL ||
        task->priority > ARMITO_PRIORITY_LOW)
        return ARMITO_CORE_ERROR;

    ArmitoCoreConfig config;

    status = ArmitoCoreConfigGet(&config);

    if (status != ARMITO_CORE_SUCCESS)
        return status;

    if (task_count >= config.max_tasks ||
        task_count >= ARMITO_SCHEDULER_MAX_TASKS)
        return ARMITO_CORE_OUT_OF_RESOURCES;

    if (next_task_id == ARMITO_TASK_ID_INVALID)
        return ARMITO_CORE_OUT_OF_RESOURCES;

    for (uint32_t i = 0; i < ARMITO_SCHEDULER_MAX_TASKS; i++) {
        if (!task_table[i].registered) {
            task_table[i].descriptor = *task;
            task_table[i].descriptor.id = next_task_id;
            task_table[i].state = ARMITO_TASK_PENDING;
            task_table[i].registered = 1;

            if (assigned_id != NULL)
                *assigned_id = next_task_id;

            next_task_id++;
            task_count++;

            return ARMITO_CORE_SUCCESS;
        }
    }

    return ARMITO_CORE_OUT_OF_RESOURCES;
}

ArmitoCoreStatus ArmitoCoreSchedulerExecuteNext(
    ArmitoTaskID *executed_task)
{
    if (!initialized)
        return ARMITO_CORE_NOT_INITIALIZED;

    ArmitoCoreStatus status = ArmitoCoreSchedulerCheckEnabled();

    if (status != ARMITO_CORE_SUCCESS)
        return status;

    ArmitoSchedulerEntry *selected = NULL;

    for (uint32_t i = 0; i < ARMITO_SCHEDULER_MAX_TASKS; i++) {
        if (!task_table[i].registered ||
            task_table[i].state != ARMITO_TASK_PENDING)
            continue;

        if (selected == NULL ||
            task_table[i].descriptor.priority <
            selected->descriptor.priority)
            selected = &task_table[i];
    }

    if (selected == NULL)
        return ARMITO_CORE_INVALID_STATE;

    if (ArmitoCoreRegistryGetState(selected->descriptor.module_id) !=
        ARMITO_MODULE_RUNNING)
        return ARMITO_CORE_INVALID_STATE;

    selected->state = ARMITO_TASK_EXECUTING;

    status = selected->descriptor.execute(selected->descriptor.context);

    if (status == ARMITO_CORE_SUCCESS)
        selected->state = ARMITO_TASK_COMPLETED;
    else
        selected->state = ARMITO_TASK_FAILED;

    if (executed_task != NULL)
        *executed_task = selected->descriptor.id;

    return status;
}

ArmitoCoreStatus ArmitoCoreSchedulerGetTaskState(
    ArmitoTaskID task_id,
    ArmitoTaskState *state)
{
    if (!initialized)
        return ARMITO_CORE_NOT_INITIALIZED;

    ArmitoCoreStatus status = ArmitoCoreSchedulerCheckEnabled();

    if (status != ARMITO_CORE_SUCCESS)
        return status;

    if (task_id == ARMITO_TASK_ID_INVALID || state == NULL)
        return ARMITO_CORE_ERROR;

    for (uint32_t i = 0; i < ARMITO_SCHEDULER_MAX_TASKS; i++) {
        if (task_table[i].registered &&
            task_table[i].descriptor.id == task_id) {
            *state = task_table[i].state;
            return ARMITO_CORE_SUCCESS;
        }
    }

    return ARMITO_CORE_ERROR;
}

void ArmitoCoreSchedulerReset(void)
{
    for (uint32_t i = 0; i < ARMITO_SCHEDULER_MAX_TASKS; i++) {
        task_table[i].registered = 0;
        task_table[i].state = ARMITO_TASK_PENDING;
    }

    task_count = 0;
    next_task_id = 1;
    initialized = 0;
}