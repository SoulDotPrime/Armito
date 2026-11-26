#include "Includes/Decoder.h"
#include "AARCH64/Includes/AARCH64.h"
#include "AARCH64/Catalog/AARCH64Catalog.h"
#include "AARCH64/Includes/AARCH64Operands.h"

#include "Includes/IR.h"

static ArmitoDecoderConfig armito_decoder_config;
static ArmitoDecoderState armito_decoder_state = ARMITO_DECODER_UNINITIALIZED;
static ArmitoIRTypeID armito_type_u32 = ARMITO_IR_TYPE_INVALID;
static ArmitoIRTypeID armito_type_u64 = ARMITO_IR_TYPE_INVALID;
static ArmitoIRTypeID armito_type_flags = ARMITO_IR_TYPE_INVALID;
static ArmitoIRValueID armito_register_values[ARMITO_DECODER_GPR_COUNT + 1];
static uint8_t armito_registers_mapped = 0;
static AARCH64OperandExtractorFunction volatile armito_operand_extractor;

static ArmitoDecoderStatus ArmitoEmitUnsupported(
    const ArmitoDecoderInstruction *instruction,
    uint32_t *operations
)
{
    ArmitoIROperationID operation_id;
    ArmitoIRStatus ir_status;

    *operations = 0;
    if (!armito_decoder_config.record_unsupported)
        return ARMITO_DECODER_SUCCESS;

    ir_status = ArmitoIROperationEmit(
        ARMITO_IR_OP_UNSUPPORTED,
        ARMITO_IR_TYPE_INVALID,
        ARMITO_IR_VALUE_INVALID,
        NULL,
        0,
        &operation_id
    );
    if (ir_status != ARMITO_IR_SUCCESS)
        return ARMITO_DECODER_IR_ERROR;

    ir_status = ArmitoIROperationSetSource(
        operation_id,
        (uint32_t)(instruction->address & 0xFFFFFFFFu),
        instruction->encoding,
        ARMITO_DECODER_INSTRUCTION_SIZE
    );
    if (ir_status != ARMITO_IR_SUCCESS)
        return ARMITO_DECODER_IR_ERROR;

    *operations = 1;
    return ARMITO_DECODER_SUCCESS;
}

static ArmitoDecoderStatus ArmitoMapRegisters(void)
{
    uint8_t i;
    ArmitoIRStatus ir_status;

    if (armito_registers_mapped)
        return ARMITO_DECODER_SUCCESS;

    for (i = 0; i < ARMITO_DECODER_GPR_COUNT; i++) {
        ir_status = ArmitoIRValuesDeclare(
            ARMITO_IR_VALUE_VIRTUAL_REGISTER,
            armito_type_u64,
            ArmitoDecoderRegisterName((ArmitoDecoderGPR)i),
            &armito_register_values[i]
        );

        if (ir_status != ARMITO_IR_SUCCESS)
            return ARMITO_DECODER_OUT_OF_RESOURCES;
    }

    ir_status = ArmitoIRValuesDeclare(
        ARMITO_IR_VALUE_VIRTUAL_REGISTER,
        armito_type_u64,
        "sp",
        &armito_register_values[ARMITO_DECODER_GPR_COUNT]
    );

    if (ir_status != ARMITO_IR_SUCCESS)
        return ARMITO_DECODER_OUT_OF_RESOURCES;

    for (i = 0; i < ARMITO_DECODER_GPR_COUNT; i++) {
        ir_status = ArmitoIRCPUStateMap(
            ARMITO_IR_REG_GPR,
            i,
            64,
            ArmitoDecoderRegisterName((ArmitoDecoderGPR)i),
            armito_register_values[i]
        );

        if (ir_status != ARMITO_IR_SUCCESS)
            return ARMITO_DECODER_ERROR;
    }

    ir_status = ArmitoIRCPUStateMap(
        ARMITO_IR_REG_STACK_POINTER,
        31,
        64,
        "sp",
        armito_register_values[ARMITO_DECODER_GPR_COUNT]
    );

    if (ir_status != ARMITO_IR_SUCCESS)
        return ARMITO_DECODER_ERROR;

    armito_registers_mapped = 1;

    return ARMITO_DECODER_SUCCESS;
}

