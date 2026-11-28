#include <stdio.h>
#include <string.h>

#include "AARCH64/Catalog/AARCH64Catalog.h"
#include "AARCH64/Includes/AARCH64Operands.h"

typedef struct {
    uint32_t encoding;
    uint32_t form_count;
} CatalogSample;

static const CatalogSample catalog_samples[] = {
#include "Fixtures/AARCH64/AARCH64CatalogPatterns.inc"
};

static uint32_t descriptor_covered_forms;
static uint32_t descriptor_total;
static uint32_t extractable_pattern_samples;
static uint32_t extracted_sample_forms;
static uint32_t extracted_sample_operations;
static uint32_t catalog_operation_count;

static uint32_t form_status_counts[5];
static uint32_t form_unreached;
static uint32_t failed_sample_results[10];

static const AARCH64Operand *FindOperand(
    const AARCH64OperandSet *set,
    const char *name
)
{
    uint16_t index;

    for (index = 0; index < set->operand_count; index++) {
        const AARCH64Operand *operand = &set->operands[index];
        if (operand->name != 0 && strcmp(operand->name, name) == 0)
            return operand;
    }
    return 0;
}

static uint8_t Extract(
    uint32_t encoding,
    uint64_t address,
    AARCH64OperandSet *set
)
{
    AARCH64CatalogMatch match;
    AARCH64CatalogResult result = AARCH64CatalogLookup(encoding, &match);

    if (result != AARCH64_CATALOG_VALID_ENCODING &&
        result != AARCH64_CATALOG_PATTERN_MATCHED_UNVALIDATED &&
        result != AARCH64_CATALOG_EXTENSION_UNAVAILABLE)
        return 0;
    return AARCH64OperandsExtract(
        &match, result, encoding, address, set
    ) != AARCH64_OPERAND_EXTRACTION_INVALID;
}

static uint8_t TestAddImmediateAndSpecialRegisters(void)
{
    AARCH64OperandSet set;
    const AARCH64Operand *rd;
    const AARCH64Operand *rn;
    const AARCH64Operand *immediate;

    if (!Extract(0x910003E0u, 0x1000u, &set)) {
        printf("ADD immediate extraction failed\n");
        return 0;
    }
    rn = FindOperand(&set, "Rn");
    immediate = FindOperand(&set, "scaled_immediate");
    if (rn == 0 || immediate == 0 ||
        rn->register_index != 31u ||
        rn->register_special != AARCH64_REGISTER_SPECIAL_SP ||
        immediate->value != 0u) {
        printf(
            "ADD immediate operands: Rn=%p imm=%p status=%u count=%u\n",
            (const void *)rn,
            (const void *)immediate,
            set.status,
            set.operand_count
        );
        return 0;
    }
    if (!Extract(0x110003E0u, 0x1000u, &set))
        return 0;
    rn = FindOperand(&set, "Rn");
    if (rn == 0 || rn->register_width != 32u ||
        rn->register_special != AARCH64_REGISTER_SPECIAL_SP)
        return 0;

    if (!Extract(0x910003FFu, 0x1000u, &set)) {
        printf("ADD SP destination extraction failed\n");
        return 0;
    }
    rd = FindOperand(&set, "Rd");
    rn = FindOperand(&set, "Rn");
    if (rd == 0 || rn == 0 ||
        rd->register_special != AARCH64_REGISTER_SPECIAL_SP ||
        rn->register_special != AARCH64_REGISTER_SPECIAL_SP) {
        printf(
            "ADD SP special registers: Rd=%u Rn=%u\n",
            rd == 0 ? 99u : rd->register_special,
            rn == 0 ? 99u : rn->register_special
        );
        return 0;
    }

    if (!Extract(0xB10003FFu, 0x1000u, &set)) {
        printf("ADDS zero destination extraction failed\n");
        return 0;
    }
    rd = FindOperand(&set, "Rd");
    rn = FindOperand(&set, "Rn");
    if (!(rd != 0 && rn != 0 &&
        rd->register_special == AARCH64_REGISTER_SPECIAL_ZERO &&
        rn->register_special == AARCH64_REGISTER_SPECIAL_SP)) {
        printf(
            "ADDS special registers: Rd=%u Rn=%u\n",
            rd == 0 ? 99u : rd->register_special,
            rn == 0 ? 99u : rn->register_special
        );
        return 0;
    }
    return 1;
}

static uint8_t TestLogicalImmediate(void)
{
    AARCH64OperandSet set;
    const AARCH64Operand *immediate;

    if (!Extract(0xB2401FE0u, 0x2000u, &set))
        return 0;
    immediate = FindOperand(&set, "logical_immediate");
    return immediate != 0 &&
        immediate->status == AARCH64_OPERAND_VALUE_INTERPRETED &&
        immediate->value == 0xFFu &&
        (immediate->flags & AARCH64_OPERAND_FLAG_BITMASK) != 0u;
}

