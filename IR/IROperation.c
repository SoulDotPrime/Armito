#include "Includes/IROperation.h"
#include "Includes/IRValue.h"
#include "Includes/IRTypeset.h"

static ArmitoIROperation ir_operation_table[ARMITO_IR_MAX_OPERATIONS];
static uint32_t ir_operation_total = 0;

static const char *const ir_operation_name_table[] = {
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
    "unsupported",
    "pac.sign",
    "pac.authenticate",
    "pac.strip",
    "pac.branch.auth",
    "pac.call.auth",
    "pac.return.auth",
    "pac.load.auth",
    "pac.authentication.failure"
};

ArmitoIRStatus ArmitoIROperationsReset(void)
{
    uint32_t i;
    uint32_t j;

    for (i = 0; i < ARMITO_IR_MAX_OPERATIONS; i++) {
        ir_operation_table[i].id = 0;
        ir_operation_table[i].kind = ARMITO_IR_OP_NOP;
        ir_operation_table[i].result_type = ARMITO_IR_TYPE_INVALID;
        ir_operation_table[i].result = ARMITO_IR_VALUE_INVALID;

        for (j = 0; j < ARMITO_IR_MAX_INPUTS; j++)
            ir_operation_table[i].inputs[j] = ARMITO_IR_VALUE_INVALID;

        ir_operation_table[i].input_count = 0;
        ir_operation_table[i].flag_mask = 0;
        ir_operation_table[i].target = ARMITO_IR_LABEL_INVALID;
        ir_operation_table[i].target_false = ARMITO_IR_LABEL_INVALID;
        ir_operation_table[i].source_offset = 0;
        ir_operation_table[i].source_encoding = 0;
        ir_operation_table[i].source_length = 0;
        ir_operation_table[i].aux = 0;
        ir_operation_table[i].pointer_auth_key = ARMITO_IR_PAC_KEY_INVALID;
        ir_operation_table[i].pointer_auth_flags = 0;
        ir_operation_table[i].pointer_auth_discriminator = 0;
        ir_operation_table[i].has_effect_register = 0;
        ir_operation_table[i].effect_register_class = ARMITO_IR_REG_GPR;
        ir_operation_table[i].effect_register_index = 0;
    }

    ir_operation_total = 0;

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

    if (kind > ARMITO_IR_OP_PAC_AUTHENTICATION_FAILURE)
        return ARMITO_IR_INVALID_ARGUMENT;

    if (input_count > ARMITO_IR_MAX_INPUTS)
        return ARMITO_IR_INVALID_ARGUMENT;

    if (input_count > 0 && inputs == NULL)
        return ARMITO_IR_INVALID_ARGUMENT;

    if (ir_operation_total >= ARMITO_IR_MAX_OPERATIONS)
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

    operation = &ir_operation_table[ir_operation_total];
    operation->id = ir_operation_total + 1;
    operation->kind = kind;
    operation->result_type = result_type;
    operation->result = result;
    operation->input_count = input_count;
    operation->pointer_auth_key = ARMITO_IR_PAC_KEY_INVALID;
    operation->pointer_auth_flags = 0;
    operation->pointer_auth_discriminator = 0;
    operation->has_effect_register = 0;
    operation->effect_register_class = ARMITO_IR_REG_GPR;
    operation->effect_register_index = 0;

    for (i = 0; i < input_count; i++)
        operation->inputs[i] = inputs[i];

    ir_operation_total++;

    *operation_id = operation->id;

    return ARMITO_IR_SUCCESS;
}

ArmitoIRStatus ArmitoIROperationSetFlags(
    ArmitoIROperationID operation_id,
    uint8_t flag_mask
)
{
    if (operation_id == 0 || operation_id > ir_operation_total)
        return ARMITO_IR_OUT_OF_RANGE;

    if ((flag_mask & ~(uint8_t)0x0F) != 0)
        return ARMITO_IR_INVALID_ARGUMENT;

    ir_operation_table[operation_id - 1].flag_mask = flag_mask;

    return ARMITO_IR_SUCCESS;
}

ArmitoIRStatus ArmitoIROperationSetTarget(
    ArmitoIROperationID operation_id,
    ArmitoIRLabelID target,
    ArmitoIRLabelID target_false
)
{
    if (operation_id == 0 || operation_id > ir_operation_total)
        return ARMITO_IR_OUT_OF_RANGE;

    ir_operation_table[operation_id - 1].target = target;
    ir_operation_table[operation_id - 1].target_false = target_false;

    return ARMITO_IR_SUCCESS;
}

