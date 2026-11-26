#include "Includes/AARCH64.h"

#include "Includes/IR.h"

static int64_t AARCH64SignExtend(uint32_t value, uint8_t width)
{
    uint64_t sign_bit = 1ULL << (width - 1u);
    uint64_t extended = value & ((1ULL << width) - 1u);

    if ((extended & sign_bit) != 0)
        return (int64_t)extended - (int64_t)(1ULL << width);

    return (int64_t)extended;
}

static void AARCH64SetRegister(
    uint8_t index,
    uint8_t is_sp,
    uint8_t *register_index,
    uint8_t *register_is_sp,
    uint8_t *register_is_zero
)
{
    *register_index = index;
    *register_is_sp = (uint8_t)(index == 31u && is_sp);
    *register_is_zero = (uint8_t)(index == 31u && !is_sp);
}

ArmitoDecoderStatus AARCH64Classify(uint32_t encoding, AARCH64Opcode *opcode)
{
    if (opcode == NULL)
        return ARMITO_DECODER_INVALID_ARGUMENT;

    *opcode = AARCH64_OPCODE_UNKNOWN;

    if ((encoding & 0xFFFFF01Fu) == 0xD503201Fu) {
        *opcode = encoding == 0xD503201Fu
            ? AARCH64_OPCODE_NOP
            : AARCH64_OPCODE_HINT;
        return ARMITO_DECODER_SUCCESS;
    }

    if ((encoding & 0xFFFFFC1Fu) == 0xD61F0000u) {
        *opcode = AARCH64_OPCODE_BR;
        return ARMITO_DECODER_SUCCESS;
    }

    if ((encoding & 0xFFFFFC1Fu) == 0xD63F0000u) {
        *opcode = AARCH64_OPCODE_BLR;
        return ARMITO_DECODER_SUCCESS;
    }

    if ((encoding & 0xFFFFFC1Fu) == 0xD65F0000u) {
        *opcode = AARCH64_OPCODE_RET;
        return ARMITO_DECODER_SUCCESS;
    }

    if ((encoding & 0x7C000000u) == 0x14000000u) {
        *opcode = ((encoding >> 31) & 1u)
            ? AARCH64_OPCODE_BL
            : AARCH64_OPCODE_B;
        return ARMITO_DECODER_SUCCESS;
    }

    if ((encoding & 0xFF000010u) == 0x54000000u) {
        if ((encoding & 0xFu) == 0xFu)
            return ARMITO_DECODER_INVALID_INSTRUCTION;

        *opcode = AARCH64_OPCODE_B_COND;
        return ARMITO_DECODER_SUCCESS;
    }

    if ((encoding & 0x7E000000u) == 0x34000000u) {
        *opcode = ((encoding >> 24) & 1u)
            ? AARCH64_OPCODE_CBNZ
            : AARCH64_OPCODE_CBZ;
        return ARMITO_DECODER_SUCCESS;
    }

    if ((encoding & 0x7E000000u) == 0x36000000u) {
        *opcode = ((encoding >> 24) & 1u)
            ? AARCH64_OPCODE_TBNZ
            : AARCH64_OPCODE_TBZ;
        return ARMITO_DECODER_SUCCESS;
    }

    if ((encoding & 0x7FE0FFE0u) == 0x2A0003E0u) {
        *opcode = AARCH64_OPCODE_MOV_REG;
        return ARMITO_DECODER_SUCCESS;
    }

    switch (encoding & 0x7F800000u) {
    case 0x12800000u:
        *opcode = AARCH64_OPCODE_MOVN;
        return ARMITO_DECODER_SUCCESS;
    case 0x52800000u:
        *opcode = AARCH64_OPCODE_MOVZ;
        return ARMITO_DECODER_SUCCESS;
    case 0x72800000u:
        *opcode = AARCH64_OPCODE_MOVK;
        return ARMITO_DECODER_SUCCESS;
    default:
        break;
    }

    if ((encoding & 0x1F000000u) == 0x11000000u) {
        uint8_t subtract = (uint8_t)((encoding >> 30) & 1u);
        uint8_t set_flags = (uint8_t)((encoding >> 29) & 1u);

        if (subtract)
            *opcode = set_flags ? AARCH64_OPCODE_SUBS_IMM : AARCH64_OPCODE_SUB_IMM;
        else
            *opcode = set_flags ? AARCH64_OPCODE_ADDS_IMM : AARCH64_OPCODE_ADD_IMM;
        return ARMITO_DECODER_SUCCESS;
    }

    if ((encoding & 0x1F200000u) == 0x0B000000u) {
        uint8_t subtract = (uint8_t)((encoding >> 30) & 1u);
        uint8_t set_flags = (uint8_t)((encoding >> 29) & 1u);

        if (subtract)
            *opcode = set_flags
                ? AARCH64_OPCODE_SUBS_SHIFTED
                : AARCH64_OPCODE_SUB_SHIFTED;
        else
            *opcode = set_flags
                ? AARCH64_OPCODE_ADDS_SHIFTED
                : AARCH64_OPCODE_ADD_SHIFTED;
        return ARMITO_DECODER_SUCCESS;
    }

    return ARMITO_DECODER_UNSUPPORTED_INSTRUCTION;
}

