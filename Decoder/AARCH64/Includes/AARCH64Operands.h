#ifndef ARMITO_AARCH64_OPERANDS_H
#define ARMITO_AARCH64_OPERANDS_H

#include "../Catalog/AARCH64Catalog.h"

#define AARCH64_OPERANDS_MAX_COUNT 32u

typedef enum {
    AARCH64_OPERAND_EXTRACTION_INVALID = 0,
    AARCH64_OPERAND_EXTRACTION_COMPLETE,
    AARCH64_OPERAND_EXTRACTION_PARTIAL,
    AARCH64_OPERAND_EXTRACTION_RAW,
    AARCH64_OPERAND_EXTRACTION_UNSUPPORTED
} AARCH64OperandExtractionStatus;

typedef enum {
    AARCH64_OPERAND_VALUE_RAW = 0,
    AARCH64_OPERAND_VALUE_INTERPRETED,
    AARCH64_OPERAND_VALUE_PARTIAL,
    AARCH64_OPERAND_VALUE_UNSUPPORTED
} AARCH64OperandValueStatus;

typedef enum {
    AARCH64_REGISTER_SPECIAL_NONE = 0,
    AARCH64_REGISTER_SPECIAL_SP,
    AARCH64_REGISTER_SPECIAL_ZERO,
    AARCH64_REGISTER_SPECIAL_INVALID
} AARCH64RegisterSpecial;

typedef enum {
    AARCH64_POINTER_AUTH_KEY_NONE = 0,
    AARCH64_POINTER_AUTH_KEY_IA,
    AARCH64_POINTER_AUTH_KEY_IB,
    AARCH64_POINTER_AUTH_KEY_DA,
    AARCH64_POINTER_AUTH_KEY_DB
} AARCH64OperandPointerAuthKey;

typedef enum {
    AARCH64_POINTER_AUTH_ACTION_NONE = 0,
    AARCH64_POINTER_AUTH_ACTION_SIGN,
    AARCH64_POINTER_AUTH_ACTION_AUTHENTICATE,
    AARCH64_POINTER_AUTH_ACTION_STRIP,
    AARCH64_POINTER_AUTH_ACTION_BRANCH,
    AARCH64_POINTER_AUTH_ACTION_CALL,
    AARCH64_POINTER_AUTH_ACTION_RETURN,
    AARCH64_POINTER_AUTH_ACTION_LOAD
} AARCH64PointerAuthAction;

typedef enum {
    AARCH64_POINTER_AUTH_CONTEXT_NONE = 0,
    AARCH64_POINTER_AUTH_CONTEXT_REGISTER,
    AARCH64_POINTER_AUTH_CONTEXT_ZERO,
    AARCH64_POINTER_AUTH_CONTEXT_SP,
    AARCH64_POINTER_AUTH_CONTEXT_ADDRESS
} AARCH64OperandPointerAuthContext;

#define AARCH64_OPERAND_FLAG_PC_RELATIVE 0x01u
#define AARCH64_OPERAND_FLAG_COMPOSITE 0x02u
#define AARCH64_OPERAND_FLAG_BITMASK 0x04u
#define AARCH64_OPERAND_FLAG_REGISTER_LIST 0x08u
#define AARCH64_OPERAND_FLAG_SIGNED 0x10u
#define AARCH64_OPERAND_FLAG_WRITEBACK 0x20u
#define AARCH64_OPERAND_FLAG_POST_INDEX 0x40u
#define AARCH64_OPERAND_FLAG_ENCODING_SELECTOR 0x80u
#define AARCH64_OPERAND_FLAG_IMPLICIT_REGISTER 0x100u

typedef struct {
    const char *name;
    uint64_t raw_value;
    uint64_t value;
    int64_t signed_value;
    uint64_t address;
    uint16_t symbol_index;
    const char *modifier_name;
    uint8_t kind;
    uint8_t role;
    uint8_t status;
    uint8_t field_width;
    uint8_t register_index;
    uint8_t register_width;
    uint8_t register_special;
    uint8_t scale;
    uint16_t flags;
} AARCH64Operand;

typedef struct {
    uint32_t encoding;
    uint64_t instruction_address;
    AARCH64CatalogResult catalog_status;
    AARCH64OperandExtractionStatus status;
    const AARCH64CatalogForm *form;
    const AARCH64CatalogAlias *preferred_alias;
    const char *canonical_mnemonic;
    const char *operation_id;
    uint16_t operand_count;
    uint8_t family;
    uint8_t addressing_mode;
    uint8_t pointer_auth_key;
    uint8_t pointer_auth_action;
    uint8_t pointer_auth_context;
    uint8_t pointer_auth_address_diversity;
    AARCH64Operand operands[AARCH64_OPERANDS_MAX_COUNT];
} AARCH64OperandSet;

typedef AARCH64OperandExtractionStatus (*AARCH64OperandExtractorFunction)(
    const AARCH64CatalogMatch *match,
    AARCH64CatalogResult catalog_status,
    uint32_t encoding,
    uint64_t instruction_address,
    const AARCH64CatalogGuestProfile *profile,
    AARCH64OperandSet *operands
);

AARCH64OperandExtractionStatus AARCH64OperandsExtract(
    const AARCH64CatalogMatch *match,
    AARCH64CatalogResult catalog_status,
    uint32_t encoding,
    uint64_t instruction_address,
    AARCH64OperandSet *operands
);
AARCH64OperandExtractionStatus AARCH64OperandsExtractWithProfile(
    const AARCH64CatalogMatch *match,
    AARCH64CatalogResult catalog_status,
    uint32_t encoding,
    uint64_t instruction_address,
    const AARCH64CatalogGuestProfile *profile,
    AARCH64OperandSet *operands
);

#endif
