#include <stddef.h>
#include <string.h>

#include "AARCH64Catalog.h"
#include "AARCH64CatalogData.inc"

typedef struct {
    const void *data;
    uint32_t bytes;
} AARCH64CatalogTableReference;

static const AARCH64CatalogTableReference AARCH64CatalogTableReferences[] = {
    { AARCH64CatalogStringPool, sizeof(AARCH64CatalogStringPool) },
    { AARCH64CatalogConditions, sizeof(AARCH64CatalogConditions) },
    { AARCH64CatalogConditionCode, sizeof(AARCH64CatalogConditionCode) },
    { AARCH64CatalogConditionValues, sizeof(AARCH64CatalogConditionValues) },
    { AARCH64CatalogConditionSetValues, sizeof(AARCH64CatalogConditionSetValues) },
    { AARCH64CatalogFields, sizeof(AARCH64CatalogFields) },
    { AARCH64CatalogOperandDescriptors, sizeof(AARCH64CatalogOperandDescriptors) },
    { AARCH64CatalogFeatureNames, sizeof(AARCH64CatalogFeatureNames) },
    { AARCH64CatalogSymbolNames, sizeof(AARCH64CatalogSymbolNames) },
    { AARCH64CatalogFeatureStrings, sizeof(AARCH64CatalogFeatureStrings) },
    { AARCH64CatalogGuards, sizeof(AARCH64CatalogGuards) },
    { AARCH64CatalogFormGroupIndices, sizeof(AARCH64CatalogFormGroupIndices) },
    { AARCH64CatalogGroups, sizeof(AARCH64CatalogGroups) },
    { AARCH64CatalogInstructionSets, sizeof(AARCH64CatalogInstructionSets) },
    { AARCH64CatalogForms, sizeof(AARCH64CatalogForms) },
    { AARCH64CatalogAliases, sizeof(AARCH64CatalogAliases) },
    { AARCH64CatalogPatterns, sizeof(AARCH64CatalogPatterns) },
    { AARCH64CatalogBucketOffsets, sizeof(AARCH64CatalogBucketOffsets) },
    { AARCH64CatalogBucketPatterns, sizeof(AARCH64CatalogBucketPatterns) }
};

#if AARCH64_CATALOG_GENERATED_MAX_CONDITION_STACK > \
    AARCH64_CATALOG_MAX_CONDITION_STACK
#error AARCH64_catalog_condition_stack_limit_exceeded
#endif

typedef enum {
    AARCH64_CATALOG_VALUE = 0,
    AARCH64_CATALOG_BOOLEAN
} AARCH64CatalogValueKind;

typedef enum {
    AARCH64_CATALOG_KNOWN = 0,
    AARCH64_CATALOG_UNKNOWN,
    AARCH64_CATALOG_FEATURE_FALSE
} AARCH64CatalogValueState;

typedef struct {
    uint64_t value;
    uint64_t mask;
    uint8_t width;
    uint8_t kind;
    uint8_t state;
} AARCH64CatalogEvalValue;

typedef enum {
    AARCH64_CATALOG_FORM_INVALID = 0,
    AARCH64_CATALOG_FORM_VALID,
    AARCH64_CATALOG_FORM_EXTENSION_UNAVAILABLE,
    AARCH64_CATALOG_FORM_UNVALIDATED
} AARCH64CatalogFormResult;

static AARCH64CatalogGuestProfile AARCH64CatalogActiveGuestProfile;
static uint8_t AARCH64CatalogHasGuestProfile;

static uint64_t AARCH64CatalogWidthMask(uint8_t width)
{
    if (width >= 64u)
        return UINT64_MAX;
    if (width == 0u)
        return 0u;
    return (1ull << width) - 1ull;
}

static AARCH64CatalogEvalValue AARCH64CatalogUnknownValue(void)
{
    AARCH64CatalogEvalValue value;

    value.value = 0;
    value.mask = 0;
    value.width = 0;
    value.kind = AARCH64_CATALOG_VALUE;
    value.state = AARCH64_CATALOG_UNKNOWN;
    return value;
}

static AARCH64CatalogEvalValue AARCH64CatalogBoolean(
    uint8_t result,
    AARCH64CatalogValueState state
)
{
    AARCH64CatalogEvalValue value;

    value.value = result ? 1u : 0u;
    value.mask = 1u;
    value.width = 1u;
    value.kind = AARCH64_CATALOG_BOOLEAN;
    value.state = (uint8_t)state;
    return value;
}

static uint8_t AARCH64CatalogFieldsMatch(
    const AARCH64CatalogField *fields,
    uint16_t field_count,
    uint32_t encoding
)
{
    uint16_t index;

    for (index = 0; index < field_count; index++) {
        const AARCH64CatalogField *field = &fields[index];

        if (((encoding >> field->lsb) & field->should_be_mask) !=
            field->should_be_value)
            return 0;
    }

    return 1;
}

static uint8_t AARCH64CatalogGroupNameEquals(const char *actual, const char *expected)
{
    if (actual == 0 || expected == 0)
        return 0;
    while (*actual != 0 && *actual == *expected) {
        actual++;
        expected++;
    }
    return *actual == *expected;
}

uint8_t AARCH64CatalogFormEncodingValid(
    const AARCH64CatalogForm *form,
    uint32_t encoding
)
{
    uint16_t index;
    const AARCH64CatalogInstructionSet *instruction_set;

    if (form == 0 ||
        form->instruction_set_index >= AARCH64_CATALOG_INSTRUCTION_SET_COUNT)
        return 0;

    if (form->family == AARCH64_CATALOG_FAMILY_DP_REGISTER ||
        form->family == AARCH64_CATALOG_FAMILY_DP_IMMEDIATE) {
        const char *group_name = 0;

        for (index = 0; index < form->group_count; index++) {
            uint16_t group_index = form->group_indices[index];

            if (group_index >= AARCH64_CATALOG_GROUP_COUNT)
                return 0;
            group_name = AARCH64CatalogGroups[group_index].name;
        }
        if (group_name != 0) {
            if (AARCH64CatalogGroupNameEquals(group_name, "addsub_ext") &&
                ((encoding >> 10) & 7u) > 4u)
                return 0;
            if ((AARCH64CatalogGroupNameEquals(group_name, "addsub_shift") ||
                 AARCH64CatalogGroupNameEquals(group_name, "log_shift")) &&
                (encoding & 0x80000000u) == 0u &&
                (encoding & 0x00008000u) != 0u)
                return 0;
            if (AARCH64CatalogGroupNameEquals(group_name, "addsub_shift") &&
                ((encoding >> 22) & 3u) == 3u)
                return 0;
            if (AARCH64CatalogGroupNameEquals(group_name, "bitfield") &&
                (encoding & 0x80000000u) == 0u &&
                (encoding & 0x00208000u) != 0u)
                return 0;
        }
    }

    instruction_set =
        &AARCH64CatalogInstructionSets[form->instruction_set_index];
    if (!AARCH64CatalogFieldsMatch(
            instruction_set->encoding_fields,
            instruction_set->encoding_field_count,
            encoding
        ))
        return 0;

    for (index = 0; index < form->group_count; index++) {
        uint16_t group_index = form->group_indices[index];
        const AARCH64CatalogGroup *group;

        if (group_index >= AARCH64_CATALOG_GROUP_COUNT)
            return 0;
        group = &AARCH64CatalogGroups[group_index];
        if (!AARCH64CatalogFieldsMatch(
                group->encoding_fields,
                group->encoding_field_count,
                encoding
            ))
            return 0;
    }

    return AARCH64CatalogFieldsMatch(
        form->encoding_fields,
        form->encoding_field_count,
        encoding
    );
}

