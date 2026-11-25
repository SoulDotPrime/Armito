#ifndef ARMITO_IR_VALUE_H
#define ARMITO_IR_VALUE_H

#include "Includes/IRTypes.h"

typedef enum {
    ARMITO_IR_VALUE_VIRTUAL_REGISTER = 0,
    ARMITO_IR_VALUE_IMMEDIATE,
    ARMITO_IR_VALUE_PC,
    ARMITO_IR_VALUE_MEMORY,
    ARMITO_IR_VALUE_FLAGS
} ArmitoIRValueKind;

typedef enum {
    ARMITO_IR_MEM_NONE = 0,
    ARMITO_IR_MEM_LOAD,
    ARMITO_IR_MEM_STORE
} ArmitoIRMemoryKind;

typedef struct {
    ArmitoIRValueID id;
    ArmitoIRValueKind kind;
    ArmitoIRTypeID type_id;
    uint8_t role;
    uint8_t reg_index;
    uint8_t flags_role;
    uint64_t immediate;
    ArmitoIRMemoryKind memory_kind;
    ArmitoIRValueID address;
    uint8_t address_offset;
    int64_t displacement;
    char source_name[ARMITO_IR_MAX_NAME];
} ArmitoIRValue;

ArmitoIRStatus ArmitoIRValuesReset(void);
ArmitoIRStatus ArmitoIRValuesDeclare(
    ArmitoIRValueKind kind,
    ArmitoIRTypeID type_id,
    const char *name,
    ArmitoIRValueID *value_id
);
ArmitoIRStatus ArmitoIRValuesGet(ArmitoIRValueID value_id, ArmitoIRValue *value);
ArmitoIRStatus ArmitoIRValuesSetImmediate(
    ArmitoIRValueID value_id,
    uint64_t immediate
);
ArmitoIRStatus ArmitoIRValuesSetMemory(
    ArmitoIRValueID value_id,
    ArmitoIRMemoryKind memory_kind,
    ArmitoIRValueID address,
    int64_t displacement
);
uint32_t ArmitoIRValuesCount(void);

#endif
