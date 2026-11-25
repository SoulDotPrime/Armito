#ifndef ARMITO_IR_TYPESET_H
#define ARMITO_IR_TYPESET_H

#include "Includes/IRTypes.h"

typedef enum {
    ARMITO_IR_CLASS_VOID = 0,
    ARMITO_IR_CLASS_INT,
    ARMITO_IR_CLASS_FLOAT,
    ARMITO_IR_CLASS_VECTOR,
    ARMITO_IR_CLASS_FLAGS
} ArmitoIRTypeClass;

typedef struct {
    ArmitoIRTypeID id;
    ArmitoIRTypeClass type_class;
    uint16_t bit_width;
    uint16_t lane_count;
    uint8_t is_signed;
    uint8_t is_pointer;
    char name[ARMITO_IR_MAX_NAME];
} ArmitoIRType;

ArmitoIRStatus ArmitoIRTypesReset(void);
ArmitoIRStatus ArmitoIRTypesRegister(
    ArmitoIRTypeClass type_class,
    uint16_t bit_width,
    uint16_t lane_count,
    uint8_t is_signed,
    uint8_t is_pointer,
    const char *name,
    ArmitoIRTypeID *type_id
);
ArmitoIRStatus ArmitoIRTypesGet(ArmitoIRTypeID type_id, ArmitoIRType *type);
ArmitoIRStatus ArmitoIRTypesLookup(
    ArmitoIRTypeClass type_class,
    uint16_t bit_width,
    uint16_t lane_count,
    uint8_t is_signed,
    ArmitoIRTypeID *type_id
);
ArmitoIRStatus ArmitoIRTypesSize(ArmitoIRTypeID type_id, uint32_t *size_bytes);
uint32_t ArmitoIRTypesCount(void);

#endif
