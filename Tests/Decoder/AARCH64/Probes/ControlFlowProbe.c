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
    int authenticated = argc == 2 && strcmp(argv[1], "--auth") == 0;

    if (argc > 2 || (argc == 2 && !authenticated))
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
        if (result != AARCH64_CATALOG_VALID_ENCODING || match.form == 0 ||
            strcmp(match.form->id, identifier) != 0 ||
            AARCH64OperandsExtractWithProfile(&match, result, (uint32_t)word,
                (uint64_t)address, &profile, &set) !=
                    AARCH64_OPERAND_EXTRACTION_COMPLETE) {
            printf("%s\t%08X\tREJECT\n", identifier, word);
        } else {
            printf("%s\t%08X\tCOMPLETE\t%u:%u:%u:%u:%u:%u:%u:%u:%llu:%llu",
                identifier, word, set.control_flow, set.branch_link,
                set.branch_fallthrough, set.branch_condition,
                set.branch_reads_nzcv, set.branch_reads_lr,
                set.branch_target_register, set.branch_compare_width,
                (unsigned long long)set.branch_target_address,
                (unsigned long long)set.branch_fallthrough_address);
            if (authenticated)
                printf("\tAUTH:%u:%u:%u:%u:%u:%u:%u:%u:%u:%u:%u:%u:%u:%u:%u:%u:%u:%u:%u:%u",
                    set.pointer_auth_key, set.pointer_auth_action,
                    set.pointer_auth_context, set.branch_modifier_register,
                    set.branch_reads_pc, set.branch_reads_sp,
                    set.branch_reads_pstate, set.branch_reads_elr,
                    set.branch_reads_spsr, set.branch_reads_debug_link,
                    set.branch_reads_debug_status, set.branch_writes_pstate,
                    set.branch_writes_elr, set.branch_writes_spsr,
                    set.exception_kind, set.exception_min_el,
                    set.exception_level_selector, set.exception_immediate,
                    set.exception_requires_el2, set.exception_requires_el3);
            for (index = 0; index < set.operand_count; index++) {
                const AARCH64Operand *operand = &set.operands[index];
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
        if (strcmp(disabled, "-") != 0 &&
            AARCH64CatalogGuestProfileSetFeature(&profile, disabled, 1u) !=
                AARCH64_CATALOG_PROFILE_CONFIGURED)
            return 1;
    }
    return ferror(stdin) || ferror(stdout) ? 1 : 0;
}
