#include "Includes/AARCH64.h"

#include "Includes/IR.h"

static const char *const opcode_names[] = {
    "unknown",
    "nop",
    "ret",
    "br",
    "bl",
    "b.cond",
    "cbz",
    "cbnz",
    "tbz",
    "tbnz",
    "mov.reg",
    "mov.imm",
    "add.shifted",
    "sub.shifted",
    "load.store"
};

ArmitoDecoderStatus AARCH64Classify(uint32_t encoding, AARCH64Opcode *opcode)
{
    uint32_t op0;
    uint32_t op1;
    uint32_t top8;

    if (opcode == NULL)
        return ARMITO_DECODER_INVALID_ARGUMENT;

    *opcode = AARCH64_OPCODE_UNKNOWN;

    if ((encoding & 0xFFFFFC1Fu) == 0xD503201Fu) {
        *opcode = AARCH64_OPCODE_NOP;
        return ARMITO_DECODER_SUCCESS;
    }

    op1 = (encoding >> 25) & 0x3Fu;

    if ((encoding & 0x1C000000u) == 0x14000000u &&
        op1 != AARCH64_UNCOND_BRANCH_REG) {
        switch (op1) {
        case AARCH64_UNCOND_BRANCH_IMM:
            *opcode = ((encoding >> AARCH64_BRANCH_OPC_SHIFT) & 1u)
                ? AARCH64_OPCODE_BL
                : AARCH64_OPCODE_BR;
            return ARMITO_DECODER_SUCCESS;
        case AARCH64_COND_BRANCH_IMM:
            *opcode = AARCH64_OPCODE_B_COND;
            return ARMITO_DECODER_SUCCESS;
        case AARCH64_COMPARE_BRANCH_IMM:
            *opcode = ((encoding >> 24) & 1u)
                ? AARCH64_OPCODE_CBNZ
                : AARCH64_OPCODE_CBZ;
            return ARMITO_DECODER_SUCCESS;
        case AARCH64_EXCEPTION_GEN:
            *opcode = ((encoding >> 24) & 1u)
                ? AARCH64_OPCODE_TBNZ
                : AARCH64_OPCODE_TBZ;
            return ARMITO_DECODER_SUCCESS;
        default:
            return ARMITO_DECODER_UNSUPPORTED_INSTRUCTION;
        }
    }

    if ((encoding & 0xFF000000u) == 0xD6000000u) {
        if ((encoding & 0xFFFFFC1Fu) == 0xD61F0000u) {
            *opcode = AARCH64_OPCODE_BR;
            return ARMITO_DECODER_SUCCESS;
        }
        if ((encoding & 0xFFFFFC1Fu) == 0xD63F0000u) {
            *opcode = AARCH64_OPCODE_BL;
            return ARMITO_DECODER_SUCCESS;
        }
        if ((encoding & 0xFFFFFC1Fu) == 0xD65F0000u) {
            *opcode = AARCH64_OPCODE_RET;
            return ARMITO_DECODER_SUCCESS;
        }
        return ARMITO_DECODER_UNSUPPORTED_INSTRUCTION;
    }

    top8 = (encoding & AARCH64_MASK_TOP8) >> AARCH64_SHIFT_TOP8;

    op0 = (encoding >> AARCH64_SHIFT_OP0) & 0x1Fu;

    if ((encoding & 0x1C000000u) == 0x08000000u) {
        if ((encoding & 0x1F000000u) == 0x0A000000u &&
            (op0 == 0x0Bu || op0 == 0x0Fu)) {
            *opcode = ((encoding >> 30) & 1u)
                ? AARCH64_OPCODE_SUB_SHIFTED
                : AARCH64_OPCODE_ADD_SHIFTED;
            return ARMITO_DECODER_SUCCESS;
        }

        if ((encoding & 0x7FE0FFE0u) == 0x2A0003E0u ||
            (encoding & 0x7FE0FFE0u) == 0x2A0003E0u) {
            *opcode = AARCH64_OPCODE_MOV_REG;
            return ARMITO_DECODER_SUCCESS;
        }

        return ARMITO_DECODER_UNSUPPORTED_INSTRUCTION;
    }

    if ((encoding & 0x1C000000u) == 0x10000000u) {
        if (top8 == 0x02u) {
            *opcode = AARCH64_OPCODE_MOV_IMM;
            return ARMITO_DECODER_SUCCESS;
        }

        return ARMITO_DECODER_UNSUPPORTED_INSTRUCTION;
    }

    if ((encoding & 0x1C000000u) == 0x04000000u ||
        (encoding & 0x1C000000u) == 0x0C000000u)
        return ARMITO_DECODER_UNSUPPORTED_INSTRUCTION;

    return ARMITO_DECODER_UNSUPPORTED_INSTRUCTION;
}