ArmitoDecoderStatus AARCH64ClassifyGeneric(uint32_t encoding, AARCH64Opcode *opcode)
{
    return AARCH64Classify(encoding, opcode);
}

ArmitoDecoderStatus AARCH64ClassifyMnemonic(uint32_t encoding, const char **mnemonic)
{
    static const char *const conditions[16] = {
        "b.eq", "b.ne", "b.hs", "b.lo", "b.mi", "b.pl", "b.vs", "b.vc",
        "b.hi", "b.ls", "b.ge", "b.lt", "b.gt", "b.le", "b.al", "b.nv"
    };
    AARCH64Opcode opcode;
    ArmitoDecoderStatus status;

    if (mnemonic == NULL)
        return ARMITO_DECODER_INVALID_ARGUMENT;

    *mnemonic = NULL;
    status = AARCH64Classify(encoding, &opcode);
    if (status != ARMITO_DECODER_SUCCESS)
        return status;

    if (opcode == AARCH64_OPCODE_B_COND) {
        *mnemonic = conditions[encoding & 0xFu];
        return ARMITO_DECODER_SUCCESS;
    }

    *mnemonic = AARCH64OpcodeName(opcode);
    return ARMITO_DECODER_SUCCESS;
}

ArmitoDecoderStatus AARCH64Decode(uint32_t encoding, AARCH64Instruction *instruction)
{
    AARCH64Opcode opcode;
    ArmitoDecoderStatus status;
    uint8_t rd;
    uint8_t rn;
    uint8_t rm;

    if (instruction == NULL)
        return ARMITO_DECODER_INVALID_ARGUMENT;

    status = AARCH64Classify(encoding, &opcode);
    if (status != ARMITO_DECODER_SUCCESS)
        return status;

    instruction->opcode = opcode;
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
    instruction->encoding = encoding;

    if (opcode == AARCH64_OPCODE_NOP || opcode == AARCH64_OPCODE_HINT)
        return ARMITO_DECODER_SUCCESS;

    if (opcode == AARCH64_OPCODE_B || opcode == AARCH64_OPCODE_BL) {
        instruction->immediate = AARCH64SignExtend(encoding & 0x03FFFFFFu, 26) * 4;
        return ARMITO_DECODER_SUCCESS;
    }

    if (opcode == AARCH64_OPCODE_B_COND) {
        instruction->condition = (uint8_t)(encoding & 0xFu);
        instruction->immediate =
            AARCH64SignExtend((encoding >> 5) & 0x7FFFFu, 19) * 4;
        return ARMITO_DECODER_SUCCESS;
    }

    if (opcode == AARCH64_OPCODE_CBZ || opcode == AARCH64_OPCODE_CBNZ) {
        instruction->width_bits = (uint8_t)(((encoding >> 31) & 1u) ? 64 : 32);
        rd = (uint8_t)(encoding & 0x1Fu);
        AARCH64SetRegister(
            rd, 0, &instruction->rd_index,
            &instruction->rd_is_sp, &instruction->rd_is_zero
        );
        instruction->immediate =
            AARCH64SignExtend((encoding >> 5) & 0x7FFFFu, 19) * 4;
        return ARMITO_DECODER_SUCCESS;
    }

    if (opcode == AARCH64_OPCODE_TBZ || opcode == AARCH64_OPCODE_TBNZ) {
        instruction->width_bits = (uint8_t)(((encoding >> 31) & 1u) ? 64 : 32);
        rd = (uint8_t)(encoding & 0x1Fu);
        AARCH64SetRegister(
            rd, 0, &instruction->rd_index,
            &instruction->rd_is_sp, &instruction->rd_is_zero
        );
        instruction->shift_amount = (uint8_t)(
            ((encoding >> 19) & 0x1Fu) | (((encoding >> 31) & 1u) << 5)
        );
        instruction->immediate =
            AARCH64SignExtend((encoding >> 5) & 0x3FFFu, 14) * 4;
        return ARMITO_DECODER_SUCCESS;
    }

    if (opcode == AARCH64_OPCODE_BR || opcode == AARCH64_OPCODE_BLR ||
        opcode == AARCH64_OPCODE_RET) {
        rn = (uint8_t)((encoding >> 5) & 0x1Fu);
        if (opcode == AARCH64_OPCODE_RET && rn == 31u)
            rn = 30u;
        if (rn == 31u)
            return ARMITO_DECODER_INVALID_INSTRUCTION;
        AARCH64SetRegister(
            rn, 0, &instruction->rn_index,
            &instruction->rn_is_sp, &instruction->rn_is_zero
        );
        return ARMITO_DECODER_SUCCESS;
    }

    if (opcode == AARCH64_OPCODE_MOV_REG) {
        instruction->width_bits = (uint8_t)(((encoding >> 31) & 1u) ? 64 : 32);
        rd = (uint8_t)(encoding & 0x1Fu);
        rm = (uint8_t)((encoding >> 16) & 0x1Fu);
        AARCH64SetRegister(
            rd, 0, &instruction->rd_index,
            &instruction->rd_is_sp, &instruction->rd_is_zero
        );
        AARCH64SetRegister(
            rm, 0, &instruction->rm_index,
            &instruction->rm_is_sp, &instruction->rm_is_zero
        );
        instruction->rn_index = 31u;
        instruction->rn_is_zero = 1;
        return ARMITO_DECODER_SUCCESS;
    }

    if (opcode == AARCH64_OPCODE_MOVZ || opcode == AARCH64_OPCODE_MOVN ||
        opcode == AARCH64_OPCODE_MOVK) {
        uint8_t shift = (uint8_t)(((encoding >> 21) & 3u) * 16u);

        instruction->width_bits = (uint8_t)(((encoding >> 31) & 1u) ? 64 : 32);
        if (instruction->width_bits == 32 && shift >= 32)
            return ARMITO_DECODER_INVALID_INSTRUCTION;
        rd = (uint8_t)(encoding & 0x1Fu);
        AARCH64SetRegister(
            rd, 0, &instruction->rd_index,
            &instruction->rd_is_sp, &instruction->rd_is_zero
        );
        instruction->shift_amount = shift;
        instruction->immediate = (int64_t)(
            (uint64_t)((encoding >> 5) & 0xFFFFu) << shift
        );
        instruction->is_64bit_immediate = (uint8_t)(instruction->width_bits == 64);
        return ARMITO_DECODER_SUCCESS;
    }

    if (opcode == AARCH64_OPCODE_ADD_IMM || opcode == AARCH64_OPCODE_ADDS_IMM ||
        opcode == AARCH64_OPCODE_SUB_IMM || opcode == AARCH64_OPCODE_SUBS_IMM) {
        uint8_t set_flags = (uint8_t)((encoding >> 29) & 1u);
        uint8_t use_sp = (uint8_t)!set_flags;
        uint8_t shift = (uint8_t)(((encoding >> 22) & 1u) * 12u);

        instruction->width_bits = (uint8_t)(((encoding >> 31) & 1u) ? 64 : 32);
        instruction->set_flags = set_flags;
        rd = (uint8_t)(encoding & 0x1Fu);
        rn = (uint8_t)((encoding >> 5) & 0x1Fu);
        AARCH64SetRegister(
            rd, use_sp, &instruction->rd_index,
            &instruction->rd_is_sp, &instruction->rd_is_zero
        );
        AARCH64SetRegister(
            rn, 1, &instruction->rn_index,
            &instruction->rn_is_sp, &instruction->rn_is_zero
        );
        instruction->shift_amount = shift;
        instruction->immediate = (int64_t)(((encoding >> 10) & 0xFFFu) << shift);
        instruction->is_64bit_immediate = (uint8_t)(instruction->width_bits == 64);
        return ARMITO_DECODER_SUCCESS;
    }

    if (opcode == AARCH64_OPCODE_ADD_SHIFTED || opcode == AARCH64_OPCODE_ADDS_SHIFTED ||
        opcode == AARCH64_OPCODE_SUB_SHIFTED || opcode == AARCH64_OPCODE_SUBS_SHIFTED) {
        instruction->width_bits = (uint8_t)(((encoding >> 31) & 1u) ? 64 : 32);
        instruction->set_flags = (uint8_t)((encoding >> 29) & 1u);
        rd = (uint8_t)(encoding & 0x1Fu);
        rn = (uint8_t)((encoding >> 5) & 0x1Fu);
        rm = (uint8_t)((encoding >> 16) & 0x1Fu);
        AARCH64SetRegister(
            rd, 0, &instruction->rd_index,
            &instruction->rd_is_sp, &instruction->rd_is_zero
        );
        AARCH64SetRegister(
            rn, 0, &instruction->rn_index,
            &instruction->rn_is_sp, &instruction->rn_is_zero
        );
        AARCH64SetRegister(
            rm, 0, &instruction->rm_index,
            &instruction->rm_is_sp, &instruction->rm_is_zero
        );
        instruction->shift_type = (uint8_t)((encoding >> 22) & 3u);
        instruction->shift_amount = (uint8_t)((encoding >> 10) & 0x3Fu);
        if (instruction->shift_type == 3u ||
            (instruction->width_bits == 32 && instruction->shift_amount >= 32u))
            return ARMITO_DECODER_INVALID_INSTRUCTION;
        return ARMITO_DECODER_SUCCESS;
    }

    return ARMITO_DECODER_INVALID_INSTRUCTION;
}

