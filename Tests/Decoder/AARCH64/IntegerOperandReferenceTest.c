#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "AARCH64/Catalog/AARCH64Catalog.h"
#include "AARCH64/Includes/AARCH64Operands.h"

static const AARCH64Operand *Find(const AARCH64OperandSet *set, const char *name)
{
    uint16_t index;

    for (index = 0; index < set->operand_count; index++)
        if (set->operands[index].name != 0 &&
            strcmp(set->operands[index].name, name) == 0)
            return &set->operands[index];
    return 0;
}

static void PrintRegister(const AARCH64Operand *operand)
{
    if (operand == 0) {
        fputs("-", stdout);
        return;
    }
    if (operand->register_special == AARCH64_REGISTER_SPECIAL_SP)
        fputs(operand->register_width == 32u ? "wsp" : "sp", stdout);
    else if (operand->register_special == AARCH64_REGISTER_SPECIAL_ZERO)
        fputs(operand->register_width == 32u ? "wzr" : "xzr", stdout);
    else
        printf("%c%u", operand->register_width == 32u ? 'w' : 'x',
            operand->register_index);
}

int main(void)
{
    static const uint32_t cases[] = {
        0x91000420u, 0x8B224820u, 0x9B220C20u,
        0x9AC24C20u, 0x9A820020u, 0xCB420C20u
    };
    uint32_t index;
    AARCH64CatalogGuestProfile profile;
    uint32_t feature;

    AARCH64CatalogGuestProfileInitialize(&profile, AARCH64_CATALOG_ABI_AARCH64);
    for (feature = 0; feature < AARCH64CatalogFeatureCount(); feature++)
        if (AARCH64CatalogGuestProfileSetFeature(&profile,
                AARCH64CatalogFeatureName(feature), 1u) !=
            AARCH64_CATALOG_PROFILE_CONFIGURED)
            return 1;

    for (index = 0; index < sizeof(cases) / sizeof(cases[0]); index++) {
        AARCH64CatalogMatch match;
        AARCH64OperandSet set;
        AARCH64CatalogResult result = AARCH64CatalogLookupWithProfile(
            cases[index], &profile, &match);
        const AARCH64Operand *modifier;

        if (result != AARCH64_CATALOG_VALID_ENCODING ||
            AARCH64OperandsExtractWithProfile(&match, result, cases[index],
                0u, &profile, &set) !=
                AARCH64_OPERAND_EXTRACTION_COMPLETE)
            return 1;
        modifier = Find(&set, "option");
        if (modifier == 0)
            modifier = Find(&set, "shift");
        printf("%08X\t", cases[index]);
        PrintRegister(Find(&set, "Rd"));
        putchar(',');
        PrintRegister(Find(&set, "Rn"));
        putchar(',');
        PrintRegister(Find(&set, "Rm"));
        printf("\t%s\n", modifier == 0 || modifier->modifier_name == 0
            ? "-" : modifier->modifier_name);
    }
    return 0;
}