#include "Includes/CoreIPC.h"
#include "Includes/CoreRegistry.h"
#include "Includes/CoreConfig.h"
#include "Includes/CoreMemory.h"

typedef struct {
    ArmitoIPCMessage message;
    uint64_t sequence;
    uint8_t occupied;
} ArmitoIPCEntry;

static ArmitoIPCEntry message_queue[ARMITO_IPC_MAX_MESSAGES];
static ArmitoMessageID next_message_id = 1;
static uint64_t next_sequence = 1;
static uint32_t pending_count = 0;
static uint8_t initialized = 0;

static ArmitoCoreStatus ArmitoCoreIPCCheckEnabled(void)
{
    uint8_t enabled = 0;

    ArmitoCoreStatus status =
        ArmitoCoreConfigIsFeatureEnabled(
            ARMITO_CORE_FEATURE_IPC,
            &enabled
        );

    if (status != ARMITO_CORE_SUCCESS)
        return status;

    return enabled ? ARMITO_CORE_SUCCESS : ARMITO_CORE_NOT_SUPPORTED;
}

ArmitoCoreStatus ArmitoCoreIPCInitialize(void)
{
    if (initialized)
        return ARMITO_CORE_ALREADY_INITIALIZED;

    ArmitoCoreStatus status = ArmitoCoreIPCCheckEnabled();

    if (status != ARMITO_CORE_SUCCESS)
        return status;

    for (uint32_t i = 0; i < ARMITO_IPC_MAX_MESSAGES; i++) {
        message_queue[i].occupied = 0;
        message_queue[i].sequence = 0;
    }

    next_message_id = 1;
    next_sequence = 1;
    pending_count = 0;
    initialized = 1;

    return ARMITO_CORE_SUCCESS;
}

ArmitoCoreStatus ArmitoCoreIPCSend(
    ArmitoModuleID sender,
    ArmitoModuleID receiver,
    ArmitoMessageType type,
    const void *payload,
    uint32_t size,
    ArmitoMessageID *message_id
)
{
    if (!initialized)
        return ARMITO_CORE_NOT_INITIALIZED;

    ArmitoCoreStatus status = ArmitoCoreIPCCheckEnabled();

    if (status != ARMITO_CORE_SUCCESS)
        return status;

    if (sender == ARMITO_MODULE_ID_INVALID ||
        receiver == ARMITO_MODULE_ID_INVALID)
        return ARMITO_CORE_ERROR;

    if (ArmitoCoreRegistryGet(sender) == NULL ||
        ArmitoCoreRegistryGet(receiver) == NULL)
        return ARMITO_CORE_ERROR;

    if (size > ARMITO_IPC_MAX_PAYLOAD)
        return ARMITO_CORE_OUT_OF_RESOURCES;

    if (size > 0 && payload == NULL)
        return ARMITO_CORE_ERROR;

    ArmitoCoreConfig config;

    status = ArmitoCoreConfigGet(&config);

    if (status != ARMITO_CORE_SUCCESS)
        return status;

    if (size > config.max_ipc_payload)
        return ARMITO_CORE_OUT_OF_RESOURCES;

    if (pending_count >= config.max_ipc_messages ||
        pending_count >= ARMITO_IPC_MAX_MESSAGES)
        return ARMITO_CORE_OUT_OF_RESOURCES;

    if (next_message_id == ARMITO_MESSAGE_ID_INVALID ||
        next_message_id == UINT32_MAX)
        return ARMITO_CORE_OUT_OF_RESOURCES;

    for (uint32_t i = 0; i < ARMITO_IPC_MAX_MESSAGES; i++) {
        if (!message_queue[i].occupied) {
            ArmitoIPCEntry *entry = &message_queue[i];

            entry->message.id = next_message_id;
            entry->message.sender = sender;
            entry->message.receiver = receiver;
            entry->message.type = type;
            entry->message.size = size;

            if (size > 0)
                ArmitoMemoryCopy(entry->message.payload, payload, size);

            entry->sequence = next_sequence++;
            entry->occupied = 1;

            if (message_id != NULL)
                *message_id = next_message_id;

            next_message_id++;
            pending_count++;

            return ARMITO_CORE_SUCCESS;
        }
    }

    return ARMITO_CORE_OUT_OF_RESOURCES;
}

ArmitoCoreStatus ArmitoCoreIPCReceive(
    ArmitoModuleID receiver,
    ArmitoIPCMessage *message
)
{
    if (!initialized)
        return ARMITO_CORE_NOT_INITIALIZED;

    ArmitoCoreStatus status = ArmitoCoreIPCCheckEnabled();

    if (status != ARMITO_CORE_SUCCESS)
        return status;

    if (receiver == ARMITO_MODULE_ID_INVALID || message == NULL)
        return ARMITO_CORE_ERROR;

    if (ArmitoCoreRegistryGet(receiver) == NULL)
        return ARMITO_CORE_ERROR;

    ArmitoIPCEntry *selected = NULL;

    for (uint32_t i = 0; i < ARMITO_IPC_MAX_MESSAGES; i++) {
        ArmitoIPCEntry *entry = &message_queue[i];

        if (!entry->occupied || entry->message.receiver != receiver)
            continue;

        if (selected == NULL || entry->sequence < selected->sequence)
            selected = entry;
    }

    if (selected == NULL)
        return ARMITO_CORE_INVALID_STATE;

    *message = selected->message;

    selected->occupied = 0;
    selected->sequence = 0;

    if (pending_count > 0)
        pending_count--;

    return ARMITO_CORE_SUCCESS;
}

uint32_t ArmitoCoreICPPendingCount(void)
{
    if (!initialized)
        return 0;

    if (ArmitoCoreIPCCheckEnabled() != ARMITO_CORE_SUCCESS)
        return 0;

    return pending_count;
}

void ArmitoCoreIPCReset(void)
{
    for (uint32_t i = 0; i < ARMITO_IPC_MAX_MESSAGES; i++) {
        message_queue[i].occupied = 0;
        message_queue[i].sequence = 0;
    }

    next_message_id = 1;
    next_sequence = 1;
    pending_count = 0;
    initialized = 0;
}