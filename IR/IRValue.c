#include "Includes/IRValue.h"
#include "Includes/IRTypeset.h"

static ArmitoIRValue values[ARMITO_IR_MAX_VALUES];
static uint32_t value_count = 0;

static void CopyName(char *dest, const char *src)
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
        values[i].id = ARMITO_IR_VALUE_INVALID;
        values[i].kind = ARMITO_IR_VALUE_VIRTUAL_REGISTER;
        values[i].type_id = ARMITO_IR_TYPE_INVALID;
        values[i].role = 0;
        values[i].reg_index = 0;
        values[i].flags_role = 0;
        values[i].immediate = 0;
        values[i].memory_kind = ARMITO_IR_MEM_NONE;
        values[i].address = ARMITO_IR_VALUE_INVALID;
        values[i].address_offset = 0;
        values[i].displacement = 0;
        values[i].source_name[0] = '\0';
    }

    value_count = 0;

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

    if (value_count >= ARMITO_IR_MAX_VALUES)
        return ARMITO_IR_OUT_OF_RESOURCES;

    if (type_id != ARMITO_IR_TYPE_INVALID && type_id > ArmitoIRTypesCount())
        return ARMITO_IR_OUT_OF_RANGE;

    value = &values[value_count];
    value->id = value_count + 1;
    value->kind = kind;
    value->type_id = type_id;
    CopyName(value->source_name, name);

    value_count++;

    *value_id = value->id;

    return ARMITO_IR_SUCCESS;
}

ArmitoIRStatus ArmitoIRValuesGet(ArmitoIRValueID value_id, ArmitoIRValue *value)
{
    if (value == NULL)
        return ARMITO_IR_INVALID_ARGUMENT;

    if (value_id == ARMITO_IR_VALUE_INVALID || value_id > value_count)
        return ARMITO_IR_OUT_OF_RANGE;

    *value = values[value_id - 1];

    return ARMITO_IR_SUCCESS;
}

ArmitoIRStatus ArmitoIRValuesSetImmediate(
    ArmitoIRValueID value_id,
    uint64_t immediate
)
{
    if (value_id == ARMITO_IR_VALUE_INVALID || value_id > value_count)
        return ARMITO_IR_OUT_OF_RANGE;

    if (values[value_id - 1].kind != ARMITO_IR_VALUE_IMMEDIATE)
        return ARMITO_IR_INVALID_STATE;

    values[value_id - 1].immediate = immediate;

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

    if (value_id == ARMITO_IR_VALUE_INVALID || value_id > value_count)
        return ARMITO_IR_OUT_OF_RANGE;

    if (memory_kind == ARMITO_IR_MEM_NONE)
        return ARMITO_IR_INVALID_ARGUMENT;

    if (address == ARMITO_IR_VALUE_INVALID || address > value_count)
        return ARMITO_IR_OUT_OF_RANGE;

    value = &values[value_id - 1];

    if (value->kind != ARMITO_IR_VALUE_MEMORY)
        return ARMITO_IR_INVALID_STATE;

    value->memory_kind = memory_kind;
    value->address = address;
    value->displacement = displacement;

    return ARMITO_IR_SUCCESS;
}

uint32_t ArmitoIRValuesCount(void)
{
    return value_count;
}