static const AARCH64CatalogField *AARCH64CatalogFindField(
    const AARCH64CatalogField *fields,
    uint16_t field_count,
    uint16_t symbol_index
)
{
    uint16_t index;

    for (index = 0; index < field_count; index++) {
        if (fields[index].symbol_index == symbol_index)
            return &fields[index];
    }
    return 0;
}

static const AARCH64CatalogField *AARCH64CatalogResolveField(
    const AARCH64CatalogForm *form,
    const AARCH64CatalogGuard *guard,
    uint16_t symbol_index
)
{
    const AARCH64CatalogInstructionSet *instruction_set;
    int32_t group_position;
    const AARCH64CatalogField *field = 0;

    if (form == 0 ||
        form->instruction_set_index >= AARCH64_CATALOG_INSTRUCTION_SET_COUNT)
        return 0;
    instruction_set =
        &AARCH64CatalogInstructionSets[form->instruction_set_index];

    if (guard->scope_kind == AARCH64_CATALOG_SCOPE_FORM) {
        if (guard->scope_index >= AARCH64_CATALOG_FORM_COUNT)
            return 0;
        field = AARCH64CatalogFindField(
            form->encoding_fields,
            form->encoding_field_count,
            symbol_index
        );
        if (field != 0)
            return field;
        group_position = (int32_t)form->group_count - 1;
    } else if (guard->scope_kind == AARCH64_CATALOG_SCOPE_GROUP) {
        uint16_t index;
        uint8_t found = 0;

        group_position = -1;
        for (index = 0; index < form->group_count; index++) {
            if (form->group_indices[index] == guard->scope_index) {
                group_position = (int32_t)index;
                found = 1;
                break;
            }
        }
        if (!found)
            return 0;
    } else if (guard->scope_kind == AARCH64_CATALOG_SCOPE_INSTRUCTION_SET) {
        if (guard->scope_index != form->instruction_set_index)
            return 0;
        return AARCH64CatalogFindField(
            instruction_set->encoding_fields,
            instruction_set->encoding_field_count,
            symbol_index
        );
    } else {
        return 0;
    }

    while (group_position >= 0) {
        uint16_t group_index = form->group_indices[group_position];

        if (group_index >= AARCH64_CATALOG_GROUP_COUNT)
            return 0;
        field = AARCH64CatalogFindField(
            AARCH64CatalogGroups[group_index].encoding_fields,
            AARCH64CatalogGroups[group_index].encoding_field_count,
            symbol_index
        );
        if (field != 0)
            return field;
        group_position--;
    }

    return AARCH64CatalogFindField(
        instruction_set->encoding_fields,
        instruction_set->encoding_field_count,
        symbol_index
    );
}

static AARCH64CatalogEvalValue AARCH64CatalogFieldValue(
    const AARCH64CatalogForm *form,
    const AARCH64CatalogGuard *guard,
    uint16_t symbol_index,
    uint32_t encoding
)
{
    const AARCH64CatalogField *field =
        AARCH64CatalogResolveField(form, guard, symbol_index);
    AARCH64CatalogEvalValue value;
    uint32_t mask;

    if (field == 0 || field->width == 0u || field->width > 32u)
        return AARCH64CatalogUnknownValue();

    mask = field->width == 32u
        ? UINT32_MAX
        : ((1u << field->width) - 1u);
    value.value = (encoding >> field->lsb) & mask;
    value.mask = mask;
    value.width = field->width;
    value.kind = AARCH64_CATALOG_VALUE;
    value.state = AARCH64_CATALOG_KNOWN;
    return value;
}

static AARCH64CatalogEvalValue AARCH64CatalogFeatureValue(
    uint16_t feature_index,
    const AARCH64CatalogGuestProfile *profile
)
{
    uint8_t enabled;

    if (profile == 0)
        return AARCH64CatalogUnknownValue();
    if (feature_index >= AARCH64_CATALOG_FEATURE_COUNT)
        return AARCH64CatalogUnknownValue();

    enabled = (uint8_t)(
        (profile->enabled_features[feature_index / 64u] >>
         (feature_index % 64u)) & 1u
    );
    return AARCH64CatalogBoolean(
        enabled,
        enabled ? AARCH64_CATALOG_KNOWN : AARCH64_CATALOG_FEATURE_FALSE
    );
}

static uint8_t AARCH64CatalogValueMatches(
    const AARCH64CatalogEvalValue *value,
    const AARCH64CatalogExpressionValue *pattern
)
{
    uint64_t pattern_mask;

    if (value->state != AARCH64_CATALOG_KNOWN)
        return 0;
    pattern_mask = pattern->mask & AARCH64CatalogWidthMask(pattern->width);
    if (pattern->width != value->width)
        return 0;
    return (value->value & pattern_mask) == pattern->value;
}

static uint8_t AARCH64CatalogValueIsKnown(
    const AARCH64CatalogEvalValue *value
)
{
    return value->state == AARCH64_CATALOG_KNOWN;
}

static uint8_t AARCH64CatalogDecodeLogicalImmediate(
    uint64_t n,
    uint64_t imms,
    uint64_t immr,
    uint8_t width,
    uint64_t *result
)
{
    uint32_t selector;
    uint8_t length = 0;
    uint8_t element_width;
    uint8_t levels;
    uint8_t s;
    uint8_t r;
    uint64_t element_mask;
    uint64_t element;
    uint64_t rotated;
    uint8_t position;
    uint64_t value = 0;

    if (result == 0 || (width != 32u && width != 64u) ||
        (width == 32u && n != 0u))
        return 0;

    selector = (uint32_t)((n << 6) | ((~imms) & 0x3Fu));
    while (selector > 1u) {
        selector >>= 1;
        length++;
    }
    if (length < 1u)
        return 0;
    levels = (uint8_t)((1u << length) - 1u);
    s = (uint8_t)(imms & levels);
    r = (uint8_t)(immr & levels);
    if (s == levels)
        return 0;
    element_width = (uint8_t)(1u << length);
    if (element_width > width)
        return 0;
    element_mask = AARCH64CatalogWidthMask(element_width);
    element = AARCH64CatalogWidthMask((uint8_t)(s + 1u));
    r = (uint8_t)(r % element_width);
    if (r == 0u)
        rotated = element;
    else
        rotated = ((element >> r) |
            (element << (element_width - r))) & element_mask;
    for (position = 0; position < width; position += element_width)
        value |= rotated << position;
    *result = value & AARCH64CatalogWidthMask(width);
    return 1;
}

