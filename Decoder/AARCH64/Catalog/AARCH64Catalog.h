#ifndef ARMITO_AARCH64_CATALOG_H
#define ARMITO_AARCH64_CATALOG_H

#include <stdint.h>

typedef enum {
    AARCH64_CATALOG_INVALID_ARGUMENT = 0,
    AARCH64_CATALOG_NO_MATCH,
    AARCH64_CATALOG_VALID_ENCODING,
    AARCH64_CATALOG_PATTERN_MATCHED_UNVALIDATED,
    AARCH64_CATALOG_RESERVED_PATTERN,
    AARCH64_CATALOG_INVALID_ENCODING,
    AARCH64_CATALOG_EXTENSION_UNAVAILABLE,
    AARCH64_CATALOG_AMBIGUOUS,
    AARCH64_CATALOG_PROFILE_CONFIGURED
} AARCH64CatalogResult;

typedef enum {
    AARCH64_CATALOG_ABI_AARCH64 = 0,
    AARCH64_CATALOG_ABI_APPLE_ARM64E
} AARCH64CatalogGuestABI;

typedef enum {
    AARCH64_CATALOG_SCOPE_INSTRUCTION_SET = 0,
    AARCH64_CATALOG_SCOPE_GROUP,
    AARCH64_CATALOG_SCOPE_FORM
} AARCH64CatalogConditionScope;

typedef enum {
    AARCH64_CATALOG_EXPR_UNKNOWN = 0,
    AARCH64_CATALOG_EXPR_PUSH_FIELD,
    AARCH64_CATALOG_EXPR_PUSH_VALUE,
    AARCH64_CATALOG_EXPR_PUSH_FEATURE,
    AARCH64_CATALOG_EXPR_BINARY,
    AARCH64_CATALOG_EXPR_NOT,
    AARCH64_CATALOG_EXPR_IN_SET,
    AARCH64_CATALOG_EXPR_CONCAT,
    AARCH64_CATALOG_EXPR_BIT_SELECT,
    AARCH64_CATALOG_EXPR_BIT_COUNT,
    AARCH64_CATALOG_EXPR_UINT,
    AARCH64_CATALOG_EXPR_IS_ZERO,
    AARCH64_CATALOG_EXPR_IS_ONES,
    AARCH64_CATALOG_EXPR_PUSH_BOOLEAN,
    AARCH64_CATALOG_EXPR_PREFERRED
} AARCH64CatalogExpressionOpcode;

typedef enum {
    AARCH64_CATALOG_PREFERRED_BFX = 1,
    AARCH64_CATALOG_PREFERRED_MOVE_WIDE,
    AARCH64_CATALOG_PREFERRED_SVE_MOVE_MASK
} AARCH64CatalogPreferredCondition;

typedef enum {
    AARCH64_CATALOG_BINARY_EQ = 1,
    AARCH64_CATALOG_BINARY_NE,
    AARCH64_CATALOG_BINARY_AND,
    AARCH64_CATALOG_BINARY_OR,
    AARCH64_CATALOG_BINARY_GT,
    AARCH64_CATALOG_BINARY_GE,
    AARCH64_CATALOG_BINARY_LT,
    AARCH64_CATALOG_BINARY_LE,
    AARCH64_CATALOG_BINARY_ADD,
    AARCH64_CATALOG_BINARY_SUB,
    AARCH64_CATALOG_BINARY_BIT_AND,
    AARCH64_CATALOG_BINARY_BIT_OR,
    AARCH64_CATALOG_BINARY_BIT_XOR
} AARCH64CatalogBinaryOperator;

typedef struct {
    uint64_t enabled_features[2];
    AARCH64CatalogGuestABI abi;
} AARCH64CatalogGuestProfile;

typedef struct {
    const char *name;
    uint8_t lsb;
    uint8_t width;
    uint8_t kind;
    uint8_t reserved;
    uint16_t symbol_index;
    uint32_t value_mask;
    uint32_t value;
    uint32_t should_be_mask;
    uint32_t should_be_value;
} AARCH64CatalogField;

