#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "Decoder/Includes/Decoder.h"
#include "AARCH64/Includes/AARCH64.h"
#include "AARCH64/Catalog/AARCH64Catalog.h"

static void PrintOperandSet(const AARCH64OperandSet *set)
{
    uint64_t metadata[] = {
        set->memory_kind, set->memory_data_bank,
        set->memory_register_list_start,
        set->memory_structure_count,
        set->memory_interleave_factor,
        set->memory_replicate,
        set->memory_merge_inactive, set->memory_lane_valid,
        set->memory_predicate_register,
        set->memory_depends_vl, set->memory_depends_svl,
        set->memory_mops_stage,
        set->memory_mops_source_register,
        set->memory_mops_destination_register,
        set->memory_mops_length_register,
        set->memory_mops_reads_carry,
        set->memory_mops_writes_nzcv,
        set->memory_state_dependent_size,
        set->memory_min_el,
        set->memory_tag_granule_bytes,
        set->memory_tag_block_symbolic,
        set->memory_tag_checked,
        set->memory_offset_kind,
        set->memory_list_stride,
        set->memory_mops_options,
        set->memory_tag_zero_data,
        set->memory_atomic_read_result,
        set->memory_writeback_register_count,
        set->memory_total_bytes,
        set->memory_postindex_immediate,
        set->memory_base_address,
        set->memory_mops_destination_discarded,
        set->memory_mops_source_zero
    };
    unsigned index;

    printf("\tMEM:%u:%u:%u:%u:%u:%u:%u:%u:%u:%u:%u:%u:%u:%u:%llu",
        set->memory_access_width, set->memory_element_count,
        set->memory_direction, set->memory_writeback,
        set->memory_base_register, set->memory_data_register,
        set->memory_pair_register, set->memory_sign_extend,
        set->memory_zero_extend, set->memory_literal,
        set->memory_base_is_sp, set->memory_base_is_zero,
        set->memory_data_is_sp, set->memory_data_is_zero,
        (unsigned long long)set->memory_offset);

    printf("\tMX:%u:%u:%u:%u:%u:%u:%u:%u:%u:%u:%u:%u:%u:%u:%u:%u:%u:%u:%u",
        set->memory_element_width, set->memory_vector_width,
        set->memory_lane, set->memory_register_count,
        set->memory_wraparound, set->memory_nontemporal,
        set->memory_unprivileged, set->memory_acquire,
        set->memory_release, set->memory_exclusive,
        set->memory_atomic, set->memory_tagged, set->memory_phase,
        set->memory_offset_register, set->memory_offset_extend,
        set->memory_offset_scale,
        set->memory_address_expression_resolved,
        set->memory_static_overlap,
        set->memory_execution_overlap);

    printf("\tSM");
    for (index = 0; index < sizeof(metadata) / sizeof(metadata[0]); index++)
        printf(":%llu", (unsigned long long)metadata[index]);

    printf("\tALIAS:%s",
        set->preferred_alias != 0
        ? set->preferred_alias->mnemonic : "-");

    for (index = 0; index < set->operand_count; index++) {
        const AARCH64Operand *operand = &set->operands[index];
        printf("\t%s:%u:%u:%u:%llu:%llu:%lld:%llu:%u:%u:%u:%u",
            operand->name, operand->kind, operand->role, operand->status,
            (unsigned long long)operand->value,
            (unsigned long long)operand->raw_value,
            (long long)operand->signed_value,
            (unsigned long long)operand->address, operand->scale,
            operand->flags, operand->register_width,
            operand->register_special);
    }
}

int main(int argc, char **argv)
{
    ArmitoDecoderConfig config;
    ArmitoDecoderInstruction instruction;
    ArmitoDecodeResult result;
    AARCH64OperandSet operands;
    char identifier[160];
    unsigned word;
    unsigned long long address;
    ArmitoDecoderStatus status;

    if (argc != 1)
        return 1;

    (void)argv;

    memset(&config, 0, sizeof(config));
    config.isa = ARMITO_ISA_AARCH64;
    config.entry_address = 0x1000;
    config.max_instructions = 16;
    config.stop_at_branch = 1;
    config.record_unsupported = 0;

    if (ArmitoDecoderInitialize(&config) != ARMITO_DECODER_SUCCESS)
        return 1;

    while (scanf("%159s %x %llx", identifier, &word, &address) == 3) {
        instruction.offset = 0;
        instruction.address = address;
        instruction.encoding = (uint32_t)word;

        memset(&operands, 0, sizeof(operands));
        status = ArmitoDecoderDecodeInstructionWithOperands(
            &instruction, &result, &operands
        );

        if (status == ARMITO_DECODER_UNSUPPORTED_INSTRUCTION) {
            printf("%s\t%08X\tUNSUPPORTED\n", identifier, (uint32_t)word);
            continue;
        }

        if (status == ARMITO_DECODER_INVALID_INSTRUCTION) {
            printf("%s\t%08X\tINVALID\n", identifier, (uint32_t)word);
            continue;
        }

        if (status != ARMITO_DECODER_SUCCESS) {
            printf("%s\t%08X\tERROR:%d\n", identifier, (uint32_t)word, status);
            continue;
        }

        if (result == ARMITO_DECODE_OK &&
            operands.status == AARCH64_OPERAND_EXTRACTION_COMPLETE) {
            printf("%s\t%08X\tOK\tCOMPLETE", identifier, (uint32_t)word);
            PrintOperandSet(&operands);
            putchar('\n');
        } else if (result == ARMITO_DECODE_OK) {
            printf("%s\t%08X\tOK\tNO-OPERANDS\n", identifier, (uint32_t)word);
        } else if (result == ARMITO_DECODE_UNSUPPORTED) {
            printf("%s\t%08X\tUNSUPPORTED\n", identifier, (uint32_t)word);
        } else {
            printf("%s\t%08X\tINVALID\n", identifier, (uint32_t)word);
        }
    }

    ArmitoDecoderShutdown();

    return ferror(stdin) || ferror(stdout) ? 1 : 0;
}