#include "Includes/DecoderStats.h"

static ArmitoDecoderStats stats;

ArmitoDecoderStatus ArmitoDecoderStatsReset(void)
{
    stats.instructions_seen = 0;
    stats.instructions_decoded = 0;
    stats.instructions_unsupported = 0;
    stats.instructions_invalid = 0;
    stats.operations_emitted = 0;
    stats.last_offset = 0;
    stats.last_encoding = 0;

    return ARMITO_DECODER_SUCCESS;
}

ArmitoDecoderStatus ArmitoDecoderStatsGet(ArmitoDecoderStats *out)
{
    if (out == NULL)
        return ARMITO_DECODER_INVALID_ARGUMENT;

    *out = stats;

    return ARMITO_DECODER_SUCCESS;
}

ArmitoDecoderStatus ArmitoDecoderStatsRecord(
    ArmitoDecodeResult result,
    uint64_t offset,
    uint32_t encoding,
    uint32_t operations
)
{
    stats.instructions_seen++;
    stats.last_offset = offset;
    stats.last_encoding = encoding;
    stats.operations_emitted += operations;

    switch (result) {
    case ARMITO_DECODE_OK:
        stats.instructions_decoded++;
        break;
    case ARMITO_DECODE_UNSUPPORTED:
        stats.instructions_unsupported++;
        break;
    case ARMITO_DECODE_INVALID:
        stats.instructions_invalid++;
        break;
    default:
        return ARMITO_DECODER_INVALID_ARGUMENT;
    }

    return ARMITO_DECODER_SUCCESS;
}
