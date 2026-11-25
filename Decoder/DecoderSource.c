#include "Includes/DecoderSource.h"

static ArmitoDecoderSource source;
static uint8_t source_set = 0;

ArmitoDecoderStatus ArmitoDecoderSourceSet(
    const uint8_t *base,
    uint64_t size,
    uint64_t address,
    uint8_t executable,
    uint8_t readable
)
{
    if (base == NULL && size > 0)
        return ARMITO_DECODER_INVALID_ARGUMENT;

    if (address % ARMITO_DECODER_ALIGNMENT != 0)
        return ARMITO_DECODER_INVALID_ARGUMENT;

    source.base = base;
    source.size = size;
    source.address = address;
    source.executable = executable ? 1 : 0;
    source.readable = readable ? 1 : 0;
    source_set = 1;

    return ARMITO_DECODER_SUCCESS;
}

ArmitoDecoderStatus ArmitoDecoderSourceClear(void)
{
    source.base = NULL;
    source.size = 0;
    source.address = 0;
    source.executable = 0;
    source.readable = 0;
    source_set = 0;

    return ARMITO_DECODER_SUCCESS;
}

ArmitoDecoderStatus ArmitoDecoderSourceGet(ArmitoDecoderSource *out)
{
    if (out == NULL)
        return ARMITO_DECODER_INVALID_ARGUMENT;

    if (!source_set)
        return ARMITO_DECODER_INVALID_STATE;

    *out = source;

    return ARMITO_DECODER_SUCCESS;
}

ArmitoDecoderStatus ArmitoDecoderSourceFetch(
    uint64_t offset,
    ArmitoDecoderInstruction *instruction
)
{
    const uint8_t *bytes;
    uint32_t encoding;

    if (instruction == NULL)
        return ARMITO_DECODER_INVALID_ARGUMENT;

    if (!source_set)
        return ARMITO_DECODER_INVALID_STATE;

    if (!source.executable)
        return ARMITO_DECODER_INVALID_STATE;

    if (offset % ARMITO_DECODER_ALIGNMENT != 0)
        return ARMITO_DECODER_INVALID_ARGUMENT;

    if (offset >= source.size)
        return ARMITO_DECODER_OUT_OF_RANGE;

    if (source.size - offset < ARMITO_DECODER_INSTRUCTION_SIZE)
        return ARMITO_DECODER_TRUNCATED;

    bytes = source.base + offset;

    encoding = (uint32_t)bytes[0] |
               ((uint32_t)bytes[1] << 8) |
               ((uint32_t)bytes[2] << 16) |
               ((uint32_t)bytes[3] << 24);

    instruction->offset = offset;
    instruction->address = source.address + offset;
    instruction->encoding = encoding;

    return ARMITO_DECODER_SUCCESS;
}
