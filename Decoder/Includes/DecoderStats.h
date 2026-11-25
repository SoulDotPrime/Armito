#ifndef ARMITO_DECODER_STATS_H
#define ARMITO_DECODER_STATS_H

#include "Includes/DecoderTypes.h"

typedef struct {
    uint64_t instructions_seen;
    uint64_t instructions_decoded;
    uint64_t instructions_unsupported;
    uint64_t instructions_invalid;
    uint64_t operations_emitted;
    uint64_t last_offset;
    uint32_t last_encoding;
} ArmitoDecoderStats;

ArmitoDecoderStatus ArmitoDecoderStatsReset(void);
ArmitoDecoderStatus ArmitoDecoderStatsGet(ArmitoDecoderStats *stats);
ArmitoDecoderStatus ArmitoDecoderStatsRecord(
    ArmitoDecodeResult result,
    uint64_t offset,
    uint32_t encoding,
    uint32_t operations
);

#endif