static uint8_t AARCH64CatalogBFXPreferred(
    uint64_t sf,
    uint64_t opc,
    uint64_t imms,
    uint64_t immr
)
{
    uint64_t width = sf == 0u ? 32u : 64u;

    if (sf > 1u || opc > 1u || imms > 63u || immr > 63u ||
        imms < immr || imms == width - 1u)
        return 0;
    if (immr == 0u) {
        if (sf == 0u && (imms == 7u || imms == 15u))
            return 0;
        if (sf != 0u && opc == 0u &&
            (imms == 7u || imms == 15u || imms == 31u))
            return 0;
    }
    return 1;
}

static uint8_t AARCH64CatalogMoveWidePreferred(
    uint64_t sf,
    uint64_t n,
    uint64_t imms,
    uint64_t immr
)
{
    uint64_t value;
    uint64_t inverted;
    uint8_t width;
    uint8_t chunks;
    uint8_t index;
    uint8_t nonzero_chunks = 0;
    uint8_t nonones_chunks = 0;

    if (sf > 1u || n > 1u || imms > 63u || immr > 63u)
        return 0;
    width = sf == 0u ? 32u : 64u;
    if (!AARCH64CatalogDecodeLogicalImmediate(
            n, imms, immr, width, &value))
        return 0;

    inverted = (~value) & AARCH64CatalogWidthMask(width);
    chunks = (uint8_t)(width / 16u);
    for (index = 0; index < chunks; index++) {
        uint16_t chunk = (uint16_t)(value >> (index * 16u));
        uint16_t inverted_chunk =
            (uint16_t)(inverted >> (index * 16u));
        if (chunk != 0u)
            nonzero_chunks++;
        if (inverted_chunk != 0u)
            nonones_chunks++;
    }
    return nonzero_chunks <= 1u || nonones_chunks <= 1u;
}

static uint8_t AARCH64CatalogSVERepeatChunks(
    uint64_t value,
    uint8_t chunk_width
)
{
    uint64_t mask = AARCH64CatalogWidthMask(chunk_width);
    uint64_t chunk = value & mask;
    uint8_t position;

    for (position = chunk_width; position < 64u; position += chunk_width) {
        if (((value >> position) & mask) != chunk)
            return 0;
    }
    return 1;
}

static uint8_t AARCH64CatalogSVEAboveIs(
    uint64_t value,
    uint8_t first_bit,
    uint8_t all_ones
)
{
    uint8_t width = (uint8_t)(64u - first_bit);
    uint64_t upper = value >> first_bit;
    return all_ones
        ? upper == AARCH64CatalogWidthMask(width)
        : upper == 0u;
}

static uint8_t AARCH64CatalogSVEMoveMaskPreferred(uint64_t imm13)
{
    uint64_t value;
    uint32_t low32;
    uint32_t high32;
    uint16_t low16;
    uint16_t high16;
    uint8_t zero_low_byte;

    if (imm13 > 0x1FFFu ||
        !AARCH64CatalogDecodeLogicalImmediate(
            (imm13 >> 12) & 1u,
            imm13 & 0x3Fu,
            (imm13 >> 6) & 0x3Fu,
            64u,
            &value
        ))
        return 0;

    zero_low_byte = (value & 0xFFu) == 0u;
    low32 = (uint32_t)value;
    high32 = (uint32_t)(value >> 32);
    low16 = (uint16_t)value;
    high16 = (uint16_t)(value >> 16);

    if (!zero_low_byte) {
        if (AARCH64CatalogSVEAboveIs(value, 7u, 0u) ||
            AARCH64CatalogSVEAboveIs(value, 7u, 1u))
            return 0;
        if (low32 == high32 &&
            (((low32 >> 7) == 0u) ||
             ((low32 >> 7) == 0x1FFFFFFu)))
            return 0;
        if (low32 == high32 && low16 == high16 &&
            (((low16 >> 7) == 0u) || ((low16 >> 7) == 0x1FFu)))
            return 0;
        if (AARCH64CatalogSVERepeatChunks(value, 8u))
            return 0;
    } else {
        if (AARCH64CatalogSVEAboveIs(value, 15u, 0u) ||
            AARCH64CatalogSVEAboveIs(value, 15u, 1u))
            return 0;
        if (low32 == high32 &&
            (((low32 >> 15) == 0u) ||
             ((low32 >> 15) == 0x1FFFFu)))
            return 0;
        if (low32 == high32 && low16 == high16)
            return 0;
    }
    return 1;
}

static AARCH64CatalogEvalValue AARCH64CatalogEvaluatePreferred(
    uint16_t preferred_condition,
    const AARCH64CatalogEvalValue *arguments,
    uint8_t argument_count
)
{
    uint8_t index;

    for (index = 0; index < argument_count; index++) {
        if (!AARCH64CatalogValueIsKnown(&arguments[index]))
            return AARCH64CatalogUnknownValue();
    }

    switch (preferred_condition) {
    case AARCH64_CATALOG_PREFERRED_BFX:
        if (argument_count != 4u)
            return AARCH64CatalogUnknownValue();
        return AARCH64CatalogBoolean(
            AARCH64CatalogBFXPreferred(
                arguments[0].value,
                arguments[1].value,
                arguments[2].value,
                arguments[3].value
            ),
            AARCH64_CATALOG_KNOWN
        );
    case AARCH64_CATALOG_PREFERRED_MOVE_WIDE:
        if (argument_count != 4u)
            return AARCH64CatalogUnknownValue();
        return AARCH64CatalogBoolean(
            AARCH64CatalogMoveWidePreferred(
                arguments[0].value,
                arguments[1].value,
                arguments[2].value,
                arguments[3].value
            ),
            AARCH64_CATALOG_KNOWN
        );
    case AARCH64_CATALOG_PREFERRED_SVE_MOVE_MASK:
        if (argument_count != 1u)
            return AARCH64CatalogUnknownValue();
        return AARCH64CatalogBoolean(
            AARCH64CatalogSVEMoveMaskPreferred(arguments[0].value),
            AARCH64_CATALOG_KNOWN
        );
    default:
        return AARCH64CatalogUnknownValue();
    }
}