ArmitoDecoderStatus AARCH64ToIR(
    const AARCH64Instruction *instruction,
    uint64_t address,
    uint32_t *operations_emitted
)
{
    ArmitoIROperationID operation_id = 0;
    ArmitoIRValueID input = ARMITO_IR_VALUE_INVALID;
    ArmitoIRValueID target = ARMITO_IR_VALUE_INVALID;
    ArmitoIRStatus ir_status;
    ArmitoIROperationKind kind = ARMITO_IR_OP_UNSUPPORTED;
    uint8_t input_count = 0;

    if (instruction == NULL || operations_emitted == NULL)
        return ARMITO_DECODER_INVALID_ARGUMENT;

    *operations_emitted = 0;

    switch (instruction->opcode) {
    case AARCH64_OPCODE_NOP:
        kind = ARMITO_IR_OP_NOP;
        break;
    case AARCH64_OPCODE_RET:
    case AARCH64_OPCODE_BR:
        if (ArmitoIRCPUStateLookup(
                ARMITO_IR_REG_GPR,
                instruction->rn_index,
                &input
            ) != ARMITO_IR_SUCCESS)
            return ARMITO_DECODER_IR_ERROR;
        input_count = 1;
        kind = instruction->opcode == AARCH64_OPCODE_RET
            ? ARMITO_IR_OP_RETURN
            : ARMITO_IR_OP_BRANCH_INDIRECT;
        break;
    case AARCH64_OPCODE_B:
        ir_status = ArmitoIRValuesDeclare(
            ARMITO_IR_VALUE_IMMEDIATE,
            ARMITO_IR_TYPE_INVALID,
            "branch-target",
            &target
        );
        if (ir_status != ARMITO_IR_SUCCESS)
            return ARMITO_DECODER_IR_ERROR;
        ir_status = ArmitoIRValuesSetImmediate(
            target,
            address + (uint64_t)instruction->immediate
        );
        if (ir_status != ARMITO_IR_SUCCESS)
            return ARMITO_DECODER_IR_ERROR;
        input = target;
        input_count = 1;
        kind = ARMITO_IR_OP_BRANCH;
        break;
    default:
        break;
    }

    ir_status = ArmitoIROperationEmit(
        kind,
        ARMITO_IR_TYPE_INVALID,
        ARMITO_IR_VALUE_INVALID,
        input_count ? &input : NULL,
        input_count,
        &operation_id
    );
    if (ir_status != ARMITO_IR_SUCCESS)
        return ARMITO_DECODER_IR_ERROR;

    ir_status = ArmitoIROperationSetSource(
        operation_id,
        (uint32_t)(address & 0xFFFFFFFFu),
        instruction->encoding,
        ARMITO_DECODER_INSTRUCTION_SIZE
    );
    if (ir_status != ARMITO_IR_SUCCESS)
        return ARMITO_DECODER_IR_ERROR;

    *operations_emitted = 1;
    return kind == ARMITO_IR_OP_UNSUPPORTED
        ? ARMITO_DECODER_UNSUPPORTED_INSTRUCTION
        : ARMITO_DECODER_SUCCESS;
}

