#include "Includes/AARCH64.h"

#include "Includes/IR.h"



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

static const char *AARCH64NameRange0(uint32_t index)
{
    switch (index) {
    case 0: return "invalid";
    case 1: return "nop";
    case 2: return "hint";
    case 3: return "ret";
    case 4: return "br";
    case 5: return "bl";
    case 6: return "b.pl";
    case 7: return "cbz";
    case 8: return "cbnz";
    case 9: return "tbz";
    case 10: return "tbnz";
    case 11: return "mov";
    case 12: return "mov_imm";
    case 13: return "movn";
    case 14: return "movz";
    case 15: return "movk";
    case 16: return "csel";
    case 17: return "csinc";
    case 18: return "csinv";
    case 19: return "csneg";
    case 20: return "madd";
    case 21: return "msub";
    case 22: return "add";
    case 23: return "adds";
    case 24: return "sub";
    case 25: return "subs";
    case 26: return "add_shifted";
    case 27: return "adds_shifted";
    case 28: return "sub_shifted";
    case 29: return "subs_shifted";
    case 30: return "add_extended";
    case 31: return "sub_extended";
    default: return "unknown";
    }
}

static const char *AARCH64NameRange32(uint32_t index)
{
    switch (index) {
    case 32: return "add_reg";
    case 33: return "sub_reg";
    case 34: return "neg";
    case 35: return "negs";
    case 36: return "mul";
    case 37: return "sdiv";
    case 38: return "udiv";
    case 39: return "lsl";
    case 40: return "lsr";
    case 41: return "asr";
    case 42: return "ror";
    case 43: return "ubfm";
    case 44: return "sbfm";
    case 45: return "bfm";
    case 46: return "extr";
    case 47: return "and";
    case 48: return "orr";
    case 49: return "eor";
    case 50: return "bic";
    case 51: return "orn";
    case 52: return "eon";
    case 53: return "mvn";
    case 54: return "tst";
    case 55: return "and_imm";
    case 56: return "orr_imm";
    case 57: return "eor_imm";
    case 58: return "ands";
    case 59: return "bfm_reg";
    case 60: return "and_shifted";
    case 61: return "orr_shifted";
    case 62: return "eor_shifted";
    case 63: return "bic_shifted";
    default: return "unknown";
    }
}

static const char *AARCH64NameRange64(uint32_t index)
{
    switch (index) {
    case 64: return "orn_shifted";
    case 65: return "eon_shifted";
    case 66: return "ands_shifted";
    case 67: return "cmp";
    case 68: return "cmp_shifted";
    case 69: return "cmp_extended";
    case 70: return "adr";
    case 71: return "adrp";
    case 72: return "ldrh";
    case 73: return "strh";
    case 74: return "stp";
    case 75: return "load_literal";
    case 76: return "load_store_exclusive";
    case 77: return "load_store_atomic";
    case 78: return "dc";
    case 79: return "ic";
    case 80: return "mrs";
    case 81: return "msr";
    case 82: return "sys";
    case 83: return "sysl";
    case 84: return "barrier";
    case 85: return "pacia";
    case 86: return "pacib";
    case 87: return "paciza";
    case 88: return "pacizb";
    case 89: return "autia";
    case 90: return "autib";
    case 91: return "autiza";
    case 92: return "autizb";
    case 93: return "pacia1716";
    case 94: return "pacib1716";
    case 95: return "autia1716";
    default: return "unknown";
    }
}

static const char *AARCH64NameRange96(uint32_t index)
{
    switch (index) {
    case 96: return "autib1716";
    case 97: return "paciaz";
    case 98: return "pacibz";
    case 99: return "autiaz";
    case 100: return "autibz";
    case 101: return "pacga";
    case 102: return "pacda";
    case 103: return "pacdb";
    case 104: return "autda";
    case 105: return "autdb";
    case 106: return "pacdza";
    case 107: return "pacdzb";
    case 108: return "autdza";
    case 109: return "autdzb";
    case 110: return "xpaci";
    case 111: return "xpacd";
    case 112: return "blraa";
    case 113: return "blrab";
    case 114: return "blraaz";
    case 115: return "blrabz";
    case 116: return "braa";
    case 117: return "brab";
    case 118: return "braaz";
    case 119: return "brabz";
    case 120: return "retaa";
    case 121: return "retab";
    case 122: return "stg";
    case 123: return "ldg";
    case 124: return "stzg";
    case 125: return "ldzg";
    case 126: return "stzg2";
    case 127: return "ldzg2";
    default: return "unknown";
    }
}

static const char *AARCH64NameRange128(uint32_t index)
{
    switch (index) {
    case 128: return "getg";
    case 129: return "fp_load_store";
    case 130: return "fp_arith";
    case 131: return "fp_compare";
    case 132: return "fp_convert";
    case 133: return "fmov";
    case 134: return "vector_move";
    case 135: return "vector_arith";
    case 136: return "vector_logic";
    case 137: return "vector_shift";
    case 138: return "vector_permute";
    case 139: return "vector_lane";
    case 140: return "vector_load_store";
    case 141: return "vector_structure";
    case 142: return "vector_crypto";
    case 143: return "crc";
    case 144: return "aes";
    case 145: return "sha";
    case 146: return "pac_ret_reg";
    case 147: return "bti";
    case 148: return "movk64";
    case 149: return "subp";
    case 150: return "addp";
    case 151: return "tst_imm";
    case 152: return "tst_shifted";
    case 153: return "mov_imm_logical";
    case 154: return "b_cond_bc";
    case 155: return "ccmp";
    case 156: return "ccmn";
    case 157: return "brk";
    case 158: return "hlt";
    case 159: return "udf";
    default: return "unknown";
    }
}

