#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "AARCH64/Catalog/AARCH64Catalog.h"

typedef struct {
    uint32_t encoding;
    const char *mnemonic;
} AARCH64ReferenceCase;

static const AARCH64ReferenceCase reference_cases[] = {
    {0xD2800000u, "movz"},
    {0x8B020020u, "add"},
    {0xCB020020u, "sub"},
    {0xD65F03C0u, "ret"},
    {0xD61F0000u, "br"},
    {0xD63F0000u, "blr"},
    {0x14000000u, "b"},
    {0x94000000u, "bl"},
    {0xB4000040u, "cbz"},
    {0xB5000040u, "cbnz"},
    {0x92800020u, "movn"},
    {0x91003020u, "add"},
    {0xCB050083u, "sub"},
    {0x8A020020u, "and"},
    {0xAA050083u, "orr"},
    {0xCA0800E6u, "eor"},
    {0xD37DF020u, "ubfm"},
    {0x10000080u, "adr"},
    {0x9A820020u, "csel"},
    {0x9B020C20u, "madd"},
    {0x9AC608A4u, "udiv"},
    {0x39400020u, "ldrb"},
    {0x39001062u, "strb"},
    {0xA94107E0u, "ldp"},
    {0xA9BF0FE2u, "stp"},
    {0x54000080u, "b"},
    {0x36180080u, "tbz"},
    {0xD4000001u, "svc"},
    {0xD503233Fu, "paciasp"},
    {0xD50323BFu, "autiasp"},
    {0xD65F0FFFu, "retab"},
    {0xD71F0801u, "braa"},
    {0xC8A07C41u, "cas"}
};

int main(void)
{
    uint32_t index;
    uint32_t failures = 0;
    uint32_t feature_index;
    uint32_t count = (uint32_t)(
        sizeof(reference_cases) / sizeof(reference_cases[0])
    );
    AARCH64CatalogGuestProfile profile;

    AARCH64CatalogGuestProfileInitialize(
        &profile, AARCH64_CATALOG_ABI_AARCH64
    );
    for (feature_index = 0;
         feature_index < AARCH64CatalogFeatureCount();
         feature_index++) {
        const char *name = AARCH64CatalogFeatureName(feature_index);

        if (name == 0 ||
            AARCH64CatalogGuestProfileSetFeature(&profile, name, 1) !=
                AARCH64_CATALOG_PROFILE_CONFIGURED)
            failures++;
    }
    if (AARCH64CatalogSetGuestProfile(&profile) !=
        AARCH64_CATALOG_PROFILE_CONFIGURED)
        failures++;

    for (index = 0; index < count; index++) {
        const AARCH64ReferenceCase *reference = &reference_cases[index];
        AARCH64CatalogMatch match;
        AARCH64CatalogResult result = AARCH64CatalogLookup(
            reference->encoding, &match
        );
        uint32_t form_index;
        uint8_t matched = 0;

        if ((result == AARCH64_CATALOG_VALID_ENCODING ||
             result == AARCH64_CATALOG_PATTERN_MATCHED_UNVALIDATED) &&
            match.pattern != 0) {
            for (form_index = 0;
                 form_index < match.pattern->form_count;
                 form_index++) {
                const char *mnemonic =
                    match.pattern->forms[form_index].mnemonic;
                uint32_t character;

                if (mnemonic == 0)
                    continue;

                for (character = 0; mnemonic[character] != '\0'; character++) {
                    if ((char)tolower((unsigned char)mnemonic[character]) !=
                        reference->mnemonic[character])
                        break;
                }

                if (mnemonic[character] == '\0' &&
                    reference->mnemonic[character] == '\0') {
                    matched = 1;
                    break;
                }
            }
        }

        if (!matched) {
            failures++;
            printf(
                "FAIL catalog 0x%08X expected %s\n",
                reference->encoding,
                reference->mnemonic
            );
        }

        printf("%08X\t%s\t", reference->encoding, reference->mnemonic);
        if (match.pattern != 0) {
            for (form_index = 0;
                 form_index < match.pattern->form_count;
                 form_index++) {
                const char *mnemonic =
                    match.pattern->forms[form_index].mnemonic;
                if (form_index != 0)
                    putchar(',');
                if (mnemonic != 0)
                    fputs(mnemonic, stdout);
            }
        }
        putchar('\n');
    }

    fprintf(
        stderr,
        "AARCHMRS reference cases: %u, %u failures\n",
        count,
        failures
    );
    AARCH64CatalogClearGuestProfile();
    return failures == 0 ? 0 : 1;
}
