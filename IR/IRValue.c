#include "Includes/IRValue.h"
#include "Includes/IRTypeset.h"

static ArmitoIRValue ir_value_table[ARMITO_IR_MAX_VALUES];
static uint32_t ir_value_total = 0;

static void IRValueCopyName(char *dest, const char *src)
{
    uint32_t i = 0;

    if (dest == NULL)
        return;

    if (src == NULL) {
        dest[0] = '\0';
        return;
    }

    while (src[i] != '\0' && i < (ARMITO_IR_MAX_NAME - 1)) {
        dest[i] = src[i];
        i++;
    }

    dest[i] = '\0';
}

ArmitoIRStatus ArmitoIRValuesReset(void)
{
    uint32_t i;

    for (i = 0; i < ARMITO_IR_MAX_VALUES; i++) {
        ir_value_table[i].id = ARMITO_IR_VALUE_INVALID;
        ir_value_table[i].kind = ARMITO_IR_VALUE_VIRTUAL_REGISTER;
        ir_value_table[i].type_id = ARMITO_IR_TYPE_INVALID;
        ir_value_table[i].role = 0;
        ir_value_table[i].reg_index = 0;
        ir_value_table[i].flags_role = 0;
        ir_value_table[i].immediate = 0;
        ir_value_table[i].memory_kind = ARMITO_IR_MEM_NONE;
        ir_value_table[i].address = ARMITO_IR_VALUE_INVALID;
        ir_value_table[i].address_offset = 0;
        ir_value_table[i].displacement = 0;
        ir_value_table[i].source_name[0] = '\0';
    }

    ir_value_total = 0;

    return ARMITO_IR_SUCCESS;
}

ArmitoIRStatus ArmitoIRValuesDeclare(
    ArmitoIRValueKind kind,
    ArmitoIRTypeID type_id,
    const char *name,
    ArmitoIRValueID *value_id
)
{
    ArmitoIRValue *value;

    if (value_id == NULL)
        return ARMITO_IR_INVALID_ARGUMENT;

    *value_id = ARMITO_IR_VALUE_INVALID;

    if (ir_value_total >= ARMITO_IR_MAX_VALUES)
        return ARMITO_IR_OUT_OF_RESOURCES;

    if (type_id != ARMITO_IR_TYPE_INVALID && type_id > ArmitoIRTypesCount())
        return ARMITO_IR_OUT_OF_RANGE;

    value = &ir_value_table[ir_value_total];
    value->id = ir_value_total + 1;
    value->kind = kind;
    value->type_id = type_id;
    IRValueCopyName(value->source_name, name);

    ir_value_total++;

    *value_id = value->id;

    return ARMITO_IR_SUCCESS;
}

ArmitoIRStatus ArmitoIRValuesGet(ArmitoIRValueID value_id, ArmitoIRValue *value)
{
    if (value == NULL)
        return ARMITO_IR_INVALID_ARGUMENT;

    if (value_id == ARMITO_IR_VALUE_INVALID || value_id > ir_value_total)
        return ARMITO_IR_OUT_OF_RANGE;

    *value = ir_value_table[value_id - 1];

    return ARMITO_IR_SUCCESS;
}

ArmitoIRStatus ArmitoIRValuesSetImmediate(
    ArmitoIRValueID value_id,
    uint64_t immediate
)
{
    if (value_id == ARMITO_IR_VALUE_INVALID || value_id > ir_value_total)
        return ARMITO_IR_OUT_OF_RANGE;

    if (ir_value_table[value_id - 1].kind != ARMITO_IR_VALUE_IMMEDIATE)
        return ARMITO_IR_INVALID_STATE;

    ir_value_table[value_id - 1].immediate = immediate;

    return ARMITO_IR_SUCCESS;
}

ArmitoIRStatus ArmitoIRValuesSetMemory(
    ArmitoIRValueID value_id,
    ArmitoIRMemoryKind memory_kind,
    ArmitoIRValueID address,
    int64_t displacement
)
{
    ArmitoIRValue *value;

    if (value_id == ARMITO_IR_VALUE_INVALID || value_id > ir_value_total)
        return ARMITO_IR_OUT_OF_RANGE;

    if (memory_kind == ARMITO_IR_MEM_NONE)
        return ARMITO_IR_INVALID_ARGUMENT;

    if (address == ARMITO_IR_VALUE_INVALID || address > ir_value_total)
        return ARMITO_IR_OUT_OF_RANGE;

    value = &ir_value_table[value_id - 1];

    if (value->kind != ARMITO_IR_VALUE_MEMORY)
        return ARMITO_IR_INVALID_STATE;

    value->memory_kind = memory_kind;
    value->address = address;
    value->displacement = displacement;

    return ARMITO_IR_SUCCESS;
}

uint32_t ArmitoIRValuesCount(void)
{
    return ir_value_total;
}