ArmitoIRStatus ArmitoIROperationSetSource(
    ArmitoIROperationID operation_id,
    uint32_t source_offset,
    uint32_t source_encoding,
    uint32_t source_length
)
{
    if (operation_id == 0 || operation_id > ir_operation_total)
        return ARMITO_IR_OUT_OF_RANGE;

    if (source_length > ARMITO_IR_MAX_SOURCE_BYTES)
        return ARMITO_IR_OUT_OF_RANGE;

    ir_operation_table[operation_id - 1].source_offset = source_offset;
    ir_operation_table[operation_id - 1].source_encoding = source_encoding;
    ir_operation_table[operation_id - 1].source_length = source_length;

    return ARMITO_IR_SUCCESS;
}

ArmitoIRStatus ArmitoIROperationSetAux(
    ArmitoIROperationID operation_id,
    uint16_t aux
)
{
    if (operation_id == 0 || operation_id > ir_operation_total)
        return ARMITO_IR_OUT_OF_RANGE;

    ir_operation_table[operation_id - 1].aux = aux;

    return ARMITO_IR_SUCCESS;
}

ArmitoIRStatus ArmitoIROperationSetPointerAuth(
    ArmitoIROperationID operation_id,
    ArmitoIRPointerAuthKey key,
    uint8_t flags,
    uint16_t discriminator
)
{
    ArmitoIROperation *operation;

    if (operation_id == 0 || operation_id > ir_operation_total)
        return ARMITO_IR_OUT_OF_RANGE;

    operation = &ir_operation_table[operation_id - 1];
    if (operation->kind < ARMITO_IR_OP_PAC_SIGN ||
        operation->kind > ARMITO_IR_OP_PAC_AUTHENTICATION_FAILURE)
        return ARMITO_IR_INVALID_STATE;

    if ((uint32_t)key > (uint32_t)ARMITO_IR_PAC_KEY_DB)
        return ARMITO_IR_INVALID_ARGUMENT;

    if ((flags & (uint8_t)~0x1Fu) != 0)
        return ARMITO_IR_INVALID_ARGUMENT;

    operation->pointer_auth_key = (uint8_t)key;
    operation->pointer_auth_flags = flags;
    operation->pointer_auth_discriminator = discriminator;

    return ARMITO_IR_SUCCESS;
}

ArmitoIRStatus ArmitoIROperationSetEffectRegister(
    ArmitoIROperationID operation_id,
    ArmitoIRRegisterClass reg_class,
    uint8_t index
)
{
    ArmitoIROperation *operation;

    if (operation_id == 0 || operation_id > ir_operation_total)
        return ARMITO_IR_OUT_OF_RANGE;

    operation = &ir_operation_table[operation_id - 1];
    if (operation->kind < ARMITO_IR_OP_PAC_SIGN ||
        operation->kind > ARMITO_IR_OP_PAC_AUTHENTICATION_FAILURE)
        return ARMITO_IR_INVALID_STATE;

    if (reg_class > ARMITO_IR_REG_STACK_POINTER)
        return ARMITO_IR_INVALID_ARGUMENT;

    if ((reg_class == ARMITO_IR_REG_GPR && index >= 31u) ||
        (reg_class == ARMITO_IR_REG_VECTOR && index >= 32u) ||
        (reg_class == ARMITO_IR_REG_FLAGS && index != 0u) ||
        (reg_class == ARMITO_IR_REG_PC && index != 0u) ||
        (reg_class == ARMITO_IR_REG_STACK_POINTER && index != 31u))
        return ARMITO_IR_OUT_OF_RANGE;

    operation->has_effect_register = 1;
    operation->effect_register_class = (uint8_t)reg_class;
    operation->effect_register_index = index;
    return ARMITO_IR_SUCCESS;
}

ArmitoIRStatus ArmitoIROperationGet(
    ArmitoIROperationID operation_id,
    ArmitoIROperation *operation
)
{
    if (operation == NULL)
        return ARMITO_IR_INVALID_ARGUMENT;

    if (operation_id == 0 || operation_id > ir_operation_total)
        return ARMITO_IR_OUT_OF_RANGE;

    *operation = ir_operation_table[operation_id - 1];

    return ARMITO_IR_SUCCESS;
}

uint32_t ArmitoIROperationsCount(void)
{
    return ir_operation_total;
}

const char *ArmitoIROperationName(ArmitoIROperationKind kind)
{
    if (kind > ARMITO_IR_OP_PAC_AUTHENTICATION_FAILURE)
        return "invalid";

    return ir_operation_name_table[kind];
}
