#include "Includes/AARCH64.h"

#include "Catalog/AARCH64Catalog.h"
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

static ArmitoDecoderStatus AARCH64ClassifyPointerAuth(
    uint32_t encoding,
    AARCH64Opcode *opcode
)
{
    uint32_t masked;

    switch (encoding) {
    case 0xD503233Fu: *opcode = AARCH64_OPCODE_PACIASP; return ARMITO_DECODER_SUCCESS;
    case 0xD503237Fu: *opcode = AARCH64_OPCODE_PACIBSP; return ARMITO_DECODER_SUCCESS;
    case 0xD50323BFu: *opcode = AARCH64_OPCODE_AUTIASP; return ARMITO_DECODER_SUCCESS;
    case 0xD50323FFu: *opcode = AARCH64_OPCODE_AUTIBSP; return ARMITO_DECODER_SUCCESS;
    case 0xD65F0BFFu: *opcode = AARCH64_OPCODE_RETAA; return ARMITO_DECODER_SUCCESS;
    case 0xD65F0FFFu: *opcode = AARCH64_OPCODE_RETAB; return ARMITO_DECODER_SUCCESS;
    default: break;
    }

    masked = encoding & 0xFFFFFC00u;
    switch (masked) {
    case 0xDAC10000u: *opcode = AARCH64_OPCODE_PACIA; return ARMITO_DECODER_SUCCESS;
    case 0xDAC10400u: *opcode = AARCH64_OPCODE_PACIB; return ARMITO_DECODER_SUCCESS;
    case 0xDAC10800u: *opcode = AARCH64_OPCODE_PACDA; return ARMITO_DECODER_SUCCESS;
    case 0xDAC10C00u: *opcode = AARCH64_OPCODE_PACDB; return ARMITO_DECODER_SUCCESS;
    case 0xDAC11000u: *opcode = AARCH64_OPCODE_AUTIA; return ARMITO_DECODER_SUCCESS;
    case 0xDAC11400u: *opcode = AARCH64_OPCODE_AUTIB; return ARMITO_DECODER_SUCCESS;
    case 0xDAC11800u: *opcode = AARCH64_OPCODE_AUTDA; return ARMITO_DECODER_SUCCESS;
    case 0xDAC11C00u: *opcode = AARCH64_OPCODE_AUTDB; return ARMITO_DECODER_SUCCESS;
    case 0xD71F0800u: *opcode = AARCH64_OPCODE_BRAA; return ARMITO_DECODER_SUCCESS;
    case 0xD71F0C00u: *opcode = AARCH64_OPCODE_BRAB; return ARMITO_DECODER_SUCCESS;
    case 0xD73F0800u: *opcode = AARCH64_OPCODE_BLRAA; return ARMITO_DECODER_SUCCESS;
    case 0xD73F0C00u: *opcode = AARCH64_OPCODE_BLRAB; return ARMITO_DECODER_SUCCESS;
    case 0xF8200400u: *opcode = AARCH64_OPCODE_LDRAA; return ARMITO_DECODER_SUCCESS;
    case 0xF8A00400u: *opcode = AARCH64_OPCODE_LDRAB; return ARMITO_DECODER_SUCCESS;
    default: break;
    }

    masked = encoding & 0xFFFFFFE0u;
    switch (masked) {
    case 0xDAC123E0u: *opcode = AARCH64_OPCODE_PACIZA; return ARMITO_DECODER_SUCCESS;
    case 0xDAC127E0u: *opcode = AARCH64_OPCODE_PACIZB; return ARMITO_DECODER_SUCCESS;
    case 0xDAC12BE0u: *opcode = AARCH64_OPCODE_PACDZA; return ARMITO_DECODER_SUCCESS;
    case 0xDAC12FE0u: *opcode = AARCH64_OPCODE_PACDZB; return ARMITO_DECODER_SUCCESS;
    case 0xDAC133E0u: *opcode = AARCH64_OPCODE_AUTIZA; return ARMITO_DECODER_SUCCESS;
    case 0xDAC137E0u: *opcode = AARCH64_OPCODE_AUTIZB; return ARMITO_DECODER_SUCCESS;
    case 0xDAC13BE0u: *opcode = AARCH64_OPCODE_AUTDZA; return ARMITO_DECODER_SUCCESS;
    case 0xDAC13FE0u: *opcode = AARCH64_OPCODE_AUTDZB; return ARMITO_DECODER_SUCCESS;
    case 0xDAC143E0u: *opcode = AARCH64_OPCODE_XPACI; return ARMITO_DECODER_SUCCESS;
    case 0xDAC147E0u: *opcode = AARCH64_OPCODE_XPACD; return ARMITO_DECODER_SUCCESS;
    default: break;
    }

    masked = encoding & 0xFFFFFC1Fu;
    switch (masked) {
    case 0xD61F081Fu: *opcode = AARCH64_OPCODE_BRAAZ; return ARMITO_DECODER_SUCCESS;
    case 0xD61F0C1Fu: *opcode = AARCH64_OPCODE_BRABZ; return ARMITO_DECODER_SUCCESS;
    case 0xD63F081Fu: *opcode = AARCH64_OPCODE_BLRAAZ; return ARMITO_DECODER_SUCCESS;
    case 0xD63F0C1Fu: *opcode = AARCH64_OPCODE_BLRABZ; return ARMITO_DECODER_SUCCESS;
    default: break;
    }

    if ((encoding & 0xFFA00C00u) == 0xF8200400u) {
        *opcode = AARCH64_OPCODE_LDRAA;
        return ARMITO_DECODER_SUCCESS;
    }
    if ((encoding & 0xFFA00C00u) == 0xF8A00400u) {
        *opcode = AARCH64_OPCODE_LDRAB;
        return ARMITO_DECODER_SUCCESS;
    }

    return ARMITO_DECODER_UNSUPPORTED_INSTRUCTION;
}