static uint8_t TestPcRelativeBranch(void)
{
    AARCH64OperandSet set;
    const AARCH64Operand *immediate;
    AARCH64CatalogMatch match;
    AARCH64CatalogResult catalog_status;

    if (!Extract(0x14000002u, 0x4000u, &set))
        return 0;
    immediate = FindOperand(&set, "imm26");
    if (immediate == 0 ||
        immediate->signed_value != 8 ||
        immediate->address != 0x4008u ||
        (immediate->flags & AARCH64_OPERAND_FLAG_PC_RELATIVE) == 0u) {
        return 0;
    }

    catalog_status = AARCH64CatalogLookup(0x14000002u, &match);
    if (catalog_status != AARCH64_CATALOG_VALID_ENCODING &&
        catalog_status != AARCH64_CATALOG_PATTERN_MATCHED_UNVALIDATED)
        return 0;
    return AARCH64OperandsExtract(
        &match,
        catalog_status,
        0x1C000002u,
        0x4000u,
        &set
    ) == AARCH64_OPERAND_EXTRACTION_INVALID;
}

static uint8_t TestPcRelativeAddress(void)
{
    AARCH64OperandSet set;
    const AARCH64Operand *address;

    if (!Extract(0x90000020u, 0x1FFCu, &set))
        return 0;
    address = FindOperand(&set, "pc_relative_address");
    return address != 0 &&
        address->address == 0x5000u &&
        address->signed_value == 0x4000 &&
        (address->flags & AARCH64_OPERAND_FLAG_PC_RELATIVE) != 0u;
}

static uint8_t TestMoveWideImmediate(void)
{
    AARCH64OperandSet set;
    const AARCH64Operand *immediate;

    if (!Extract(0xD2A24680u, 0x3000u, &set))
        return 0;
    immediate = FindOperand(&set, "move_wide_immediate");
    return immediate != 0 &&
        immediate->value == 0x12340000u &&
        immediate->scale == 16u;
}

static uint8_t TestMemoryAndSystemOperands(void)
{
    AARCH64OperandSet set;
    const AARCH64Operand *offset;
    const AARCH64Operand *system_register;

    if (!Extract(0xF9400420u, 0x6000u, &set))
        return 0;
    offset = FindOperand(&set, "scaled_memory_offset");
    if (offset == 0 || offset->value != 8u) {
        printf("LDR offset failed: %p value=%llu\n", (const void *)offset,
            (unsigned long long)(offset == 0 ? 0u : offset->value));
        return 0;
    }

    if (!Extract(0xA94107E0u, 0x6000u, &set))
        return 0;
    offset = FindOperand(&set, "pair_memory_offset");
    if (offset == 0 || offset->signed_value != 16) {
        printf("LDP offset failed: %p value=%lld\n", (const void *)offset,
            (long long)(offset == 0 ? 0 : offset->signed_value));
        return 0;
    }

    if (!Extract(0xA9BF0FE2u, 0x6000u, &set))
        return 0;
    offset = FindOperand(&set, "pair_memory_offset");
    if (offset == 0 || offset->signed_value != -16) {
        printf("STP offset failed: %p value=%lld\n", (const void *)offset,
            (long long)(offset == 0 ? 0 : offset->signed_value));
        return 0;
    }

    if (!Extract(0xD53B4200u, 0x6000u, &set))
        return 0;
    system_register = FindOperand(&set, "system_register_encoding");
    if (system_register == 0 || system_register->value != 0xDA10u) {
        uint16_t index;
        printf(
            "MRS system register failed: %p value=%llx form=%s\n",
            (const void *)system_register,
            (unsigned long long)(system_register == 0
                ? 0u : system_register->value),
            set.operation_id == 0 ? "(null)" : set.operation_id
        );
        for (index = 0; index < set.operand_count; index++) {
            const AARCH64Operand *operand = &set.operands[index];
            if (strcmp(operand->name, "op0") == 0 ||
                strcmp(operand->name, "op1") == 0 ||
                strcmp(operand->name, "CRn") == 0 ||
                strcmp(operand->name, "CRm") == 0 ||
                strcmp(operand->name, "op2") == 0)
                printf(
                    "%s raw=%llu width=%u\n",
                    operand->name,
                    (unsigned long long)operand->raw_value,
                    operand->field_width
                );
        }
        return 0;
    }
    return 1;
}

