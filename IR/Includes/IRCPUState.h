#ifndef ARMITO_IR_CPU_STATE_H
#define ARMITO_IR_CPU_STATE_H

#include "Includes/IRTypes.h"
#include "Includes/IRValue.h"

#define ARMITO_IR_MAX_CPU_REGISTERS 64

typedef enum {
    ARMITO_IR_REG_GPR = 0,
    ARMITO_IR_REG_VECTOR,
    ARMITO_IR_REG_FLAGS,
    ARMITO_IR_REG_PC,
    ARMITO_IR_REG_STACK_POINTER
} ArmitoIRRegisterClass;

typedef struct {
    uint8_t index;
    ArmitoIRRegisterClass reg_class;
    ArmitoIRValueID value;
    uint16_t bit_width;
    char source_name[ARMITO_IR_MAX_NAME];
} ArmitoIRCPURegister;

ArmitoIRStatus ArmitoIRCPUStateReset(void);
ArmitoIRStatus ArmitoIRCPUStateMap(
    ArmitoIRRegisterClass reg_class,
    uint8_t index,
    uint16_t bit_width,
    const char *source_name,
    ArmitoIRValueID value
);
ArmitoIRStatus ArmitoIRCPUStateLookup(
    ArmitoIRRegisterClass reg_class,
    uint8_t index,
    ArmitoIRValueID *value
);
ArmitoIRStatus ArmitoIRCPUStateGet(
    uint8_t slot,
    ArmitoIRCPURegister *reg
);
uint32_t ArmitoIRCPUStateCount(void);

#endif
