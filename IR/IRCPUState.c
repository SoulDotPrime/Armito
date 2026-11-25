#include "Includes/IRCPUState.h"

static ArmitoIRCPURegister registers[ARMITO_IR_MAX_CPU_REGISTERS];
static uint32_t register_count = 0;

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

ArmitoIRStatus ArmitoIRCPUStateReset(void)
{
    uint32_t i;

    for (i = 0; i < ARMITO_IR_MAX_CPU_REGISTERS; i++) {
        registers[i].index = 0;
        registers[i].reg_class = ARMITO_IR_REG_GPR;
        registers[i].value = ARMITO_IR_VALUE_INVALID;
        registers[i].bit_width = 0;
        registers[i].source_name[0] = '\0';
    }

    register_count = 0;

    return ARMITO_IR_SUCCESS;
}

ArmitoIRStatus ArmitoIRCPUStateMap(
    ArmitoIRRegisterClass reg_class,
    uint8_t index,
    uint16_t bit_width,
    const char *source_name,
    ArmitoIRValueID value
)
{
    ArmitoIRCPURegister *reg;

    if (reg_class > ARMITO_IR_REG_STACK_POINTER)
        return ARMITO_IR_INVALID_ARGUMENT;

    if (bit_width == 0)
        return ARMITO_IR_INVALID_ARGUMENT;

    if (value == ARMITO_IR_VALUE_INVALID || value > ArmitoIRValuesCount())
        return ARMITO_IR_OUT_OF_RANGE;

    if (ArmitoIRCPUStateLookup(reg_class, index, NULL) == ARMITO_IR_SUCCESS)
        return ARMITO_IR_INVALID_STATE;

    if (register_count >= ARMITO_IR_MAX_CPU_REGISTERS)
        return ARMITO_IR_OUT_OF_RESOURCES;

    reg = &registers[register_count];
    reg->index = index;
    reg->reg_class = reg_class;
    reg->value = value;
    reg->bit_width = bit_width;
    CopyName(reg->source_name, source_name);

    register_count++;

    return ARMITO_IR_SUCCESS;
}

ArmitoIRStatus ArmitoIRCPUStateLookup(
    ArmitoIRRegisterClass reg_class,
    uint8_t index,
    ArmitoIRValueID *value
)
{
    uint32_t i;

    for (i = 0; i < register_count; i++) {
        if (registers[i].reg_class != reg_class)
            continue;

        if (registers[i].index != index)
            continue;

        if (value != NULL)
            *value = registers[i].value;

        return ARMITO_IR_SUCCESS;
    }

    if (value != NULL)
        *value = ARMITO_IR_VALUE_INVALID;

    return ARMITO_IR_NOT_SUPPORTED;
}

ArmitoIRStatus ArmitoIRCPUStateGet(uint8_t slot, ArmitoIRCPURegister *reg)
{
    if (reg == NULL)
        return ARMITO_IR_INVALID_ARGUMENT;

    if (slot >= register_count)
        return ARMITO_IR_OUT_OF_RANGE;

    *reg = registers[slot];

    return ARMITO_IR_SUCCESS;
}

uint32_t ArmitoIRCPUStateCount(void)
{
    return register_count;
}