static uint8_t TestIndexedMemoryOffsets(void)
{
    AARCH64OperandSet set;
    const AARCH64Operand *offset;

    if (!Extract(0xF85F8C20u, 0x6000u, &set))
        return 0;
    offset = FindOperand(&set, "indexed_memory_offset");
    if (offset == 0 || offset->signed_value != -8 ||
        (offset->flags & AARCH64_OPERAND_FLAG_WRITEBACK) == 0u ||
        (offset->flags & AARCH64_OPERAND_FLAG_POST_INDEX) != 0u)
        return 0;
    if (!Extract(0xF8408420u, 0x6000u, &set))
        return 0;
    offset = FindOperand(&set, "indexed_memory_offset");
    if (offset == 0 || offset->signed_value != 8 ||
        (offset->flags & AARCH64_OPERAND_FLAG_POST_INDEX) == 0u)
        return 0;
    if (!Extract(0xA9BF0FE2u, 0x6000u, &set))
        return 0;
    offset = FindOperand(&set, "pair_memory_offset");
    return offset != 0 && offset->signed_value == -16 &&
        (offset->flags & AARCH64_OPERAND_FLAG_WRITEBACK) != 0u &&
        (offset->flags & AARCH64_OPERAND_FLAG_POST_INDEX) == 0u;
}

static uint8_t TestIntegerWidthsAndModifiers(void)
{
    AARCH64OperandSet set;
    const AARCH64Operand *rd;
    const AARCH64Operand *rn;
    const AARCH64Operand *rm;
    const AARCH64Operand *option;
    const AARCH64Operand *shift;

    if (!Extract(0x8B224820u, 0u, &set) ||
        set.status != AARCH64_OPERAND_EXTRACTION_COMPLETE)
        return 0;
    rd = FindOperand(&set, "Rd");
    rn = FindOperand(&set, "Rn");
    rm = FindOperand(&set, "Rm");
    option = FindOperand(&set, "option");
    if (rd == 0 || rn == 0 || rm == 0 || option == 0 ||
        rd->register_width != 64u || rn->register_width != 64u ||
        rm->register_width != 32u ||
        option->modifier_name == 0 ||
        strcmp(option->modifier_name, "UXTW") != 0)
        return 0;
    if (!Extract(0x9B220C20u, 0u, &set))
        return 0;
    rn = FindOperand(&set, "Rn");
    rm = FindOperand(&set, "Rm");
    rd = FindOperand(&set, "Rd");
    if (rn == 0 || rm == 0 || rd == 0 ||
        rn->register_width != 32u || rm->register_width != 32u ||
        rd->register_width != 64u)
        return 0;
    if (!Extract(0x9AC24C20u, 0u, &set))
        return 0;
    rm = FindOperand(&set, "Rm");
    rd = FindOperand(&set, "Rd");
    if (rm == 0 || rd == 0 || rm->register_width != 64u ||
        rd->register_width != 32u)
        return 0;
    if (!Extract(0xCB420C20u, 0u, &set))
        return 0;
    shift = FindOperand(&set, "shift");
    return shift != 0 && shift->modifier_name != 0 &&
        strcmp(shift->modifier_name, "LSR") == 0;
}

static uint8_t TestIntegerBoundaryFields(void)
{
    AARCH64OperandSet set;
    const AARCH64Operand *operand;
    AARCH64CatalogMatch match;
    AARCH64CatalogResult result;

    if (!Extract(0xB10003FFu, 0u, &set) ||
        set.status != AARCH64_OPERAND_EXTRACTION_COMPLETE)
        return 0;
    operand = FindOperand(&set, "Rd");
    if (operand == 0 || operand->register_width != 64u ||
        operand->register_special != AARCH64_REGISTER_SPECIAL_ZERO)
        return 0;
    if (!Extract(0x910003FFu, 0u, &set) ||
        set.status != AARCH64_OPERAND_EXTRACTION_COMPLETE)
        return 0;
    operand = FindOperand(&set, "Rd");
    if (operand == 0 || operand->register_special != AARCH64_REGISTER_SPECIAL_SP)
        return 0;
    if (!Extract(0xB2401FE0u, 0u, &set))
        return 0;
    operand = FindOperand(&set, "logical_immediate");
    if (operand == 0 || operand->value != 0xFFu ||
        (operand->flags & AARCH64_OPERAND_FLAG_BITMASK) == 0u)
        return 0;
    if (!Extract(0xD2A24680u, 0u, &set))
        return 0;
    operand = FindOperand(&set, "hw");
    if (operand == 0 || operand->value != 16u ||
        operand->modifier_name == 0 ||
        strcmp(operand->modifier_name, "LSL") != 0)
        return 0;
    result = AARCH64CatalogLookup(0xB2401FE0u, &match);
    if (result != AARCH64_CATALOG_VALID_ENCODING)
        return 0;
    return AARCH64OperandsExtract(&match, result, 0x92001FE0u,
        0u, &set) == AARCH64_OPERAND_EXTRACTION_INVALID;
}

