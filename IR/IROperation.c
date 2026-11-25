#include "Includes/IROperation.h"
#include "Includes/IRValue.h"
#include "Includes/IRTypeset.h"

static ArmitoIROperation operations[ARMITO_IR_MAX_OPERATIONS];
static uint32_t operation_count = 0;

static const char *const operation_names[] = {
    "nop",
    "move",
    "select",
    "load",
    "store",
    "add",
    "sub",
    "mul",
    "divs",
    "divu",
    "rems",
    "remu",
    "neg",
    "and",
    "or",
    "xor",
    "not",
    "shl",
    "lsr",
    "asr",
    "rol",
    "ror",
    "cmp.eq",
    "cmp.ne",
    "cmp.lt.s",
    "cmp.lt.u",
    "cmp.le.s",
    "cmp.le.u",
    "cmp.gt.s",
    "cmp.gt.u",
    "cmp.ge.s",
    "cmp.ge.u",
    "cmp.zero",
    "sext",
    "zext",
    "trunc",
    "bitcast",
    "extract",
    "insert",
    "branch",
    "branch.cond",
    "branch.indirect",
    "call",
    "return",
    "trap",
    "barrier",
    "unsupported"
};

ArmitoIRStatus ArmitoIROperationsReset(void)
{
    uint32_t i;
    uint32_t j;

    for (i = 0; i < ARMITO_IR_MAX_OPERATIONS; i++) {
        operations[i].id = 0;
        operations[i].kind = ARMITO_IR_OP_NOP;
        operations[i].result_type = ARMITO_IR_TYPE_INVALID;
        operations[i].result = ARMITO_IR_VALUE_INVALID;

        for (j = 0; j < ARMITO_IR_MAX_INPUTS; j++)
            operations[i].inputs[j] = ARMITO_IR_VALUE_INVALID;

        operations[i].input_count = 0;
        operations[i].flag_mask = 0;
        operations[i].target = ARMITO_IR_LABEL_INVALID;
        operations[i].target_false = ARMITO_IR_LABEL_INVALID;
        operations[i].source_offset = 0;
        operations[i].source_encoding = 0;
        operations[i].source_length = 0;
        operations[i].aux = 0;
    }

    operation_count = 0;

    return ARMITO_IR_SUCCESS;
}

ArmitoIRStatus ArmitoIROperationEmit(
    ArmitoIROperationKind kind,
    ArmitoIRTypeID result_type,
    ArmitoIRValueID result,
    const ArmitoIRValueID *inputs,
    uint8_t input_count,
    ArmitoIROperationID *operation_id
)
{
    ArmitoIROperation *operation;
    uint8_t i;

    if (operation_id == NULL)
        return ARMITO_IR_INVALID_ARGUMENT;

    *operation_id = 0;

    if (kind > ARMITO_IR_OP_UNSUPPORTED)
        return ARMITO_IR_INVALID_ARGUMENT;

    if (input_count > ARMITO_IR_MAX_INPUTS)
        return ARMITO_IR_INVALID_ARGUMENT;

    if (input_count > 0 && inputs == NULL)
        return ARMITO_IR_INVALID_ARGUMENT;

    if (operation_count >= ARMITO_IR_MAX_OPERATIONS)
        return ARMITO_IR_OUT_OF_RESOURCES;

    if (result != ARMITO_IR_VALUE_INVALID && result > ArmitoIRValuesCount())
        return ARMITO_IR_OUT_OF_RANGE;

    if (result_type != ARMITO_IR_TYPE_INVALID &&
        result_type > ArmitoIRTypesCount())
        return ARMITO_IR_OUT_OF_RANGE;

    for (i = 0; i < input_count; i++) {
        if (inputs[i] == ARMITO_IR_VALUE_INVALID ||
            inputs[i] > ArmitoIRValuesCount())
            return ARMITO_IR_OUT_OF_RANGE;
    }

    operation = &operations[operation_count];
    operation->id = operation_count + 1;
    operation->kind = kind;
    operation->result_type = result_type;
    operation->result = result;
    operation->input_count = input_count;

    for (i = 0; i < input_count; i++)
        operation->inputs[i] = inputs[i];

    operation_count++;

    *operation_id = operation->id;

    return ARMITO_IR_SUCCESS;
}

ArmitoIRStatus ArmitoIROperationSetFlags(
    ArmitoIROperationID operation_id,
    uint8_t flag_mask
)
{
    if (operation_id == 0 || operation_id > operation_count)
        return ARMITO_IR_OUT_OF_RANGE;

    if ((flag_mask & ~(uint8_t)0x0F) != 0)
        return ARMITO_IR_INVALID_ARGUMENT;

    operations[operation_id - 1].flag_mask = flag_mask;

    return ARMITO_IR_SUCCESS;
}

ArmitoIRStatus ArmitoIROperationSetTarget(
    ArmitoIROperationID operation_id,
    ArmitoIRLabelID target,
    ArmitoIRLabelID target_false
)
{
    if (operation_id == 0 || operation_id > operation_count)
        return ARMITO_IR_OUT_OF_RANGE;

    operations[operation_id - 1].target = target;
    operations[operation_id - 1].target_false = target_false;

    return ARMITO_IR_SUCCESS;
}

ArmitoIRStatus ArmitoIROperationSetSource(
    ArmitoIROperationID operation_id,
    uint32_t source_offset,
    uint32_t source_encoding,
    uint32_t source_length
)
{
    if (operation_id == 0 || operation_id > operation_count)
        return ARMITO_IR_OUT_OF_RANGE;

    if (source_length > ARMITO_IR_MAX_SOURCE_BYTES)
        return ARMITO_IR_OUT_OF_RANGE;

    operations[operation_id - 1].source_offset = source_offset;
    operations[operation_id - 1].source_encoding = source_encoding;
    operations[operation_id - 1].source_length = source_length;

    return ARMITO_IR_SUCCESS;
}

ArmitoIRStatus ArmitoIROperationSetAux(
    ArmitoIROperationID operation_id,
    uint16_t aux
)
{
    if (operation_id == 0 || operation_id > operation_count)
        return ARMITO_IR_OUT_OF_RANGE;

    operations[operation_id - 1].aux = aux;

    return ARMITO_IR_SUCCESS;
}

ArmitoIRStatus ArmitoIROperationGet(
    ArmitoIROperationID operation_id,
    ArmitoIROperation *operation
)
{
    if (operation == NULL)
        return ARMITO_IR_INVALID_ARGUMENT;

    if (operation_id == 0 || operation_id > operation_count)
        return ARMITO_IR_OUT_OF_RANGE;

    *operation = operations[operation_id - 1];

    return ARMITO_IR_SUCCESS;
}

uint32_t ArmitoIROperationsCount(void)
{
    return operation_count;
}

const char *ArmitoIROperationName(ArmitoIROperationKind kind)
{
    if (kind > ARMITO_IR_OP_UNSUPPORTED)
        return "invalid";

    return operation_names[kind];
}
