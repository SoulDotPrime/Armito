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
    int memory = argc == 2 && strcmp(argv[1], "--mem") == 0;

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
                printf("\tMX:%u:%u:%u:%u:%u:%u:%u:%u:%u:%u:%u:%u:%u",
                    set.memory_element_width, set.memory_vector_width,
                    set.memory_lane, set.memory_register_count,
                    set.memory_wraparound, set.memory_nontemporal,
                    set.memory_unprivileged, set.memory_acquire,
                    set.memory_release, set.memory_exclusive,
                    set.memory_atomic, set.memory_tagged, set.memory_phase);
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