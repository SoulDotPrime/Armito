#ifndef ARMITO_AARCH64_OPERANDS_H
#define ARMITO_AARCH64_OPERANDS_H

#include "../Catalog/AARCH64Catalog.h"
#include "../../../Architecture/AARCH64/Includes/AARCH64SysReg.h"

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
    AARCH64_POINTER_AUTH_KEY_DB,
    AARCH64_POINTER_AUTH_KEY_GA
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

typedef enum {
    AARCH64_CONTROL_FLOW_NONE = 0,
    AARCH64_CONTROL_FLOW_DIRECT,
    AARCH64_CONTROL_FLOW_INDIRECT,
    AARCH64_CONTROL_FLOW_CONDITIONAL,
    AARCH64_CONTROL_FLOW_RETURN,
    AARCH64_CONTROL_FLOW_AUTHENTICATED_INDIRECT,
    AARCH64_CONTROL_FLOW_AUTHENTICATED_RETURN,
    AARCH64_CONTROL_FLOW_EXCEPTION_ENTRY,
    AARCH64_CONTROL_FLOW_EXCEPTION_RETURN,
    AARCH64_CONTROL_FLOW_DEBUG_CONTROL,
    AARCH64_CONTROL_FLOW_TRANSACTION_ABORT
} AARCH64ControlFlowKind;

typedef enum {
    AARCH64_EXCEPTION_NONE = 0,
    AARCH64_EXCEPTION_SVC,
    AARCH64_EXCEPTION_HVC,
    AARCH64_EXCEPTION_SMC,
    AARCH64_EXCEPTION_BRK,
    AARCH64_EXCEPTION_HLT,
    AARCH64_EXCEPTION_DCPS,
    AARCH64_EXCEPTION_TCANCEL,
    AARCH64_EXCEPTION_ERET,
    AARCH64_EXCEPTION_DRPS
} AARCH64ExceptionKind;

#define AARCH64_OPERAND_FLAG_PC_RELATIVE 0x01u
#define AARCH64_OPERAND_FLAG_COMPOSITE 0x02u
#define AARCH64_OPERAND_FLAG_BITMASK 0x04u
#define AARCH64_OPERAND_FLAG_REGISTER_LIST 0x08u
#define AARCH64_OPERAND_FLAG_SIGNED 0x10u
#define AARCH64_OPERAND_FLAG_WRITEBACK 0x20u
#define AARCH64_OPERAND_FLAG_POST_INDEX 0x40u
#define AARCH64_OPERAND_FLAG_ENCODING_SELECTOR 0x80u
#define AARCH64_OPERAND_FLAG_IMPLICIT_REGISTER 0x100u

