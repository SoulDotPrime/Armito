#ifndef ARMITO_IR_OPERATION_H
#define ARMITO_IR_OPERATION_H

#include "Includes/IRTypes.h"

typedef enum {
    ARMITO_IR_OP_NOP = 0,
    ARMITO_IR_OP_MOVE,
    ARMITO_IR_OP_SELECT,
    ARMITO_IR_OP_LOAD,
    ARMITO_IR_OP_STORE,

    ARMITO_IR_OP_ADD,
    ARMITO_IR_OP_SUB,
    ARMITO_IR_OP_MUL,
    ARMITO_IR_OP_DIV_SIGNED,
    ARMITO_IR_OP_DIV_UNSIGNED,
    ARMITO_IR_OP_REM_SIGNED,
    ARMITO_IR_OP_REM_UNSIGNED,
    ARMITO_IR_OP_NEG,

    ARMITO_IR_OP_AND,
    ARMITO_IR_OP_OR,
    ARMITO_IR_OP_XOR,
    ARMITO_IR_OP_NOT,

    ARMITO_IR_OP_SHL,
    ARMITO_IR_OP_LSR,
    ARMITO_IR_OP_ASR,
    ARMITO_IR_OP_ROL,
    ARMITO_IR_OP_ROR,

    ARMITO_IR_OP_CMP_EQ,
    ARMITO_IR_OP_CMP_NE,
    ARMITO_IR_OP_CMP_LT_SIGNED,
    ARMITO_IR_OP_CMP_LT_UNSIGNED,
    ARMITO_IR_OP_CMP_LE_SIGNED,
    ARMITO_IR_OP_CMP_LE_UNSIGNED,
    ARMITO_IR_OP_CMP_GT_SIGNED,
    ARMITO_IR_OP_CMP_GT_UNSIGNED,
    ARMITO_IR_OP_CMP_GE_SIGNED,
    ARMITO_IR_OP_CMP_GE_UNSIGNED,
    ARMITO_IR_OP_CMP_ZERO,

    ARMITO_IR_OP_EXTEND_SIGNED,
    ARMITO_IR_OP_EXTEND_UNSIGNED,
    ARMITO_IR_OP_TRUNCATE,
    ARMITO_IR_OP_BITCAST,
    ARMITO_IR_OP_EXTRACT,
    ARMITO_IR_OP_INSERT,

    ARMITO_IR_OP_BRANCH,
    ARMITO_IR_OP_BRANCH_CONDITIONAL,
    ARMITO_IR_OP_BRANCH_INDIRECT,
    ARMITO_IR_OP_CALL,
    ARMITO_IR_OP_RETURN,
    ARMITO_IR_OP_TRAP,

    ARMITO_IR_OP_BARRIER,
    ARMITO_IR_OP_UNSUPPORTED
} ArmitoIROperationKind;

typedef enum {
    ARMITO_IR_FLAG_N = 1u << 0,
    ARMITO_IR_FLAG_Z = 1u << 1,
    ARMITO_IR_FLAG_C = 1u << 2,
    ARMITO_IR_FLAG_V = 1u << 3
} ArmitoIRFlag;

typedef struct {
    ArmitoIROperationID id;
    ArmitoIROperationKind kind;
    ArmitoIRTypeID result_type;
    ArmitoIRValueID result;
    ArmitoIRValueID inputs[ARMITO_IR_MAX_INPUTS];
    uint8_t input_count;
    uint8_t flag_mask;
    ArmitoIRLabelID target;
    ArmitoIRLabelID target_false;
    uint32_t source_offset;
    uint32_t source_encoding;
    uint32_t source_length;
    uint16_t aux;
} ArmitoIROperation;

ArmitoIRStatus ArmitoIROperationsReset(void);
ArmitoIRStatus ArmitoIROperationEmit(
    ArmitoIROperationKind kind,
    ArmitoIRTypeID result_type,
    ArmitoIRValueID result,
    const ArmitoIRValueID *inputs,
    uint8_t input_count,
    ArmitoIROperationID *operation_id
);
ArmitoIRStatus ArmitoIROperationSetFlags(
    ArmitoIROperationID operation_id,
    uint8_t flag_mask
);
ArmitoIRStatus ArmitoIROperationSetTarget(
    ArmitoIROperationID operation_id,
    ArmitoIRLabelID target,
    ArmitoIRLabelID target_false
);
ArmitoIRStatus ArmitoIROperationSetSource(
    ArmitoIROperationID operation_id,
    uint32_t source_offset,
    uint32_t source_encoding,
    uint32_t source_length
);
ArmitoIRStatus ArmitoIROperationSetAux(
    ArmitoIROperationID operation_id,
    uint16_t aux
);
ArmitoIRStatus ArmitoIROperationGet(
    ArmitoIROperationID operation_id,
    ArmitoIROperation *operation
);
uint32_t ArmitoIROperationsCount(void);
const char *ArmitoIROperationName(ArmitoIROperationKind kind);

#endif
