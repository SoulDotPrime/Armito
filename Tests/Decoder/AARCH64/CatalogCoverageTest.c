#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "AARCH64/Includes/AARCH64.h"
#include "AARCH64/Catalog/AARCH64Catalog.h"

typedef struct {
    uint32_t encoding;
    uint32_t form_count;
} AARCH64CatalogSample;

static const AARCH64CatalogSample catalog_samples[] = {
#include "Fixtures/AARCH64/AARCH64CatalogPatterns.inc"
};

static int TestCatalogProductionPath(void)
{
    AARCH64CatalogMatch match;
    AARCH64CatalogResult catalog_result;
    AARCH64Instruction instruction;
    AARCH64Opcode opcode;
    ArmitoDecoderStatus status;
    uint32_t index;
    uint8_t has_load_store_form = 0;

    catalog_result = AARCH64CatalogLookup(0xF9400020u, &match);
    if ((catalog_result != AARCH64_CATALOG_VALID_ENCODING &&
         catalog_result != AARCH64_CATALOG_PATTERN_MATCHED_UNVALIDATED) ||
        match.pattern == 0 || match.pattern->form_count == 0)
        return 0;

    for (index = 0; index < match.pattern->form_count; index++) {
        const AARCH64CatalogForm *form = &match.pattern->forms[index];
        if (form->decoder_module != 0 &&
            strcmp(form->decoder_module, "load_store_atomic") == 0 &&
            form->operation_id != 0 &&
            form->encoding_field_count != 0) {
            has_load_store_form = 1;
            break;
        }
    }

    if (!has_load_store_form)
        return 0;

    status = AARCH64Classify(0xF9400020u, &opcode);
    if (status != ARMITO_DECODER_UNSUPPORTED_INSTRUCTION ||
        opcode != AARCH64_OPCODE_CATALOG_ONLY)
        return 0;

    status = AARCH64Decode(0xF9400020u, &instruction);
    return status == ARMITO_DECODER_UNSUPPORTED_INSTRUCTION;
}

int main(void)
{
    uint32_t total = (uint32_t)(
        sizeof(catalog_samples) / sizeof(catalog_samples[0])
    );
    uint32_t recognized = 0;
    uint32_t recognized_forms = 0;
    uint32_t operand_decoded = 0;
    uint32_t unsupported = 0;
    uint32_t decode_unsupported = 0;
    uint32_t decode_invalid = 0;
    uint32_t invalid_samples = 0;
    uint32_t failures = 0;
    uint32_t i;
    AARCH64Opcode opcode;
    AARCH64Instruction instruction;

    for (i = 0; i < total; i++) {
        ArmitoDecoderStatus status = AARCH64Classify(
            catalog_samples[i].encoding, &opcode
        );

        if (status == ARMITO_DECODER_SUCCESS) {
            recognized++;
            recognized_forms += catalog_samples[i].form_count;
        } else if (status == ARMITO_DECODER_UNSUPPORTED_INSTRUCTION) {
            unsupported++;
        } else if (status == ARMITO_DECODER_INVALID_INSTRUCTION) {
            invalid_samples++;
        } else {
            failures++;
        }

        status = AARCH64Decode(catalog_samples[i].encoding, &instruction);
        if (status == ARMITO_DECODER_SUCCESS)
            operand_decoded++;
        else if (status == ARMITO_DECODER_UNSUPPORTED_INSTRUCTION)
            decode_unsupported++;
        else
            decode_invalid++;
    }

    if (AARCH64Classify(0xD503201Fu, &opcode) != ARMITO_DECODER_SUCCESS ||
        opcode != AARCH64_OPCODE_NOP)
        failures++;

    if (AARCH64Classify(0xFFFFFFFFu, &opcode) !=
        ARMITO_DECODER_UNSUPPORTED_INSTRUCTION)
        failures++;

    if (!TestCatalogProductionPath())
        failures++;

    printf(
        "AArch64 catalog representative patterns: %u total, "
        "%u classified (%u forms), %u unsupported, %u invalid/reserved; "
        "%u operand-decoded, %u decode-unsupported, %u decode-invalid\n",
        total,
        recognized,
        recognized_forms,
        unsupported,
        invalid_samples,
        operand_decoded,
        decode_unsupported,
        decode_invalid
    );

    return failures == 0 ? 0 : 1;
}