ArmitoDecoderStatus ArmitoDecoderInitialize(const ArmitoDecoderConfig *config)
{
    ArmitoIRStatus ir_status;

    if (config == NULL)
        return ARMITO_DECODER_INVALID_ARGUMENT;

    if (config->isa != ARMITO_ISA_AARCH64)
        return ARMITO_DECODER_ERROR;

    if (config->max_instructions == 0)
        return ARMITO_DECODER_INVALID_ARGUMENT;

    if (!AARCH64CatalogInitialize())
        return ARMITO_DECODER_ERROR;

    armito_decoder_config = *config;
    armito_decoder_config.stop_at_branch = config->stop_at_branch ? 1 : 0;
    armito_decoder_config.record_unsupported = config->record_unsupported ? 1 : 0;

    ir_status = ArmitoIRReset();

    if (ir_status != ARMITO_IR_SUCCESS)
        return ARMITO_DECODER_IR_ERROR;

    ir_status = ArmitoIRTypesRegister(
        ARMITO_IR_CLASS_INT, 32, 1, 0, 0, "u32", &armito_type_u32
    );

    if (ir_status != ARMITO_IR_SUCCESS)
        return ARMITO_DECODER_IR_ERROR;

    ir_status = ArmitoIRTypesRegister(
        ARMITO_IR_CLASS_INT, 64, 1, 0, 0, "u64", &armito_type_u64
    );

    if (ir_status != ARMITO_IR_SUCCESS)
        return ARMITO_DECODER_IR_ERROR;

    ir_status = ArmitoIRTypesRegister(
        ARMITO_IR_CLASS_FLAGS, 4, 1, 0, 0, "flags", &armito_type_flags
    );

    if (ir_status != ARMITO_IR_SUCCESS)
        return ARMITO_DECODER_IR_ERROR;

    armito_registers_mapped = 0;

    if (ArmitoMapRegisters() != ARMITO_DECODER_SUCCESS)
        return ARMITO_DECODER_IR_ERROR;

    armito_operand_extractor = AARCH64OperandsExtractWithProfile;
    if (armito_operand_extractor == NULL)
        return ARMITO_DECODER_ERROR;

    ArmitoDecoderStatsReset();
    ArmitoDecoderSourceClear();

    armito_decoder_state = ARMITO_DECODER_READY;

    return ARMITO_DECODER_SUCCESS;
}

ArmitoDecoderStatus ArmitoDecoderReset(void)
{
    if (armito_decoder_state == ARMITO_DECODER_UNINITIALIZED)
        return ARMITO_DECODER_INVALID_STATE;

    armito_decoder_state = ARMITO_DECODER_READY;
    ArmitoDecoderStatsReset();
    ArmitoDecoderSourceClear();
    armito_registers_mapped = 0;

    return ARMITO_DECODER_SUCCESS;
}

ArmitoDecoderStatus ArmitoDecoderShutdown(void)
{
    if (armito_decoder_state == ARMITO_DECODER_UNINITIALIZED)
        return ARMITO_DECODER_SUCCESS;

    armito_decoder_state = ARMITO_DECODER_UNINITIALIZED;
    armito_type_u32 = ARMITO_IR_TYPE_INVALID;
    armito_type_u64 = ARMITO_IR_TYPE_INVALID;
    armito_type_flags = ARMITO_IR_TYPE_INVALID;
    armito_registers_mapped = 0;
    armito_operand_extractor = NULL;
    ArmitoDecoderStatsReset();
    ArmitoDecoderSourceClear();

    return ARMITO_DECODER_SUCCESS;
}

ArmitoDecoderState ArmitoDecoderGetState(void)
{
    return armito_decoder_state;
}

AARCH64OperandExtractorFunction ArmitoDecoderGetOperandExtractor(void)
{
    if (armito_decoder_state == ARMITO_DECODER_UNINITIALIZED)
        return NULL;
    return armito_operand_extractor;
}