typedef enum {
    AARCH64_CATALOG_OPERAND_RAW = 0,
    AARCH64_CATALOG_OPERAND_GPR,
    AARCH64_CATALOG_OPERAND_FP_SIMD,
    AARCH64_CATALOG_OPERAND_SVE_VECTOR,
    AARCH64_CATALOG_OPERAND_SME_TILE,
    AARCH64_CATALOG_OPERAND_PREDICATE,
    AARCH64_CATALOG_OPERAND_IMMEDIATE,
    AARCH64_CATALOG_OPERAND_CONDITION,
    AARCH64_CATALOG_OPERAND_SYSTEM_FIELD,
    AARCH64_CATALOG_OPERAND_SHIFT,
    AARCH64_CATALOG_OPERAND_EXTEND,
    AARCH64_CATALOG_OPERAND_LANE
} AARCH64CatalogOperandKind;

typedef enum {
    AARCH64_CATALOG_OPERAND_ROLE_FIELD = 0,
    AARCH64_CATALOG_OPERAND_ROLE_DESTINATION,
    AARCH64_CATALOG_OPERAND_ROLE_SOURCE,
    AARCH64_CATALOG_OPERAND_ROLE_BASE,
    AARCH64_CATALOG_OPERAND_ROLE_OFFSET,
    AARCH64_CATALOG_OPERAND_ROLE_CONTROL
} AARCH64CatalogOperandRole;

typedef enum {
    AARCH64_CATALOG_FAMILY_CONTROL = 0,
    AARCH64_CATALOG_FAMILY_DP_IMMEDIATE,
    AARCH64_CATALOG_FAMILY_DP_REGISTER,
    AARCH64_CATALOG_FAMILY_LOAD_STORE,
    AARCH64_CATALOG_FAMILY_SIMD_FP,
    AARCH64_CATALOG_FAMILY_SVE,
    AARCH64_CATALOG_FAMILY_SME,
    AARCH64_CATALOG_FAMILY_RESERVED
} AARCH64CatalogFamily;

typedef enum {
    AARCH64_CATALOG_ADDRESS_NONE = 0,
    AARCH64_CATALOG_ADDRESS_OFFSET,
    AARCH64_CATALOG_ADDRESS_PRE_INDEX,
    AARCH64_CATALOG_ADDRESS_POST_INDEX,
    AARCH64_CATALOG_ADDRESS_REGISTER_OFFSET,
    AARCH64_CATALOG_ADDRESS_PAIR_OFFSET,
    AARCH64_CATALOG_ADDRESS_PAIR_PRE_INDEX,
    AARCH64_CATALOG_ADDRESS_PAIR_POST_INDEX,
    AARCH64_CATALOG_ADDRESS_PC_RELATIVE,
    AARCH64_CATALOG_ADDRESS_LITERAL
} AARCH64CatalogAddressingMode;

#define AARCH64_CATALOG_OPERAND_SIGNED 0x01u
#define AARCH64_CATALOG_OPERAND_PC_RELATIVE 0x02u
#define AARCH64_CATALOG_OPERAND_SP_OR_ZERO 0x04u
#define AARCH64_CATALOG_OPERAND_REGISTER_LIST 0x08u

typedef struct {
    uint16_t symbol_index;
    uint8_t lsb;
    uint8_t width;
    uint8_t kind;
    uint8_t role;
    uint8_t flags;
    uint8_t register_width;
} AARCH64CatalogOperandDescriptor;

typedef struct {
    uint8_t opcode;
    uint8_t argument_count;
    uint16_t operand;
    uint16_t auxiliary;
} AARCH64CatalogExpressionInstruction;

typedef struct {
    uint32_t value;
    uint32_t mask;
    uint8_t width;
    uint8_t reserved[3];
} AARCH64CatalogExpressionValue;

typedef struct {
    uint32_t first_instruction;
    uint16_t instruction_count;
    uint16_t max_stack;
} AARCH64CatalogCondition;

typedef struct {
    uint16_t condition_index;
    uint16_t scope_index;
    uint8_t scope_kind;
    uint8_t reserved;
} AARCH64CatalogGuard;

typedef struct {
    const char *id;
    const char *name;
    uint16_t parent_index;
    const char *condition_id;
    uint16_t condition_index;
    const AARCH64CatalogField *encoding_fields;
    uint16_t encoding_field_count;
    const char *const *feature_identifiers;
    uint16_t feature_count;
} AARCH64CatalogGroup;

typedef struct {
    const char *id;
    const char *name;
    const char *condition_id;
    uint16_t condition_index;
    const AARCH64CatalogField *encoding_fields;
    uint16_t encoding_field_count;
    const char *const *feature_identifiers;
    uint16_t feature_count;
} AARCH64CatalogInstructionSet;