static uint8_t TestBitfieldMasks(void)
{
    AARCH64OperandSet set;
    const AARCH64Operand *write_mask;
    const AARCH64Operand *top_mask;

    if (!Extract(0xD3432820u, 0u, &set))
        return 0;
    write_mask = FindOperand(&set, "bitfield_write_mask");
    top_mask = FindOperand(&set, "bitfield_top_mask");
    if (write_mask == 0 || top_mask == 0 ||
        write_mask->status != AARCH64_OPERAND_VALUE_INTERPRETED ||
        top_mask->status != AARCH64_OPERAND_VALUE_INTERPRETED ||
        write_mask->value != 0xE0000000000000FFull ||
        top_mask->value != 0xFFu ||
        write_mask->field_width != 64u ||
        top_mask->field_width != 64u)
        return 0;
    if (!Extract(0xD3430820u, 0u, &set))
        return 0;
    write_mask = FindOperand(&set, "bitfield_write_mask");
    top_mask = FindOperand(&set, "bitfield_top_mask");
    return write_mask != 0 && top_mask != 0 &&
        write_mask->field_width == 64u && top_mask->field_width == 64u &&
        write_mask->value == 0xE000000000000000ull &&
        top_mask->value == UINT64_MAX;
}

static uint8_t TestVectorOperandPartialStatus(void)
{
    AARCH64OperandSet set;
    uint16_t index;

    if (!Extract(0x05C00E00u, 0x7000u, &set) ||
        set.status != AARCH64_OPERAND_EXTRACTION_PARTIAL)
        return 0;
    for (index = 0; index < set.operand_count; index++) {
        const AARCH64Operand *operand = &set.operands[index];
        if (operand->kind == AARCH64_CATALOG_OPERAND_SVE_VECTOR)
            return operand->status == AARCH64_OPERAND_VALUE_PARTIAL;
    }
    return 0;
}

static uint8_t TestPointerAuthMetadata(void)
{
    AARCH64OperandSet set;

    if (!Extract(0xD503233Fu, 0x4000u, &set) ||
        set.pointer_auth_key != AARCH64_POINTER_AUTH_KEY_IA ||
        set.pointer_auth_action != AARCH64_POINTER_AUTH_ACTION_SIGN ||
        set.pointer_auth_context != AARCH64_POINTER_AUTH_CONTEXT_SP) {
        printf(
            "PACIASP metadata: key=%u action=%u context=%u op=%s\n",
            set.pointer_auth_key,
            set.pointer_auth_action,
            set.pointer_auth_context,
            set.operation_id == 0 ? "(null)" : set.operation_id
        );
        return 0;
    }

    if (!Extract(0xDAC10020u, 0x4000u, &set) ||
        set.pointer_auth_key != AARCH64_POINTER_AUTH_KEY_IA ||
        set.pointer_auth_action != AARCH64_POINTER_AUTH_ACTION_SIGN ||
        set.pointer_auth_context != AARCH64_POINTER_AUTH_CONTEXT_REGISTER) {
        printf(
            "PACIA metadata: key=%u action=%u context=%u op=%s\n",
            set.pointer_auth_key,
            set.pointer_auth_action,
            set.pointer_auth_context,
            set.operation_id == 0 ? "(null)" : set.operation_id
        );
        return 0;
    }

    if (!Extract(0xD65F0BFFu, 0x4000u, &set) ||
        set.pointer_auth_key != AARCH64_POINTER_AUTH_KEY_IA ||
        set.pointer_auth_action != AARCH64_POINTER_AUTH_ACTION_RETURN ||
        set.pointer_auth_context != AARCH64_POINTER_AUTH_CONTEXT_SP) {
        printf(
            "RETAA metadata: key=%u action=%u context=%u op=%s mnemonic=%s\n",
            set.pointer_auth_key,
            set.pointer_auth_action,
            set.pointer_auth_context,
            set.operation_id == 0 ? "(null)" : set.operation_id,
            set.canonical_mnemonic == 0 ? "(null)" :
                set.canonical_mnemonic
        );
        return 0;
    }
    return 1;
}

