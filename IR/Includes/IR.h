#ifndef ARMITO_IR_H
#define ARMITO_IR_H

#include "Includes/IRTypes.h"
#include "Includes/IRTypeset.h"
#include "Includes/IRValue.h"
#include "Includes/IROperation.h"
#include "Includes/IRBlock.h"
#include "Includes/IRCPUState.h"

ArmitoIRStatus ArmitoIRInitialize(void);
ArmitoIRStatus ArmitoIRReset(void);
ArmitoIRStatus ArmitoIRFinalize(void);
ArmitoIRState ArmitoIRGetState(void);

ArmitoIRStatus ArmitoIRValidate(void);

uint32_t ArmitoIRTypeCount(void);
uint32_t ArmitoIRValueCount(void);
uint32_t ArmitoIROperationCount(void);
uint32_t ArmitoIRBlockCount(void);
uint32_t ArmitoIRCPURegisterCount(void);

#endif