static AARCH64CatalogEvalValue AARCH64CatalogEvaluateBinary(
    uint16_t operator_id,
    AARCH64CatalogEvalValue left,
    AARCH64CatalogEvalValue right
)
{
    uint64_t full_left;
    uint64_t full_right;
    uint8_t truth;

    if (operator_id == AARCH64_CATALOG_BINARY_AND ||
        operator_id == AARCH64_CATALOG_BINARY_OR) {
        if (left.kind != AARCH64_CATALOG_BOOLEAN ||
            right.kind != AARCH64_CATALOG_BOOLEAN)
            return AARCH64CatalogUnknownValue();

        if (operator_id == AARCH64_CATALOG_BINARY_AND) {
            if (!left.value || !right.value) {
                if ((!left.value &&
                     left.state == AARCH64_CATALOG_KNOWN) ||
                    (!right.value &&
                     right.state == AARCH64_CATALOG_KNOWN))
                    return AARCH64CatalogBoolean(
                        0, AARCH64_CATALOG_KNOWN
                    );
                if (left.state == AARCH64_CATALOG_UNKNOWN ||
                    right.state == AARCH64_CATALOG_UNKNOWN)
                    return AARCH64CatalogUnknownValue();
                return AARCH64CatalogBoolean(
                    0, AARCH64_CATALOG_FEATURE_FALSE
                );
            }
            if (left.state == AARCH64_CATALOG_UNKNOWN ||
                right.state == AARCH64_CATALOG_UNKNOWN)
                return AARCH64CatalogUnknownValue();
            return AARCH64CatalogBoolean(1, AARCH64_CATALOG_KNOWN);
        }

        if (left.value || right.value)
            return AARCH64CatalogBoolean(1, AARCH64_CATALOG_KNOWN);
        if (left.state == AARCH64_CATALOG_UNKNOWN ||
            right.state == AARCH64_CATALOG_UNKNOWN)
            return AARCH64CatalogUnknownValue();
        if (left.state == AARCH64_CATALOG_FEATURE_FALSE ||
            right.state == AARCH64_CATALOG_FEATURE_FALSE)
            return AARCH64CatalogBoolean(
                0, AARCH64_CATALOG_FEATURE_FALSE
            );
        return AARCH64CatalogBoolean(0, AARCH64_CATALOG_KNOWN);
    }

    if (!AARCH64CatalogValueIsKnown(&left) ||
        !AARCH64CatalogValueIsKnown(&right))
        return AARCH64CatalogUnknownValue();

    if (operator_id == AARCH64_CATALOG_BINARY_EQ ||
        operator_id == AARCH64_CATALOG_BINARY_NE) {
        uint8_t equal;

        if (right.mask == AARCH64CatalogWidthMask(right.width))
            equal = left.width == right.width &&
                (left.value & right.mask) == right.value;
        else if (left.mask == AARCH64CatalogWidthMask(left.width))
            equal = left.width == right.width &&
                (left.value & right.mask) == right.value;
        else
            return AARCH64CatalogUnknownValue();
        return AARCH64CatalogBoolean(
            operator_id == AARCH64_CATALOG_BINARY_EQ ? equal : !equal,
            AARCH64_CATALOG_KNOWN
        );
    }

    full_left = AARCH64CatalogWidthMask(left.width);
    full_right = AARCH64CatalogWidthMask(right.width);
    if (left.mask != full_left || right.mask != full_right)
        return AARCH64CatalogUnknownValue();

    switch (operator_id) {
    case AARCH64_CATALOG_BINARY_GT:
        truth = left.value > right.value;
        return AARCH64CatalogBoolean(truth, AARCH64_CATALOG_KNOWN);
    case AARCH64_CATALOG_BINARY_GE:
        truth = left.value >= right.value;
        return AARCH64CatalogBoolean(truth, AARCH64_CATALOG_KNOWN);
    case AARCH64_CATALOG_BINARY_LT:
        truth = left.value < right.value;
        return AARCH64CatalogBoolean(truth, AARCH64_CATALOG_KNOWN);
    case AARCH64_CATALOG_BINARY_LE:
        truth = left.value <= right.value;
        return AARCH64CatalogBoolean(truth, AARCH64_CATALOG_KNOWN);
    case AARCH64_CATALOG_BINARY_ADD:
        left.value += right.value;
        left.width = left.width > right.width ? left.width : right.width;
        left.mask = AARCH64CatalogWidthMask(left.width);
        left.value &= left.mask;
        return left;
    case AARCH64_CATALOG_BINARY_SUB:
        left.value -= right.value;
        left.width = left.width > right.width ? left.width : right.width;
        left.mask = AARCH64CatalogWidthMask(left.width);
        left.value &= left.mask;
        return left;
    case AARCH64_CATALOG_BINARY_BIT_AND:
        left.value &= right.value;
        left.width = left.width > right.width ? left.width : right.width;
        left.mask = AARCH64CatalogWidthMask(left.width);
        return left;
    case AARCH64_CATALOG_BINARY_BIT_OR:
        left.value |= right.value;
        left.width = left.width > right.width ? left.width : right.width;
        left.mask = AARCH64CatalogWidthMask(left.width);
        return left;
    case AARCH64_CATALOG_BINARY_BIT_XOR:
        left.value ^= right.value;
        left.width = left.width > right.width ? left.width : right.width;
        left.mask = AARCH64CatalogWidthMask(left.width);
        return left;
    default:
        return AARCH64CatalogUnknownValue();
    }
}

