#ifndef ARMITO_DECODER_SOURCE_H
#define ARMITO_DECODER_SOURCE_H

#include "Includes/DecoderTypes.h"

typedef struct {
    const uint8_t *base;
    uint64_t size;
    uint64_t address;
    uint8_t executable;
    uint8_t readable;
} ArmitoDecoderSource;

typedef struct {
    uint64_t offset;
    uint64_t address;
    uint32_t encoding;
} ArmitoDecoderInstruction;

ArmitoDecoderStatus ArmitoDecoderSourceSet(
    const uint8_t *base,
    uint64_t size,
    uint64_t address,
    uint8_t executable,
    uint8_t readable
);
ArmitoDecoderStatus ArmitoDecoderSourceClear(void);
ArmitoDecoderStatus ArmitoDecoderSourceGet(ArmitoDecoderSource *source);
ArmitoDecoderStatus ArmitoDecoderSourceFetch(
    uint64_t offset,
    ArmitoDecoderInstruction *instruction
);

#endif