static const char *AARCH64NameRange160(uint32_t index)
{
    switch (index) {
    case 160: return "svc";
    case 161: return "hvc";
    case 162: return "smc";
    case 163: return "eret";
    case 164: return "drps";
    case 165: return "wfe";
    case 166: return "wfi";
    case 167: return "sev";
    case 168: return "sevl";
    case 169: return "yield";
    case 170: return "isb";
    case 171: return "dsb";
    case 172: return "dmb";
    case 173: return "sb";
    case 174: return "clrex";
    case 175: return "esb";
    case 176: return "pssbb";
    case 177: return "csdb";
    case 178: return "adc";
    case 179: return "adcs";
    case 180: return "sbc";
    case 181: return "sbcs";
    case 182: return "ngc";
    case 183: return "cmn";
    case 184: return "addv";
    case 185: return "bfi";
    case 186: return "bfxil";
    case 187: return "sbfiz";
    case 188: return "sbfx";
    case 189: return "ubfiz";
    case 190: return "ubfx";
    case 191: return "clz";
    default: return "unknown";
    }
}

static const char *AARCH64NameRange192(uint32_t index)
{
    switch (index) {
    case 192: return "rbit";
    case 193: return "rev";
    case 194: return "rev32";
    case 195: return "rev64";
    case 196: return "sxtb";
    case 197: return "sxth";
    case 198: return "sxtw";
    case 199: return "uxtb";
    case 200: return "smull";
    case 201: return "umull";
    case 202: return "smaddl";
    case 203: return "umaddl";
    case 204: return "smulh";
    case 205: return "umulh";
    case 206: return "mneg";
    case 207: return "cset";
    case 208: return "csetm";
    case 209: return "cinc";
    case 210: return "cinv";
    case 211: return "fcsel";
    case 212: return "ldnp";
    case 213: return "stnp";
    case 214: return "ldur";
    case 215: return "stur";
    case 216: return "ldurb";
    case 217: return "sturb";
    case 218: return "ldurh";
    case 219: return "sturh";
    case 220: return "ldrsb";
    case 221: return "ldrsh";
    case 222: return "ldrsw";
    case 223: return "ldxr";
    default: return "unknown";
    }
}

static const char *AARCH64NameRange224(uint32_t index)
{
    switch (index) {
    case 224: return "stxrb";
    case 225: return "stlxr";
    case 226: return "ldar";
    case 227: return "ldarb";
    case 228: return "ldarh";
    case 229: return "stlr";
    case 230: return "stlrb";
    case 231: return "stlrh";
    case 232: return "prfm";
    case 233: return "tlbi";
    case 234: return "at";
    case 235: return "clrex_alias";
    case 236: return "movi";
    case 237: return "dup";
    case 238: return "bsl";
    case 239: return "cmeq";
    case 240: return "cmhi";
    case 241: return "ext";
    case 242: return "tbl";
    case 243: return "uzp1";
    case 244: return "zip1";
    case 245: return "xtn";
    case 246: return "shl";
    case 247: return "sshr";
    case 248: return "ushr";
    case 249: return "ushl";
    case 250: return "sshll";
    case 251: return "ushll";
    case 252: return "ushll2";
    case 253: return "shl_2";
    case 254: return "umaxv";
    case 255: return "uminv";
    default: return "unknown";
    }
}

static const char *AARCH64NameRange256(uint32_t index)
{
    switch (index) {
    case 256: return "ld1";
    case 257: return "ld4";
    case 258: return "st1";
    case 259: return "shrn";
    case 260: return "shrn2";
    case 261: return "adr_small";
    case 262: return "b_al";
    case 263: return "ldr_pac";
    case 264: return "b";
    case 265: return "blr";
    case 266: return "cneg";
    case 267: return "shll";
    case 268: return "shll2";
    case 269: return "bics";
    case 270: return "invalid-encoding";
    default: return "unknown";
    }
}

/* Nome do opcode em faixas de 32: um switch unico com 271 casos faz o GCC
 * 13.3 ligar o indice ao .LC errado (bug reproduzido neste objeto). */
const char *AARCH64OpcodeName(AARCH64Opcode opcode)
{
    uint32_t index = (uint32_t)opcode;

    if (index < 32u)
        return AARCH64NameRange0(index);
    if (index < 64u)
        return AARCH64NameRange32(index);
    if (index < 96u)
        return AARCH64NameRange64(index);
    if (index < 128u)
        return AARCH64NameRange96(index);
    if (index < 160u)
        return AARCH64NameRange128(index);
    if (index < 192u)
        return AARCH64NameRange160(index);
    if (index < 224u)
        return AARCH64NameRange192(index);
    if (index < 256u)
        return AARCH64NameRange224(index);
    if (index < 271u)
        return AARCH64NameRange256(index);
    return "unknown";
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
