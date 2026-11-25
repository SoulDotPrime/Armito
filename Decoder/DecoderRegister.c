#include "Includes/DecoderRegister.h"

static const char *const gpr_names[ARMITO_DECODER_GPR_XZR + 1] = {
    "x0", "x1", "x2", "x3", "x4", "x5", "x6", "x7",
    "x8", "x9", "x10", "x11", "x12", "x13", "x14", "x15",
    "x16", "x17", "x18", "x19", "x20", "x21", "x22", "x23",
    "x24", "x25", "x26", "x27", "x28", "x29", "x30",
    "sp", "xzr"
};

ArmitoDecoderStatus ArmitoDecoderRegisterDecode(
    uint8_t field,
    uint8_t is_sp_allowed,
    uint8_t width_bits,
    ArmitoDecoderRegisterRef *reg
)
{
    if (reg == NULL)
        return ARMITO_DECODER_INVALID_ARGUMENT;

    if (width_bits != 32 && width_bits != 64)
        return ARMITO_DECODER_INVALID_ARGUMENT;

    reg->is_sp = 0;
    reg->is_zero = 0;
    reg->index = field;
    reg->width_bits = width_bits;

    if (field > 31)
        return ARMITO_DECODER_INVALID_ARGUMENT;

    if (field == 31) {
        if (is_sp_allowed) {
            reg->is_sp = 1;
            reg->gpr = ARMITO_DECODER_GPR_SP;
        } else {
            reg->is_zero = 1;
            reg->gpr = ARMITO_DECODER_GPR_XZR;
        }

        return ARMITO_DECODER_SUCCESS;
    }

    reg->gpr = (ArmitoDecoderGPR)field;

    return ARMITO_DECODER_SUCCESS;
}

const char *ArmitoDecoderRegisterName(ArmitoDecoderGPR gpr)
{
    if ((uint32_t)gpr > (uint32_t)ARMITO_DECODER_GPR_XZR)
        return "invalid";

    return gpr_names[gpr];
}