static uint8_t TestPreferredAliases(void)
{
    AARCH64OperandSet set;

    if (!Extract(0xD3432820u, 0x5000u, &set) ||
        set.preferred_alias == 0 ||
        strcmp(set.preferred_alias->name, "UBFX") != 0) {
        printf("UBFX alias failed: %s\n", set.preferred_alias == 0
            ? "(none)" : set.preferred_alias->name);
        return 0;
    }
    if (!Extract(0xD3430820u, 0x5000u, &set) ||
        set.preferred_alias == 0 ||
        strcmp(set.preferred_alias->name, "UBFIZ") != 0) {
        printf("UBFIZ alias failed: %s\n", set.preferred_alias == 0
            ? "(none)" : set.preferred_alias->name);
        return 0;
    }
    if (!Extract(0xB2001FE0u, 0x5000u, &set) ||
        set.preferred_alias == 0 ||
        strcmp(set.preferred_alias->name, "MOV") != 0) {
        printf("logical MOV alias failed: %s\n", set.preferred_alias == 0
            ? "(none)" : set.preferred_alias->name);
        return 0;
    }
    if (!Extract(0xB2401FE0u, 0x5000u, &set) ||
        set.preferred_alias != 0) {
        printf("move-wide preferred alias failed: %s\n",
            set.preferred_alias == 0 ? "(none)" :
                set.preferred_alias->name);
        return 0;
    }
    if (!Extract(0x05C00E00u, 0x5000u, &set) ||
        set.preferred_alias != 0) {
        printf("SVE DUPM alias failed: %s\n", set.preferred_alias == 0
            ? "(none)" : set.preferred_alias->name);
        return 0;
    }
    if (!Extract(0x05C201E0u, 0x5000u, &set) ||
        set.preferred_alias == 0 ||
        strcmp(set.preferred_alias->mnemonic, "MOV") != 0) {
        printf("SVE MOV alias failed: %s\n", set.preferred_alias == 0
            ? "(none)" : set.preferred_alias->name);
        return 0;
    }
    return AARCH64CatalogAliasCount() == 288u;
}

static uint8_t TestGeneratedDescriptorCoverage(void)
{
    uint32_t descriptor_count = 0;
    uint32_t forms_with_descriptors = 0;
    uint32_t pattern_samples_extracted = 0;
    uint32_t form_index;
    uint32_t sample_index;
    uint32_t form_count = AARCH64CatalogFormCount();
    uint32_t operation_count = 0;
    const char *operation_names[form_count];
    uint16_t form_operation_indices[form_count];
    uint8_t extracted_forms[form_count];
    uint8_t extracted_operations[form_count];

    if (!AARCH64CatalogInitialize())
        return 0;

    for (form_index = 0; form_index < form_count; form_index++) {
        const AARCH64CatalogForm *form =
            AARCH64CatalogGetForm(form_index);
        uint16_t operand_index;
        uint32_t operation_index;

        if (form == 0 ||
            form->operand_descriptor_count > AARCH64_OPERANDS_MAX_COUNT)
            return 0;
        if (form->operand_descriptor_count != 0u &&
            form->operand_descriptors == 0)
            return 0;
        if (form->operation_id == 0)
            return 0;
        if (form->operand_descriptor_count != 0u)
            forms_with_descriptors++;
        descriptor_count += form->operand_descriptor_count;
        for (operation_index = 0; operation_index < operation_count;
             operation_index++) {
            if (strcmp(operation_names[operation_index], form->operation_id) == 0)
                break;
        }
        if (operation_index == operation_count)
            operation_names[operation_count++] = form->operation_id;
        form_operation_indices[form_index] = (uint16_t)operation_index;
        extracted_forms[form_index] = 0u;

        for (operand_index = 0;
             operand_index < form->operand_descriptor_count;
             operand_index++) {
            const AARCH64CatalogOperandDescriptor *descriptor =
                &form->operand_descriptors[operand_index];
            if (descriptor->width == 0u ||
                descriptor->width > 32u ||
                descriptor->lsb + descriptor->width > 32u ||
                AARCH64CatalogSymbolName(descriptor->symbol_index) == 0)
                return 0;
        }
    }

    if (forms_with_descriptors != form_count ||
        descriptor_count != AARCH64CatalogOperandDescriptorCount())
        return 0;
    descriptor_covered_forms = forms_with_descriptors;
    descriptor_total = descriptor_count;
    catalog_operation_count = operation_count;
    for (operation_count = 0; operation_count < catalog_operation_count;
         operation_count++)
        extracted_operations[operation_count] = 0u;

    for (sample_index = 0;
         sample_index < sizeof(catalog_samples) / sizeof(catalog_samples[0]);
         sample_index++) {
        AARCH64CatalogMatch match;
        AARCH64OperandSet set;
        AARCH64CatalogResult result = AARCH64CatalogLookup(
            catalog_samples[sample_index].encoding, &match
        );

        if (result == AARCH64_CATALOG_VALID_ENCODING ||
            result == AARCH64_CATALOG_PATTERN_MATCHED_UNVALIDATED ||
            result == AARCH64_CATALOG_EXTENSION_UNAVAILABLE) {
            AARCH64OperandExtractionStatus extraction =
                AARCH64OperandsExtract(
                    &match,
                    result,
                    catalog_samples[sample_index].encoding,
                    0u,
                    &set
                );
            if (extraction == AARCH64_OPERAND_EXTRACTION_INVALID ||
                extraction == AARCH64_OPERAND_EXTRACTION_UNSUPPORTED)
                return 0;
            if (match.form_index >= form_count)
                return 0;
            extracted_forms[match.form_index] = 1u;
            pattern_samples_extracted++;
        } else {
            if ((unsigned)result < 10u)
                failed_sample_results[result]++;
            printf("Unextracted pattern sample: 0x%08X result=%u pattern=%u form=%u\n",
                catalog_samples[sample_index].encoding, (unsigned)result,
                (unsigned)match.pattern_index, (unsigned)match.form_index);
        }
    }

    for (form_index = 0; form_index < form_count; form_index++) {
        uint16_t operation_index = form_operation_indices[form_index];

        if (extracted_forms[form_index]) {
            extracted_sample_forms++;
            if (!extracted_operations[operation_index]) {
                extracted_operations[operation_index] = 1u;
                extracted_sample_operations++;
            }
        }
    }

    extractable_pattern_samples = pattern_samples_extracted;
    return pattern_samples_extracted != 0u;
}