static AARCH64CatalogEvalValue AARCH64CatalogEvaluateCondition(
    uint16_t condition_index,
    const AARCH64CatalogForm *form,
    const AARCH64CatalogGuard *guard,
    uint32_t encoding,
    const AARCH64CatalogGuestProfile *profile
)
{
    AARCH64CatalogEvalValue stack[AARCH64_CATALOG_MAX_CONDITION_STACK];
    const AARCH64CatalogCondition *condition;
    uint16_t stack_count = 0;
    uint16_t code_index;

    if (condition_index >= AARCH64_CATALOG_CONDITION_COUNT ||
        guard == 0)
        return AARCH64CatalogUnknownValue();
    condition = &AARCH64CatalogConditions[condition_index];

    for (code_index = 0; code_index < condition->instruction_count; code_index++) {
        const AARCH64CatalogExpressionInstruction *instruction =
            &AARCH64CatalogConditionCode[
                condition->first_instruction + code_index
            ];
        AARCH64CatalogEvalValue value;
        uint16_t argument_count = instruction->argument_count;

        switch (instruction->opcode) {
        case AARCH64_CATALOG_EXPR_PUSH_FIELD:
            value = AARCH64CatalogFieldValue(
                form, guard, instruction->operand, encoding
            );
            if (stack_count >= AARCH64_CATALOG_MAX_CONDITION_STACK)
                return AARCH64CatalogUnknownValue();
            stack[stack_count++] = value;
            break;
        case AARCH64_CATALOG_EXPR_PUSH_VALUE:
            if (instruction->operand >= AARCH64_CATALOG_CONDITION_VALUE_COUNT)
                return AARCH64CatalogUnknownValue();
            value.value =
                AARCH64CatalogConditionValues[instruction->operand].value;
            value.mask =
                AARCH64CatalogConditionValues[instruction->operand].mask;
            value.width =
                AARCH64CatalogConditionValues[instruction->operand].width;
            value.kind = AARCH64_CATALOG_VALUE;
            value.state = AARCH64_CATALOG_KNOWN;
            if (stack_count >= AARCH64_CATALOG_MAX_CONDITION_STACK)
                return AARCH64CatalogUnknownValue();
            stack[stack_count++] = value;
            break;
        case AARCH64_CATALOG_EXPR_PUSH_FEATURE:
            value = AARCH64CatalogFeatureValue(instruction->operand, profile);
            if (stack_count >= AARCH64_CATALOG_MAX_CONDITION_STACK)
                return AARCH64CatalogUnknownValue();
            stack[stack_count++] = value;
            break;
        case AARCH64_CATALOG_EXPR_PUSH_BOOLEAN:
            if (instruction->operand > 1u)
                return AARCH64CatalogUnknownValue();
            value = AARCH64CatalogBoolean(
                (uint8_t)instruction->operand,
                AARCH64_CATALOG_KNOWN
            );
            if (stack_count >= AARCH64_CATALOG_MAX_CONDITION_STACK)
                return AARCH64CatalogUnknownValue();
            stack[stack_count++] = value;
            break;
        case AARCH64_CATALOG_EXPR_BINARY:
            if (stack_count < 2u)
                return AARCH64CatalogUnknownValue();
            value = AARCH64CatalogEvaluateBinary(
                instruction->operand,
                stack[stack_count - 2u],
                stack[stack_count - 1u]
            );
            stack_count -= 2u;
            stack[stack_count++] = value;
            break;
        case AARCH64_CATALOG_EXPR_NOT:
            if (stack_count < 1u)
                return AARCH64CatalogUnknownValue();
            value = stack[--stack_count];
            if (value.kind != AARCH64_CATALOG_BOOLEAN ||
                value.state == AARCH64_CATALOG_UNKNOWN)
                value = AARCH64CatalogUnknownValue();
            else
                value = AARCH64CatalogBoolean(
                    !value.value,
                    AARCH64_CATALOG_KNOWN
                );
            stack[stack_count++] = value;
            break;
        case AARCH64_CATALOG_EXPR_IN_SET:
            if (stack_count < 1u ||
                (uint32_t)instruction->operand + instruction->auxiliary >
                    AARCH64_CATALOG_CONDITION_SET_COUNT)
                return AARCH64CatalogUnknownValue();
            value = stack[--stack_count];
            {
                uint16_t member;
                uint8_t found = 0;
                uint8_t unknown = value.state == AARCH64_CATALOG_UNKNOWN;

                for (member = 0; member < instruction->auxiliary; member++) {
                    uint16_t value_index = AARCH64CatalogConditionSetValues[
                        instruction->operand + member
                    ];
                    if (value_index >= AARCH64_CATALOG_CONDITION_VALUE_COUNT)
                        return AARCH64CatalogUnknownValue();
                    if (AARCH64CatalogValueMatches(
                            &value,
                            &AARCH64CatalogConditionValues[value_index]
                        )) {
                        found = 1;
                        break;
                    }
                }
                if (unknown)
                    value = AARCH64CatalogUnknownValue();
                else
                    value = AARCH64CatalogBoolean(
                        found, AARCH64_CATALOG_KNOWN
                    );
            }
            stack[stack_count++] = value;
            break;
        case AARCH64_CATALOG_EXPR_CONCAT:
            if (argument_count == 0u || stack_count < argument_count)
                return AARCH64CatalogUnknownValue();
            {
                uint16_t first = stack_count - argument_count;
                uint16_t item;
                uint8_t width = 0;
                uint64_t combined = 0;
                uint8_t valid = 1;

                for (item = first; item < stack_count; item++) {
                    if (!AARCH64CatalogValueIsKnown(&stack[item]) ||
                        stack[item].width > 32u ||
                        (uint16_t)width + stack[item].width > 32u) {
                        valid = 0;
                        break;
                    }
                    combined = (combined << stack[item].width) |
                        (stack[item].value & AARCH64CatalogWidthMask(
                            stack[item].width
                        ));
                    width = (uint8_t)(width + stack[item].width);
                }
                stack_count = first;
                if (!valid)
                    value = AARCH64CatalogUnknownValue();
                else {
                    value.value = combined;
                    value.width = width;
                    value.mask = AARCH64CatalogWidthMask(width);
                    value.kind = AARCH64_CATALOG_VALUE;
                    value.state = AARCH64_CATALOG_KNOWN;
                }
                stack[stack_count++] = value;
            }
            break;
        case AARCH64_CATALOG_EXPR_BIT_SELECT:
            if (stack_count < 2u)
                return AARCH64CatalogUnknownValue();
            {
                AARCH64CatalogEvalValue bit =
                    stack[stack_count - 1u];
                AARCH64CatalogEvalValue source =
                    stack[stack_count - 2u];

                stack_count -= 2u;
                if (!AARCH64CatalogValueIsKnown(&source) ||
                    !AARCH64CatalogValueIsKnown(&bit) ||
                    bit.value >= source.width)
                    value = AARCH64CatalogUnknownValue();
                else
                    value = AARCH64CatalogBoolean(
                        (uint8_t)((source.value >> bit.value) & 1u),
                        AARCH64_CATALOG_KNOWN
                    );
                stack[stack_count++] = value;
            }
            break;
        case AARCH64_CATALOG_EXPR_BIT_COUNT:
        case AARCH64_CATALOG_EXPR_UINT:
        case AARCH64_CATALOG_EXPR_IS_ZERO:
        case AARCH64_CATALOG_EXPR_IS_ONES:
            if (stack_count < 1u)
                return AARCH64CatalogUnknownValue();
            value = stack[--stack_count];
            if (!AARCH64CatalogValueIsKnown(&value)) {
                value = AARCH64CatalogUnknownValue();
            } else if (instruction->opcode == AARCH64_CATALOG_EXPR_UINT) {
                if (value.mask != AARCH64CatalogWidthMask(value.width))
                    value = AARCH64CatalogUnknownValue();
            } else if (instruction->opcode == AARCH64_CATALOG_EXPR_BIT_COUNT) {
                uint64_t bits = value.value;
                uint8_t count = 0;

                if (value.mask != AARCH64CatalogWidthMask(value.width)) {
                    value = AARCH64CatalogUnknownValue();
                } else {
                    while (bits != 0u) {
                        bits &= bits - 1u;
                        count++;
                    }
                    value.value = count;
                    value.width = 8u;
                    value.mask = 0xFFu;
                }
            } else {
                uint8_t result = instruction->opcode ==
                    AARCH64_CATALOG_EXPR_IS_ZERO
                    ? value.value == 0u
                    : value.value == AARCH64CatalogWidthMask(value.width);

                if (value.mask != AARCH64CatalogWidthMask(value.width))
                    value = AARCH64CatalogUnknownValue();
                else
                    value = AARCH64CatalogBoolean(
                        result, AARCH64_CATALOG_KNOWN
                    );
            }
            stack[stack_count++] = value;
            break;
        case AARCH64_CATALOG_EXPR_PREFERRED:
            if (argument_count == 0u ||
                stack_count < argument_count)
                return AARCH64CatalogUnknownValue();
            value = AARCH64CatalogEvaluatePreferred(
                instruction->operand,
                &stack[stack_count - argument_count],
                (uint8_t)argument_count
            );
            stack_count -= argument_count;
            if (stack_count >= AARCH64_CATALOG_MAX_CONDITION_STACK)
                return AARCH64CatalogUnknownValue();
            stack[stack_count++] = value;
            break;
        case AARCH64_CATALOG_EXPR_UNKNOWN:
        default:
            if (stack_count < argument_count)
                return AARCH64CatalogUnknownValue();
            stack_count -= argument_count;
            if (stack_count >= AARCH64_CATALOG_MAX_CONDITION_STACK)
                return AARCH64CatalogUnknownValue();
            stack[stack_count++] = AARCH64CatalogUnknownValue();
            break;
        }
    }

    if (stack_count != 1u ||
        stack[0].kind != AARCH64_CATALOG_BOOLEAN)
        return AARCH64CatalogUnknownValue();
    return stack[0];
}

