#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include "AARCH64/Catalog/AARCH64Catalog.h"

static uint32_t tests_run;
static uint32_t tests_failed;
static uint64_t lookups_run;
static uint64_t candidate_total;
static uint32_t candidate_max;
static AARCH64CatalogGuestProfile validation_profile;

static uint32_t CountBits(uint32_t value)
{
    uint32_t count = 0;

    while (value != 0) {
        value &= value - 1u;
        count++;
    }

    return count;
}

static uint32_t NextRandom(uint32_t *state)
{
    uint32_t value = *state;

    value ^= value << 13;
    value ^= value >> 17;
    value ^= value << 5;
    *state = value;
    return value;
}

static AARCH64CatalogResult LinearLookup(
    uint32_t encoding,
    const AARCH64CatalogGuestProfile *profile,
    uint16_t *selected_index,
    uint8_t *raw_match
)
{
    const AARCH64CatalogPattern *selected = 0;
    uint32_t index;
    uint16_t best_index = 0;
    uint16_t best_priority = 0;
    AARCH64CatalogResult selected_result = AARCH64_CATALOG_NO_MATCH;
    uint8_t ambiguous = 0;
    uint8_t saw_extension_unavailable = 0;

    for (index = 0; index < AARCH64CatalogPatternCount(); index++) {
        const AARCH64CatalogPattern *candidate =
            AARCH64CatalogGetPattern(index);
        AARCH64CatalogResult result;

        if ((encoding & candidate->mask) != candidate->value)
            continue;

        *raw_match = 1;
        result = AARCH64CatalogValidatePattern(
            index, encoding, profile
        );
        if (result == AARCH64_CATALOG_INVALID_ENCODING)
            continue;
        if (result == AARCH64_CATALOG_EXTENSION_UNAVAILABLE) {
            saw_extension_unavailable = 1;
            continue;
        }

        if (selected == 0 || candidate->priority > best_priority) {
            selected = candidate;
            best_index = (uint16_t)index;
            best_priority = candidate->priority;
            selected_result = result;
            ambiguous = 0;
        } else if (candidate->priority == best_priority) {
            if (result == AARCH64_CATALOG_PATTERN_MATCHED_UNVALIDATED ||
                selected_result == AARCH64_CATALOG_PATTERN_MATCHED_UNVALIDATED) {
                selected_result = AARCH64_CATALOG_PATTERN_MATCHED_UNVALIDATED;
            } else {
                ambiguous = 1;
            }
        }
    }

    if (selected == 0)
        return saw_extension_unavailable
            ? AARCH64_CATALOG_EXTENSION_UNAVAILABLE
            : *raw_match
                ? AARCH64_CATALOG_INVALID_ENCODING
                : AARCH64_CATALOG_NO_MATCH;

    *selected_index = best_index;

    if (ambiguous)
        return AARCH64_CATALOG_AMBIGUOUS;
    return selected_result;
}

static void CheckLookup(uint32_t encoding)
{
    AARCH64CatalogMatch match;
    uint16_t linear_index = 0;
    uint8_t raw_match = 0;
    AARCH64CatalogResult indexed;
    AARCH64CatalogResult linear;

    indexed = AARCH64CatalogLookupWithProfile(
        encoding, &validation_profile, &match
    );
    linear = LinearLookup(
        encoding, &validation_profile, &linear_index, &raw_match
    );

    tests_run++;
    lookups_run++;

    if (indexed != linear ||
        (indexed == AARCH64_CATALOG_VALID_ENCODING &&
         match.pattern_index != linear_index) ||
        (indexed == AARCH64_CATALOG_PATTERN_MATCHED_UNVALIDATED &&
         match.pattern_index != linear_index) ||
        (indexed == AARCH64_CATALOG_RESERVED_PATTERN &&
         match.pattern_index != linear_index) ||
        (indexed == AARCH64_CATALOG_AMBIGUOUS && match.pattern != 0) ||
        match.raw_pattern_matched != raw_match) {
        tests_failed++;
        if (tests_failed < 8)
            printf(
                "FAIL lookup 0x%08X: index status %u pattern %u, "
                "linear status %u pattern %u\n",
                encoding,
                (unsigned)indexed,
                match.pattern_index,
                (unsigned)linear,
                linear_index
            );
    }

    candidate_total += match.candidates_examined;
    if (match.candidates_examined > candidate_max)
        candidate_max = match.candidates_examined;
}