typedef struct {
    const char *id;
    const char *operation_id;
    const char *mnemonic;
    const char *decoder_module;
    const char *condition_id;
    uint16_t condition_index;
    const AARCH64CatalogField *encoding_fields;
    uint16_t encoding_field_count;
    const char *const *feature_identifiers;
    uint16_t feature_count;
    const AARCH64CatalogGuard *guards;
    uint16_t guard_count;
    const uint16_t *group_indices;
    uint16_t group_count;
    uint16_t instruction_set_index;
    const AARCH64CatalogOperandDescriptor *operand_descriptors;
    uint16_t operand_descriptor_count;
    uint8_t family;
    uint8_t addressing_mode;
} AARCH64CatalogForm;

typedef struct {
    uint32_t mask;
    uint32_t value;
    uint16_t priority;
    uint16_t first_form;
    uint16_t form_count;
    uint16_t flags;
    const AARCH64CatalogForm *forms;
} AARCH64CatalogPattern;

typedef struct {
    const AARCH64CatalogPattern *pattern;
    const AARCH64CatalogForm *form;
    uint16_t pattern_index;
    uint16_t form_index;
    uint16_t candidates_examined;
    uint8_t raw_pattern_matched;
} AARCH64CatalogMatch;

typedef struct {
    const char *name;
    const char *operation_id;
    const char *mnemonic;
    uint16_t form_index;
    uint16_t condition_index;
    uint16_t preferred_condition_index;
} AARCH64CatalogAlias;

#define AARCH64_CATALOG_PATTERN_RESERVED 0x0001u
#define AARCH64_CATALOG_NO_PARENT 0xFFFFu
#define AARCH64_CATALOG_NO_CONDITION 0xFFFFu
#define AARCH64_CATALOG_GUEST_FEATURE_WORDS 2u
#define AARCH64_CATALOG_MAX_CONDITION_STACK 64u

AARCH64CatalogResult AARCH64CatalogLookup(
    uint32_t encoding,
    AARCH64CatalogMatch *match
);
uint8_t AARCH64CatalogInitialize(void);
AARCH64CatalogResult AARCH64CatalogLookupWithProfile(
    uint32_t encoding,
    const AARCH64CatalogGuestProfile *profile,
    AARCH64CatalogMatch *match
);
AARCH64CatalogResult AARCH64CatalogValidatePattern(
    uint32_t pattern_index,
    uint32_t encoding,
    const AARCH64CatalogGuestProfile *profile
);
AARCH64CatalogResult AARCH64CatalogSetGuestProfile(
    const AARCH64CatalogGuestProfile *profile
);
void AARCH64CatalogClearGuestProfile(void);
void AARCH64CatalogGuestProfileInitialize(
    AARCH64CatalogGuestProfile *profile,
    AARCH64CatalogGuestABI abi
);
AARCH64CatalogResult AARCH64CatalogGuestProfileSetFeature(
    AARCH64CatalogGuestProfile *profile,
    const char *feature_name,
    uint8_t enabled
);
uint32_t AARCH64CatalogFeatureCount(void);
const char *AARCH64CatalogFeatureName(uint32_t index);
uint32_t AARCH64CatalogConditionCount(void);
uint32_t AARCH64CatalogSymbolCount(void);
const char *AARCH64CatalogSymbolName(uint32_t index);
uint32_t AARCH64CatalogFormCount(void);
const AARCH64CatalogForm *AARCH64CatalogGetForm(uint32_t index);
uint32_t AARCH64CatalogOperandDescriptorCount(void);
uint32_t AARCH64CatalogAliasCount(void);
AARCH64CatalogResult AARCH64CatalogFindPreferredAlias(
    uint16_t form_index,
    uint32_t encoding,
    const AARCH64CatalogGuestProfile *profile,
    const AARCH64CatalogAlias **alias
);
uint32_t AARCH64CatalogUnsupportedConditionCount(void);
uint32_t AARCH64CatalogUnsupportedGuardConditionCount(void);
const AARCH64CatalogPattern *AARCH64CatalogGetPattern(uint32_t index);
const AARCH64CatalogGroup *AARCH64CatalogGetGroup(uint32_t index);
const AARCH64CatalogInstructionSet *AARCH64CatalogGetInstructionSet(
    uint32_t index
);
uint8_t AARCH64CatalogFormEncodingValid(
    const AARCH64CatalogForm *form,
    uint32_t encoding
);
uint32_t AARCH64CatalogPatternCount(void);
uint32_t AARCH64CatalogIndexEntryCount(void);
uint32_t AARCH64CatalogStorageBytes(void);

#endif