static AARCH64CatalogFormResult AARCH64CatalogValidateForm(
    const AARCH64CatalogForm *form,
    uint32_t encoding,
    const AARCH64CatalogGuestProfile *profile
)
{
    uint16_t index;
    uint8_t saw_unvalidated = 0;
    uint8_t saw_extension_unavailable = 0;

    if (!AARCH64CatalogFormEncodingValid(form, encoding))
        return AARCH64_CATALOG_FORM_INVALID;

    for (index = 0; index < form->guard_count; index++) {
        const AARCH64CatalogGuard *guard = &form->guards[index];
        AARCH64CatalogEvalValue result = AARCH64CatalogEvaluateCondition(
            guard->condition_index,
            form,
            guard,
            encoding,
            profile
        );

        if (result.state == AARCH64_CATALOG_UNKNOWN) {
            saw_unvalidated = 1;
            continue;
        }
        if (result.kind != AARCH64_CATALOG_BOOLEAN)
            return AARCH64_CATALOG_FORM_UNVALIDATED;
        if (result.value == 0u) {
            if (result.state == AARCH64_CATALOG_FEATURE_FALSE)
                saw_extension_unavailable = 1;
            else
                return AARCH64_CATALOG_FORM_INVALID;
        }
    }

    if (saw_unvalidated)
        return AARCH64_CATALOG_FORM_UNVALIDATED;
    if (saw_extension_unavailable)
        return AARCH64_CATALOG_FORM_EXTENSION_UNAVAILABLE;
    return AARCH64_CATALOG_FORM_VALID;
}

static AARCH64CatalogResult AARCH64CatalogValidateCandidate(
    const AARCH64CatalogPattern *pattern,
    uint32_t encoding,
    const AARCH64CatalogGuestProfile *profile,
    const AARCH64CatalogForm **selected_form,
    uint16_t *selected_form_index
)
{
    uint16_t index;
    uint8_t saw_unvalidated = 0;
    uint8_t saw_extension_unavailable = 0;
    const AARCH64CatalogForm *unvalidated_form = 0;
    const AARCH64CatalogForm *unavailable_form = 0;
    uint16_t unvalidated_index = 0;
    uint16_t unavailable_index = 0;

    for (index = 0; index < pattern->form_count; index++) {
        const AARCH64CatalogForm *form = &pattern->forms[index];
        AARCH64CatalogFormResult result =
            AARCH64CatalogValidateForm(form, encoding, profile);

        if (result == AARCH64_CATALOG_FORM_VALID) {
            *selected_form = form;
            *selected_form_index = (uint16_t)(pattern->first_form + index);
            return (pattern->flags & AARCH64_CATALOG_PATTERN_RESERVED) != 0
                ? AARCH64_CATALOG_RESERVED_PATTERN
                : AARCH64_CATALOG_VALID_ENCODING;
        }
        if (result == AARCH64_CATALOG_FORM_UNVALIDATED) {
            saw_unvalidated = 1;
            if (unvalidated_form == 0) {
                unvalidated_form = form;
                unvalidated_index =
                    (uint16_t)(pattern->first_form + index);
            }
        } else if (result == AARCH64_CATALOG_FORM_EXTENSION_UNAVAILABLE) {
            saw_extension_unavailable = 1;
            if (unavailable_form == 0) {
                unavailable_form = form;
                unavailable_index =
                    (uint16_t)(pattern->first_form + index);
            }
        }
    }

    if (saw_unvalidated) {
        *selected_form = unvalidated_form;
        *selected_form_index = unvalidated_index;
        return AARCH64_CATALOG_PATTERN_MATCHED_UNVALIDATED;
    }
    if (saw_extension_unavailable) {
        *selected_form = unavailable_form;
        *selected_form_index = unavailable_index;
        return AARCH64_CATALOG_EXTENSION_UNAVAILABLE;
    }
    return AARCH64_CATALOG_INVALID_ENCODING;
}

static AARCH64CatalogEvalValue AARCH64CatalogEvaluateAliasCondition(
    uint16_t form_index,
    uint16_t condition_index,
    uint32_t encoding,
    const AARCH64CatalogGuestProfile *profile
)
{
    AARCH64CatalogGuard guard;
    const AARCH64CatalogGuestProfile *effective_profile = profile;

    if (condition_index == AARCH64_CATALOG_NO_CONDITION)
        return AARCH64CatalogBoolean(1u, AARCH64_CATALOG_KNOWN);
    if (form_index >= AARCH64_CATALOG_FORM_COUNT)
        return AARCH64CatalogUnknownValue();
    if (effective_profile == 0 && AARCH64CatalogHasGuestProfile)
        effective_profile = &AARCH64CatalogActiveGuestProfile;

    guard.condition_index = condition_index;
    guard.scope_index = form_index;
    guard.scope_kind = AARCH64_CATALOG_SCOPE_FORM;
    guard.reserved = 0u;
    return AARCH64CatalogEvaluateCondition(
        condition_index,
        &AARCH64CatalogForms[form_index],
        &guard,
        encoding,
        effective_profile
    );
}