ArmitoDecoderStatus AARCH64Classify(uint32_t encoding, AARCH64Opcode *opcode)
{
    ArmitoDecoderStatus pointer_auth_status;

    if (opcode == NULL)
        return ARMITO_DECODER_INVALID_ARGUMENT;

    *opcode = AARCH64_OPCODE_UNKNOWN;

    pointer_auth_status = AARCH64ClassifyPointerAuth(encoding, opcode);
    if (pointer_auth_status == ARMITO_DECODER_SUCCESS)
        return pointer_auth_status;

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

    {
        AARCH64CatalogMatch catalog_match;
        AARCH64CatalogResult catalog_result =
            AARCH64CatalogLookup(encoding, &catalog_match);

        if (catalog_result == AARCH64_CATALOG_VALID_ENCODING ||
            catalog_result == AARCH64_CATALOG_PATTERN_MATCHED_UNVALIDATED ||
            catalog_result == AARCH64_CATALOG_EXTENSION_UNAVAILABLE) {
            *opcode = AARCH64_OPCODE_CATALOG_ONLY;
            return ARMITO_DECODER_UNSUPPORTED_INSTRUCTION;
        }

        if (catalog_result == AARCH64_CATALOG_RESERVED_PATTERN ||
            catalog_result == AARCH64_CATALOG_INVALID_ENCODING ||
            catalog_result == AARCH64_CATALOG_AMBIGUOUS) {
            *opcode = AARCH64_OPCODE_INVALID;
            return ARMITO_DECODER_INVALID_INSTRUCTION;
        }
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
    instruction->auth_key = 0;
    instruction->auth_modifier_is_sp = 0;
    instruction->auth_modifier_is_zero = 0;
    instruction->auth_address_diversity = 0;
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

    if (opcode == AARCH64_OPCODE_PACIA || opcode == AARCH64_OPCODE_PACIB ||
        opcode == AARCH64_OPCODE_PACDA || opcode == AARCH64_OPCODE_PACDB ||
        opcode == AARCH64_OPCODE_AUTIA || opcode == AARCH64_OPCODE_AUTIB ||
        opcode == AARCH64_OPCODE_AUTDA || opcode == AARCH64_OPCODE_AUTDB) {
        rd = (uint8_t)(encoding & 0x1Fu);
        rn = (uint8_t)((encoding >> 5) & 0x1Fu);
        AARCH64SetRegister(
            rd, 0, &instruction->rd_index,
            &instruction->rd_is_sp, &instruction->rd_is_zero
        );
        AARCH64SetRegister(
            rn, 1, &instruction->rn_index,
            &instruction->rn_is_sp, &instruction->rn_is_zero
        );
        instruction->auth_key = (uint8_t)(
            opcode == AARCH64_OPCODE_PACIA || opcode == AARCH64_OPCODE_AUTIA
                ? AARCH64_AUTH_KEY_IA
                : opcode == AARCH64_OPCODE_PACIB || opcode == AARCH64_OPCODE_AUTIB
                    ? AARCH64_AUTH_KEY_IB
                    : opcode == AARCH64_OPCODE_PACDA || opcode == AARCH64_OPCODE_AUTDA
                        ? AARCH64_AUTH_KEY_DA
                        : AARCH64_AUTH_KEY_DB
        );
        instruction->auth_modifier_is_zero = instruction->rn_is_zero;
        return ARMITO_DECODER_SUCCESS;
    }

    if (opcode == AARCH64_OPCODE_PACIZA || opcode == AARCH64_OPCODE_PACIZB ||
        opcode == AARCH64_OPCODE_PACDZA || opcode == AARCH64_OPCODE_PACDZB ||
        opcode == AARCH64_OPCODE_AUTIZA || opcode == AARCH64_OPCODE_AUTIZB ||
        opcode == AARCH64_OPCODE_AUTDZA || opcode == AARCH64_OPCODE_AUTDZB ||
        opcode == AARCH64_OPCODE_XPACI || opcode == AARCH64_OPCODE_XPACD) {
        rd = (uint8_t)(encoding & 0x1Fu);
        AARCH64SetRegister(
            rd, 0, &instruction->rd_index,
            &instruction->rd_is_sp, &instruction->rd_is_zero
        );
        instruction->rn_index = 31u;
        instruction->rn_is_zero = 1;
        instruction->auth_modifier_is_zero = 1;
        if (opcode == AARCH64_OPCODE_PACIZB ||
            opcode == AARCH64_OPCODE_AUTIZB) {
            instruction->auth_key = AARCH64_AUTH_KEY_IB;
        } else if (opcode == AARCH64_OPCODE_PACDZA ||
                   opcode == AARCH64_OPCODE_AUTDZA ||
                   opcode == AARCH64_OPCODE_XPACD) {
            instruction->auth_key = AARCH64_AUTH_KEY_DA;
        } else if (opcode == AARCH64_OPCODE_PACDZB ||
                   opcode == AARCH64_OPCODE_AUTDZB) {
            instruction->auth_key = AARCH64_AUTH_KEY_DB;
        } else {
            instruction->auth_key = AARCH64_AUTH_KEY_IA;
        }
        return ARMITO_DECODER_SUCCESS;
    }

    if (opcode == AARCH64_OPCODE_PACIASP || opcode == AARCH64_OPCODE_PACIBSP ||
        opcode == AARCH64_OPCODE_AUTIASP || opcode == AARCH64_OPCODE_AUTIBSP ||
        opcode == AARCH64_OPCODE_RETAA || opcode == AARCH64_OPCODE_RETAB) {
        instruction->rd_index = 30u;
        instruction->rn_index = 31u;
        instruction->rn_is_sp = 1;
        instruction->auth_modifier_is_sp = 1;
        instruction->auth_key =
            (opcode == AARCH64_OPCODE_PACIBSP ||
             opcode == AARCH64_OPCODE_AUTIBSP ||
             opcode == AARCH64_OPCODE_RETAB)
                ? AARCH64_AUTH_KEY_IB
                : AARCH64_AUTH_KEY_IA;
        return ARMITO_DECODER_SUCCESS;
    }

    if (opcode == AARCH64_OPCODE_BRAA || opcode == AARCH64_OPCODE_BRAB ||
        opcode == AARCH64_OPCODE_BLRAA || opcode == AARCH64_OPCODE_BLRAB ||
        opcode == AARCH64_OPCODE_BRAAZ || opcode == AARCH64_OPCODE_BRABZ ||
        opcode == AARCH64_OPCODE_BLRAAZ || opcode == AARCH64_OPCODE_BLRABZ) {
        rn = (uint8_t)((encoding >> 5) & 0x1Fu);
        rm = (uint8_t)(encoding & 0x1Fu);
        AARCH64SetRegister(
            rn, 0, &instruction->rn_index,
            &instruction->rn_is_sp, &instruction->rn_is_zero
        );
        AARCH64SetRegister(
            rm, 0, &instruction->rm_index,
            &instruction->rm_is_sp, &instruction->rm_is_zero
        );
        instruction->auth_key =
            (opcode == AARCH64_OPCODE_BRAB ||
             opcode == AARCH64_OPCODE_BLRAB ||
             opcode == AARCH64_OPCODE_BRABZ ||
             opcode == AARCH64_OPCODE_BLRABZ)
                ? AARCH64_AUTH_KEY_IB
                : AARCH64_AUTH_KEY_IA;
        instruction->auth_modifier_is_zero = instruction->rm_is_zero;
        return ARMITO_DECODER_SUCCESS;
    }

    if (opcode == AARCH64_OPCODE_LDRAA || opcode == AARCH64_OPCODE_LDRAB) {
        rd = (uint8_t)(encoding & 0x1Fu);
        rn = (uint8_t)((encoding >> 5) & 0x1Fu);
        AARCH64SetRegister(
            rd, 0, &instruction->rd_index,
            &instruction->rd_is_sp, &instruction->rd_is_zero
        );
        AARCH64SetRegister(
            rn, 1, &instruction->rn_index,
            &instruction->rn_is_sp, &instruction->rn_is_zero
        );
        instruction->immediate = AARCH64SignExtend(
            ((encoding >> 12) & 0x1FFu) | (((encoding >> 22) & 1u) << 9),
            10
        ) * 8;
        instruction->auth_key =
            opcode == AARCH64_OPCODE_LDRAA
                ? AARCH64_AUTH_KEY_DA
                : AARCH64_AUTH_KEY_DB;
        instruction->auth_address_diversity = 1;
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

static ArmitoIRStatus AARCH64IRGetRegisterValue(
    uint8_t index,
    uint8_t is_sp,
    uint8_t is_zero,
    ArmitoIRTypeID type_u64,
    ArmitoIRValueID *value
)
{
    ArmitoIRStatus status;

    if (value == NULL)
        return ARMITO_IR_INVALID_ARGUMENT;

    if (is_zero) {
        status = ArmitoIRValuesDeclare(
            ARMITO_IR_VALUE_IMMEDIATE, type_u64, "xzr", value
        );
        if (status != ARMITO_IR_SUCCESS)
            return status;
        return ArmitoIRValuesSetImmediate(*value, 0);
    }

    if (is_sp)
        return ArmitoIRCPUStateLookup(
            ARMITO_IR_REG_STACK_POINTER, 31, value
        );

    return ArmitoIRCPUStateLookup(ARMITO_IR_REG_GPR, index, value);
}

static ArmitoDecoderStatus AARCH64PointerAuthToIR(
    const AARCH64Instruction *instruction,
    uint64_t address,
    uint32_t *operations_emitted
)
{
    ArmitoIROperationKind kind;
    ArmitoIRTypeID type_u64;
    ArmitoIRValueID inputs[2] = {
        ARMITO_IR_VALUE_INVALID,
        ARMITO_IR_VALUE_INVALID
    };
    ArmitoIRValueID result = ARMITO_IR_VALUE_INVALID;
    ArmitoIROperationID operation_id;
    ArmitoIRPointerAuthKey key;
    uint8_t input_count = 0;
    uint8_t flags = 0;
    uint8_t has_result = 0;
    uint8_t effect_register = 0;
    ArmitoIRRegisterClass effect_class = ARMITO_IR_REG_GPR;
    uint8_t effect_index = 0;
    ArmitoIRStatus ir_status;

    if (ArmitoIRTypesLookup(
            ARMITO_IR_CLASS_INT, 64, 1, 0, &type_u64
        ) != ARMITO_IR_SUCCESS)
        return ARMITO_DECODER_IR_ERROR;

    key = (ArmitoIRPointerAuthKey)instruction->auth_key;

    if (instruction->opcode == AARCH64_OPCODE_PACIA ||
        instruction->opcode == AARCH64_OPCODE_PACIB ||
        instruction->opcode == AARCH64_OPCODE_PACDA ||
        instruction->opcode == AARCH64_OPCODE_PACDB ||
        instruction->opcode == AARCH64_OPCODE_PACIZA ||
        instruction->opcode == AARCH64_OPCODE_PACIZB ||
        instruction->opcode == AARCH64_OPCODE_PACDZA ||
        instruction->opcode == AARCH64_OPCODE_PACDZB ||
        instruction->opcode == AARCH64_OPCODE_PACIASP ||
        instruction->opcode == AARCH64_OPCODE_PACIBSP) {
        kind = ARMITO_IR_OP_PAC_SIGN;
        has_result = 1;
    } else if (instruction->opcode == AARCH64_OPCODE_AUTIA ||
               instruction->opcode == AARCH64_OPCODE_AUTIB ||
               instruction->opcode == AARCH64_OPCODE_AUTDA ||
               instruction->opcode == AARCH64_OPCODE_AUTDB ||
               instruction->opcode == AARCH64_OPCODE_AUTIZA ||
               instruction->opcode == AARCH64_OPCODE_AUTIZB ||
               instruction->opcode == AARCH64_OPCODE_AUTDZA ||
               instruction->opcode == AARCH64_OPCODE_AUTDZB ||
               instruction->opcode == AARCH64_OPCODE_AUTIASP ||
               instruction->opcode == AARCH64_OPCODE_AUTIBSP) {
        kind = ARMITO_IR_OP_PAC_AUTHENTICATE;
        has_result = 1;
        flags |= ARMITO_IR_PAC_FLAG_FAIL_CLOSED;
    } else if (instruction->opcode == AARCH64_OPCODE_XPACI ||
               instruction->opcode == AARCH64_OPCODE_XPACD) {
        kind = ARMITO_IR_OP_PAC_STRIP;
        has_result = 1;
    } else if (instruction->opcode == AARCH64_OPCODE_BRAA ||
               instruction->opcode == AARCH64_OPCODE_BRAB ||
               instruction->opcode == AARCH64_OPCODE_BRAAZ ||
               instruction->opcode == AARCH64_OPCODE_BRABZ) {
        kind = ARMITO_IR_OP_PAC_BRANCH_AUTHENTICATED;
        flags |= ARMITO_IR_PAC_FLAG_FAIL_CLOSED;
        effect_register = 1;
        effect_class = ARMITO_IR_REG_PC;
    } else if (instruction->opcode == AARCH64_OPCODE_BLRAA ||
               instruction->opcode == AARCH64_OPCODE_BLRAB ||
               instruction->opcode == AARCH64_OPCODE_BLRAAZ ||
               instruction->opcode == AARCH64_OPCODE_BLRABZ) {
        kind = ARMITO_IR_OP_PAC_CALL_AUTHENTICATED;
        flags |= ARMITO_IR_PAC_FLAG_FAIL_CLOSED |
                 ARMITO_IR_PAC_FLAG_WRITES_LINK_REGISTER;
        effect_register = 1;
        effect_class = ARMITO_IR_REG_PC;
    } else if (instruction->opcode == AARCH64_OPCODE_RETAA ||
               instruction->opcode == AARCH64_OPCODE_RETAB) {
        kind = ARMITO_IR_OP_PAC_RETURN_AUTHENTICATED;
        flags |= ARMITO_IR_PAC_FLAG_FAIL_CLOSED |
                 ARMITO_IR_PAC_FLAG_MODIFIER_IS_SP;
        effect_register = 1;
        effect_class = ARMITO_IR_REG_PC;
    } else if (instruction->opcode == AARCH64_OPCODE_LDRAA ||
               instruction->opcode == AARCH64_OPCODE_LDRAB) {
        kind = ARMITO_IR_OP_PAC_LOAD_AUTHENTICATED;
        flags |= ARMITO_IR_PAC_FLAG_ADDRESS_DIVERSITY |
                 ARMITO_IR_PAC_FLAG_FAIL_CLOSED;
        has_result = 1;
    } else {
        return ARMITO_DECODER_UNSUPPORTED_INSTRUCTION;
    }

    if (kind == ARMITO_IR_OP_PAC_RETURN_AUTHENTICATED) {
        if (AARCH64IRGetRegisterValue(30, 0, 0, type_u64, &inputs[0]) !=
                ARMITO_IR_SUCCESS ||
            AARCH64IRGetRegisterValue(31, 1, 0, type_u64, &inputs[1]) !=
                ARMITO_IR_SUCCESS)
            return ARMITO_DECODER_IR_ERROR;
        input_count = 2;
        effect_index = 0;
    } else if (kind == ARMITO_IR_OP_PAC_BRANCH_AUTHENTICATED ||
               kind == ARMITO_IR_OP_PAC_CALL_AUTHENTICATED) {
        if (AARCH64IRGetRegisterValue(
                instruction->rn_index,
                instruction->rn_is_sp,
                instruction->rn_is_zero,
                type_u64,
                &inputs[0]
            ) != ARMITO_IR_SUCCESS ||
            AARCH64IRGetRegisterValue(
                instruction->rm_index,
                instruction->rm_is_sp,
                instruction->rm_is_zero,
                type_u64,
                &inputs[1]
            ) != ARMITO_IR_SUCCESS)
            return ARMITO_DECODER_IR_ERROR;
        input_count = 2;
        effect_index = 0;
        if (instruction->auth_modifier_is_zero)
            flags |= ARMITO_IR_PAC_FLAG_MODIFIER_IS_ZERO;
    } else if (kind == ARMITO_IR_OP_PAC_LOAD_AUTHENTICATED) {
        if (AARCH64IRGetRegisterValue(
                instruction->rn_index,
                instruction->rn_is_sp,
                instruction->rn_is_zero,
                type_u64,
                &inputs[0]
            ) != ARMITO_IR_SUCCESS)
            return ARMITO_DECODER_IR_ERROR;
        ir_status = ArmitoIRValuesDeclare(
            ARMITO_IR_VALUE_IMMEDIATE, type_u64, "pac-load-offset", &inputs[1]
        );
        if (ir_status != ARMITO_IR_SUCCESS)
            return ARMITO_DECODER_IR_ERROR;
        ir_status = ArmitoIRValuesSetImmediate(
            inputs[1], (uint64_t)instruction->immediate
        );
        if (ir_status != ARMITO_IR_SUCCESS)
            return ARMITO_DECODER_IR_ERROR;
        input_count = 2;
        effect_index = instruction->rd_index;
        effect_register = (uint8_t)!instruction->rd_is_zero;
    } else {
        if (AARCH64IRGetRegisterValue(
                instruction->rd_index,
                instruction->rd_is_sp,
                instruction->rd_is_zero,
                type_u64,
                &inputs[0]
            ) != ARMITO_IR_SUCCESS)
            return ARMITO_DECODER_IR_ERROR;
        input_count = 1;
        if (kind != ARMITO_IR_OP_PAC_STRIP) {
            if (instruction->auth_modifier_is_sp) {
                if (AARCH64IRGetRegisterValue(
                        31, 1, 0, type_u64, &inputs[1]
                    ) != ARMITO_IR_SUCCESS)
                    return ARMITO_DECODER_IR_ERROR;
                flags |= ARMITO_IR_PAC_FLAG_MODIFIER_IS_SP;
            } else if (instruction->auth_modifier_is_zero) {
                if (AARCH64IRGetRegisterValue(
                        31, 0, 1, type_u64, &inputs[1]
                    ) != ARMITO_IR_SUCCESS)
                    return ARMITO_DECODER_IR_ERROR;
                flags |= ARMITO_IR_PAC_FLAG_MODIFIER_IS_ZERO;
            } else if (AARCH64IRGetRegisterValue(
                           instruction->rn_index,
                           instruction->rn_is_sp,
                           instruction->rn_is_zero,
                           type_u64,
                           &inputs[1]
                       ) != ARMITO_IR_SUCCESS) {
                return ARMITO_DECODER_IR_ERROR;
            }
            input_count = 2;
        }
        effect_index = instruction->rd_index;
        effect_register = (uint8_t)!instruction->rd_is_zero;
        if (instruction->auth_modifier_is_sp)
            flags |= ARMITO_IR_PAC_FLAG_MODIFIER_IS_SP;
    }

    if (has_result) {
        ir_status = ArmitoIRValuesDeclare(
            ARMITO_IR_VALUE_VIRTUAL_REGISTER,
            type_u64,
            "pac-result",
            &result
        );
        if (ir_status != ARMITO_IR_SUCCESS)
            return ARMITO_DECODER_IR_ERROR;
    }

    ir_status = ArmitoIROperationEmit(
        kind,
        has_result ? type_u64 : ARMITO_IR_TYPE_INVALID,
        result,
        inputs,
        input_count,
        &operation_id
    );
    if (ir_status != ARMITO_IR_SUCCESS)
        return ARMITO_DECODER_IR_ERROR;

    ir_status = ArmitoIROperationSetPointerAuth(
        operation_id, key, flags, 0
    );
    if (ir_status != ARMITO_IR_SUCCESS)
        return ARMITO_DECODER_IR_ERROR;

    if (effect_register) {
        ir_status = ArmitoIROperationSetEffectRegister(
            operation_id,
            kind == ARMITO_IR_OP_PAC_LOAD_AUTHENTICATED ||
                    kind == ARMITO_IR_OP_PAC_SIGN ||
                    kind == ARMITO_IR_OP_PAC_AUTHENTICATE ||
                    kind == ARMITO_IR_OP_PAC_STRIP
                ? ARMITO_IR_REG_GPR
                : effect_class,
            effect_index
        );
        if (ir_status != ARMITO_IR_SUCCESS)
            return ARMITO_DECODER_IR_ERROR;
    }

    ir_status = ArmitoIROperationSetSource(
        operation_id,
        (uint32_t)(address & 0xFFFFFFFFu),
        instruction->encoding,
        ARMITO_DECODER_INSTRUCTION_SIZE
    );
    if (ir_status != ARMITO_IR_SUCCESS)
        return ARMITO_DECODER_IR_ERROR;

    *operations_emitted = 1;
    return ARMITO_DECODER_UNSUPPORTED_INSTRUCTION;
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
    case AARCH64_OPCODE_PACIA:
    case AARCH64_OPCODE_PACIB:
    case AARCH64_OPCODE_PACDA:
    case AARCH64_OPCODE_PACDB:
    case AARCH64_OPCODE_PACIZA:
    case AARCH64_OPCODE_PACIZB:
    case AARCH64_OPCODE_PACDZA:
    case AARCH64_OPCODE_PACDZB:
    case AARCH64_OPCODE_AUTIA:
    case AARCH64_OPCODE_AUTIB:
    case AARCH64_OPCODE_AUTDA:
    case AARCH64_OPCODE_AUTDB:
    case AARCH64_OPCODE_AUTIZA:
    case AARCH64_OPCODE_AUTIZB:
    case AARCH64_OPCODE_AUTDZA:
    case AARCH64_OPCODE_AUTDZB:
    case AARCH64_OPCODE_XPACI:
    case AARCH64_OPCODE_XPACD:
    case AARCH64_OPCODE_PACIASP:
    case AARCH64_OPCODE_PACIBSP:
    case AARCH64_OPCODE_AUTIASP:
    case AARCH64_OPCODE_AUTIBSP:
    case AARCH64_OPCODE_RETAA:
    case AARCH64_OPCODE_RETAB:
    case AARCH64_OPCODE_BRAA:
    case AARCH64_OPCODE_BRAB:
    case AARCH64_OPCODE_BRAAZ:
    case AARCH64_OPCODE_BRABZ:
    case AARCH64_OPCODE_BLRAA:
    case AARCH64_OPCODE_BLRAB:
    case AARCH64_OPCODE_BLRAAZ:
    case AARCH64_OPCODE_BLRABZ:
    case AARCH64_OPCODE_LDRAA:
    case AARCH64_OPCODE_LDRAB:
        return AARCH64PointerAuthToIR(instruction, address, operations_emitted);
    default:
        break;
    }

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
    case AARCH64_OPCODE_CATALOG_ONLY: return "catalog-only";
    case AARCH64_OPCODE_NOP: return "nop";
    case AARCH64_OPCODE_HINT: return "hint";
    case AARCH64_OPCODE_B: return "b";
    case AARCH64_OPCODE_BL: return "bl";
    case AARCH64_OPCODE_BR: return "br";
    case AARCH64_OPCODE_BLR: return "blr";
    case AARCH64_OPCODE_RET: return "ret";
    case AARCH64_OPCODE_PACIA: return "pacia";
    case AARCH64_OPCODE_PACIB: return "pacib";
    case AARCH64_OPCODE_PACDA: return "pacda";
    case AARCH64_OPCODE_PACDB: return "pacdb";
    case AARCH64_OPCODE_PACIZA: return "paciza";
    case AARCH64_OPCODE_PACIZB: return "pacizb";
    case AARCH64_OPCODE_PACDZA: return "pacdza";
    case AARCH64_OPCODE_PACDZB: return "pacdzb";
    case AARCH64_OPCODE_AUTIA: return "autia";
    case AARCH64_OPCODE_AUTIB: return "autib";
    case AARCH64_OPCODE_AUTDA: return "autda";
    case AARCH64_OPCODE_AUTDB: return "autdb";
    case AARCH64_OPCODE_AUTIZA: return "autiza";
    case AARCH64_OPCODE_AUTIZB: return "autizb";
    case AARCH64_OPCODE_AUTDZA: return "autdza";
    case AARCH64_OPCODE_AUTDZB: return "autdzb";
    case AARCH64_OPCODE_XPACI: return "xpaci";
    case AARCH64_OPCODE_XPACD: return "xpacd";
    case AARCH64_OPCODE_PACIASP: return "paciasp";
    case AARCH64_OPCODE_PACIBSP: return "pacibsp";
    case AARCH64_OPCODE_AUTIASP: return "autiasp";
    case AARCH64_OPCODE_AUTIBSP: return "autibsp";
    case AARCH64_OPCODE_RETAA: return "retaa";
    case AARCH64_OPCODE_RETAB: return "retab";
    case AARCH64_OPCODE_BRAA: return "braa";
    case AARCH64_OPCODE_BRAB: return "brab";
    case AARCH64_OPCODE_BRAAZ: return "braaz";
    case AARCH64_OPCODE_BRABZ: return "brabz";
    case AARCH64_OPCODE_BLRAA: return "blraa";
    case AARCH64_OPCODE_BLRAB: return "blrab";
    case AARCH64_OPCODE_BLRAAZ: return "blraaz";
    case AARCH64_OPCODE_BLRABZ: return "blrabz";
    case AARCH64_OPCODE_LDRAA: return "ldraa";
    case AARCH64_OPCODE_LDRAB: return "ldrab";
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
