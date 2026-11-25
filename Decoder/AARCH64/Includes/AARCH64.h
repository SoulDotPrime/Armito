#ifndef ARMITO_AARCH64_H
#define ARMITO_AARCH64_H

#include "Decoder/Includes/DecoderTypes.h"
#include "Decoder/Includes/DecoderRegister.h"
#include "Includes/IRTypes.h"
#include "Includes/IROperation.h"

#define AARCH64_MASK_OP0      0x1F000000u
#define AARCH64_SHIFT_OP0     25u
#define AARCH64_MASK_GROUP    0x18000000u
#define AARCH64_SHIFT_GROUP   26u
#define AARCH64_SHIFT_OP1     25u
#define AARCH64_SHIFT_OP2     21u
#define AARCH64_MASK_OP1      0x3Fu
#define AARCH64_MASK_TOP8     0x0F000000u
#define AARCH64_SHIFT_TOP8    24u

#define AARCH64_GROUP_DP_IMM     0u
#define AARCH64_GROUP_BRANCH     1u
#define AARCH64_GROUP_LOAD_STORE 2u
#define AARCH64_GROUP_DP_REG     2u

#define AARCH64_BRANCH_OPC_SHIFT 31u
#define AARCH64_BRANCH_OP_SHIFT  26u
#define AARCH64_BRANCH_OP_MASK   0x1Fu

#define AARCH64_UNCOND_BRANCH_IMM  0x05u
#define AARCH64_UNCOND_BRANCH_REG  0x0Du
#define AARCH64_COND_BRANCH_IMM    0x0Au
#define AARCH64_COMPARE_BRANCH_IMM 0x0Bu
#define AARCH64_TEST_BRANCH_IMM    0x0Bu
#define AARCH64_EXCEPTION_GEN      0x0Eu
#define AARCH64_EXCEPTION_UNCOND   0x15u

#define AARCH64_SF_BIT       31u
#define AARCH64_OPC_ADD_SHIFT 0x0Bu
#define AARCH64_OPC_SUB_SHIFT 0x0Fu

typedef enum {
    AARCH64_OPCODE_UNKNOWN = 0,
    AARCH64_OPCODE_NOP,
    AARCH64_OPCODE_RET,
    AARCH64_OPCODE_BR,
    AARCH64_OPCODE_BL,
    AARCH64_OPCODE_B_COND,
    AARCH64_OPCODE_CBZ,
    AARCH64_OPCODE_CBNZ,
    AARCH64_OPCODE_TBZ,
    AARCH64_OPCODE_TBNZ,
    AARCH64_OPCODE_MOV_REG,
    AARCH64_OPCODE_MOV_IMM,
    AARCH64_OPCODE_ADD_SHIFTED,
    AARCH64_OPCODE_SUB_SHIFTED,
    AARCH64_OPCODE_LOAD_STORE
} AARCH64Opcode;

typedef struct {
    AARCH64Opcode opcode;
    uint8_t width_bits;
    uint8_t set_flags;
    uint8_t rd_index;
    uint8_t rn_index;
    uint8_t rm_index;
    uint8_t rd_is_sp;
    uint8_t rn_is_sp;
    uint8_t rm_is_sp;
    uint8_t rd_is_zero;
    uint8_t rn_is_zero;
    uint8_t rm_is_zero;
    uint8_t shift_type;
    uint8_t shift_amount;
    uint8_t condition;
    uint8_t is_64bit_immediate;
    int64_t immediate;
    uint64_t branch_target;
    uint32_t encoding;
} AARCH64Instruction;

ArmitoDecoderStatus AARCH64Classify(uint32_t encoding, AARCH64Opcode *opcode);
ArmitoDecoderStatus AARCH64Decode(uint32_t encoding, AARCH64Instruction *instruction);
ArmitoDecoderStatus AARCH64ToIR(
    const AARCH64Instruction *instruction,
    uint64_t address,
    uint32_t *operations_emitted
);
const char *AARCH64OpcodeName(AARCH64Opcode opcode);
ArmitoDecodeResult AARCH64ResultFor(ArmitoDecoderStatus status);

#endif