static uint8_t TestPerFormSampleAudit(void)
{
    uint32_t pattern_index;
    uint32_t examined = 0;
    uint32_t integer_forms = 0;
    uint32_t integer_operations = 0;
    uint32_t integer_status[5] = {0};
    uint32_t integer_unreached = 0;
    const char *integer_operation_names[195];
    AARCH64CatalogGuestProfile profile;
    uint32_t feature_index;

    AARCH64CatalogGuestProfileInitialize(&profile, AARCH64_CATALOG_ABI_AARCH64);
    for (feature_index = 0; feature_index < AARCH64CatalogFeatureCount(); feature_index++) {
        if (AARCH64CatalogGuestProfileSetFeature(&profile,
                AARCH64CatalogFeatureName(feature_index), 1u) !=
            AARCH64_CATALOG_PROFILE_CONFIGURED)
            return 0;
    }
    for (pattern_index = 0; pattern_index < AARCH64CatalogPatternCount(); pattern_index++) {
        const AARCH64CatalogPattern *pattern = AARCH64CatalogGetPattern(pattern_index);
        uint32_t local_index;
        for (local_index = 0; local_index < pattern->form_count; local_index++) {
            const AARCH64CatalogForm *form = &pattern->forms[local_index];
            uint8_t integer = form->family == AARCH64_CATALOG_FAMILY_DP_IMMEDIATE ||
                form->family == AARCH64_CATALOG_FAMILY_DP_REGISTER;
            uint32_t attempt;
            uint32_t seed = pattern->value ^ (local_index * 0x9e3779b9u);
            uint8_t found = 0;
            examined++;
            if (integer) {
                uint32_t op;
                integer_forms++;
                for (op = 0; op < integer_operations; op++)
                    if (strcmp(integer_operation_names[op], form->operation_id) == 0)
                        break;
                if (op == integer_operations)
                    integer_operation_names[integer_operations++] = form->operation_id;
            }
            for (attempt = 0; attempt < 96u; attempt++) {
                AARCH64CatalogMatch match;
                AARCH64OperandSet set;
                AARCH64CatalogResult result;
                AARCH64OperandExtractionStatus extraction;
                uint32_t candidate;
                uint16_t field_index;
                seed ^= seed << 13;
                seed ^= seed >> 17;
                seed ^= seed << 5;
                candidate = pattern->value | (seed & ~pattern->mask);
                if (attempt == 0u)
                    candidate = pattern->value;
                for (field_index = 0; field_index < form->encoding_field_count; field_index++) {
                    const AARCH64CatalogField *field = &form->encoding_fields[field_index];
                    if (field->width == 0u || field->width > 32u)
                        continue;
                    if (attempt == 1u)
                        candidate = (candidate & ~field->value_mask) | field->value;
                }
                result = AARCH64CatalogLookupWithProfile(candidate, &profile, &match);
                if (match.form != form || match.pattern != pattern ||
                    (result != AARCH64_CATALOG_VALID_ENCODING &&
                     result != AARCH64_CATALOG_PATTERN_MATCHED_UNVALIDATED &&
                     result != AARCH64_CATALOG_EXTENSION_UNAVAILABLE))
                    continue;
                extraction = AARCH64OperandsExtractWithProfile(&match, result,
                    candidate, 0u, &profile, &set);
                if ((unsigned)extraction < 5u)
                    form_status_counts[extraction]++;
                if (integer && (unsigned)extraction < 5u) {
                    integer_status[extraction]++;
                    if (extraction != AARCH64_OPERAND_EXTRACTION_COMPLETE) {
                        uint16_t oi;
                        printf("Integer gap: %s operation=%s status=%u word=%08X\n",
                            form->id, form->operation_id, (unsigned)extraction, candidate);
                        for (oi = 0; oi < set.operand_count; oi++) {
                            const AARCH64Operand *o = &set.operands[oi];
                            if (o->status != AARCH64_OPERAND_VALUE_INTERPRETED)
                                printf("  gap operand=%s kind=%u status=%u width=%u flags=%u\n",
                                    o->name, o->kind, o->status, o->register_width, o->flags);
                        }
                    }
                }
                found = 1;
                break;
            }
            if (!found) {
                form_unreached++;
                if (integer)
                    integer_unreached++;
                printf("Unreached form: %u pattern=%u id=%s operation=%s\n",
                    pattern->first_form + local_index, pattern_index,
                    form->id, form->operation_id);
            }
        }
    }
    printf("Per-form audit (96 deterministic candidates maximum): examined=%u complete=%u partial=%u raw=%u unsupported=%u invalid=%u unreached=%u\n",
        examined, form_status_counts[AARCH64_OPERAND_EXTRACTION_COMPLETE],
        form_status_counts[AARCH64_OPERAND_EXTRACTION_PARTIAL],
        form_status_counts[AARCH64_OPERAND_EXTRACTION_RAW],
        form_status_counts[AARCH64_OPERAND_EXTRACTION_UNSUPPORTED],
        form_status_counts[AARCH64_OPERAND_EXTRACTION_INVALID], form_unreached);
    printf("Failed pattern sample catalog results: no-match=%u reserved=%u invalid=%u unavailable=%u ambiguous=%u\n",
        failed_sample_results[AARCH64_CATALOG_NO_MATCH],
        failed_sample_results[AARCH64_CATALOG_RESERVED_PATTERN],
        failed_sample_results[AARCH64_CATALOG_INVALID_ENCODING],
        failed_sample_results[AARCH64_CATALOG_EXTENSION_UNAVAILABLE],
        failed_sample_results[AARCH64_CATALOG_AMBIGUOUS]);
    printf("Integer/Data Processing: forms=%u operations=%u complete=%u partial=%u raw=%u unsupported=%u invalid=%u unreached=%u\n",
        integer_forms, integer_operations, integer_status[1], integer_status[2],
        integer_status[3], integer_status[4], integer_status[0], integer_unreached);
    return examined == AARCH64CatalogFormCount() &&
        integer_forms == 195u && integer_operations == 112u &&
        integer_forms == integer_unreached + integer_status[0] +
            integer_status[1] + integer_status[2] + integer_status[3] +
            integer_status[4] &&
        examined == form_unreached + form_status_counts[0] +
            form_status_counts[1] + form_status_counts[2] +
            form_status_counts[3] + form_status_counts[4];
}

