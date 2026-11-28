#ifndef ARMITO_CORE_IPC_H
#define ARMITO_CORE_IPC_H

#include "../../Includes/Core.h"

#define ARMITO_IPC_MAX_MESSAGES 64
#define ARMITO_IPC_MAX_PAYLOAD 256

typedef uint32_t ArmitoMessageID;
typedef uint32_t ArmitoMessageType;

#define ARMITO_MESSAGE_ID_INVALID 0

typedef struct {
    ArmitoMessageID id;
    ArmitoModuleID sender;
    ArmitoModuleID receiver;
    ArmitoMessageType type;
    uint32_t size;
    uint8_t payload[ARMITO_IPC_MAX_PAYLOAD];
} ArmitoIPCMessage;

ArmitoCoreStatus ArmitoCoreIPCInitialize(void);

ArmitoCoreStatus ArmitoCoreIPCSend(
    ArmitoModuleID sender,
    ArmitoModuleID receiver,
    ArmitoMessageType type,
    const void *payload,
    uint32_t size,
    ArmitoMessageID *message_id
);

ArmitoCoreStatus ArmitoCoreIPCReceive(
    ArmitoModuleID receiver,
    ArmitoIPCMessage *message
);

uint32_t ArmitoCoreICPPendingCount(void);

void ArmitoCoreIPCReset(void);

#endif