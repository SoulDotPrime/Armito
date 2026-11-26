#include "Includes/IRCPUState.h"

static ArmitoIRCPURegister ir_register_table[ARMITO_IR_MAX_CPU_REGISTERS];
static uint32_t ir_register_total = 0;

static void IRCPUStateCopyName(char *dest, const char *src)
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
        ir_register_table[i].index = 0;
        ir_register_table[i].reg_class = ARMITO_IR_REG_GPR;
        ir_register_table[i].value = ARMITO_IR_VALUE_INVALID;
        ir_register_table[i].bit_width = 0;
        ir_register_table[i].source_name[0] = '\0';
    }

    ir_register_total = 0;

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

    if (ir_register_total >= ARMITO_IR_MAX_CPU_REGISTERS)
        return ARMITO_IR_OUT_OF_RESOURCES;

    reg = &ir_register_table[ir_register_total];
    reg->index = index;
    reg->reg_class = reg_class;
    reg->value = value;
    reg->bit_width = bit_width;
    IRCPUStateCopyName(reg->source_name, source_name);

    ir_register_total++;

    return ARMITO_IR_SUCCESS;
}

ArmitoIRStatus ArmitoIRCPUStateLookup(
    ArmitoIRRegisterClass reg_class,
    uint8_t index,
    ArmitoIRValueID *value
)
{
    uint32_t i;

    for (i = 0; i < ir_register_total; i++) {
        if (ir_register_table[i].reg_class != reg_class)
            continue;

        if (ir_register_table[i].index != index)
            continue;

        if (value != NULL)
            *value = ir_register_table[i].value;

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

    if (slot >= ir_register_total)
        return ARMITO_IR_OUT_OF_RANGE;

    *reg = ir_register_table[slot];

    return ARMITO_IR_SUCCESS;
}

uint32_t ArmitoIRCPUStateCount(void)
{
    return ir_register_total;
}