const char *AARCH64OpcodeName(AARCH64Opcode opcode)
{
    switch (opcode) {
    case AARCH64_OPCODE_UNKNOWN: return "unknown";
    case AARCH64_OPCODE_INVALID:
    case AARCH64_OPCODE_INVALID_ENCODING: return "invalid";
    case AARCH64_OPCODE_NOP: return "nop";
    case AARCH64_OPCODE_HINT: return "hint";
    case AARCH64_OPCODE_B: return "b";
    case AARCH64_OPCODE_BL: return "bl";
    case AARCH64_OPCODE_BR: return "br";
    case AARCH64_OPCODE_BLR: return "blr";
    case AARCH64_OPCODE_RET: return "ret";
    case AARCH64_OPCODE_B_COND: return "b.cond";
    case AARCH64_OPCODE_CBZ: return "cbz";
    case AARCH64_OPCODE_CBNZ: return "cbnz";
    case AARCH64_OPCODE_TBZ: return "tbz";
    case AARCH64_OPCODE_TBNZ: return "tbnz";
    case AARCH64_OPCODE_MOV_REG: return "mov";
    case AARCH64_OPCODE_MOV_IMM: return "mov";
    case AARCH64_OPCODE_MOVN: return "movn";
    case AARCH64_OPCODE_MOVZ: return "movz";
    case AARCH64_OPCODE_MOVK: return "movk";
    case AARCH64_OPCODE_ADD_IMM: return "add";
    case AARCH64_OPCODE_ADDS_IMM: return "adds";
    case AARCH64_OPCODE_SUB_IMM: return "sub";
    case AARCH64_OPCODE_SUBS_IMM: return "subs";
    case AARCH64_OPCODE_ADD_SHIFTED: return "add.shifted";
    case AARCH64_OPCODE_ADDS_SHIFTED: return "adds.shifted";
    case AARCH64_OPCODE_SUB_SHIFTED: return "sub.shifted";
    case AARCH64_OPCODE_SUBS_SHIFTED: return "subs.shifted";
    default: return "unknown";
    }
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