static void CheckCatalogStructure(void)
{
    uint32_t pattern_index;

    if (AARCH64CatalogPatternCount() != 3099u) {
        tests_failed++;
        printf(
            "FAIL expected 3099 patterns, found %u\n",
            AARCH64CatalogPatternCount()
        );
    }
    tests_run++;
    if (AARCH64CatalogConditionCount() != 599u ||
        AARCH64CatalogFeatureCount() != 105u ||
        AARCH64CatalogSymbolCount() == 0u ||
        AARCH64CatalogUnsupportedGuardConditionCount() != 0u)
        tests_failed++;

    tests_run++;
    for (pattern_index = 0;
         pattern_index < AARCH64CatalogFeatureCount();
         pattern_index++) {
        if (AARCH64CatalogFeatureName(pattern_index) == 0)
            tests_failed++;
    }

    tests_run++;

    if (AARCH64CatalogIndexEntryCount() < AARCH64CatalogPatternCount()) {
        tests_failed++;
        printf("FAIL index has fewer entries than catalog patterns\n");
    }
    tests_run++;

    for (pattern_index = 0;
         pattern_index < AARCH64CatalogPatternCount();
         pattern_index++) {
        const AARCH64CatalogPattern *pattern =
            AARCH64CatalogGetPattern(pattern_index);
        uint32_t form_index;

        tests_run++;
        if (pattern == 0 || pattern->form_count == 0 ||
            (pattern->value & ~pattern->mask) != 0 ||
            pattern->priority != CountBits(pattern->mask) ||
            pattern->forms == 0) {
            tests_failed++;
            if (tests_failed < 8)
                printf("FAIL invalid pattern descriptor %u\n", pattern_index);
            continue;
        }

        for (form_index = 0; form_index < pattern->form_count; form_index++) {
            const AARCH64CatalogForm *form = &pattern->forms[form_index];
            uint32_t field_index;
            uint32_t group_index;

            tests_run++;
            if (form->id == 0 || form->operation_id == 0 ||
                form->decoder_module == 0 || form->condition_id == 0 ||
                form->condition_index >= AARCH64CatalogConditionCount() ||
                form->guards == 0 || form->guard_count == 0 ||
                form->group_count == 0 || form->group_indices == 0) {
                tests_failed++;
                if (tests_failed < 8)
                    printf(
                        "FAIL incomplete form metadata %s\n",
                        form->id != 0 ? form->id : "(null)"
                    );
                continue;
            }

            for (field_index = 0;
                 field_index < form->encoding_field_count;
                 field_index++) {
                const AARCH64CatalogField *field =
                    &form->encoding_fields[field_index];

                tests_run++;
                if (field->width == 0 || field->width > 32 ||
                    field->lsb + field->width > 32 ||
                    (field->value & ~field->value_mask) != 0 ||
                    (field->should_be_value & ~field->should_be_mask) != 0 ||
                    (field->name != 0 &&
                     field->symbol_index >= AARCH64CatalogSymbolCount())) {
                    tests_failed++;
                    if (tests_failed < 8)
                        printf("FAIL invalid field in form %s\n", form->id);
                }
            }

            for (group_index = 0; group_index < form->group_count;
                 group_index++) {
                uint16_t index = form->group_indices[group_index];
                const AARCH64CatalogGroup *group =
                    AARCH64CatalogGetGroup(index);

                tests_run++;
                if (group == 0 ||
                    (group_index == 0 &&
                     group->parent_index != AARCH64_CATALOG_NO_PARENT) ||
                    (group_index != 0 &&
                     group->parent_index != form->group_indices[group_index - 1])) {
                    tests_failed++;
                    if (tests_failed < 8)
                        printf("FAIL invalid group path in form %s\n", form->id);
                }
            }
        }
    }

    tests_run++;
    if (AARCH64CatalogGetPattern(AARCH64CatalogPatternCount()) != 0)
        tests_failed++;

    tests_run++;
    if (AARCH64CatalogGetGroup(761u) != 0)
        tests_failed++;

    tests_run++;
    if (AARCH64CatalogGetInstructionSet(1u) != 0)
        tests_failed++;
}