static uint8_t TestIntegerRegisterCauses(void)
{
    static const struct {
        uint32_t word;
        const char *name;
        uint8_t width;
        uint8_t special;
        uint16_t flags;
    } cases[] = {
        {0x1200001Fu, "Rd", 32u, AARCH64_REGISTER_SPECIAL_SP, 0u},
        {0x9200001Fu, "Rd", 64u, AARCH64_REGISTER_SPECIAL_SP, 0u},
        {0x7200001Fu, "Rd", 32u, AARCH64_REGISTER_SPECIAL_ZERO, 0u},
        {0x9AC0101Fu, "Rd", 64u, AARCH64_REGISTER_SPECIAL_SP, 0u},
        {0x9AC013E0u, "Rn", 64u, AARCH64_REGISTER_SPECIAL_SP, 0u},
        {0xDAC103E0u, "Rn", 64u, AARCH64_REGISTER_SPECIAL_SP, 0u},
        {0x9A00201Fu, "Rd", 64u, AARCH64_REGISTER_SPECIAL_SP, 0u},
        {0x9B407C00u, "Ra", 64u, AARCH64_REGISTER_SPECIAL_ZERO,
            AARCH64_OPERAND_FLAG_IMPLICIT_REGISTER},
        {0xDAC143E0u, "Rn", 64u, AARCH64_REGISTER_SPECIAL_ZERO,
            AARCH64_OPERAND_FLAG_IMPLICIT_REGISTER},
        {0xDAC18BFEu, "Rd", 64u, AARCH64_REGISTER_SPECIAL_NONE,
            AARCH64_OPERAND_FLAG_IMPLICIT_REGISTER},
        {0x8B202000u, "Rm", 32u, AARCH64_REGISTER_SPECIAL_NONE, 0u},
        {0x8B206000u, "Rm", 64u, AARCH64_REGISTER_SPECIAL_NONE, 0u}
    };
    AARCH64CatalogGuestProfile profile;
    uint32_t index;
    uint32_t feature;

    AARCH64CatalogGuestProfileInitialize(&profile, AARCH64_CATALOG_ABI_AARCH64);
    for (feature = 0; feature < AARCH64CatalogFeatureCount(); feature++)
        if (AARCH64CatalogGuestProfileSetFeature(&profile,
                AARCH64CatalogFeatureName(feature), 1u) !=
            AARCH64_CATALOG_PROFILE_CONFIGURED)
            return 0;
    for (index = 0; index < sizeof(cases) / sizeof(cases[0]); index++) {
        AARCH64CatalogMatch match;
        AARCH64OperandSet set;
        const AARCH64Operand *operand;
        AARCH64CatalogResult result = AARCH64CatalogLookupWithProfile(
            cases[index].word, &profile, &match);

        if (result != AARCH64_CATALOG_VALID_ENCODING ||
            AARCH64OperandsExtractWithProfile(&match, result, cases[index].word,
                0u, &profile, &set) != AARCH64_OPERAND_EXTRACTION_COMPLETE)
            return 0;
        operand = FindOperand(&set, cases[index].name);
        if (operand == 0 || operand->register_width != cases[index].width ||
            operand->register_special != cases[index].special ||
            (operand->flags & AARCH64_OPERAND_FLAG_IMPLICIT_REGISTER) !=
                cases[index].flags)
            return 0;
    }
    return 1;
}

