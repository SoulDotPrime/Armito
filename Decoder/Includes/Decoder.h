#ifndef ARMITO_DECODER_H
#define ARMITO_DECODER_H

#include "Includes/DecoderTypes.h"
#include "Includes/DecoderSource.h"
#include "Includes/DecoderRegister.h"
#include "Includes/DecoderStats.h"
#include "AARCH64/Includes/AARCH64Operands.h"

typedef struct {
    ArmitoISA isa;
    uint64_t entry_address;
    uint64_t max_instructions;
    uint8_t stop_at_branch;
    uint8_t record_unsupported;
} ArmitoDecoderConfig;

typedef struct {
    ArmitoISA isa;
    uint64_t instructions_decoded;
    uint64_t instructions_unsupported;
    uint64_t instructions_invalid;
    uint64_t operations_emitted;
    uint64_t blocks_emitted;
    uint64_t bytes_consumed;
    uint64_t next_address;
    ArmitoDecodeResult last_result;
} ArmitoDecoderSummary;

ArmitoDecoderStatus ArmitoDecoderInitialize(const ArmitoDecoderConfig *config);
ArmitoDecoderStatus ArmitoDecoderReset(void);
ArmitoDecoderStatus ArmitoDecoderShutdown(void);
ArmitoDecoderState ArmitoDecoderGetState(void);
AARCH64OperandExtractorFunction ArmitoDecoderGetOperandExtractor(void);

ArmitoDecoderStatus ArmitoDecoderDecodeBlock(
    uint64_t guest_address,
    ArmitoDecoderSummary *summary
);

ArmitoDecoderStatus ArmitoDecoderDecodeInstruction(
    const ArmitoDecoderInstruction *instruction,
    ArmitoDecodeResult *result
);

ArmitoDecoderStatus ArmitoDecoderDecodeInstructionWithOperands(
    const ArmitoDecoderInstruction *instruction,
    ArmitoDecodeResult *result,
    AARCH64OperandSet *operands
);

ArmitoDecoderStatus ArmitoDecoderGetStats(ArmitoDecoderStats *stats);
ArmitoISA ArmitoDecoderGetISA(void);
const char *ArmitoDecoderStatusName(ArmitoDecoderStatus status);

#endif