static void CheckArchitecturalSamples(void)
{
    uint32_t pattern_index;
    uint32_t random_state = 0xA64E2026u;
    uint32_t patterns_with_positive = 0;
    uint8_t found_feature_sensitive = 0;
    AARCH64CatalogGuestProfile no_features;

    AARCH64CatalogGuestProfileInitialize(
        &validation_profile, AARCH64_CATALOG_ABI_AARCH64
    );
    for (pattern_index = 0;
         pattern_index < AARCH64CatalogFeatureCount();
         pattern_index++) {
        const char *name = AARCH64CatalogFeatureName(pattern_index);
        tests_run++;
        if (name == 0 ||
            AARCH64CatalogGuestProfileSetFeature(
                &validation_profile, name, 1
            ) != AARCH64_CATALOG_PROFILE_CONFIGURED)
            tests_failed++;
    }

    AARCH64CatalogGuestProfileInitialize(
        &no_features, AARCH64_CATALOG_ABI_AARCH64
    );

    for (pattern_index = 0;
         pattern_index < AARCH64CatalogPatternCount();
         pattern_index++) {
        const AARCH64CatalogPattern *pattern =
            AARCH64CatalogGetPattern(pattern_index);
        uint32_t sample = pattern->value;
        uint32_t attempt;
        uint8_t positive = 0;
        uint32_t fixed_bit = pattern->mask & (0u - pattern->mask);

        for (attempt = 0; attempt < 8192u; attempt++) {
            AARCH64CatalogResult result =
                AARCH64CatalogValidatePattern(
                    pattern_index, sample, &validation_profile
                );
            tests_run++;
            if (result == AARCH64_CATALOG_VALID_ENCODING ||
                result == AARCH64_CATALOG_RESERVED_PATTERN) {
                positive = 1;
                break;
            }
            sample = pattern->value |
                (NextRandom(&random_state) & ~pattern->mask);
        }

        if (positive)
            patterns_with_positive++;
        else {
            tests_failed++;
            if (tests_failed < 8u)
                printf("FAIL no valid positive sample for pattern %u\n",
                    pattern_index);
        }

        tests_run++;
        if (AARCH64CatalogValidatePattern(
                pattern_index,
                pattern->value ^ fixed_bit,
                &validation_profile
            ) != AARCH64_CATALOG_NO_MATCH)
            tests_failed++;

        if (!found_feature_sensitive && positive) {
            AARCH64CatalogResult without_profile =
                AARCH64CatalogValidatePattern(
                    pattern_index, sample, 0
                );
            AARCH64CatalogResult without_features =
                AARCH64CatalogValidatePattern(
                    pattern_index, sample, &no_features
                );

            if (without_profile ==
                    AARCH64_CATALOG_PATTERN_MATCHED_UNVALIDATED &&
                without_features ==
                    AARCH64_CATALOG_EXTENSION_UNAVAILABLE) {
                AARCH64CatalogGuestProfile arm64e_profile;

                AARCH64CatalogGuestProfileInitialize(
                    &arm64e_profile,
                    AARCH64_CATALOG_ABI_APPLE_ARM64E
                );
                tests_run++;
                if (AARCH64CatalogValidatePattern(
                        pattern_index, sample, &arm64e_profile
                    ) != AARCH64_CATALOG_EXTENSION_UNAVAILABLE)
                    tests_failed++;

                tests_run++;
                if (AARCH64CatalogSetGuestProfile(&no_features) !=
                        AARCH64_CATALOG_PROFILE_CONFIGURED ||
                    AARCH64CatalogLookup(sample, &(AARCH64CatalogMatch){0}) !=
                        AARCH64_CATALOG_EXTENSION_UNAVAILABLE)
                    tests_failed++;

                AARCH64CatalogClearGuestProfile();
                tests_run++;
                if (AARCH64CatalogLookup(sample, &(AARCH64CatalogMatch){0}) !=
                    AARCH64_CATALOG_PATTERN_MATCHED_UNVALIDATED)
                    tests_failed++;

                tests_run++;
                if (AARCH64CatalogSetGuestProfile(&validation_profile) !=
                        AARCH64_CATALOG_PROFILE_CONFIGURED ||
                    AARCH64CatalogLookup(sample, &(AARCH64CatalogMatch){0}) !=
                        AARCH64_CATALOG_VALID_ENCODING)
                    tests_failed++;
                AARCH64CatalogClearGuestProfile();
                found_feature_sensitive = 1;
            }
        }
    }

    tests_run++;
    if (!found_feature_sensitive)
        tests_failed++;

    printf(
        "Architectural validation samples: %u/%u patterns positive, "
        "%u conditions (%u unsupported, %u guarded unsupported), "
        "%u guest features, %u failures\n",
        patterns_with_positive,
        AARCH64CatalogPatternCount(),
        AARCH64CatalogConditionCount(),
        AARCH64CatalogUnsupportedConditionCount(),
        AARCH64CatalogUnsupportedGuardConditionCount(),
        AARCH64CatalogFeatureCount(),
        tests_failed
    );
}