ArmitoDecoderStatus ArmitoDecoderDecodeInstruction(
    const ArmitoDecoderInstruction *instruction,
    ArmitoDecodeResult *result
)
{
    AARCH64Instruction decoded;
    ArmitoDecoderStatus status;
    uint32_t operations = 0;

    if (instruction == NULL || result == NULL)
        return ARMITO_DECODER_INVALID_ARGUMENT;

    if (armito_decoder_state != ARMITO_DECODER_READY &&
        armito_decoder_state != ARMITO_DECODER_DECODING)
        return ARMITO_DECODER_INVALID_STATE;

    *result = ARMITO_DECODE_INVALID;

    status = AARCH64Decode(instruction->encoding, &decoded);

    if (status != ARMITO_DECODER_SUCCESS) {
        *result = AARCH64ResultFor(status);
        if (status == ARMITO_DECODER_UNSUPPORTED_INSTRUCTION) {
            ArmitoDecoderStatus emit_status =
                ArmitoEmitUnsupported(instruction, &operations);
            if (emit_status != ARMITO_DECODER_SUCCESS)
                return emit_status;
        }
        ArmitoDecoderStatsRecord(
            *result, instruction->offset, instruction->encoding, operations
        );
        return status;
    }

    status = AARCH64ToIR(&decoded, instruction->address, &operations);

    if (status != ARMITO_DECODER_SUCCESS) {
        *result = AARCH64ResultFor(status);
        ArmitoDecoderStatsRecord(
            *result, instruction->offset, instruction->encoding, operations
        );
        return status;
    }

    *result = ARMITO_DECODE_OK;

    ArmitoDecoderStatsRecord(
        ARMITO_DECODE_OK, instruction->offset, instruction->encoding, operations
    );

    return ARMITO_DECODER_SUCCESS;
}