typedef enum {
    AARCH64_MEMORY_KIND_NONE = 0,
    AARCH64_MEMORY_KIND_FP_SCALAR,
    AARCH64_MEMORY_KIND_ADVSIMD_STRUCTURE,
    AARCH64_MEMORY_KIND_ADVSIMD_LANE,
    AARCH64_MEMORY_KIND_ADVSIMD_REPLICATE,
    AARCH64_MEMORY_KIND_UNPRIVILEGED,
    AARCH64_MEMORY_KIND_MOPS_COPY,
    AARCH64_MEMORY_KIND_MOPS_SET,
    AARCH64_MEMORY_KIND_MTE_TAG,
    AARCH64_MEMORY_KIND_ATOMIC_RMW,
    AARCH64_MEMORY_KIND_PREFETCH
} AARCH64MemoryKind;

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
    uint8_t pstate_pacm_write;
    uint8_t control_flow;
    uint8_t branch_link;
    uint8_t branch_fallthrough;
    uint8_t branch_condition;
    uint8_t branch_reads_nzcv;
    uint8_t branch_reads_lr;
    uint8_t branch_target_register;
    uint8_t branch_compare_width;
    uint8_t branch_modifier_register;
    uint8_t branch_reads_pc;
    uint8_t branch_reads_sp;
    uint8_t branch_reads_pstate;
    uint8_t branch_reads_elr;
    uint8_t branch_reads_spsr;
    uint8_t branch_reads_debug_link;
    uint8_t branch_reads_debug_status;
    uint8_t branch_writes_pstate;
    uint8_t branch_writes_elr;
    uint8_t branch_writes_spsr;
    uint8_t exception_kind;
    uint8_t exception_min_el;
    uint8_t exception_level_selector;
    uint8_t exception_requires_el2;
    uint8_t exception_requires_el3;
    uint16_t exception_immediate;
    uint64_t branch_target_address;
    uint64_t branch_fallthrough_address;
    uint8_t memory_access_width;
    uint8_t memory_element_count;
    uint8_t memory_direction;
    uint8_t memory_writeback;
    uint8_t memory_base_register;
    uint8_t memory_data_register;
    uint8_t memory_pair_register;
    uint8_t memory_sign_extend;
    uint8_t memory_zero_extend;
    uint8_t memory_literal;
    uint8_t memory_base_is_sp;
    uint8_t memory_base_is_zero;
    uint8_t memory_data_is_sp;
    uint8_t memory_data_is_zero;
    uint64_t memory_offset;
    uint64_t memory_base_address;
    uint8_t memory_offset_register;
    uint8_t memory_offset_extend;
    uint8_t memory_offset_scale;
    uint8_t memory_address_expression_resolved;
    uint8_t memory_static_overlap;
    uint8_t memory_execution_overlap;
    uint8_t memory_element_width;
    uint8_t memory_vector_width;
    uint8_t memory_lane;
    uint8_t memory_register_count;
    uint8_t memory_wraparound;
    uint8_t memory_nontemporal;
    uint8_t memory_unprivileged;
    uint8_t memory_acquire;
    uint8_t memory_release;
    uint8_t memory_exclusive;
    uint8_t memory_atomic;
    uint8_t memory_tagged;
    uint8_t memory_phase;
    uint8_t memory_kind;
    uint8_t memory_data_bank;
    uint8_t memory_register_list_start;
    uint8_t memory_structure_count;
    uint8_t memory_interleave_factor;
    uint8_t memory_replicate;
    uint8_t memory_merge_inactive;
    uint8_t memory_lane_valid;
    uint8_t memory_predicate_register;
    uint8_t memory_depends_vl;
    uint8_t memory_depends_svl;
    uint8_t memory_mops_stage;
    uint8_t memory_mops_source_register;
    uint8_t memory_mops_destination_register;
    uint8_t memory_mops_length_register;
    uint8_t memory_mops_reads_carry;
    uint8_t memory_mops_writes_nzcv;
    uint8_t memory_state_dependent_size;
    uint8_t memory_min_el;
    uint8_t memory_tag_granule_bytes;
    uint8_t memory_tag_block_symbolic;
    uint8_t memory_tag_checked;
    uint8_t memory_offset_kind;
    uint8_t memory_list_stride;
    uint8_t memory_mops_options;
    uint8_t memory_tag_zero_data;
    uint8_t memory_atomic_read_result;
    uint8_t memory_writeback_register_count;
    uint16_t memory_total_bytes;
    uint16_t memory_postindex_immediate;
    uint8_t memory_mops_destination_discarded;
    uint8_t memory_mops_source_zero;
    uint8_t system_op0;
    uint8_t system_op1;
    uint8_t system_crn;
    uint8_t system_crm;
    uint8_t system_op2;
    uint8_t system_rt;
    uint8_t system_access;
    uint8_t system_is_implementation_defined;
    uint8_t system_is_sysl;
    uint8_t barrier_domain;
    uint8_t barrier_type;
    uint8_t hint_immediate;
    uint8_t pstate_field;
    uint16_t system_register;
    AARCH64SysRegIdentity system_register_identity;
    const char *system_register_name;
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