AARCH64CatalogResult AARCH64CatalogFindPreferredAlias(
    uint16_t form_index,
    uint32_t encoding,
    const AARCH64CatalogGuestProfile *profile,
    const AARCH64CatalogAlias **alias
)
{
    const AARCH64CatalogAlias *selected = 0;
    uint8_t saw_unknown = 0;
    uint32_t index;

    if (alias == 0)
        return AARCH64_CATALOG_INVALID_ARGUMENT;
    *alias = 0;
    if (form_index >= AARCH64_CATALOG_FORM_COUNT)
        return AARCH64_CATALOG_INVALID_ARGUMENT;
    if (!AARCH64CatalogFormEncodingValid(
            &AARCH64CatalogForms[form_index], encoding))
        return AARCH64_CATALOG_INVALID_ENCODING;

    for (index = 0; index < AARCH64_CATALOG_ALIAS_COUNT; index++) {
        const AARCH64CatalogAlias *candidate =
            &AARCH64CatalogAliases[index];
        AARCH64CatalogEvalValue condition;
        AARCH64CatalogEvalValue preferred;

        if (candidate->form_index != form_index)
            continue;
        condition = AARCH64CatalogEvaluateAliasCondition(
            form_index,
            candidate->condition_index,
            encoding,
            profile
        );
        if (condition.state == AARCH64_CATALOG_UNKNOWN) {
            saw_unknown = 1u;
            continue;
        }
        if (condition.kind != AARCH64_CATALOG_BOOLEAN)
            return AARCH64_CATALOG_PATTERN_MATCHED_UNVALIDATED;
        if (condition.value == 0u)
            continue;
        preferred = AARCH64CatalogEvaluateAliasCondition(
            form_index,
            candidate->preferred_condition_index,
            encoding,
            profile
        );
        if (preferred.state == AARCH64_CATALOG_UNKNOWN) {
            saw_unknown = 1u;
            continue;
        }
        if (preferred.kind != AARCH64_CATALOG_BOOLEAN)
            return AARCH64_CATALOG_PATTERN_MATCHED_UNVALIDATED;
        if (preferred.value == 0u)
            continue;
        if (selected != 0)
            return AARCH64_CATALOG_AMBIGUOUS;
        selected = candidate;
    }

    if (selected != 0) {
        *alias = selected;
        return AARCH64_CATALOG_VALID_ENCODING;
    }
    return saw_unknown
        ? AARCH64_CATALOG_PATTERN_MATCHED_UNVALIDATED
        : AARCH64_CATALOG_NO_MATCH;
}

AARCH64CatalogResult AARCH64CatalogValidatePattern(
    uint32_t pattern_index,
    uint32_t encoding,
    const AARCH64CatalogGuestProfile *profile
)
{
    const AARCH64CatalogPattern *pattern =
        AARCH64CatalogGetPattern(pattern_index);
    const AARCH64CatalogForm *form = 0;
    uint16_t form_index = 0;

    if (pattern == 0)
        return AARCH64_CATALOG_INVALID_ARGUMENT;
    if (profile != 0 &&
        profile->abi > AARCH64_CATALOG_ABI_APPLE_ARM64E)
        return AARCH64_CATALOG_INVALID_ARGUMENT;
    if ((encoding & pattern->mask) != pattern->value)
        return AARCH64_CATALOG_NO_MATCH;
    return AARCH64CatalogValidateCandidate(
        pattern,
        encoding,
        profile,
        &form,
        &form_index
    );
}

AARCH64CatalogResult AARCH64CatalogLookupWithProfile(
    uint32_t encoding,
    const AARCH64CatalogGuestProfile *profile,
    AARCH64CatalogMatch *match
)
{
    uint32_t bucket;
    uint32_t index;
    uint32_t start;
    uint32_t end;
    const AARCH64CatalogPattern *selected = 0;
    const AARCH64CatalogForm *selected_form = 0;
    const AARCH64CatalogPattern *unavailable_pattern = 0;
    const AARCH64CatalogForm *unavailable_form = 0;
    uint16_t selected_index = 0;
    uint16_t selected_form_index = 0;
    uint16_t selected_priority = 0;
    uint16_t unavailable_index = 0;
    uint16_t unavailable_form_index = 0;
    uint16_t unavailable_priority = 0;
    AARCH64CatalogResult selected_result = AARCH64_CATALOG_NO_MATCH;
    uint8_t saw_mask_match = 0;
    uint8_t saw_invalid = 0;
    uint8_t saw_extension_unavailable = 0;
    uint8_t ambiguous = 0;

    if (match == 0 ||
        (profile != 0 &&
         profile->abi > AARCH64_CATALOG_ABI_APPLE_ARM64E))
        return AARCH64_CATALOG_INVALID_ARGUMENT;

    match->pattern = 0;
    match->form = 0;
    match->pattern_index = 0;
    match->form_index = 0;
    match->candidates_examined = 0;
    match->raw_pattern_matched = 0;

    bucket = encoding >> 26;
    start = AARCH64CatalogBucketOffsets[bucket];
    end = AARCH64CatalogBucketOffsets[bucket + 1u];

    for (index = start; index < end; index++) {
        uint16_t pattern_index = AARCH64CatalogBucketPatterns[index];
        const AARCH64CatalogPattern *candidate =
            &AARCH64CatalogPatterns[pattern_index];
        const AARCH64CatalogForm *candidate_form = 0;
        uint16_t candidate_form_index = 0;
        AARCH64CatalogResult candidate_result;

        match->candidates_examined++;
        if (selected != 0 && candidate->priority < selected_priority)
            break;
        if ((encoding & candidate->mask) != candidate->value)
            continue;

        saw_mask_match = 1;
        match->raw_pattern_matched = 1;
        candidate_result = AARCH64CatalogValidateCandidate(
            candidate,
            encoding,
            profile,
            &candidate_form,
            &candidate_form_index
        );

        if (candidate_result == AARCH64_CATALOG_INVALID_ENCODING) {
            saw_invalid = 1;
            continue;
        }
        if (candidate_result == AARCH64_CATALOG_EXTENSION_UNAVAILABLE) {
            saw_extension_unavailable = 1;
            if (unavailable_pattern == 0 ||
                candidate->priority > unavailable_priority) {
                unavailable_pattern = candidate;
                unavailable_form = candidate_form;
                unavailable_index = pattern_index;
                unavailable_form_index = candidate_form_index;
                unavailable_priority = candidate->priority;
            }
            continue;
        }

        if (selected == 0) {
            selected = candidate;
            selected_form = candidate_form;
            selected_index = pattern_index;
            selected_form_index = candidate_form_index;
            selected_priority = candidate->priority;
            selected_result = candidate_result;
            continue;
        }

        if (candidate->priority != selected_priority)
            continue;
        if (candidate_result == AARCH64_CATALOG_PATTERN_MATCHED_UNVALIDATED ||
            selected_result == AARCH64_CATALOG_PATTERN_MATCHED_UNVALIDATED) {
            selected_result = AARCH64_CATALOG_PATTERN_MATCHED_UNVALIDATED;
            continue;
        }
        if (pattern_index != selected_index)
            ambiguous = 1;
    }

    if (ambiguous)
        return AARCH64_CATALOG_AMBIGUOUS;
    if (selected != 0) {
        match->pattern = selected;
        match->form = selected_form;
        match->pattern_index = selected_index;
        match->form_index = selected_form_index;
        return selected_result;
    }
    if (saw_extension_unavailable) {
        match->pattern = unavailable_pattern;
        match->form = unavailable_form;
        match->pattern_index = unavailable_index;
        match->form_index = unavailable_form_index;
        return AARCH64_CATALOG_EXTENSION_UNAVAILABLE;
    }
    if (saw_invalid || saw_mask_match)
        return AARCH64_CATALOG_INVALID_ENCODING;
    return AARCH64_CATALOG_NO_MATCH;
}

