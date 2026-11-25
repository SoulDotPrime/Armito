#ifndef ARMITO_IR_BLOCK_H
#define ARMITO_IR_BLOCK_H

#include "Includes/IRTypes.h"
#include "Includes/IROperation.h"

typedef struct {
    ArmitoIRLabelID id;
    uint32_t first_operation;
    uint32_t operation_count;
    uint64_t guest_address;
    uint8_t terminated;
    char name[ARMITO_IR_MAX_NAME];
} ArmitoIRBlock;

ArmitoIRStatus ArmitoIRBlocksReset(void);
ArmitoIRStatus ArmitoIRBlockBegin(
    uint64_t guest_address,
    const char *name,
    ArmitoIRLabelID *block_id
);
ArmitoIRStatus ArmitoIRBlockEnd(void);
ArmitoIRStatus ArmitoIRBlockGet(ArmitoIRLabelID block_id, ArmitoIRBlock *block);
uint32_t ArmitoIRBlocksCount(void);
uint32_t ArmitoIRBlockOperationCount(ArmitoIRLabelID block_id);

#endif
