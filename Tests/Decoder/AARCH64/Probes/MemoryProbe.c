#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "AARCH64/Catalog/AARCH64Catalog.h"
#include "AARCH64/Includes/AARCH64Operands.h"

int main(int argc, char **argv)
{
    AARCH64CatalogGuestProfile profile;
    uint32_t feature;
    char identifier[160];
    char disabled[80];
    unsigned word;
    unsigned long long address;
    int specialized = argc == 2 && strcmp(argv[1], "--specialized") == 0;
    int memory = specialized ||
        (argc == 2 && strcmp(argv[1], "--mem") == 0);

    if (argc > 2 || (argc == 2 && !memory))
        return 1;
    AARCH64CatalogGuestProfileInitialize(&profile, AARCH64_CATALOG_ABI_AARCH64);
    for (feature = 0; feature < AARCH64CatalogFeatureCount(); feature++) {
        if (AARCH64CatalogGuestProfileSetFeature(&profile,
                AARCH64CatalogFeatureName(feature), 1u) !=
            AARCH64_CATALOG_PROFILE_CONFIGURED)
            return 1;
    }
    while (scanf("%159s %x %llx %79s", identifier, &word, &address,
                 disabled) == 4) {
        AARCH64CatalogMatch match;
        AARCH64OperandSet set;
        AARCH64CatalogResult result;
        uint16_t index;

        if (strcmp(disabled, "-") != 0 &&
            AARCH64CatalogGuestProfileSetFeature(&profile, disabled, 0u) !=
                AARCH64_CATALOG_PROFILE_CONFIGURED)
            return 1;
        result = AARCH64CatalogLookupWithProfile((uint32_t)word, &profile,
                                                  &match);
        AARCH64OperandExtractionStatus extraction =
            AARCH64OperandsExtractWithProfile(&match, result, (uint32_t)word,
                (uint64_t)address, &profile, &set);
        if (result != AARCH64_CATALOG_VALID_ENCODING || match.form == 0 ||
            strcmp(match.form->id, identifier) != 0 ||
            extraction != AARCH64_OPERAND_EXTRACTION_COMPLETE) {
             printf("%s\t%08X\tREJECT\n", identifier, word);
        } else {
            printf("%s\t%08X\tCOMPLETE", identifier, word);
            if (memory) {
                printf("\tMEM:%u:%u:%u:%u:%u:%u:%u:%u:%u:%u:%u:%u:%u:%u:%llu",
                    set.memory_access_width, set.memory_element_count,
                    set.memory_direction, set.memory_writeback,
                    set.memory_base_register, set.memory_data_register,
                    set.memory_pair_register, set.memory_sign_extend,
                    set.memory_zero_extend, set.memory_literal,
                    set.memory_base_is_sp, set.memory_base_is_zero,
                    set.memory_data_is_sp, set.memory_data_is_zero,
                    (unsigned long long)set.memory_offset);
                printf("\tMX:%u:%u:%u:%u:%u:%u:%u:%u:%u:%u:%u:%u:%u:%u:%u:%u:%u:%u:%u",
                    set.memory_element_width, set.memory_vector_width,
                    set.memory_lane, set.memory_register_count,
                    set.memory_wraparound, set.memory_nontemporal,
                    set.memory_unprivileged, set.memory_acquire,
                    set.memory_release, set.memory_exclusive,
                    set.memory_atomic, set.memory_tagged, set.memory_phase,
                    set.memory_offset_register, set.memory_offset_extend,
                    set.memory_offset_scale,
                    set.memory_address_expression_resolved,
                    set.memory_static_overlap,
                    set.memory_execution_overlap);
                if (specialized) {
                    uint64_t metadata[] = {
                        set.memory_kind, set.memory_data_bank,
                        set.memory_register_list_start,
                        set.memory_structure_count,
                        set.memory_interleave_factor,
                        set.memory_replicate,
                        set.memory_merge_inactive, set.memory_lane_valid,
                        set.memory_predicate_register,
                        set.memory_depends_vl, set.memory_depends_svl,
                        set.memory_mops_stage,
                        set.memory_mops_source_register,
                        set.memory_mops_destination_register,
                        set.memory_mops_length_register,
                        set.memory_mops_reads_carry,
                        set.memory_mops_writes_nzcv,
                        set.memory_state_dependent_size,
                        set.memory_min_el,
                        set.memory_tag_granule_bytes,
                        set.memory_tag_block_symbolic,
                        set.memory_tag_checked,
                        set.memory_offset_kind,
                        set.memory_list_stride,
                        set.memory_mops_options,
                        set.memory_tag_zero_data,
                        set.memory_atomic_read_result,
                        set.memory_writeback_register_count,
                        set.memory_total_bytes,
                        set.memory_postindex_immediate,
                        set.memory_base_address,
                        set.memory_mops_destination_discarded,
                        set.memory_mops_source_zero
                    };
                    unsigned metadata_index;
                    printf("\tSM");
                    for (metadata_index = 0;
                         metadata_index < sizeof(metadata) / sizeof(metadata[0]);
                         metadata_index++)
                        printf(":%llu",
                            (unsigned long long)metadata[metadata_index]);
                    printf("\tALIAS:%s",
                        set.preferred_alias != 0
                        ? set.preferred_alias->mnemonic : "-");
                }
            }
            for (index = 0; index < set.operand_count; index++) {
                const AARCH64Operand *operand = &set.operands[index];
                printf("\t%s:%u:%u:%u:%llu:%llu:%lld:%llu:%u:%u:%u:%u",
                    operand->name, operand->kind, operand->role, operand->status,
                    (unsigned long long)operand->value,
                    (unsigned long long)operand->raw_value,
                    (long long)operand->signed_value,
                    (unsigned long long)operand->address, operand->scale,
                    operand->flags, operand->register_width,
                    operand->register_special);
            }
            putchar('\n');
        }
        if (strcmp(disabled, "-") != 0 &&
            AARCH64CatalogGuestProfileSetFeature(&profile, disabled, 1u) !=
                AARCH64_CATALOG_PROFILE_CONFIGURED)
            return 1;
    }
    return ferror(stdin) || ferror(stdout) ? 1 : 0;
}
