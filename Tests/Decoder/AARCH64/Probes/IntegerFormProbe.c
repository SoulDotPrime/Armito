#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "AARCH64/Catalog/AARCH64Catalog.h"
#include "AARCH64/Includes/AARCH64Operands.h"

int main(void)
{
    AARCH64CatalogGuestProfile profile;
    uint32_t feature;
    char requested_form[160];
    unsigned word;

    AARCH64CatalogGuestProfileInitialize(&profile, AARCH64_CATALOG_ABI_AARCH64);
    for (feature = 0; feature < AARCH64CatalogFeatureCount(); feature++) {
        if (AARCH64CatalogGuestProfileSetFeature(&profile,
                AARCH64CatalogFeatureName(feature), 1u) !=
            AARCH64_CATALOG_PROFILE_CONFIGURED)
            return 1;
    }
    while (scanf("%159s %x", requested_form, &word) == 2) {
        AARCH64CatalogMatch match;
        AARCH64OperandSet set;
        AARCH64CatalogResult result = AARCH64CatalogLookupWithProfile(
            (uint32_t)word, &profile, &match);
        uint16_t index;

        if (result != AARCH64_CATALOG_VALID_ENCODING ||
            match.form == 0 || strcmp(match.form->id, requested_form) != 0 ||
            AARCH64OperandsExtractWithProfile(&match, result, (uint32_t)word,
                0u, &profile, &set) != AARCH64_OPERAND_EXTRACTION_COMPLETE) {
            printf("%s\t%08X\tREJECT\n", requested_form, word);
            continue;
        }
        printf("%s\t%08X\t%s\t%s", requested_form, word,
            set.canonical_mnemonic,
            set.preferred_alias == 0 ? "-" : set.preferred_alias->mnemonic);
        for (index = 0; index < set.operand_count; index++) {
            const AARCH64Operand *operand = &set.operands[index];
            printf("\t%s:%u:%u:%llu:%u:%u:%u:%s:%llu:%lld:%u", operand->name,
                operand->kind, operand->role,
                (unsigned long long)operand->value,
                operand->register_width, operand->register_special,
                operand->flags,
                operand->modifier_name == 0 ? "-" : operand->modifier_name,
                (unsigned long long)operand->raw_value,
                (long long)operand->signed_value, operand->scale);
        }
        putchar('\n');
    }
    return ferror(stdout) ? 1 : 0;
}