ArmitoDecoderStatus AARCH64Decode(uint32_t encoding, AARCH64Instruction *instruction)
{
    AARCH64Opcode opcode;
    ArmitoDecoderStatus status;

    if (instruction == NULL)
        return ARMITO_DECODER_INVALID_ARGUMENT;

    status = AARCH64Classify(encoding, &opcode);

    if (status != ARMITO_DECODER_SUCCESS)
        return status;

    instruction->opcode = opcode;
    instruction->encoding = encoding;
    instruction->width_bits = 64;
    instruction->set_flags = 0;
    instruction->rd_index = 0;
    instruction->rn_index = 0;
    instruction->rm_index = 0;
    instruction->rd_is_sp = 0;
    instruction->rn_is_sp = 0;
    instruction->rm_is_sp = 0;
    instruction->rd_is_zero = 0;
    instruction->rn_is_zero = 0;
    instruction->rm_is_zero = 0;
    instruction->shift_type = 0;
    instruction->shift_amount = 0;
    instruction->condition = 0;
    instruction->is_64bit_immediate = 0;
    instruction->immediate = 0;
    instruction->branch_target = 0;

    return ARMITO_DECODER_SUCCESS;
}

ArmitoDecoderStatus AARCH64ToIR(
    const AARCH64Instruction *instruction,
    uint64_t address,
    uint32_t *operations_emitted
)
{
    ArmitoIROperationID operation_id = 0;
    ArmitoIRStatus ir_status;

    if (instruction == NULL || operations_emitted == NULL)
        return ARMITO_DECODER_INVALID_ARGUMENT;

    *operations_emitted = 0;

    switch (instruction->opcode) {
    case AARCH64_OPCODE_NOP:
    case AARCH64_OPCODE_RET:
        ir_status = ArmitoIROperationEmit(
            ARMITO_IR_OP_NOP,
            ARMITO_IR_TYPE_INVALID,
            ARMITO_IR_VALUE_INVALID,
            NULL,
            0,
            &operation_id
        );
        break;
    default:
        ir_status = ArmitoIROperationEmit(
            ARMITO_IR_OP_UNSUPPORTED,
            ARMITO_IR_TYPE_INVALID,
            ARMITO_IR_VALUE_INVALID,
            NULL,
            0,
            &operation_id
        );
        break;
    }

    if (ir_status != ARMITO_IR_SUCCESS)
        return ARMITO_DECODER_IR_ERROR;

    ArmitoIROperationSetSource(
        operation_id, (uint32_t)(address & 0xFFFFFFFFu),
        instruction->encoding, ARMITO_DECODER_INSTRUCTION_SIZE
    );

    *operations_emitted = 1;

    return ARMITO_DECODER_SUCCESS;
}

const char *AARCH64OpcodeName(AARCH64Opcode opcode)
{
    if ((uint32_t)opcode > (uint32_t)AARCH64_OPCODE_LOAD_STORE)
        return "invalid";

    return opcode_names[opcode];
}

ArmitoDecodeResult AARCH64ResultFor(ArmitoDecoderStatus status)
{
    switch (status) {
    case ARMITO_DECODER_SUCCESS:
        return ARMITO_DECODE_OK;
    case ARMITO_DECODER_UNSUPPORTED_INSTRUCTION:
        return ARMITO_DECODE_UNSUPPORTED;
    case ARMITO_DECODER_INVALID_INSTRUCTION:
        return ARMITO_DECODE_INVALID;
    default:
        return ARMITO_DECODE_INVALID;
    }
}