ArmitoDecoderStatus ArmitoDecoderDecodeBlock(
    uint64_t guest_address,
    ArmitoDecoderSummary *summary
)
{
    ArmitoDecoderSource src;
    ArmitoDecoderInstruction instruction;
    ArmitoDecoderSummary local;
    ArmitoDecoderStatus status;
    ArmitoDecoderStatus terminal_status = ARMITO_DECODER_SUCCESS;
    ArmitoDecodeResult result;
    ArmitoIRLabelID block_id = ARMITO_IR_LABEL_INVALID;
    ArmitoIRStatus ir_status;
    uint64_t offset;
    uint64_t decoded = 0;
    AARCH64Opcode opcode;

    if (summary == NULL)
        return ARMITO_DECODER_INVALID_ARGUMENT;

    status = ArmitoDecoderSourceGet(&src);

    if (status != ARMITO_DECODER_SUCCESS)
        return status;

    if (guest_address < src.address)
        return ARMITO_DECODER_OUT_OF_RANGE;

    offset = guest_address - src.address;

    if (offset % ARMITO_DECODER_ALIGNMENT != 0)
        return ARMITO_DECODER_INVALID_ARGUMENT;

    if (offset >= src.size)
        return ARMITO_DECODER_OUT_OF_RANGE;

    if (armito_decoder_state != ARMITO_DECODER_READY &&
        armito_decoder_state != ARMITO_DECODER_DECODING)
        return ARMITO_DECODER_INVALID_STATE;

    local.isa = ARMITO_ISA_AARCH64;
    local.instructions_decoded = 0;
    local.instructions_unsupported = 0;
    local.instructions_invalid = 0;
    local.operations_emitted = 0;
    local.blocks_emitted = 0;
    local.bytes_consumed = 0;
    local.next_address = guest_address;
    local.last_result = ARMITO_DECODE_OK;

    armito_decoder_state = ARMITO_DECODER_DECODING;

    ir_status = ArmitoIRBlockBegin(guest_address, "block", &block_id);

    if (ir_status != ARMITO_IR_SUCCESS) {
        armito_decoder_state = ARMITO_DECODER_READY;
        return ARMITO_DECODER_IR_ERROR;
    }

    local.blocks_emitted = 1;

    while (decoded < armito_decoder_config.max_instructions) {
        status = ArmitoDecoderSourceFetch(offset, &instruction);

        if (status != ARMITO_DECODER_SUCCESS) {
            terminal_status = status;
            local.last_result = ARMITO_DECODE_INVALID;
            break;
        }

        status = ArmitoDecoderDecodeInstruction(&instruction, &result);

        local.last_result = result;

        if (result == ARMITO_DECODE_INVALID) {
            local.instructions_invalid++;
            terminal_status = status;
            break;
        }

        if (result == ARMITO_DECODE_UNSUPPORTED)
            local.instructions_unsupported++;
        else
            local.instructions_decoded++;

        decoded++;
        offset += ARMITO_DECODER_INSTRUCTION_SIZE;
        local.bytes_consumed += ARMITO_DECODER_INSTRUCTION_SIZE;
        local.next_address = src.address + offset;

        AARCH64Classify(instruction.encoding, &opcode);
        if (armito_decoder_config.stop_at_branch &&
            (opcode == AARCH64_OPCODE_B ||
             opcode == AARCH64_OPCODE_BL ||
             opcode == AARCH64_OPCODE_BR ||
             opcode == AARCH64_OPCODE_BLR ||
             opcode == AARCH64_OPCODE_B_COND ||
             opcode == AARCH64_OPCODE_RET ||
             opcode == AARCH64_OPCODE_BRAA ||
             opcode == AARCH64_OPCODE_BRAB ||
             opcode == AARCH64_OPCODE_BRAAZ ||
             opcode == AARCH64_OPCODE_BRABZ ||
             opcode == AARCH64_OPCODE_BLRAA ||
             opcode == AARCH64_OPCODE_BLRAB ||
             opcode == AARCH64_OPCODE_BLRAAZ ||
             opcode == AARCH64_OPCODE_BLRABZ ||
             opcode == AARCH64_OPCODE_RETAA ||
             opcode == AARCH64_OPCODE_RETAB ||
             opcode == AARCH64_OPCODE_CBZ ||
             opcode == AARCH64_OPCODE_CBNZ ||
             opcode == AARCH64_OPCODE_TBZ ||
             opcode == AARCH64_OPCODE_TBNZ))
            break;

        if (offset >= src.size)
            break;
    }

    ir_status = ArmitoIRBlockEnd();

    if (ir_status != ARMITO_IR_SUCCESS) {
        armito_decoder_state = ARMITO_DECODER_FAILED;
        return ARMITO_DECODER_IR_ERROR;
    }

    {
        ArmitoDecoderStats stats;

        ArmitoDecoderStatsGet(&stats);
        local.operations_emitted = stats.operations_emitted;
    }

    *summary = local;

    armito_decoder_state = ARMITO_DECODER_FINISHED;

    return terminal_status;
}

ArmitoDecoderStatus ArmitoDecoderGetStats(ArmitoDecoderStats *stats)
{
    return ArmitoDecoderStatsGet(stats);
}

ArmitoISA ArmitoDecoderGetISA(void)
{
    return armito_decoder_config.isa;
}

const char *ArmitoDecoderStatusName(ArmitoDecoderStatus status)
{
    switch (status) {
    case ARMITO_DECODER_SUCCESS: return "success";
    case ARMITO_DECODER_ERROR: return "error";
    case ARMITO_DECODER_INVALID_ARGUMENT: return "invalid-argument";
    case ARMITO_DECODER_INVALID_STATE: return "invalid-state";
    case ARMITO_DECODER_OUT_OF_RESOURCES: return "out-of-resources";
    case ARMITO_DECODER_OUT_OF_RANGE: return "out-of-range";
    case ARMITO_DECODER_TRUNCATED: return "truncated";
    case ARMITO_DECODER_INVALID_INSTRUCTION: return "invalid-instruction";
    case ARMITO_DECODER_UNSUPPORTED_INSTRUCTION: return "unsupported-instruction";
    case ARMITO_DECODER_IR_ERROR: return "ir-error";
    default: return "unknown";
    }
}