int main(void)
{
    uint8_t add_ok = TestAddImmediateAndSpecialRegisters();
    uint8_t logical_ok = TestLogicalImmediate();
    uint8_t branch_ok = TestPcRelativeBranch();
    uint8_t address_ok = TestPcRelativeAddress();
    uint8_t move_wide_ok = TestMoveWideImmediate();
    uint8_t memory_system_ok = TestMemoryAndSystemOperands();
    uint8_t indexed_memory_ok = TestIndexedMemoryOffsets();
    uint8_t integer_widths_ok = TestIntegerWidthsAndModifiers();
    uint8_t integer_boundaries_ok = TestIntegerBoundaryFields();
    uint8_t bitfield_masks_ok = TestBitfieldMasks();
    uint8_t vector_partial_ok = TestVectorOperandPartialStatus();
    uint8_t pointer_auth_ok = TestPointerAuthMetadata();
    uint8_t aliases_ok = TestPreferredAliases();
    uint8_t coverage_ok = TestGeneratedDescriptorCoverage();
    uint8_t form_audit_ok = TestPerFormSampleAudit();
    uint8_t register_causes_ok = TestIntegerRegisterCauses();
    uint32_t failures =
        !add_ok + !logical_ok + !branch_ok + !address_ok + !move_wide_ok +
        !memory_system_ok + !indexed_memory_ok + !integer_widths_ok +
        !integer_boundaries_ok + !bitfield_masks_ok +
        !vector_partial_ok + !pointer_auth_ok +
        !aliases_ok + !coverage_ok + !form_audit_ok + !register_causes_ok;

    printf(
        "AArch64 operand extraction focused cases: add=%u logical=%u "
        "branch=%u pc-address=%u move-wide=%u memory-system=%u "
        "vector-partial=%u "
        "PAC=%u aliases=%u catalog=%u "
        "(%u failures)\n",
        add_ok,
        logical_ok,
        branch_ok,
        address_ok,
        move_wide_ok,
        memory_system_ok,
        vector_partial_ok,
        pointer_auth_ok,
        aliases_ok,
        coverage_ok,
        failures
    );
    if (coverage_ok) {
        printf(
            "Operand coverage: descriptors %u/%u forms (%u), samples "
            "%u/%u patterns; extracted samples touch %u/%u forms and "
            "%u/%u operation IDs\n",
            descriptor_covered_forms,
            AARCH64CatalogFormCount(),
            descriptor_total,
            extractable_pattern_samples,
            (uint32_t)(sizeof(catalog_samples) / sizeof(catalog_samples[0])),
            extracted_sample_forms,
            AARCH64CatalogFormCount(),
            extracted_sample_operations,
            catalog_operation_count
        );
    }
    return failures == 0u ? 0 : 1;
}
