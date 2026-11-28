#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "Decoder/Includes/Decoder.h"
#include "AARCH64/Includes/AARCH64.h"
#include "AARCH64/Catalog/AARCH64Catalog.h"

int main(int argc, char **argv)
{
    ArmitoDecoderConfig config;
    ArmitoDecoderInstruction instruction;
    ArmitoDecoderStatus status;
    ArmitoDecodeResult result;
    AARCH64OperandSet operands;
    char identifier[160];
    uint32_t word;
    uint64_t address;
    uint16_t index;
    char disabled[80];
    AARCH64CatalogGuestProfile enabled_profile;
    uint32_t feature;

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

    AARCH64CatalogGuestProfileInitialize(
        &enabled_profile, AARCH64_CATALOG_ABI_AARCH64);
    for (feature = 0; feature < AARCH64CatalogFeatureCount(); feature++)
        if (AARCH64CatalogGuestProfileSetFeature(&enabled_profile,
                AARCH64CatalogFeatureName(feature), 1u) !=
                AARCH64_CATALOG_PROFILE_CONFIGURED)
            return 1;

    while (scanf("%159s %" SCNx32 " %" SCNx64 " %79s",
                 identifier, &word, &address, disabled) == 4) {
        AARCH64CatalogGuestProfile profile = enabled_profile;
        AARCH64CatalogMatch match;
        AARCH64CatalogResult catalog_result;

        if (strcmp(disabled, "-") != 0)
            if (AARCH64CatalogGuestProfileSetFeature(&profile,
                    disabled, 0u) != AARCH64_CATALOG_PROFILE_CONFIGURED)
                return 1;
        catalog_result = AARCH64CatalogLookupWithProfile(word, &profile, &match);
        if (catalog_result == AARCH64_CATALOG_RESERVED_PATTERN) {
            printf("%s\t%08X\tRESERVED\n", identifier, word);
            continue;
        }
        if (strcmp(disabled, "-") != 0) {
            if (catalog_result == AARCH64_CATALOG_EXTENSION_UNAVAILABLE) {
                printf("%s\t%08X\tFEATURE_UNAVAILABLE\n", identifier, word);
                continue;
            }
            if (catalog_result == AARCH64_CATALOG_VALID_ENCODING &&
                match.form != NULL && strcmp(match.form->id, identifier) != 0) {
                printf("%s\t%08X\tFEATURE_FALLBACK:%s\n", identifier,
                       word, match.form->id);
                continue;
            }
            if (catalog_result != AARCH64_CATALOG_VALID_ENCODING ||
                match.form == NULL || strcmp(match.form->id, identifier) != 0 ||
                AARCH64OperandsExtractWithProfile(&match, catalog_result, word,
                    address, &profile, &operands) !=
                    AARCH64_OPERAND_EXTRACTION_COMPLETE) {
                printf("%s\t%08X\tPROFILE_REJECT_%u\n", identifier,
                       word, (unsigned)catalog_result);
                continue;
            }
        }
        if (catalog_result == AARCH64_CATALOG_PATTERN_MATCHED_UNVALIDATED &&
            match.form != NULL && strcmp(match.form->id, identifier) == 0) {
            printf("%s\t%08X\tUNVALIDATED\n", identifier, word);
            continue;
        }
        instruction.offset = 0;
        instruction.address = address;
        instruction.encoding = (uint32_t)word;

        memset(&operands, 0, sizeof(operands));
        status = ArmitoDecoderDecodeInstructionWithOperands(
            &instruction, &result, &operands
        );

        if (status != ARMITO_DECODER_SUCCESS ||
            result != ARMITO_DECODE_OK) {
            printf("%s\t%08X\tUNSUPPORTED\n", identifier, (uint32_t)word);
            continue;
        }

        if (operands.status != AARCH64_OPERAND_EXTRACTION_COMPLETE) {
            printf("%s\t%08X\tNO-OPERANDS\n", identifier, (uint32_t)word);
            continue;
        }

        if (operands.form == NULL ||
            strcmp(operands.form->id, identifier) != 0) {
            printf("%s\t%08X\tREJECT\n", identifier, word);
            continue;
        }
        printf("%s\t%08X\tOK\tCOMPLETE", identifier, (uint32_t)word);
        printf("\tSEC:%u:%u:%u:%u:%u:%u:%u:%u:%u:%u:%u:%u:%u:%u:%u:%u:%u:%u:%u:%u:%u:%" PRIu64 ":%u:%u",
            operands.pointer_auth_key,
            operands.pointer_auth_action,
            operands.pointer_auth_context,
            operands.pointer_auth_address_diversity,
            operands.memory_tagged,
            operands.memory_tag_granule_bytes,
            operands.memory_tag_block_symbolic,
            operands.memory_tag_checked,
            operands.memory_tag_zero_data,
            operands.exception_kind,
            operands.exception_min_el,
            operands.exception_level_selector,
            operands.exception_requires_el2,
            operands.exception_requires_el3,
            operands.exception_immediate,
            operands.control_flow,
            operands.memory_access_width,
            operands.memory_base_register,
            operands.memory_data_register,
            operands.memory_writeback,
            operands.pstate_pacm_write,
            operands.memory_offset,
            operands.memory_base_is_sp,
            operands.memory_data_is_zero);
        printf("\tALIAS:%s", operands.preferred_alias != NULL
            ? operands.preferred_alias->name : "-");
        for (index = 0; index < operands.operand_count; index++) {
            const AARCH64Operand *operand = &operands.operands[index];
            printf("\t%s:%u:%u:%llu:%llu:%lld:%llu:%u:%u:%u:%u:%u",
                operand->name, operand->kind, operand->role,
                (unsigned long long)operand->value,
                (unsigned long long)operand->raw_value,
                (long long)operand->signed_value,
                (unsigned long long)operand->address, operand->scale,
                operand->flags, operand->register_width,
                operand->register_special, operand->status);
        }
        putchar('\n');
    }

    ArmitoDecoderShutdown();

    return ferror(stdin) || ferror(stdout) ? 1 : 0;
}
