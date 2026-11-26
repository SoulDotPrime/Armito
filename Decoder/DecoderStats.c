#include "Includes/DecoderStats.h"

static ArmitoDecoderStats decoder_stats;

ArmitoDecoderStatus ArmitoDecoderStatsReset(void)
{
    decoder_stats.instructions_seen = 0;
    decoder_stats.instructions_decoded = 0;
    decoder_stats.instructions_unsupported = 0;
    decoder_stats.instructions_invalid = 0;
    decoder_stats.operations_emitted = 0;
    decoder_stats.last_offset = 0;
    decoder_stats.last_encoding = 0;

    return ARMITO_DECODER_SUCCESS;
}

ArmitoDecoderStatus ArmitoDecoderStatsGet(ArmitoDecoderStats *out)
{
    if (out == NULL)
        return ARMITO_DECODER_INVALID_ARGUMENT;

    *out = decoder_stats;

    return ARMITO_DECODER_SUCCESS;
}

ArmitoDecoderStatus ArmitoDecoderStatsRecord(
    ArmitoDecodeResult result,
    uint64_t offset,
    uint32_t encoding,
    uint32_t operations
)
{
    decoder_stats.instructions_seen++;
    decoder_stats.last_offset = offset;
    decoder_stats.last_encoding = encoding;
    decoder_stats.operations_emitted += operations;

    switch (result) {
    case ARMITO_DECODE_OK:
        decoder_stats.instructions_decoded++;
        break;
    case ARMITO_DECODE_UNSUPPORTED:
        decoder_stats.instructions_unsupported++;
        break;
    case ARMITO_DECODE_INVALID:
        decoder_stats.instructions_invalid++;
        break;
    default:
        return ARMITO_DECODER_INVALID_ARGUMENT;
    }

    return ARMITO_DECODER_SUCCESS;
}