int main(void)
{
    uint32_t pattern_index;
    uint32_t random_state = 0xA64C47A1u;
    clock_t start;
    clock_t finish;
    double elapsed;

    CheckArchitecturalSamples();
    CheckCatalogStructure();
    start = clock();

    for (pattern_index = 0;
         pattern_index < AARCH64CatalogPatternCount();
         pattern_index++) {
        const AARCH64CatalogPattern *pattern =
            AARCH64CatalogGetPattern(pattern_index);
        uint32_t free_bits = ~pattern->mask;
        uint32_t bit;
        uint32_t variation;

        CheckLookup(pattern->value);

        for (bit = 0; bit < 32; bit++) {
            uint32_t bit_mask = 1u << bit;
            if ((free_bits & bit_mask) != 0)
                CheckLookup(pattern->value ^ bit_mask);
        }

        for (variation = 0; variation < 3; variation++) {
            uint32_t sample =
                pattern->value | (NextRandom(&random_state) & free_bits);
            CheckLookup(sample);
        }
    }

    CheckLookup(0xFFFFFFFFu);
    CheckLookup(0x00000000u);
    CheckLookup(0xD503201Fu);
    finish = clock();
    elapsed = (double)(finish - start) / (double)CLOCKS_PER_SEC;

    if (AARCH64CatalogLookup(0, 0) != AARCH64_CATALOG_INVALID_ARGUMENT) {
        tests_failed++;
        tests_run++;
    } else {
        tests_run++;
    }

    printf(
        "AArch64 catalog index: %u patterns, %u index entries, "
        "%u lookups, %u candidate max, %.2f mean, %.3f seconds, "
        "%u static bytes, %u checks, %u failures\n",
        AARCH64CatalogPatternCount(),
        AARCH64CatalogIndexEntryCount(),
        (uint32_t)lookups_run,
        candidate_max,
        lookups_run == 0
            ? 0.0
            : (double)candidate_total / (double)lookups_run,
        elapsed,
        AARCH64CatalogStorageBytes(),
        tests_run,
        tests_failed
    );

    return tests_failed == 0 ? 0 : 1;
}
