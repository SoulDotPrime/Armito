#include "Includes/IRTypeset.h"

static ArmitoIRType types[ARMITO_IR_MAX_TYPES];
static uint32_t type_count = 0;

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

ArmitoIRStatus ArmitoIRTypesReset(void)
{
    uint32_t i;

    for (i = 0; i < ARMITO_IR_MAX_TYPES; i++) {
        types[i].id = ARMITO_IR_TYPE_INVALID;
        types[i].type_class = ARMITO_IR_CLASS_VOID;
        types[i].bit_width = 0;
        types[i].lane_count = 0;
        types[i].is_signed = 0;
        types[i].is_pointer = 0;
        types[i].name[0] = '\0';
    }

    type_count = 0;

    return ARMITO_IR_SUCCESS;
}

ArmitoIRStatus ArmitoIRTypesRegister(
    ArmitoIRTypeClass type_class,
    uint16_t bit_width,
    uint16_t lane_count,
    uint8_t is_signed,
    uint8_t is_pointer,
    const char *name,
    ArmitoIRTypeID *type_id
)
{
    ArmitoIRType *type;

    if (type_id == NULL)
        return ARMITO_IR_INVALID_ARGUMENT;

    *type_id = ARMITO_IR_TYPE_INVALID;

    if (type_count >= ARMITO_IR_MAX_TYPES)
        return ARMITO_IR_OUT_OF_RESOURCES;

    if (type_class == ARMITO_IR_CLASS_VOID && bit_width != 0)
        return ARMITO_IR_INVALID_ARGUMENT;

    if (type_class != ARMITO_IR_CLASS_VOID && bit_width == 0)
        return ARMITO_IR_INVALID_ARGUMENT;

    if (type_class == ARMITO_IR_CLASS_VECTOR && lane_count == 0)
        return ARMITO_IR_INVALID_ARGUMENT;

    if (type_class != ARMITO_IR_CLASS_VECTOR && lane_count > 1)
        return ARMITO_IR_INVALID_ARGUMENT;

    type = &types[type_count];
    type->id = type_count + 1;
    type->type_class = type_class;
    type->bit_width = bit_width;
    type->lane_count = (lane_count == 0) ? 1 : lane_count;
    type->is_signed = is_signed;
    type->is_pointer = is_pointer;
    CopyName(type->name, name);

    type_count++;

    *type_id = type->id;

    return ARMITO_IR_SUCCESS;
}

ArmitoIRStatus ArmitoIRTypesGet(ArmitoIRTypeID type_id, ArmitoIRType *type)
{
    if (type == NULL)
        return ARMITO_IR_INVALID_ARGUMENT;

    if (type_id == ARMITO_IR_TYPE_INVALID || type_id > type_count)
        return ARMITO_IR_OUT_OF_RANGE;

    *type = types[type_id - 1];

    return ARMITO_IR_SUCCESS;
}

ArmitoIRStatus ArmitoIRTypesLookup(
    ArmitoIRTypeClass type_class,
    uint16_t bit_width,
    uint16_t lane_count,
    uint8_t is_signed,
    ArmitoIRTypeID *type_id
)
{
    uint32_t i;
    uint16_t lanes = (lane_count == 0) ? 1 : lane_count;

    if (type_id == NULL)
        return ARMITO_IR_INVALID_ARGUMENT;

    for (i = 0; i < type_count; i++) {
        if (types[i].type_class != type_class)
            continue;

        if (types[i].bit_width != bit_width)
            continue;

        if (types[i].lane_count != lanes)
            continue;

        if (types[i].is_signed != is_signed)
            continue;

        *type_id = types[i].id;

        return ARMITO_IR_SUCCESS;
    }

    *type_id = ARMITO_IR_TYPE_INVALID;

    return ARMITO_IR_NOT_SUPPORTED;
}

ArmitoIRStatus ArmitoIRTypesSize(ArmitoIRTypeID type_id, uint32_t *size_bytes)
{
    ArmitoIRType type;
    ArmitoIRStatus status;

    if (size_bytes == NULL)
        return ARMITO_IR_INVALID_ARGUMENT;

    status = ArmitoIRTypesGet(type_id, &type);

    if (status != ARMITO_IR_SUCCESS)
        return status;

    if (type.type_class == ARMITO_IR_CLASS_VOID) {
        *size_bytes = 0;
        return ARMITO_IR_SUCCESS;
    }

    *size_bytes = (type.bit_width / 8u) * type.lane_count;

    if (*size_bytes == 0)
        return ARMITO_IR_OUT_OF_RANGE;

    return ARMITO_IR_SUCCESS;
}

uint32_t ArmitoIRTypesCount(void)
{
    return type_count;
}