AARCH64CatalogResult AARCH64CatalogLookup(
    uint32_t encoding,
    AARCH64CatalogMatch *match
)
{
    return AARCH64CatalogLookupWithProfile(
        encoding,
        AARCH64CatalogHasGuestProfile
            ? &AARCH64CatalogActiveGuestProfile
            : 0,
        match
    );
}

static uint8_t AARCH64CatalogStringEquals(
    const char *left,
    const char *right
)
{
    if (left == 0 || right == 0)
        return 0;
    while (*left != '\0' && *right != '\0') {
        if (*left != *right)
            return 0;
        left++;
        right++;
    }
    return *left == *right;
}

AARCH64CatalogResult AARCH64CatalogSetGuestProfile(
    const AARCH64CatalogGuestProfile *profile
)
{
    uint32_t used_bits = AARCH64_CATALOG_FEATURE_COUNT % 64u;
    uint64_t valid_high_mask;

    if (profile == 0 ||
        profile->abi > AARCH64_CATALOG_ABI_APPLE_ARM64E)
        return AARCH64_CATALOG_INVALID_ARGUMENT;

    if (used_bits != 0u) {
        valid_high_mask = (1ull << used_bits) - 1ull;
        if ((profile->enabled_features[1] & ~valid_high_mask) != 0u)
            return AARCH64_CATALOG_INVALID_ARGUMENT;
    }

    AARCH64CatalogActiveGuestProfile = *profile;
    AARCH64CatalogHasGuestProfile = 1;
    return AARCH64_CATALOG_PROFILE_CONFIGURED;
}

void AARCH64CatalogClearGuestProfile(void)
{
    memset(&AARCH64CatalogActiveGuestProfile, 0,
        sizeof(AARCH64CatalogActiveGuestProfile));
    AARCH64CatalogHasGuestProfile = 0;
}

void AARCH64CatalogGuestProfileInitialize(
    AARCH64CatalogGuestProfile *profile,
    AARCH64CatalogGuestABI abi
)
{
    if (profile == 0)
        return;
    memset(profile, 0, sizeof(*profile));
    profile->abi = abi;
}

AARCH64CatalogResult AARCH64CatalogGuestProfileSetFeature(
    AARCH64CatalogGuestProfile *profile,
    const char *feature_name,
    uint8_t enabled
)
{
    uint32_t index;

    if (profile == 0 || feature_name == 0 || enabled > 1u ||
        profile->abi > AARCH64_CATALOG_ABI_APPLE_ARM64E)
        return AARCH64_CATALOG_INVALID_ARGUMENT;

    for (index = 0; index < AARCH64_CATALOG_FEATURE_COUNT; index++) {
        if (AARCH64CatalogStringEquals(
                AARCH64CatalogFeatureNames[index],
                feature_name
            )) {
            uint64_t bit = 1ull << (index % 64u);
            if (enabled)
                profile->enabled_features[index / 64u] |= bit;
            else
                profile->enabled_features[index / 64u] &= ~bit;
            return AARCH64_CATALOG_PROFILE_CONFIGURED;
        }
    }
    return AARCH64_CATALOG_INVALID_ARGUMENT;
}

uint32_t AARCH64CatalogFeatureCount(void)
{
    return AARCH64_CATALOG_FEATURE_COUNT;
}

uint32_t AARCH64CatalogConditionCount(void)
{
    return AARCH64_CATALOG_CONDITION_COUNT;
}

uint32_t AARCH64CatalogSymbolCount(void)
{
    return AARCH64_CATALOG_SYMBOL_COUNT;
}

const char *AARCH64CatalogSymbolName(uint32_t index)
{
    if (index >= AARCH64_CATALOG_SYMBOL_COUNT)
        return 0;
    return AARCH64CatalogSymbolNames[index];
}

uint32_t AARCH64CatalogFormCount(void)
{
    return AARCH64_CATALOG_FORM_COUNT;
}

const AARCH64CatalogForm *AARCH64CatalogGetForm(uint32_t index)
{
    if (index >= AARCH64_CATALOG_FORM_COUNT)
        return 0;
    return &AARCH64CatalogForms[index];
}

uint32_t AARCH64CatalogOperandDescriptorCount(void)
{
    return AARCH64_CATALOG_OPERAND_DESCRIPTOR_COUNT;
}

uint32_t AARCH64CatalogAliasCount(void)
{
    return AARCH64_CATALOG_ALIAS_COUNT;
}

uint32_t AARCH64CatalogUnsupportedConditionCount(void)
{
    return AARCH64_CATALOG_UNSUPPORTED_CONDITION_COUNT;
}

uint32_t AARCH64CatalogUnsupportedGuardConditionCount(void)
{
    return AARCH64_CATALOG_UNSUPPORTED_GUARD_CONDITION_COUNT;
}

const char *AARCH64CatalogFeatureName(uint32_t index)
{
    if (index >= AARCH64_CATALOG_FEATURE_COUNT)
        return 0;
    return AARCH64CatalogFeatureNames[index];
}

const AARCH64CatalogPattern *AARCH64CatalogGetPattern(uint32_t index)
{
    if (index >= AARCH64_CATALOG_PATTERN_COUNT)
        return 0;

    return &AARCH64CatalogPatterns[index];
}

const AARCH64CatalogGroup *AARCH64CatalogGetGroup(uint32_t index)
{
    if (index >= AARCH64_CATALOG_GROUP_COUNT)
        return 0;

    return &AARCH64CatalogGroups[index];
}

const AARCH64CatalogInstructionSet *AARCH64CatalogGetInstructionSet(
    uint32_t index
)
{
    if (index >= AARCH64_CATALOG_INSTRUCTION_SET_COUNT)
        return 0;

    return &AARCH64CatalogInstructionSets[index];
}

uint8_t AARCH64CatalogInitialize(void)
{
    uint32_t index;
    uint32_t bytes = 0;

    for (index = 0;
         index < sizeof(AARCH64CatalogTableReferences) /
             sizeof(AARCH64CatalogTableReferences[0]);
         index++) {
        const AARCH64CatalogTableReference *table =
            &AARCH64CatalogTableReferences[index];

        if (table->data == 0 || table->bytes == 0u)
            return 0;
        bytes += table->bytes;
    }

    return (uint8_t)(bytes == AARCH64CatalogGeneratedStorageBytes);
}

uint32_t AARCH64CatalogPatternCount(void)
{
    return AARCH64_CATALOG_PATTERN_COUNT;
}

uint32_t AARCH64CatalogIndexEntryCount(void)
{
    return AARCH64_CATALOG_BUCKET_INDEX_COUNT;
}

uint32_t AARCH64CatalogStorageBytes(void)
{
    return AARCH64CatalogGeneratedStorageBytes;
}
