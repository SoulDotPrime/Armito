#include "../Includes/AARCH64Operands.h"
#include "../../../Architecture/AARCH64/Includes/AARCH64CPUState.h"

static void AARCH64OperandsClear(void *memory, uint32_t byte_count)
{
    uint8_t *bytes = (uint8_t *)memory;
    uint32_t index;

    for (index = 0; index < byte_count; index++)
        bytes[index] = 0;
}

static uint8_t AARCH64OperandsStringEquals(
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

static uint8_t AARCH64OperandsStringContains(
    const char *text,
    const char *part
)
{
    while (text != 0 && *text != '\0') {
        const char *left = text;
        const char *right = part;
        while (*left != '\0' && *right != '\0' && *left == *right) {
            left++;
            right++;
        }
        if (*right == '\0')
            return 1;
        text++;
    }
    return 0;
}

static uint8_t AARCH64OperandsStringEndsWith(
    const char *text,
    const char *suffix
)
{
    const char *text_end;
    const char *suffix_end;

    if (text == 0 || suffix == 0)
        return 0;
    text_end = text;
    suffix_end = suffix;
    while (*text_end != '\0')
        text_end++;
    while (*suffix_end != '\0')
        suffix_end++;
    if (text_end - text < suffix_end - suffix)
        return 0;
    text_end -= suffix_end - suffix;
    while (*suffix != '\0') {
        char text_char = *text_end++;
        char suffix_char = *suffix++;
        if (text_char >= 'a' && text_char <= 'z')
            text_char = (char)(text_char - 'a' + 'A');
        if (suffix_char >= 'a' && suffix_char <= 'z')
            suffix_char = (char)(suffix_char - 'a' + 'A');
        if (text_char != suffix_char)
            return 0;
    }
    return 1;
}

static uint8_t AARCH64OperandsStringStartsWithIgnoreCase(
    const char *text,
    const char *prefix
)
{
    if (text == 0 || prefix == 0)
        return 0;
    while (*prefix != '\0') {
        if (*text == '\0')
            return 0;
        char text_char = *text++;
        char prefix_char = *prefix++;
        if (text_char >= 'a' && text_char <= 'z')
            text_char = (char)(text_char - 'a' + 'A');
        if (prefix_char >= 'a' && prefix_char <= 'z')
            prefix_char = (char)(prefix_char - 'a' + 'A');
        if (text_char != prefix_char)
            return 0;
    }
    return 1;
}

static uint8_t AARCH64OperandsNameStartsWith(
    const AARCH64OperandSet *set,
    const char *prefix
)
{
    return AARCH64OperandsStringStartsWithIgnoreCase(
               set->operation_id, prefix
           ) ||
        AARCH64OperandsStringStartsWithIgnoreCase(
            set->canonical_mnemonic, prefix
        );
}

static uint64_t AARCH64OperandsWidthMask(uint8_t width)
{
    if (width >= 64u)
        return UINT64_MAX;
    if (width == 0u)
        return 0u;
    return (1ull << width) - 1ull;
}

static int64_t AARCH64OperandsSignExtend(uint64_t value, uint8_t width)
{
    uint64_t mask;

    if (width == 0u)
        return 0;
    if (width >= 64u)
        return (int64_t)value;
    mask = AARCH64OperandsWidthMask(width);
    value &= mask;
    if ((value & (1ull << (width - 1u))) == 0u)
        return (int64_t)value;
    return -1 - (int64_t)((~value) & mask);
}

static uint8_t AARCH64OperandsFindField(
    const AARCH64CatalogForm *form,
    uint32_t encoding,
    const char *name,
    uint64_t *value,
    uint8_t *width
)
{
    uint16_t index;

    if (form == 0 || name == 0 || value == 0)
        return 0;

    for (index = 0; index < form->operand_descriptor_count; index++) {
        const AARCH64CatalogOperandDescriptor *descriptor =
            &form->operand_descriptors[index];
        const char *field_name =
            AARCH64CatalogSymbolName(descriptor->symbol_index);

        if (AARCH64OperandsStringEquals(field_name, name)) {
            uint32_t mask = descriptor->width == 32u
                ? UINT32_MAX
                : ((1u << descriptor->width) - 1u);
            *value = (encoding >> descriptor->lsb) & mask;
            if (width != 0)
                *width = descriptor->width;
            return 1;
        }
    }
    return 0;
}

static uint8_t AARCH64OperandsFormHasGroup(
    const AARCH64CatalogForm *form,
    const char *name
)
{
    uint16_t index;

    for (index = 0; index < form->group_count; index++) {
        const AARCH64CatalogGroup *group =
            AARCH64CatalogGetGroup(form->group_indices[index]);
        if (group != 0 &&
            AARCH64OperandsStringEquals(group->name, name))
            return 1;
    }
    return 0;
}

static uint8_t AARCH64OperandsFormIsSystemGroup(
    const AARCH64CatalogForm *form
)
{
    static const char *const system_groups[] = {
        "systemmove", "systemmovepr", "systeminstrs",
        "syspairinstrs", "barriers", "hints", "pstate",
        "systeminstrswithreg", "systemresult"
    };
    uint32_t index;

    if (form == 0 || form->group_count == 0)
        return 0;

    for (index = 0;
         index < sizeof(system_groups) / sizeof(system_groups[0]);
         index++) {
        if (AARCH64OperandsFormHasGroup(form, system_groups[index]))
            return 1;
    }
    return 0;
}

static uint8_t AARCH64OperandsFormIsSystemOrSecurity(
    const AARCH64CatalogForm *form
)
{
    static const char *const security_groups[] = {
        "exception", "ldst_pac", "addsub_immtags"
    };
    static const char *const security_ops[] = {
        "ERET", "DRPS", "PACGA", "IRG", "ADDG", "SUBG",
        "GMI", "SUBP", "SUBPS"
    };
    uint32_t index;

    if (AARCH64OperandsFormIsSystemGroup(form))
        return 1;

    if (form == 0)
        return 0;

    if (AARCH64OperandsStringStartsWithIgnoreCase(form->operation_id, "PAC") ||
        AARCH64OperandsStringStartsWithIgnoreCase(form->operation_id, "AUT") ||
        AARCH64OperandsStringStartsWithIgnoreCase(form->operation_id, "XPAC") ||
        AARCH64OperandsStringStartsWithIgnoreCase(form->operation_id, "BRA") ||
        AARCH64OperandsStringStartsWithIgnoreCase(form->operation_id, "BLRA") ||
        AARCH64OperandsStringStartsWithIgnoreCase(form->operation_id, "RETA") ||
        AARCH64OperandsStringStartsWithIgnoreCase(form->operation_id, "ERETA"))
        return 1;

    for (index = 0;
         index < sizeof(security_ops) / sizeof(security_ops[0]);
         index++) {
        if (AARCH64OperandsStringEquals(form->operation_id,
                                        security_ops[index]))
            return 1;
    }

    for (index = 0;
         index < sizeof(security_groups) / sizeof(security_groups[0]);
         index++) {
        if (AARCH64OperandsFormHasGroup(form, security_groups[index]))
            return 1;
    }
    return 0;
}

static uint8_t AARCH64OperandsConventionalBranch(
    const AARCH64CatalogForm *form
)
{
    if (form->family != AARCH64_CATALOG_FAMILY_CONTROL)
        return 0;
    if (AARCH64OperandsFormHasGroup(form, "branch_reg"))
        return AARCH64OperandsStringEquals(form->operation_id, "BR") ||
            AARCH64OperandsStringEquals(form->operation_id, "BLR") ||
            AARCH64OperandsStringEquals(form->operation_id, "RET");
    return AARCH64OperandsFormHasGroup(form, "branch_imm") ||
        AARCH64OperandsFormHasGroup(form, "condbranch") ||
        AARCH64OperandsFormHasGroup(form, "compbranch") ||
        AARCH64OperandsFormHasGroup(form, "compbranch_imm") ||
        AARCH64OperandsFormHasGroup(form, "compbranch_regs") ||
        AARCH64OperandsFormHasGroup(form, "compbranch_regs2") ||
        AARCH64OperandsFormHasGroup(form, "testbranch");
}

static uint8_t AARCH64OperandsExceptionalControl(
    const AARCH64CatalogForm *form
)
{
    if (form->family != AARCH64_CATALOG_FAMILY_CONTROL)
        return 0;
    if (AARCH64OperandsFormHasGroup(form, "exception") ||
        AARCH64OperandsFormHasGroup(form, "miscbranch"))
        return 1;
    return AARCH64OperandsFormHasGroup(form, "branch_reg") &&
        (AARCH64OperandsStringEquals(form->operation_id, "BRA") ||
         AARCH64OperandsStringEquals(form->operation_id, "BLRA") ||
         AARCH64OperandsStringEquals(form->operation_id, "RETA") ||
         AARCH64OperandsStringEquals(form->operation_id, "RETASPPCR_reg") ||
         AARCH64OperandsStringEquals(form->operation_id, "ERET") ||
         AARCH64OperandsStringEquals(form->operation_id, "ERETA") ||
         AARCH64OperandsStringEquals(form->operation_id, "DRPS"));
}

static uint8_t AARCH64OperandsConventionalMemory(
    const AARCH64CatalogForm *form
)
{
    uint16_t index;
    uint8_t is_gpr = 0;
    const char *scan;

    if (form->family != AARCH64_CATALOG_FAMILY_LOAD_STORE ||
        form->feature_count != 0u)
        return 0;
    for (scan = form->id; *scan != '\0'; scan++) {
        if (scan[0] == '3' && scan[1] == '2') {
            is_gpr = 1;
            break;
        }
        if (scan[0] == '6' && scan[1] == '4') {
            is_gpr = 1;
            break;
        }
    }
    if (AARCH64OperandsStringStartsWithIgnoreCase(form->id, "PRFM_"))
        is_gpr = 1;
    if (!is_gpr)
        return 0;
    if (!(AARCH64OperandsFormHasGroup(form, "ldst_pos") ||
          AARCH64OperandsFormHasGroup(form, "ldst_regoff") ||
          AARCH64OperandsFormHasGroup(form, "ldst_immpost") ||
          AARCH64OperandsFormHasGroup(form, "ldst_immpre") ||
          AARCH64OperandsFormHasGroup(form, "ldst_unscaled") ||
          AARCH64OperandsFormHasGroup(form, "loadlit") ||
          AARCH64OperandsFormHasGroup(form, "ldstpair_off") ||
          AARCH64OperandsFormHasGroup(form, "ldstpair_post") ||
          AARCH64OperandsFormHasGroup(form, "ldstpair_pre") ||
          AARCH64OperandsFormHasGroup(form, "ldstnapair_offs")))
        return 0;
    for (index = 0; index < form->operand_descriptor_count; index++) {
        const AARCH64CatalogOperandDescriptor *descriptor =
            &form->operand_descriptors[index];
        if (AARCH64OperandsStringEquals(
                AARCH64CatalogSymbolName(descriptor->symbol_index), "Rt")) {
            is_gpr = descriptor->kind == AARCH64_CATALOG_OPERAND_GPR;
            break;
        }
    }
    return is_gpr;
}

static uint8_t AARCH64OperandsSpecializedMemory(
    const AARCH64CatalogForm *form
)
{
    if (form == 0 || form->family != AARCH64_CATALOG_FAMILY_LOAD_STORE)
        return 0;
    if (AARCH64OperandsFormHasGroup(form, "ldstexclr") ||
        AARCH64OperandsFormHasGroup(form, "ldstexclp") ||
        AARCH64OperandsFormHasGroup(form, "ldstexclr_unpriv") ||
        AARCH64OperandsFormHasGroup(form, "ldstord") ||
        AARCH64OperandsFormHasGroup(form, "comswap") ||
        AARCH64OperandsFormHasGroup(form, "comswappr") ||
        AARCH64OperandsFormHasGroup(form, "comswap_unpriv") ||
        AARCH64OperandsFormHasGroup(form, "comswappr_unpriv") ||
        AARCH64OperandsFormHasGroup(form, "memop") ||
        AARCH64OperandsFormHasGroup(form, "memop_128") ||
        AARCH64OperandsFormHasGroup(form, "rcwcomswap") ||
        AARCH64OperandsFormHasGroup(form, "rcwcomswappr") ||
        AARCH64OperandsFormHasGroup(form, "ldst_pac") ||
        AARCH64OperandsFormHasGroup(form, "ldst_gcs") ||
        AARCH64OperandsFormHasGroup(form, "ldapstl_unscaled") ||
        AARCH64OperandsFormHasGroup(form, "ldapstl_simd") ||
        AARCH64OperandsFormHasGroup(form, "ldapstl_writeback") ||
        AARCH64OperandsFormHasGroup(form, "ldiappstilp"))
        return 0;
    return 1;
}

static uint8_t AARCH64OperandsForwardedMemory(
    const AARCH64CatalogForm *form
)
{
    if (form == 0)
        return 0;
    return AARCH64OperandsFormHasGroup(form, "ldstexclr") ||
        AARCH64OperandsFormHasGroup(form, "ldstexclp") ||
        AARCH64OperandsFormHasGroup(form, "ldstexclr_unpriv") ||
        AARCH64OperandsFormHasGroup(form, "ldstord") ||
        AARCH64OperandsFormHasGroup(form, "comswap") ||
        AARCH64OperandsFormHasGroup(form, "comswappr") ||
        AARCH64OperandsFormHasGroup(form, "comswap_unpriv") ||
        AARCH64OperandsFormHasGroup(form, "comswappr_unpriv") ||
        AARCH64OperandsFormHasGroup(form, "memop") ||
        AARCH64OperandsFormHasGroup(form, "memop_128") ||
        AARCH64OperandsFormHasGroup(form, "rcwcomswap") ||
        AARCH64OperandsFormHasGroup(form, "rcwcomswappr") ||
        AARCH64OperandsFormHasGroup(form, "ldst_pac") ||
        AARCH64OperandsFormHasGroup(form, "ldst_gcs") ||
        AARCH64OperandsFormHasGroup(form, "ldapstl_unscaled") ||
        AARCH64OperandsFormHasGroup(form, "ldapstl_simd") ||
        AARCH64OperandsFormHasGroup(form, "ldapstl_writeback") ||
        AARCH64OperandsFormHasGroup(form, "ldiappstilp");
}

static uint8_t AARCH64OperandsMemoryRegisterWidth(
    const AARCH64CatalogForm *form,
    uint32_t encoding,
    const char *name
)
{
    uint64_t size = 0;
    uint64_t opc = 0;

    if (AARCH64OperandsStringEquals(name, "Rn"))
        return 64u;
    if (AARCH64OperandsStringEquals(name, "Rd") ||
        AARCH64OperandsStringEquals(name, "Rs"))
        return 64u;
    if (AARCH64OperandsStringEquals(name, "Rm"))
        return ((encoding >> 13) & 1u) != 0u ? 64u : 32u;
    if (AARCH64OperandsStringStartsWithIgnoreCase(form->id, "LDRSW"))
        return 64u;
    if (AARCH64OperandsFormHasGroup(form, "loadlit")) {
        AARCH64OperandsFindField(form, encoding, "opc", &opc, 0);
        return (opc & 1u) != 0u ? 64u : 32u;
    }
    if (AARCH64OperandsFormHasGroup(form, "ldstpair_off") ||
        AARCH64OperandsFormHasGroup(form, "ldstpair_post") ||
        AARCH64OperandsFormHasGroup(form, "ldstpair_pre") ||
        AARCH64OperandsFormHasGroup(form, "ldstnapair_offs")) {
        const char *scan;
        for (scan = form->id; *scan != '\0'; scan++)
            if (scan[0] == '6' && scan[1] == '4')
                return 64u;
        return 32u;
    }
    AARCH64OperandsFindField(form, encoding, "size", &size, 0);
    if (AARCH64OperandsStringStartsWithIgnoreCase(form->id, "LDRSW"))
        return 64u;
    if (AARCH64OperandsStringStartsWithIgnoreCase(form->id, "LDRSB") ||
        AARCH64OperandsStringStartsWithIgnoreCase(form->id, "LDRSH"))
        return size == 1u ? 64u : 32u;
    return size == 3u ? 64u : 32u;
}

static uint8_t AARCH64OperandsAdvSIMDMemory(
    const AARCH64CatalogForm *form
)
{
    return AARCH64OperandsFormHasGroup(form, "asisdlse") ||
        AARCH64OperandsFormHasGroup(form, "asisdlsep") ||
        AARCH64OperandsFormHasGroup(form, "asisdlso") ||
        AARCH64OperandsFormHasGroup(form, "asisdlsop");
}

static uint8_t AARCH64OperandsFPDataWidth(
    const AARCH64CatalogForm *form,
    uint32_t encoding
)
{
    const char *suffix = form->id;

    if (AARCH64OperandsAdvSIMDMemory(form))
        return (encoding & (1u << 30)) != 0u ? 128u : 64u;
    while (*suffix != '\0' && *suffix != '_')
        suffix++;
    if (*suffix != '_')
        return 0;
    suffix++;
    if (*suffix == 'B')
        return 8u;
    if (*suffix == 'H')
        return 16u;
    if (*suffix == 'S')
        return 32u;
    if (*suffix == 'D')
        return 64u;
    if (*suffix == 'Q')
        return 128u;
    return 0;
}

static uint8_t AARCH64OperandsMemoryListLength(
    const AARCH64CatalogForm *form
)
{
    const char *name = form->id;
    uint8_t count = 1u;

    if (form->operation_id[2] >= '2' && form->operation_id[2] <= '4')
        count = (uint8_t)(form->operation_id[2] - '0');
    while (*name != '\0') {
        if (*name == '_' &&
            (name[1] == 'R' || name[1] == 'I') &&
            name[2] >= '2' && name[2] <= '4' && name[3] == '_')
            count = (uint8_t)(name[2] - '0');
        name++;
    }
    return count;
}

static uint8_t AARCH64OperandsSpecializedMemoryEncodingValid(
    const AARCH64CatalogForm *form,
    uint32_t encoding
)
{
    uint64_t q = 0;
    uint64_t size = 0;
    uint64_t s = 0;
    uint64_t opcode = 0;
    uint8_t scale;

    if (!AARCH64OperandsAdvSIMDMemory(form))
        return 1;
    AARCH64OperandsFindField(form, encoding, "Q", &q, 0);
    AARCH64OperandsFindField(form, encoding, "size", &size, 0);
    if (AARCH64OperandsFormHasGroup(form, "asisdlse") ||
        AARCH64OperandsFormHasGroup(form, "asisdlsep"))
        return q != 0u || size != 3u ||
            form->operation_id[2] < '2' || form->operation_id[2] > '4';
    AARCH64OperandsFindField(form, encoding, "S", &s, 0);
    AARCH64OperandsFindField(form, encoding, "opcode", &opcode, 0);
    scale = (uint8_t)((opcode >> 1) & 3u);
    if (scale == 3u)
        return form->operation_id[0] == 'L' && s == 0u;
    if (scale == 1u)
        return (size & 1u) == 0u;
    if (scale == 2u)
        return (size & 2u) == 0u &&
            ((size & 1u) == 0u || s == 0u);
    return 1;
}

static void AARCH64OperandsSetMemoryMetadata(AARCH64OperandSet *set)
{
    const AARCH64CatalogForm *form = set->form;
    uint32_t encoding = set->encoding;
    uint64_t size = 0;
    uint16_t index;

    if (!AARCH64OperandsConventionalMemory(form) &&
        !AARCH64OperandsSpecializedMemory(form) &&
        !AARCH64OperandsForwardedMemory(form))
        return;
    set->memory_base_register = 0xffu;
    set->memory_data_register = 0xffu;
    set->memory_pair_register = 0xffu;
    set->memory_offset_register = 0xffu;
    set->memory_address_expression_resolved = 0u;
    set->memory_element_count = 1u;
    set->memory_lane = 0xffu;
    set->memory_register_count = 1u;
    set->memory_phase = AARCH64OperandsForwardedMemory(form) ? 36u : 42u;
    set->memory_atomic =
        AARCH64OperandsForwardedMemory(form) &&
        (AARCH64OperandsFormHasGroup(form, "comswap") ||
         AARCH64OperandsFormHasGroup(form, "comswappr") ||
         AARCH64OperandsFormHasGroup(form, "memop") ||
         AARCH64OperandsFormHasGroup(form, "memop_128") ||
         AARCH64OperandsFormHasGroup(form, "rcwcomswap") ||
         AARCH64OperandsFormHasGroup(form, "rcwcomswappr"));
    set->memory_exclusive =
        AARCH64OperandsFormHasGroup(form, "ldstexclr") ||
        AARCH64OperandsFormHasGroup(form, "ldstexclp") ||
        AARCH64OperandsFormHasGroup(form, "ldstexclr_unpriv");
    set->memory_acquire =
        AARCH64OperandsNameStartsWith(set, "LDAR") ||
        AARCH64OperandsNameStartsWith(set, "LDAP") ||
        AARCH64OperandsNameStartsWith(set, "LDAPR");
    set->memory_release =
        AARCH64OperandsNameStartsWith(set, "STLR") ||
        AARCH64OperandsNameStartsWith(set, "STLLR");
    set->memory_nontemporal =
        AARCH64OperandsNameStartsWith(set, "LDNP") ||
        AARCH64OperandsNameStartsWith(set, "STNP");
    set->memory_unprivileged =
        AARCH64OperandsFormHasGroup(form, "ldst_unpriv") ||
        AARCH64OperandsFormHasGroup(form, "ldstexclr_unpriv") ||
        AARCH64OperandsFormHasGroup(form, "comswap_unpriv") ||
        AARCH64OperandsFormHasGroup(form, "comswappr_unpriv");
    set->memory_tagged =
        AARCH64OperandsFormHasGroup(form, "ldsttags");
    if (AARCH64OperandsFormHasGroup(form, "loadlit")) {
        set->memory_literal = 1u;
        set->memory_access_width = 32u;
        set->memory_base_register = 0xffu;
    } else if (AARCH64OperandsFormHasGroup(form, "ldstpair_off") ||
               AARCH64OperandsFormHasGroup(form, "ldstpair_post") ||
               AARCH64OperandsFormHasGroup(form, "ldstpair_pre") ||
               AARCH64OperandsFormHasGroup(form, "ldstnapair_offs")) {
        set->memory_element_count = 2u;
        AARCH64OperandsFindField(form, encoding, "size", &size, 0);
        if (AARCH64OperandsStringStartsWithIgnoreCase(form->id, "LDTP_Q") ||
            AARCH64OperandsStringStartsWithIgnoreCase(form->id, "STTP_Q") ||
            AARCH64OperandsStringStartsWithIgnoreCase(form->id, "LDTNP_Q") ||
            AARCH64OperandsStringStartsWithIgnoreCase(form->id, "STTNP_Q"))
            set->memory_access_width = 128u;
        else
            set->memory_access_width = (uint8_t)(64u << size);
    } else {
        AARCH64OperandsFindField(form, encoding, "size", &size, 0);
        set->memory_access_width = (uint8_t)(8u << size);
    }
    set->memory_element_width = (uint8_t)(8u << size);
    set->memory_vector_width =
        set->memory_access_width < 128u
            ? set->memory_access_width : 128u;
    if (AARCH64OperandsFormHasGroup(form, "asisdlsop") ||
        AARCH64OperandsFormHasGroup(form, "asisdlso") ||
        AARCH64OperandsFormHasGroup(form, "asisdlsep") ||
        AARCH64OperandsFormHasGroup(form, "asisdlse")) {
        set->memory_element_count =
            AARCH64OperandsStringStartsWithIgnoreCase(form->id, "LD1") ||
            AARCH64OperandsStringStartsWithIgnoreCase(form->id, "ST1")
                ? 1u : set->memory_element_count;
        if (set->memory_register_count == 1u &&
            (form->id[2] == '2' || form->id[2] == '3' ||
             form->id[2] == '4'))
            set->memory_register_count =
                (uint8_t)(form->id[2] - '0');
    }
    if (AARCH64OperandsStringStartsWithIgnoreCase(form->id, "STR") ||
        AARCH64OperandsStringStartsWithIgnoreCase(form->id, "STUR") ||
        AARCH64OperandsStringStartsWithIgnoreCase(form->id, "STP") ||
        AARCH64OperandsStringStartsWithIgnoreCase(form->id, "STNP"))
        set->memory_direction = 2u;
    else if (AARCH64OperandsStringStartsWithIgnoreCase(form->id, "PRFM"))
        set->memory_direction = 3u;
    else
        set->memory_direction = 1u;
    if (AARCH64OperandsStringStartsWithIgnoreCase(form->id, "LDRSB") ||
        AARCH64OperandsStringStartsWithIgnoreCase(form->id, "LDRSH") ||
        AARCH64OperandsStringStartsWithIgnoreCase(form->id, "LDRSW"))
        set->memory_sign_extend = 1u;
    if (AARCH64OperandsStringStartsWithIgnoreCase(form->id, "LDRB") ||
        AARCH64OperandsStringStartsWithIgnoreCase(form->id, "LDRH"))
        set->memory_zero_extend = 1u;
    if (AARCH64OperandsFormHasGroup(form, "ldst_immpre"))
        set->memory_writeback = 1u;
    else if (AARCH64OperandsFormHasGroup(form, "ldst_immpost"))
        set->memory_writeback = 2u;
    for (index = 0; index < set->operand_count; index++) {
        const AARCH64Operand *op = &set->operands[index];
        if (AARCH64OperandsStringEquals(op->name, "index") ||
            AARCH64OperandsStringEquals(op->name, "lane"))
            set->memory_lane = (uint8_t)op->raw_value;
        if (AARCH64OperandsStringEquals(op->name, "Rt2") ||
            AARCH64OperandsStringEquals(op->name, "Vt2") ||
            AARCH64OperandsStringEquals(op->name, "Rt3") ||
            AARCH64OperandsStringEquals(op->name, "Vt3") ||
            AARCH64OperandsStringEquals(op->name, "Rt4") ||
            AARCH64OperandsStringEquals(op->name, "Vt4"))
            set->memory_register_count =
                set->memory_register_count == 1u
                    ? (AARCH64OperandsStringEquals(op->name, "Rt4") ||
                       AARCH64OperandsStringEquals(op->name, "Vt4"))
                        ? 4u
                        : (AARCH64OperandsStringEquals(op->name, "Rt3") ||
                           AARCH64OperandsStringEquals(op->name, "Vt3"))
                            ? 3u : 2u
                    : set->memory_register_count;
        if (op->kind != AARCH64_CATALOG_OPERAND_GPR)
            continue;
        if (AARCH64OperandsStringEquals(op->name, "Rn")) {
            set->memory_base_register = op->register_index;
            set->memory_base_is_sp =
                op->register_special == AARCH64_REGISTER_SPECIAL_SP;
            set->memory_base_is_zero =
                op->register_special == AARCH64_REGISTER_SPECIAL_ZERO;
        } else if (AARCH64OperandsStringEquals(op->name, "Rt")) {
            set->memory_data_register = op->register_index;
            set->memory_data_is_sp =
                op->register_special == AARCH64_REGISTER_SPECIAL_SP;
            set->memory_data_is_zero =
                op->register_special == AARCH64_REGISTER_SPECIAL_ZERO;
        } else if (AARCH64OperandsStringEquals(op->name, "Rt2")) {
            set->memory_pair_register = op->register_index;
        } else if (AARCH64OperandsStringEquals(op->name, "Rm")) {
            set->memory_offset_register = op->register_index;
            set->memory_address_expression_resolved = 0u;
        }
    }
    if (set->memory_base_register != 0xffu &&
        ((set->memory_data_register != 0xffu &&
          set->memory_base_register == set->memory_data_register) ||
         (set->memory_pair_register != 0xffu &&
          set->memory_base_register == set->memory_pair_register)))
        set->memory_static_overlap = 1u;
    if (set->memory_writeback && set->memory_static_overlap)
        set->memory_execution_overlap = 1u;
    if (set->memory_literal) {
        uint64_t imm19 = 0;
        AARCH64OperandsFindField(form, encoding, "imm19", &imm19, 0);
        set->memory_offset = (uint64_t)(AARCH64OperandsSignExtend(imm19, 19u) * 4);
        set->memory_base_address = set->instruction_address + set->memory_offset;
        set->memory_address_expression_resolved = 1u;
        set->memory_wraparound =
            ((int64_t)set->memory_offset >= 0 &&
             set->memory_base_address < set->instruction_address) ||
            ((int64_t)set->memory_offset < 0 &&
             set->memory_base_address > set->instruction_address);
    } else if (set->memory_base_register != 0xffu) {
        uint64_t offset = 0;
        if (AARCH64OperandsFormHasGroup(form, "ldst_pos")) {
            uint64_t imm12 = 0;
            AARCH64OperandsFindField(form, encoding, "imm12", &imm12, 0);
            offset = imm12 << (size & 3u);
        } else if (AARCH64OperandsFormHasGroup(form, "ldst_unscaled") ||
                   AARCH64OperandsFormHasGroup(form, "ldst_immpost") ||
                   AARCH64OperandsFormHasGroup(form, "ldst_immpre")) {
            uint64_t imm9 = 0;
            AARCH64OperandsFindField(form, encoding, "imm9", &imm9, 0);
            offset = (uint64_t)AARCH64OperandsSignExtend(imm9, 9u);
        } else if (AARCH64OperandsFormHasGroup(form, "ldst_regoff")) {
            uint64_t option = 0;
            uint64_t scaled = 0;
            AARCH64OperandsFindField(form, encoding, "option", &option, 0);
            AARCH64OperandsFindField(form, encoding, "S", &scaled, 0);
            set->memory_offset_extend = (uint8_t)option;
            set->memory_offset_scale =
                (uint8_t)(scaled != 0u ? set->memory_access_width / 8u : 0u);
        } else if (AARCH64OperandsFormHasGroup(form, "ldstpair_off") ||
                   AARCH64OperandsFormHasGroup(form, "ldstpair_post") ||
                   AARCH64OperandsFormHasGroup(form, "ldstpair_pre") ||
                   AARCH64OperandsFormHasGroup(form, "ldstnapair_offs")) {
            uint64_t imm7 = 0;
            AARCH64OperandsFindField(form, encoding, "imm7", &imm7, 0);
            offset = (uint64_t)(AARCH64OperandsSignExtend(imm7, 7u) *
                                (int64_t)(set->memory_access_width / 8u));
        }
        set->memory_offset = offset;
    }
    if (AARCH64OperandsStringStartsWithIgnoreCase(form->id, "PRFM_")) {
        set->memory_access_width = 0u;
        set->memory_element_width = 0u;
        set->memory_vector_width = 0u;
        set->memory_element_count = 0u;
        set->memory_register_count = 0u;
    }
}

static void AARCH64OperandsSetSpecializedMemoryMetadata(
    AARCH64OperandSet *set
)
{
    const AARCH64CatalogForm *form = set->form;
    uint64_t q = 0;
    uint64_t size = 0;
    uint64_t s = 0;
    uint64_t opcode = 0;
    uint64_t rm = 0;
    uint64_t value = 0;
    uint16_t index;

    if (!AARCH64OperandsSpecializedMemory(form) ||
        AARCH64OperandsConventionalMemory(form))
        return;
    set->memory_predicate_register = 0xffu;
    set->memory_register_list_start = 0xffu;
    set->memory_mops_source_register = 0xffu;
    set->memory_mops_destination_register = 0xffu;
    set->memory_mops_length_register = 0xffu;
    set->memory_list_stride = 0u;
    AARCH64OperandsFindField(form, set->encoding, "Q", &q, 0);
    AARCH64OperandsFindField(form, set->encoding, "size", &size, 0);
    if (form->operation_id[0] == 'S')
        set->memory_direction = 2u;
    if (AARCH64OperandsFormHasGroup(form, "ldstpair_pre"))
        set->memory_writeback = 1u;
    else if (AARCH64OperandsFormHasGroup(form, "ldstpair_post"))
        set->memory_writeback = 2u;
    if (AARCH64OperandsAdvSIMDMemory(form)) {
        uint8_t count = AARCH64OperandsMemoryListLength(form);
        uint8_t datasize = q != 0u ? 128u : 64u;
        uint8_t element = (uint8_t)(8u << size);

        set->memory_kind = AARCH64_MEMORY_KIND_ADVSIMD_STRUCTURE;
        set->memory_data_bank = AARCH64_CATALOG_OPERAND_FP_SIMD;
        set->memory_register_count = count;
        set->memory_structure_count =
            form->operation_id[2] >= '2' && form->operation_id[2] <= '4'
            ? (uint8_t)(form->operation_id[2] - '0') : 1u;
        set->memory_interleave_factor = set->memory_structure_count;
        set->memory_list_stride = 1u;
        set->memory_vector_width = datasize;
        set->memory_element_width = element;
        if (AARCH64OperandsFormHasGroup(form, "asisdlso") ||
            AARCH64OperandsFormHasGroup(form, "asisdlsop")) {
            uint8_t scale;
            AARCH64OperandsFindField(form, set->encoding, "opcode", &opcode, 0);
            AARCH64OperandsFindField(form, set->encoding, "S", &s, 0);
            scale = (uint8_t)((opcode >> 1) & 3u);
            if (scale == 3u) {
                set->memory_kind = AARCH64_MEMORY_KIND_ADVSIMD_REPLICATE;
                set->memory_replicate = 1u;
            } else {
                set->memory_kind = AARCH64_MEMORY_KIND_ADVSIMD_LANE;
                set->memory_lane_valid = 1u;
                set->memory_merge_inactive =
                    form->operation_id[0] == 'L';
                if (scale == 0u)
                    set->memory_lane =
                        (uint8_t)((q << 3) | (s << 2) | size);
                else if (scale == 1u)
                    set->memory_lane =
                        (uint8_t)((q << 2) | (s << 1) | (size >> 1));
                else if ((size & 1u) == 0u)
                    set->memory_lane = (uint8_t)((q << 1) | s);
                else
                    set->memory_lane = (uint8_t)q;
            }
            element = (uint8_t)(8u << (scale == 3u ? size : scale));
            set->memory_element_count = count;
            set->memory_total_bytes =
                (uint16_t)(count * (element / 8u));
        } else {
            set->memory_element_count =
                (uint8_t)(count * datasize / element);
            set->memory_total_bytes =
                (uint16_t)(count * datasize / 8u);
        }
        set->memory_element_width = element;
        set->memory_access_width = element;
        if (AARCH64OperandsFormHasGroup(form, "asisdlsep") ||
            AARCH64OperandsFormHasGroup(form, "asisdlsop")) {
            set->memory_writeback = 2u;
            AARCH64OperandsFindField(form, set->encoding, "Rm", &rm, 0);
            if (rm == 31u) {
                set->memory_offset_register = 0xffu;
                set->memory_offset = set->memory_total_bytes;
                set->memory_postindex_immediate = set->memory_total_bytes;
                set->memory_offset_kind = 1u;
            } else {
                set->memory_offset_register = (uint8_t)rm;
                set->memory_offset_kind = 2u;
            }
        }
    } else if (AARCH64OperandsFormHasGroup(form, "memcms")) {
        uint64_t op1 = 0;
        uint64_t op2 = 0;
        uint8_t copy = AARCH64OperandsStringStartsWithIgnoreCase(
            form->id, "CPY");

        set->memory_kind = copy ? AARCH64_MEMORY_KIND_MOPS_COPY
                                : AARCH64_MEMORY_KIND_MOPS_SET;
        set->memory_direction = copy ? 4u : 2u;
        set->memory_access_width = 8u;
        set->memory_element_width = 8u;
        set->memory_vector_width = 0u;
        set->memory_element_count = 0u;
        set->memory_register_count = 0u;
        set->memory_state_dependent_size = 1u;
        set->memory_offset_kind = 4u;
        set->memory_writeback = 3u;
        set->memory_writeback_register_count = copy ? 3u : 2u;
        set->memory_base_register = 0xffu;
        set->memory_data_register = 0xffu;
        AARCH64OperandsFindField(form, set->encoding, "Rd", &value, 0);
        set->memory_mops_destination_register = (uint8_t)value;
        set->memory_mops_destination_discarded = value == 31u;
        AARCH64OperandsFindField(form, set->encoding, "Rs", &value, 0);
        set->memory_mops_source_register = (uint8_t)value;
        set->memory_mops_source_zero = value == 31u;
        AARCH64OperandsFindField(form, set->encoding, "Rn", &value, 0);
        set->memory_mops_length_register = (uint8_t)value;
        AARCH64OperandsFindField(form, set->encoding, "op1", &op1, 0);
        AARCH64OperandsFindField(form, set->encoding, "op2", &op2, 0);
        set->memory_mops_stage = (uint8_t)(copy ? op1 + 1u
                                                : (op2 >> 2) + 1u);
        set->memory_mops_options =
            (uint8_t)(copy ? op2 : op2 & 3u);
        set->memory_mops_reads_carry = set->memory_mops_stage != 1u;
        set->memory_mops_writes_nzcv = set->memory_mops_stage == 1u;
        set->memory_nontemporal =
            copy ? (op2 & 12u) != 0u : (op2 & 2u) != 0u;
        set->memory_static_overlap =
            set->memory_mops_destination_register ==
                set->memory_mops_length_register ||
            set->memory_mops_destination_register ==
                set->memory_mops_source_register ||
            set->memory_mops_length_register ==
                set->memory_mops_source_register;
        set->memory_execution_overlap = set->memory_static_overlap;
    } else if (AARCH64OperandsFormHasGroup(form, "memop_unpriv")) {
        uint64_t acquire = 0;
        uint64_t release = 0;
        uint8_t bits = (set->encoding & (1u << 30)) != 0u ? 64u : 32u;

        set->memory_kind = AARCH64_MEMORY_KIND_ATOMIC_RMW;
        set->memory_access_width = bits;
        set->memory_element_width = bits;
        set->memory_vector_width = 0u;
        set->memory_total_bytes = (uint16_t)(bits / 8u);
        set->memory_unprivileged = 1u;
        set->memory_atomic = 1u;
        set->memory_direction = 4u;
        AARCH64OperandsFindField(form, set->encoding, "A", &acquire, 0);
        AARCH64OperandsFindField(form, set->encoding, "R", &release, 0);
        set->memory_atomic_read_result =
            set->memory_data_register != 31u;
        set->memory_acquire =
            acquire != 0u && set->memory_atomic_read_result;
        set->memory_release = release != 0u;
    } else if (AARCH64OperandsFormHasGroup(form, "ldsttags") ||
               AARCH64OperandsStringStartsWithIgnoreCase(form->id, "STGP_")) {
        set->memory_kind = AARCH64_MEMORY_KIND_MTE_TAG;
        set->memory_tagged = 1u;
        set->memory_tag_granule_bytes = 16u;
        set->memory_element_width = 4u;
        set->memory_access_width = 4u;
        set->memory_vector_width = 0u;
        set->memory_total_bytes = 0u;
        if (AARCH64OperandsStringStartsWithIgnoreCase(form->id, "STGP_")) {
            set->memory_access_width = 128u;
            set->memory_element_width = 64u;
            set->memory_element_count = 2u;
            set->memory_register_count = 2u;
            set->memory_total_bytes = 16u;
            AARCH64OperandsFindField(form, set->encoding, "simm7", &value, 0);
            set->memory_offset = (uint64_t)(
                AARCH64OperandsSignExtend(value, 7u) * 16);
        }
        if (AARCH64OperandsStringStartsWithIgnoreCase(form->id, "LDGM_") ||
            AARCH64OperandsStringStartsWithIgnoreCase(form->id, "STGM_") ||
            AARCH64OperandsStringStartsWithIgnoreCase(form->id, "STZGM_")) {
            set->memory_element_count = 0u;
            set->memory_tag_block_symbolic = 1u;
            set->memory_state_dependent_size = 1u;
            set->memory_min_el = 1u;
        }
        set->memory_tag_zero_data =
            AARCH64OperandsStringStartsWithIgnoreCase(form->id, "STZG") ||
            AARCH64OperandsStringStartsWithIgnoreCase(form->id, "STZ2G_");
        if (AARCH64OperandsStringContains(form->id, "Spre_"))
            set->memory_writeback = 1u;
        else if (AARCH64OperandsStringContains(form->id, "Spost_"))
            set->memory_writeback = 2u;
        if (set->memory_tag_zero_data &&
            !set->memory_tag_block_symbolic)
            set->memory_total_bytes =
                AARCH64OperandsStringStartsWithIgnoreCase(form->id, "STZ2G_")
                ? 32u : 16u;
        if (AARCH64OperandsFindField(form, set->encoding, "imm9", &value, 0))
            set->memory_offset = (uint64_t)(
                AARCH64OperandsSignExtend(value, 9u) * 16);
        if (set->memory_tag_block_symbolic)
            set->memory_offset_kind = 5u;
    } else if (AARCH64OperandsStringStartsWithIgnoreCase(form->id, "RPRFM_") ||
               AARCH64OperandsStringStartsWithIgnoreCase(form->id, "PRFUM_")) {
        set->memory_kind = AARCH64_MEMORY_KIND_PREFETCH;
        set->memory_direction = 3u;
        set->memory_access_width = 0u;
        set->memory_element_count = 0u;
        set->memory_data_register = 0xffu;
    } else if (AARCH64OperandsFPDataWidth(form, set->encoding) != 0u) {
        uint8_t width = AARCH64OperandsFPDataWidth(form, set->encoding);

        set->memory_kind = AARCH64_MEMORY_KIND_FP_SCALAR;
        set->memory_data_bank = AARCH64_CATALOG_OPERAND_FP_SIMD;
        set->memory_access_width = width;
        set->memory_element_width = width;
        set->memory_vector_width = width;
        set->memory_total_bytes = (uint16_t)(
            (width / 8u) * set->memory_element_count);
        set->memory_register_count = set->memory_element_count;
        if (AARCH64OperandsFormHasGroup(form, "ldst_pos")) {
            AARCH64OperandsFindField(form, set->encoding, "imm12", &value, 0);
            set->memory_offset = value * (width / 8u);
        } else if (AARCH64OperandsFormHasGroup(form, "ldst_regoff")) {
            AARCH64OperandsFindField(form, set->encoding, "S", &value, 0);
            set->memory_offset_scale = value != 0u ? width / 8u : 0u;
        } else if (AARCH64OperandsFormHasGroup(form, "ldstpair_off") ||
                   AARCH64OperandsFormHasGroup(form, "ldstpair_post") ||
                   AARCH64OperandsFormHasGroup(form, "ldstpair_pre") ||
                   AARCH64OperandsFormHasGroup(form, "ldstnapair_offs")) {
            AARCH64OperandsFindField(form, set->encoding, "imm7", &value, 0);
            set->memory_offset = (uint64_t)(
                AARCH64OperandsSignExtend(value, 7u) * (int64_t)(width / 8u));
            if (AARCH64OperandsFormHasGroup(form, "ldstpair_pre"))
                set->memory_writeback = 1u;
            else if (AARCH64OperandsFormHasGroup(form, "ldstpair_post"))
                set->memory_writeback = 2u;
        }
    } else {
        set->memory_kind = AARCH64_MEMORY_KIND_UNPRIVILEGED;
        set->memory_total_bytes =
            (uint16_t)((set->memory_access_width / 8u) *
                       set->memory_element_count);
        if (AARCH64OperandsFormHasGroup(form, "ldst_unpriv")) {
            AARCH64OperandsFindField(form, set->encoding, "imm9", &value, 0);
            set->memory_offset = (uint64_t)AARCH64OperandsSignExtend(value, 9u);
        }
        if (AARCH64OperandsStringStartsWithIgnoreCase(form->id, "LDTP_") ||
            AARCH64OperandsStringStartsWithIgnoreCase(form->id, "STTP_") ||
            AARCH64OperandsStringStartsWithIgnoreCase(form->id, "LDTNP_") ||
            AARCH64OperandsStringStartsWithIgnoreCase(form->id, "STTNP_")) {
            set->memory_unprivileged = 1u;
            set->memory_access_width = 64u;
            set->memory_total_bytes = 16u;
            AARCH64OperandsFindField(form, set->encoding, "imm7", &value, 0);
            set->memory_offset = (uint64_t)(
                AARCH64OperandsSignExtend(value, 7u) * 8);
        }
        if (AARCH64OperandsStringStartsWithIgnoreCase(form->id, "LDTRSB_") ||
            AARCH64OperandsStringStartsWithIgnoreCase(form->id, "LDTRSH_") ||
            AARCH64OperandsStringStartsWithIgnoreCase(form->id, "LDTRSW_"))
            set->memory_sign_extend = 1u;
        if (AARCH64OperandsStringStartsWithIgnoreCase(form->id, "LDTRB_") ||
            AARCH64OperandsStringStartsWithIgnoreCase(form->id, "LDTRH_"))
            set->memory_zero_extend = 1u;
    }
    for (index = 0; index < set->operand_count; index++) {
        const AARCH64Operand *operand = &set->operands[index];
        if (operand->kind == AARCH64_CATALOG_OPERAND_FP_SIMD &&
            AARCH64OperandsStringEquals(operand->name, "Rt"))
            set->memory_register_list_start = operand->register_index;
    }
    if (set->memory_kind == AARCH64_MEMORY_KIND_ADVSIMD_STRUCTURE ||
        set->memory_kind == AARCH64_MEMORY_KIND_ADVSIMD_LANE ||
        set->memory_kind == AARCH64_MEMORY_KIND_ADVSIMD_REPLICATE) {
        set->memory_data_register = 0xffu;
        set->memory_pair_register = 0xffu;
        set->memory_static_overlap = 0u;
        set->memory_execution_overlap = 0u;
    }
    if (set->memory_kind != AARCH64_MEMORY_KIND_MOPS_COPY &&
        set->memory_kind != AARCH64_MEMORY_KIND_MOPS_SET &&
        set->memory_offset_kind == 0u) {
        if (set->memory_literal)
            set->memory_offset_kind = 3u;
        else if (set->memory_offset_register != 0xffu)
            set->memory_offset_kind = 2u;
        else if (set->memory_base_register != 0xffu)
            set->memory_offset_kind = 1u;
    }
    if (set->memory_kind == AARCH64_MEMORY_KIND_ADVSIMD_STRUCTURE ||
        set->memory_kind == AARCH64_MEMORY_KIND_ADVSIMD_LANE ||
        set->memory_kind == AARCH64_MEMORY_KIND_ADVSIMD_REPLICATE ||
        set->memory_kind == AARCH64_MEMORY_KIND_FP_SCALAR)
        set->memory_tag_checked = set->memory_writeback ||
            (set->memory_base_register != 0xffu &&
             set->memory_base_register != 31u);
}

static void AARCH64OperandsSetPointerAuthMetadata(AARCH64OperandSet *set)
{
    const char *operation = set->operation_id;

    if (AARCH64OperandsStringEquals(operation, "PACM")) {
        set->pstate_pacm_write = 1u;
        return;
    }

    if (AARCH64OperandsNameStartsWith(set, "PACIA") ||
        AARCH64OperandsNameStartsWith(set, "AUTIA") ||
        AARCH64OperandsNameStartsWith(set, "PACNBIASPPC") ||
        AARCH64OperandsNameStartsWith(set, "PACIZA") ||
        AARCH64OperandsNameStartsWith(set, "AUTIZA") ||
        AARCH64OperandsNameStartsWith(set, "BRAA") ||
        AARCH64OperandsNameStartsWith(set, "BLRAA") ||
        AARCH64OperandsNameStartsWith(set, "RETAA")) {
        set->pointer_auth_key = AARCH64_POINTER_AUTH_KEY_IA;
    } else if (AARCH64OperandsNameStartsWith(set, "PACIB") ||
               AARCH64OperandsNameStartsWith(set, "AUTIB") ||
               AARCH64OperandsNameStartsWith(set, "PACNBIBSPPC") ||
               AARCH64OperandsNameStartsWith(set, "PACIZB") ||
               AARCH64OperandsNameStartsWith(set, "AUTIZB") ||
               AARCH64OperandsNameStartsWith(set, "BRAB") ||
               AARCH64OperandsNameStartsWith(set, "BLRAB") ||
               AARCH64OperandsNameStartsWith(set, "RETAB")) {
        set->pointer_auth_key = AARCH64_POINTER_AUTH_KEY_IB;
    } else if (AARCH64OperandsNameStartsWith(set, "PACDA") ||
               AARCH64OperandsNameStartsWith(set, "AUTDA") ||
               AARCH64OperandsNameStartsWith(set, "PACDZA") ||
               AARCH64OperandsNameStartsWith(set, "AUTDZA") ||
               AARCH64OperandsNameStartsWith(set, "LDRAA")) {
        set->pointer_auth_key = AARCH64_POINTER_AUTH_KEY_DA;
    } else if (AARCH64OperandsNameStartsWith(set, "PACDB") ||
               AARCH64OperandsNameStartsWith(set, "AUTDB") ||
               AARCH64OperandsNameStartsWith(set, "PACDZB") ||
               AARCH64OperandsNameStartsWith(set, "AUTDZB") ||
               AARCH64OperandsNameStartsWith(set, "LDRAB")) {
        set->pointer_auth_key = AARCH64_POINTER_AUTH_KEY_DB;
    } else if (AARCH64OperandsNameStartsWith(set, "XPACI") ||
               AARCH64OperandsNameStartsWith(set, "XPACLRI")) {
        set->pointer_auth_key = AARCH64_POINTER_AUTH_KEY_IA;
    } else if (AARCH64OperandsNameStartsWith(set, "XPACD")) {
        set->pointer_auth_key = AARCH64_POINTER_AUTH_KEY_DA;
    }

    if (AARCH64OperandsNameStartsWith(set, "PAC"))
        set->pointer_auth_action = AARCH64_POINTER_AUTH_ACTION_SIGN;
    else if (AARCH64OperandsNameStartsWith(set, "AUT"))
        set->pointer_auth_action =
            AARCH64_POINTER_AUTH_ACTION_AUTHENTICATE;
    else if (AARCH64OperandsNameStartsWith(set, "XPAC"))
        set->pointer_auth_action = AARCH64_POINTER_AUTH_ACTION_STRIP;
    else if (AARCH64OperandsNameStartsWith(set, "BRAA") ||
             AARCH64OperandsNameStartsWith(set, "BRAB"))
        set->pointer_auth_action = AARCH64_POINTER_AUTH_ACTION_BRANCH;
    else if (AARCH64OperandsNameStartsWith(set, "BLRAA") ||
             AARCH64OperandsNameStartsWith(set, "BLRAB"))
        set->pointer_auth_action = AARCH64_POINTER_AUTH_ACTION_CALL;
    else if (AARCH64OperandsNameStartsWith(set, "RETAA") ||
             AARCH64OperandsNameStartsWith(set, "RETAB"))
        set->pointer_auth_action = AARCH64_POINTER_AUTH_ACTION_RETURN;
    else if (AARCH64OperandsNameStartsWith(set, "LDRAA") ||
             AARCH64OperandsNameStartsWith(set, "LDRAB"))
        set->pointer_auth_action = AARCH64_POINTER_AUTH_ACTION_LOAD;

    if (AARCH64OperandsNameStartsWith(set, "PACIZ") ||
        AARCH64OperandsNameStartsWith(set, "PACDZ") ||
        AARCH64OperandsNameStartsWith(set, "AUTIZ") ||
        AARCH64OperandsNameStartsWith(set, "AUTDZ")) {
        set->pointer_auth_context = AARCH64_POINTER_AUTH_CONTEXT_ZERO;
    } else if (AARCH64OperandsStringEndsWith(
                    set->canonical_mnemonic, "SP"
                ) || AARCH64OperandsStringEndsWith(operation, "SP") ||
                AARCH64OperandsNameStartsWith(set, "PACNBI") ||
               AARCH64OperandsNameStartsWith(set, "RETAA") ||
               AARCH64OperandsNameStartsWith(set, "RETAB")) {
        set->pointer_auth_context = AARCH64_POINTER_AUTH_CONTEXT_SP;
    } else if (set->pointer_auth_action ==
               AARCH64_POINTER_AUTH_ACTION_LOAD) {
        set->pointer_auth_context = AARCH64_POINTER_AUTH_CONTEXT_ADDRESS;
        set->pointer_auth_address_diversity = 1u;
    } else if (set->pointer_auth_action != AARCH64_POINTER_AUTH_ACTION_NONE) {
        set->pointer_auth_context = AARCH64_POINTER_AUTH_CONTEXT_REGISTER;
    }
    if (AARCH64OperandsNameStartsWith(set, "PACNBI")) {
        set->branch_reads_sp = 1u;
        set->branch_reads_pc = 1u;
    }
}

static void AARCH64OperandsSetExceptionalControlMetadata(
    AARCH64OperandSet *set
)
{
    const AARCH64CatalogForm *form = set->form;
    const char *operation = form->operation_id;
    uint16_t index;

    if (!AARCH64OperandsExceptionalControl(form))
        return;
    set->branch_condition = 0xffu;
    set->branch_target_register = 0xffu;
    set->branch_modifier_register = 0xffu;
    if (AARCH64OperandsStringEquals(operation, "BRA") ||
        AARCH64OperandsStringEquals(operation, "BLRA")) {
        set->control_flow = AARCH64_CONTROL_FLOW_AUTHENTICATED_INDIRECT;
        set->branch_link = AARCH64OperandsStringEquals(operation, "BLRA");
        set->branch_fallthrough = set->branch_link;
        set->branch_reads_pc = set->branch_link;
        if (AARCH64OperandsStringStartsWithIgnoreCase(form->id, "BRAAZ_") ||
            AARCH64OperandsStringStartsWithIgnoreCase(form->id, "BRABZ_") ||
            AARCH64OperandsStringStartsWithIgnoreCase(form->id, "BLRAAZ_") ||
            AARCH64OperandsStringStartsWithIgnoreCase(form->id, "BLRABZ_"))
            set->pointer_auth_context = AARCH64_POINTER_AUTH_CONTEXT_ZERO;
    } else if (AARCH64OperandsStringEquals(operation, "RETA") ||
               AARCH64OperandsStringEquals(operation, "RETASPPCR_reg") ||
               AARCH64OperandsStringEquals(operation, "RETASPPC_imm")) {
        set->control_flow = AARCH64_CONTROL_FLOW_AUTHENTICATED_RETURN;
        set->branch_target_register = 30u;
        set->branch_reads_lr = 1u;
        set->branch_reads_sp = 1u;
        set->branch_reads_pc =
            !AARCH64OperandsStringEquals(operation, "RETA");
    } else if (AARCH64OperandsStringEquals(operation, "ERET") ||
               AARCH64OperandsStringEquals(operation, "ERETA")) {
        set->control_flow = AARCH64_CONTROL_FLOW_EXCEPTION_RETURN;
        set->exception_kind = AARCH64_EXCEPTION_ERET;
        set->exception_min_el = 1u;
        set->branch_reads_elr = 1u;
        set->branch_reads_spsr = 1u;
        set->branch_reads_pstate = 1u;
        set->branch_writes_pstate = 1u;
        if (AARCH64OperandsStringEquals(operation, "ERETA")) {
            set->pointer_auth_key =
                AARCH64OperandsStringStartsWithIgnoreCase(form->id, "ERETAA_")
                ? AARCH64_POINTER_AUTH_KEY_IA : AARCH64_POINTER_AUTH_KEY_IB;
            set->pointer_auth_action = AARCH64_POINTER_AUTH_ACTION_RETURN;
            set->pointer_auth_context = AARCH64_POINTER_AUTH_CONTEXT_SP;
            set->branch_reads_sp = 1u;
        }
    } else if (AARCH64OperandsStringEquals(operation, "DRPS")) {
        set->control_flow = AARCH64_CONTROL_FLOW_EXCEPTION_RETURN;
        set->exception_kind = AARCH64_EXCEPTION_DRPS;
        set->branch_reads_debug_link = 1u;
        set->branch_reads_debug_status = 1u;
        set->branch_reads_pstate = 1u;
        set->branch_writes_pstate = 1u;
    } else if (AARCH64OperandsStringEquals(operation, "TCANCEL")) {
        set->control_flow = AARCH64_CONTROL_FLOW_TRANSACTION_ABORT;
        set->exception_kind = AARCH64_EXCEPTION_TCANCEL;
    } else if (AARCH64OperandsStringStartsWithIgnoreCase(form->id, "DCPS")) {
        set->control_flow = AARCH64_CONTROL_FLOW_DEBUG_CONTROL;
        set->exception_kind = AARCH64_EXCEPTION_DCPS;
        set->exception_level_selector = (uint8_t)(form->id[4] - '0');
        set->exception_requires_el3 =
            set->exception_level_selector == 3u;
        set->branch_reads_pstate = 1u;
        set->branch_writes_pstate = 1u;
    } else {
        set->control_flow = AARCH64_CONTROL_FLOW_EXCEPTION_ENTRY;
        set->branch_reads_pc = 1u;
        set->branch_reads_pstate = 1u;
        set->branch_writes_pstate = 1u;
        set->branch_writes_elr = 1u;
        set->branch_writes_spsr = 1u;
        if (AARCH64OperandsStringEquals(operation, "SVC"))
            set->exception_kind = AARCH64_EXCEPTION_SVC;
        else if (AARCH64OperandsStringEquals(operation, "HVC"))
            set->exception_kind = AARCH64_EXCEPTION_HVC;
        else if (AARCH64OperandsStringEquals(operation, "SMC"))
            set->exception_kind = AARCH64_EXCEPTION_SMC;
        else if (AARCH64OperandsStringEquals(operation, "BRK"))
            set->exception_kind = AARCH64_EXCEPTION_BRK;
        else if (AARCH64OperandsStringEquals(operation, "HLT"))
            set->exception_kind = AARCH64_EXCEPTION_HLT;
        if (set->exception_kind == AARCH64_EXCEPTION_HVC ||
            set->exception_kind == AARCH64_EXCEPTION_SMC)
            set->exception_min_el = 1u;
        set->exception_requires_el2 =
            set->exception_kind == AARCH64_EXCEPTION_HVC;
        set->exception_requires_el3 =
            set->exception_kind == AARCH64_EXCEPTION_SMC;
    }
    if (set->branch_fallthrough)
        set->branch_fallthrough_address = set->instruction_address + 4u;
    for (index = 0; index < set->operand_count; index++) {
        const AARCH64Operand *op = &set->operands[index];
        if (AARCH64OperandsStringEquals(op->name, "imm16"))
            set->exception_immediate = (uint16_t)op->raw_value;
        if (op->kind != AARCH64_CATALOG_OPERAND_GPR ||
            op->status != AARCH64_OPERAND_VALUE_INTERPRETED)
            continue;
        if (AARCH64OperandsStringEquals(op->name, "Rn") &&
            set->control_flow == AARCH64_CONTROL_FLOW_AUTHENTICATED_INDIRECT)
            set->branch_target_register = op->register_index;
        if (AARCH64OperandsStringEquals(op->name, "Rm"))
            set->branch_modifier_register = op->register_index;
    }
}

static uint8_t AARCH64OperandsAddSynthetic(
    AARCH64OperandSet *set,
    const char *name,
    AARCH64CatalogOperandKind kind,
    AARCH64CatalogOperandRole role,
    uint64_t raw,
    uint64_t value,
    int64_t signed_value,
    uint8_t width,
    uint8_t scale,
    uint8_t flags
)
{
    AARCH64Operand *operand;

    if (set->operand_count >= AARCH64_OPERANDS_MAX_COUNT)
        return 0;
    operand = &set->operands[set->operand_count++];
    AARCH64OperandsClear(operand, sizeof(*operand));
    operand->name = name;
    operand->symbol_index = 0xFFFFu;
    operand->kind = (uint8_t)kind;
    operand->role = (uint8_t)role;
    operand->status = AARCH64_OPERAND_VALUE_INTERPRETED;
    operand->raw_value = raw;
    operand->value = value;
    operand->signed_value = signed_value;
    operand->field_width = width;
    operand->scale = scale;
    operand->flags = flags;
    return 1;
}

static uint8_t AARCH64OperandsDecodeLogicalImmediate(
    uint64_t n,
    uint64_t immr,
    uint64_t imms,
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

    if (result == 0 || (width != 32u && width != 64u))
        return 0;
    if (width == 32u && n != 0u)
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
    element_mask = AARCH64OperandsWidthMask(element_width);
    element = AARCH64OperandsWidthMask((uint8_t)(s + 1u));
    r = (uint8_t)(r % element_width);
    if (r == 0u)
        rotated = element;
    else
        rotated = ((element >> r) |
            (element << (element_width - r))) & element_mask;
    for (position = 0; position < width; position += element_width)
        value |= rotated << position;

    *result = value & AARCH64OperandsWidthMask(width);
    return 1;
}

static uint8_t AARCH64OperandsDecodeBitfieldMasks(
    uint64_t n,
    uint64_t immr,
    uint64_t imms,
    uint8_t width,
    uint64_t *write_mask,
    uint64_t *top_mask
)
{
    uint32_t selector;
    uint8_t length = 0;
    uint8_t element_width;
    uint8_t levels;
    uint8_t s;
    uint8_t r;
    uint8_t d;
    uint8_t position;
    uint64_t element;
    uint64_t rotated;
    uint64_t element_mask;
    uint64_t write_value = 0u;
    uint64_t top_value = 0u;

    if (write_mask == 0 || top_mask == 0 ||
        (width != 32u && width != 64u) || (width == 32u && n != 0u))
        return 0;
    selector = (uint32_t)((n << 6) | ((~imms) & 0x3Fu));
    while (selector > 1u) {
        selector >>= 1;
        length++;
    }
    if (length < 1u)
        return 0;
    element_width = (uint8_t)(1u << length);
    if (element_width > width)
        return 0;
    levels = (uint8_t)(element_width - 1u);
    s = (uint8_t)(imms & levels);
    r = (uint8_t)(immr & levels);
    d = (uint8_t)((s - r) & levels);
    element_mask = AARCH64OperandsWidthMask(element_width);
    element = AARCH64OperandsWidthMask((uint8_t)(s + 1u));
    rotated = r == 0u ? element :
        ((element >> r) | (element << (element_width - r))) & element_mask;
    for (position = 0u; position < width; position += element_width) {
        write_value |= rotated << position;
        top_value |= AARCH64OperandsWidthMask((uint8_t)(d + 1u)) << position;
    }
    *write_mask = write_value;
    *top_mask = top_value;
    return 1;
}

static uint8_t AARCH64OperandsGetRegisterWidth(
    const AARCH64CatalogForm *form,
    uint32_t encoding
)
{
    uint16_t index;
    uint64_t value;

    if (AARCH64OperandsStringStartsWithIgnoreCase(form->id, "LDTP_Q") ||
        AARCH64OperandsStringStartsWithIgnoreCase(form->id, "STTP_Q") ||
        AARCH64OperandsStringStartsWithIgnoreCase(form->id, "LDTNP_Q") ||
        AARCH64OperandsStringStartsWithIgnoreCase(form->id, "STTNP_Q"))
        return 128u;

    for (index = 0; index < form->operand_descriptor_count; index++) {
        const AARCH64CatalogOperandDescriptor *descriptor =
            &form->operand_descriptors[index];

        if (descriptor->kind != AARCH64_CATALOG_OPERAND_GPR)
            continue;
        if (descriptor->register_width != 0u)
            return descriptor->register_width;
    }
    if (AARCH64OperandsFindField(form, encoding, "sf", &value, 0))
        return value != 0u ? 64u : 32u;
    return 0;
}

static uint8_t AARCH64OperandsIntegerRegisterWidth(
    const AARCH64CatalogForm *form,
    uint32_t encoding,
    const char *name
)
{
    uint8_t width = AARCH64OperandsGetRegisterWidth(form, encoding);

    if (AARCH64OperandsFormHasGroup(form, "pcreladdr"))
        return 64u;
    if (AARCH64OperandsFormHasGroup(form, "dp_3src") &&
        (AARCH64OperandsStringStartsWithIgnoreCase(form->id, "SMADDL") ||
         AARCH64OperandsStringStartsWithIgnoreCase(form->id, "SMSUBL") ||
         AARCH64OperandsStringStartsWithIgnoreCase(form->id, "UMADDL") ||
         AARCH64OperandsStringStartsWithIgnoreCase(form->id, "UMSUBL") ||
         AARCH64OperandsStringStartsWithIgnoreCase(form->id, "SMULL") ||
         AARCH64OperandsStringStartsWithIgnoreCase(form->id, "UMULL")) &&
        (AARCH64OperandsStringEquals(name, "Rn") ||
         AARCH64OperandsStringEquals(name, "Rm")))
        return 32u;
    if (AARCH64OperandsStringStartsWithIgnoreCase(form->id, "CRC32") &&
        AARCH64OperandsStringEquals(name, "Rm"))
        return AARCH64OperandsStringStartsWithIgnoreCase(form->id, "CRC32X_") ||
            AARCH64OperandsStringStartsWithIgnoreCase(form->id, "CRC32CX_")
            ? 64u : 32u;
    if (AARCH64OperandsStringStartsWithIgnoreCase(form->id, "CRC32"))
        return 32u;
    if (AARCH64OperandsFormHasGroup(form, "addsub_ext") &&
        AARCH64OperandsStringEquals(name, "Rm"))
        return width == 64u && (((encoding >> 13) & 7u) == 3u ||
                                ((encoding >> 13) & 7u) == 7u)
            ? 64u : 32u;
    return width;
}

static uint8_t AARCH64OperandsIntegerEncodingSelector(const char *name)
{
    if (name == 0)
        return 0;
    if (AARCH64OperandsStringEquals(name, "sf") ||
        AARCH64OperandsStringEquals(name, "op") ||
        AARCH64OperandsStringEquals(name, "op21") ||
        AARCH64OperandsStringEquals(name, "S") ||
        AARCH64OperandsStringEquals(name, "N") ||
        AARCH64OperandsStringEquals(name, "Z") ||
        AARCH64OperandsStringEquals(name, "D") ||
        AARCH64OperandsStringEquals(name, "C") ||
        AARCH64OperandsStringEquals(name, "U") ||
        AARCH64OperandsStringEquals(name, "o0") ||
        AARCH64OperandsStringEquals(name, "o1") ||
        AARCH64OperandsStringEquals(name, "o2") ||
        AARCH64OperandsStringEquals(name, "o3") ||
        AARCH64OperandsStringEquals(name, "sz") ||
        AARCH64OperandsStringEquals(name, "opc") ||
        AARCH64OperandsStringEquals(name, "opcode") ||
        AARCH64OperandsStringEquals(name, "opcode2") ||
        AARCH64OperandsStringEquals(name, "op31") ||
        AARCH64OperandsStringEquals(name, "op54"))
        return 1;
    if (name[0] == 'o' && name[1] == 'p' &&
        name[2] >= '0' && name[2] <= '9' && name[3] == '\0')
        return 1;
    return 0;
}

static uint8_t AARCH64OperandsAddCompositeOperands(
    AARCH64OperandSet *set
)
{
    const AARCH64CatalogForm *form = set->form;
    uint64_t value_a;
    uint64_t value_b;
    uint64_t value_c;
    uint8_t width;

    if (AARCH64OperandsFormHasGroup(form, "log_imm") &&
        AARCH64OperandsFindField(form, set->encoding, "N", &value_a, 0) &&
        AARCH64OperandsFindField(form, set->encoding, "immr", &value_b, 0) &&
        AARCH64OperandsFindField(form, set->encoding, "imms", &value_c, 0)) {
        uint64_t bitmask;
        width = AARCH64OperandsGetRegisterWidth(form, set->encoding);
        if (!AARCH64OperandsDecodeLogicalImmediate(
                value_a, value_b, value_c, width, &bitmask))
            return 0;
        if (!AARCH64OperandsAddSynthetic(
                set,
                "logical_immediate",
                AARCH64_CATALOG_OPERAND_IMMEDIATE,
                AARCH64_CATALOG_OPERAND_ROLE_SOURCE,
                (value_a << 12) | (value_b << 6) | value_c,
                bitmask,
                0,
                width,
                0,
                AARCH64_OPERAND_FLAG_BITMASK |
                    AARCH64_OPERAND_FLAG_COMPOSITE
            ))
            return 0;
    }

    if (AARCH64OperandsFormHasGroup(form, "bitfield") &&
        AARCH64OperandsFindField(form, set->encoding, "N", &value_a, 0) &&
        AARCH64OperandsFindField(form, set->encoding, "immr", &value_b, 0) &&
        AARCH64OperandsFindField(form, set->encoding, "imms", &value_c, 0)) {
        uint64_t write_mask;
        uint64_t top_mask;
        width = AARCH64OperandsGetRegisterWidth(form, set->encoding);
        if (!AARCH64OperandsDecodeBitfieldMasks(value_a, value_b, value_c,
                width, &write_mask, &top_mask) ||
            !AARCH64OperandsAddSynthetic(set, "bitfield_write_mask",
                AARCH64_CATALOG_OPERAND_IMMEDIATE,
                AARCH64_CATALOG_OPERAND_ROLE_SOURCE,
                (value_a << 12) | (value_b << 6) | value_c,
                write_mask, 0, width, 0,
                AARCH64_OPERAND_FLAG_BITMASK | AARCH64_OPERAND_FLAG_COMPOSITE) ||
            !AARCH64OperandsAddSynthetic(set, "bitfield_top_mask",
                AARCH64_CATALOG_OPERAND_IMMEDIATE,
                AARCH64_CATALOG_OPERAND_ROLE_SOURCE,
                (value_a << 12) | (value_b << 6) | value_c,
                top_mask, 0, width, 0,
                AARCH64_OPERAND_FLAG_BITMASK | AARCH64_OPERAND_FLAG_COMPOSITE))
            return 0;
    }

    if (AARCH64OperandsFormHasGroup(form, "addsub_imm") &&
        AARCH64OperandsFindField(form, set->encoding, "imm12", &value_a, 0) &&
        AARCH64OperandsFindField(form, set->encoding, "sh", &value_b, 0)) {
        uint8_t shift = value_b != 0u ? 12u : 0u;
        if (!AARCH64OperandsAddSynthetic(
                set,
                "scaled_immediate",
                AARCH64_CATALOG_OPERAND_IMMEDIATE,
                AARCH64_CATALOG_OPERAND_ROLE_SOURCE,
                value_a,
                value_a << shift,
                (int64_t)(value_a << shift),
                12u,
                shift,
                AARCH64_OPERAND_FLAG_COMPOSITE
            ))
            return 0;
    }

    if (AARCH64OperandsFormHasGroup(form, "movewide") &&
        AARCH64OperandsFindField(form, set->encoding, "imm16", &value_a, 0) &&
        AARCH64OperandsFindField(form, set->encoding, "hw", &value_b, 0)) {
        uint8_t shift = (uint8_t)(value_b * 16u);
        if (!AARCH64OperandsAddSynthetic(
                set,
                "move_wide_immediate",
                AARCH64_CATALOG_OPERAND_IMMEDIATE,
                AARCH64_CATALOG_OPERAND_ROLE_SOURCE,
                value_a,
                value_a << shift,
                0,
                16u,
                shift,
                AARCH64_OPERAND_FLAG_COMPOSITE
            ))
            return 0;
    }

    if (set->addressing_mode == AARCH64_CATALOG_ADDRESS_OFFSET &&
        AARCH64OperandsFormHasGroup(form, "ldst_pos") &&
        AARCH64OperandsFindField(form, set->encoding, "imm12", &value_a, 0) &&
        AARCH64OperandsFindField(form, set->encoding, "size", &value_b, 0)) {
        uint8_t shift = (uint8_t)(value_b & 3u);
        if (!AARCH64OperandsAddSynthetic(
                set,
                "scaled_memory_offset",
                AARCH64_CATALOG_OPERAND_IMMEDIATE,
                AARCH64_CATALOG_OPERAND_ROLE_OFFSET,
                value_a,
                value_a << shift,
                (int64_t)(value_a << shift),
                12u,
                shift,
                AARCH64_OPERAND_FLAG_COMPOSITE
            ))
            return 0;
    }

    if ((set->addressing_mode == AARCH64_CATALOG_ADDRESS_PRE_INDEX ||
         set->addressing_mode == AARCH64_CATALOG_ADDRESS_POST_INDEX) &&
        AARCH64OperandsFindField(form, set->encoding, "imm9", &value_a, 0)) {
        int64_t offset = AARCH64OperandsSignExtend(value_a, 9u);
        uint8_t flags = AARCH64_OPERAND_FLAG_SIGNED |
            AARCH64_OPERAND_FLAG_COMPOSITE |
            AARCH64_OPERAND_FLAG_WRITEBACK;
        if (set->addressing_mode == AARCH64_CATALOG_ADDRESS_POST_INDEX)
            flags |= AARCH64_OPERAND_FLAG_POST_INDEX;
        if (!AARCH64OperandsAddSynthetic(
                set, "indexed_memory_offset",
                AARCH64_CATALOG_OPERAND_IMMEDIATE,
                AARCH64_CATALOG_OPERAND_ROLE_OFFSET,
                value_a, (uint64_t)offset, offset, 9u, 0u, flags
            ))
            return 0;
    }

    if ((set->addressing_mode == AARCH64_CATALOG_ADDRESS_PAIR_OFFSET ||
         set->addressing_mode == AARCH64_CATALOG_ADDRESS_PAIR_PRE_INDEX ||
         set->addressing_mode == AARCH64_CATALOG_ADDRESS_PAIR_POST_INDEX) &&
        AARCH64OperandsFindField(form, set->encoding, "imm7", &value_a, 0)) {
        uint8_t field_width = 7u;
        uint8_t register_width =
            AARCH64OperandsGetRegisterWidth(form, set->encoding);
        uint8_t known_scale = 1u;
        uint8_t shift = register_width == 128u ? 4u :
            register_width == 64u ? 3u :
            register_width == 32u ? 2u : 0u;
        int64_t signed_immediate =
            AARCH64OperandsSignExtend(value_a, field_width);
        int64_t signed_offset;
        if (register_width == 0u) {
            if (AARCH64OperandsFindField(
                    form, set->encoding, "opc", &value_b, 0
                ) && value_b <= 2u)
                shift = (uint8_t)(2u + value_b);
            else
                known_scale = 0u;
        }
        signed_offset = known_scale
            ? signed_immediate * (int64_t)(1u << shift)
            : signed_immediate;
        if (!AARCH64OperandsAddSynthetic(
                set,
                "pair_memory_offset",
                AARCH64_CATALOG_OPERAND_IMMEDIATE,
                AARCH64_CATALOG_OPERAND_ROLE_OFFSET,
                value_a,
                (uint64_t)signed_offset,
                signed_offset,
                field_width,
                known_scale ? shift : 0u,
                AARCH64_OPERAND_FLAG_SIGNED |
                    AARCH64_OPERAND_FLAG_COMPOSITE |
                    (set->addressing_mode == AARCH64_CATALOG_ADDRESS_PAIR_OFFSET
                         ? 0u : AARCH64_OPERAND_FLAG_WRITEBACK) |
                    (set->addressing_mode == AARCH64_CATALOG_ADDRESS_PAIR_POST_INDEX
                         ? AARCH64_OPERAND_FLAG_POST_INDEX : 0u)
            ))
            return 0;
        if (!known_scale)
            set->operands[set->operand_count - 1u].status =
                AARCH64_OPERAND_VALUE_PARTIAL;
    }

    if (AARCH64OperandsFindField(form, set->encoding, "immhi", &value_a, 0) &&
        AARCH64OperandsFindField(form, set->encoding, "immlo", &value_b, 0) &&
        (AARCH64OperandsStringEquals(form->operation_id, "ADR") ||
         AARCH64OperandsStringEquals(form->operation_id, "ADRP"))) {
        int64_t displacement = AARCH64OperandsSignExtend(
            (value_a << 2) | value_b, 21u
        );
        uint8_t shift = AARCH64OperandsStringEquals(
            form->operation_id, "ADRP"
        ) ? 12u : 0u;
        int64_t scaled_displacement =
            displacement * (int64_t)(1u << shift);
        uint64_t offset = (uint64_t)scaled_displacement;
        if (!AARCH64OperandsAddSynthetic(
                set,
                "pc_relative_address",
                AARCH64_CATALOG_OPERAND_IMMEDIATE,
                AARCH64_CATALOG_OPERAND_ROLE_OFFSET,
                (value_a << 2) | value_b,
                offset,
                scaled_displacement,
                21u,
                shift,
                AARCH64_OPERAND_FLAG_SIGNED |
                    AARCH64_OPERAND_FLAG_PC_RELATIVE |
                    AARCH64_OPERAND_FLAG_COMPOSITE
            ))
            return 0;
        set->operands[set->operand_count - 1u].address =
            AARCH64OperandsStringEquals(form->operation_id, "ADRP")
            ? (set->instruction_address & ~0xFFFull) + offset
            : set->instruction_address + offset;
    }

    if (AARCH64OperandsConventionalBranch(form) &&
        AARCH64OperandsFormHasGroup(form, "testbranch") &&
        AARCH64OperandsFindField(form, set->encoding, "b5", &value_a, 0) &&
        AARCH64OperandsFindField(form, set->encoding, "b40", &value_b, 0) &&
        !AARCH64OperandsAddSynthetic(set, "bit_index",
            AARCH64_CATALOG_OPERAND_IMMEDIATE,
            AARCH64_CATALOG_OPERAND_ROLE_SOURCE,
            (value_a << 5) | value_b, (value_a << 5) | value_b,
            0, 6u, 0u, AARCH64_OPERAND_FLAG_COMPOSITE))
        return 0;

    if (form->family == AARCH64_CATALOG_FAMILY_CONTROL) {
        static const char *const system_groups[] = {
            "systemmove", "systemmovepr", "systeminstrs",
            "syspairinstrs", "barriers", "hints", "pstate",
            "systeminstrswithreg", "systemresult"
        };
        uint32_t group_index;
        uint8_t is_system = 0;

        for (group_index = 0u;
             group_index < sizeof(system_groups) / sizeof(system_groups[0]);
             group_index++) {
            if (AARCH64OperandsFormHasGroup(form, system_groups[group_index])) {
                is_system = 1u;
                break;
            }
        }

        if (is_system) {
            uint8_t op0 = (uint8_t)((set->encoding >> 19) & 3u);
            uint8_t op1 = (uint8_t)((set->encoding >> 16) & 7u);
            uint8_t crn = (uint8_t)((set->encoding >> 12) & 15u);
            uint8_t crm = (uint8_t)((set->encoding >> 8) & 15u);
            uint8_t op2 = (uint8_t)((set->encoding >> 5) & 7u);
            uint8_t rt = (uint8_t)(set->encoding & 31u);
            const AARCH64SysRegDescriptor *sysreg;

            set->system_op0 = op0;
            set->system_op1 = op1;
            set->system_crn = crn;
            set->system_crm = crm;
            set->system_op2 = op2;
            set->system_rt = rt;
            set->system_register =
                AARCH64SysRegEncode(op0, op1, crn, crm, op2);

            if (AARCH64OperandsFormHasGroup(form, "systemmove") ||
                AARCH64OperandsFormHasGroup(form, "systemmovepr") ||
                AARCH64OperandsFormHasGroup(form, "systeminstrs") ||
                AARCH64OperandsFormHasGroup(form, "syspairinstrs")) {
                uint8_t is_read;
                uint8_t is_sysl =
                    AARCH64OperandsFormHasGroup(form, "systeminstrs") &&
                    ((set->encoding >> 21) & 1u) != 0u;

                if (AARCH64OperandsFormHasGroup(form, "systemmovepr"))
                    is_read = 1u;
                else if (AARCH64OperandsFormHasGroup(form, "systemmove"))
                    is_read = (uint8_t)((set->encoding >> 21) & 1u);
                else
                    is_read = 0u;

                set->system_is_sysl = is_sysl;
                set->system_access = is_read
                    ? AARCH64_SYSREG_ACCESS_READ
                    : AARCH64_SYSREG_ACCESS_WRITE;

                sysreg = AARCH64SysRegResolve(op0, op1, crn, crm, op2);
                set->system_register_identity = sysreg->identity;
                set->system_register_name = sysreg->name;
                set->system_is_implementation_defined =
                    sysreg->is_implementation_defined;

                if (!AARCH64OperandsAddSynthetic(
                        set, "system_register_encoding",
                        AARCH64_CATALOG_OPERAND_SYSTEM_FIELD,
                        AARCH64_CATALOG_OPERAND_ROLE_CONTROL,
                        set->system_register, set->system_register,
                        0, 16u, 0, AARCH64_OPERAND_FLAG_COMPOSITE))
                    return 0;
            }

            if (AARCH64OperandsFormHasGroup(form, "barriers")) {
                uint8_t opc = (uint8_t)((set->encoding >> 5) & 3u);
                uint8_t option = (uint8_t)((set->encoding >> 8) & 15u);

                set->barrier_type = opc;
                set->barrier_domain = option;
            }

            if (AARCH64OperandsFormHasGroup(form, "hints")) {
                set->hint_immediate = crm;
            }

            if (AARCH64OperandsFormHasGroup(form, "pstate")) {
                set->pstate_field = crm;
            }
        }
    }

    if (AARCH64OperandsFormHasGroup(form, "hints") &&
        (AARCH64OperandsStringStartsWithIgnoreCase(form->id, "PAC") ||
         AARCH64OperandsStringStartsWithIgnoreCase(form->id, "AUT") ||
         AARCH64OperandsStringStartsWithIgnoreCase(form->id, "XPAC")) &&
        !AARCH64OperandsStringStartsWithIgnoreCase(form->id, "PACM_")) {
        const char *name = form->mnemonic;
        uint8_t destination = AARCH64OperandsStringEndsWith(name, "1716")
            ? 17u : 30u;
        uint8_t has_modifier = !AARCH64OperandsStringEquals(name, "XPACLRI");
        uint8_t modifier = AARCH64OperandsStringEndsWith(name, "1716")
            ? 16u : 31u;
        AARCH64Operand *implicit;

        if (!AARCH64OperandsAddSynthetic(set, "implicit_destination",
                AARCH64_CATALOG_OPERAND_GPR,
                AARCH64_CATALOG_OPERAND_ROLE_DESTINATION,
                destination, destination, 0, 64u, 0,
                AARCH64_OPERAND_FLAG_COMPOSITE))
            return 0;
        implicit = &set->operands[set->operand_count - 1u];
        implicit->register_index = destination;
        implicit->register_width = 64u;
        implicit->flags |= AARCH64_OPERAND_FLAG_IMPLICIT_REGISTER;

        if (has_modifier) {
            if (!AARCH64OperandsAddSynthetic(set, "implicit_modifier",
                    AARCH64_CATALOG_OPERAND_GPR,
                    AARCH64_CATALOG_OPERAND_ROLE_SOURCE,
                    modifier, modifier, 0, 64u, 0,
                    AARCH64_OPERAND_FLAG_COMPOSITE))
                return 0;
            implicit = &set->operands[set->operand_count - 1u];
            implicit->register_index = modifier;
            implicit->register_width = 64u;
            implicit->flags |= AARCH64_OPERAND_FLAG_IMPLICIT_REGISTER;
            if (modifier == 31u)
                implicit->register_special =
                    AARCH64OperandsStringEndsWith(name, "SP")
                    ? AARCH64_REGISTER_SPECIAL_SP
                    : AARCH64_REGISTER_SPECIAL_ZERO;
        }
    }

    if (AARCH64OperandsFormHasGroup(form, "exception")) {
        uint16_t imm16 = (uint16_t)((set->encoding >> 5) & 0xFFFFu);

        set->exception_immediate = imm16;

        if (form->operand_descriptor_count == 0u &&
            !AARCH64OperandsAddSynthetic(
                set, "exception_immediate",
                AARCH64_CATALOG_OPERAND_IMMEDIATE,
                AARCH64_CATALOG_OPERAND_ROLE_SOURCE,
                imm16, imm16, 0, 16u, 0,
                AARCH64_OPERAND_FLAG_COMPOSITE))
            return 0;
    }

    if (AARCH64OperandsStringEquals(form->operation_id, "PACGA")) {
        set->pointer_auth_key = AARCH64_POINTER_AUTH_KEY_GA;
        set->pointer_auth_action = AARCH64_POINTER_AUTH_ACTION_SIGN;

        if (form->operand_descriptor_count == 0u) {
            uint8_t rd = (uint8_t)(set->encoding & 31u);
            uint8_t rn = (uint8_t)((set->encoding >> 5) & 31u);
            uint8_t rm = (uint8_t)((set->encoding >> 16) & 31u);

            if (!AARCH64OperandsAddSynthetic(
                    set, "Rd",
                    AARCH64_CATALOG_OPERAND_GPR,
                    AARCH64_CATALOG_OPERAND_ROLE_DESTINATION,
                    rd, rd, 0, 64u, 0,
                    AARCH64_OPERAND_FLAG_COMPOSITE) ||
                !AARCH64OperandsAddSynthetic(
                    set, "Rn",
                    AARCH64_CATALOG_OPERAND_GPR,
                    AARCH64_CATALOG_OPERAND_ROLE_SOURCE,
                    rn, rn, 0, 64u, 0,
                    AARCH64_OPERAND_FLAG_COMPOSITE) ||
                !AARCH64OperandsAddSynthetic(
                    set, "Rm",
                    AARCH64_CATALOG_OPERAND_GPR,
                    AARCH64_CATALOG_OPERAND_ROLE_SOURCE,
                    rm, rm, 0, 64u, 0,
                    AARCH64_OPERAND_FLAG_COMPOSITE))
                return 0;
        }
    }

    if (AARCH64OperandsFormHasGroup(form, "ldst_pac")) {
        uint8_t rt = (uint8_t)(set->encoding & 31u);
        uint8_t rn = (uint8_t)((set->encoding >> 5) & 31u);
        uint8_t writeback = (set->encoding & (1u << 11)) != 0u;
        int64_t simm10 = AARCH64OperandsSignExtend(
            ((set->encoding >> 12) & 0x1FFu) |
            (((set->encoding >> 22) & 1u) << 9), 10);
        uint64_t offset = (uint64_t)(simm10 * 8);

        set->memory_data_register = rt;
        set->memory_base_register = rn;
        set->memory_offset = offset;
        set->memory_writeback = writeback;
        set->memory_access_width = 64u;
        set->pointer_auth_key = (set->encoding & (1u << 23))
            ? AARCH64_POINTER_AUTH_KEY_DB : AARCH64_POINTER_AUTH_KEY_DA;
        set->pointer_auth_action = AARCH64_POINTER_AUTH_ACTION_LOAD;
        set->pointer_auth_address_diversity = 1u;

        if (!AARCH64OperandsAddSynthetic(
                set, "auth_offset", AARCH64_CATALOG_OPERAND_IMMEDIATE,
                AARCH64_CATALOG_OPERAND_ROLE_OFFSET,
                ((set->encoding >> 12) & 0x1FFu) |
                    (((set->encoding >> 22) & 1u) << 9),
                offset, simm10 * 8, 10u, 8u,
                AARCH64_OPERAND_FLAG_COMPOSITE |
                    AARCH64_OPERAND_FLAG_SIGNED))
            return 0;

        if (form->operand_descriptor_count == 0u &&
            (!AARCH64OperandsAddSynthetic(
                set, "Rt",
                AARCH64_CATALOG_OPERAND_GPR,
                AARCH64_CATALOG_OPERAND_ROLE_DESTINATION,
                rt, rt, 0, 64u, 0,
                AARCH64_OPERAND_FLAG_COMPOSITE) ||
            !AARCH64OperandsAddSynthetic(
                set, "Rn",
                AARCH64_CATALOG_OPERAND_GPR,
                AARCH64_CATALOG_OPERAND_ROLE_SOURCE,
                rn, rn, 0, 64u, 0,
                AARCH64_OPERAND_FLAG_COMPOSITE) ||
            !AARCH64OperandsAddSynthetic(
                set, "simm10",
                AARCH64_CATALOG_OPERAND_IMMEDIATE,
                AARCH64_CATALOG_OPERAND_ROLE_SOURCE,
                ((set->encoding >> 12) & 0x1FFu) |
                    (((set->encoding >> 22) & 1u) << 9), offset,
                simm10 * 8, 10u, 8,
                AARCH64_OPERAND_FLAG_COMPOSITE)))
            return 0;
    }

    if (AARCH64OperandsStringEquals(form->operation_id, "IRG") ||
        AARCH64OperandsStringEquals(form->operation_id, "GMI") ||
        AARCH64OperandsStringEquals(form->operation_id, "SUBP") ||
        AARCH64OperandsStringEquals(form->operation_id, "SUBPS")) {
        set->memory_tagged = 1u;
        set->memory_tag_granule_bytes = 16u;

        if (form->operand_descriptor_count == 0u) {
            uint8_t rd = (uint8_t)(set->encoding & 31u);
            uint8_t rn = (uint8_t)((set->encoding >> 5) & 31u);
            uint8_t rm = (uint8_t)((set->encoding >> 16) & 31u);

            if (!AARCH64OperandsAddSynthetic(
                    set, "Rd",
                    AARCH64_CATALOG_OPERAND_GPR,
                    AARCH64_CATALOG_OPERAND_ROLE_DESTINATION,
                    rd, rd, 0, 64u, 0,
                    AARCH64_OPERAND_FLAG_COMPOSITE) ||
                !AARCH64OperandsAddSynthetic(
                    set, "Rn",
                    AARCH64_CATALOG_OPERAND_GPR,
                    AARCH64_CATALOG_OPERAND_ROLE_SOURCE,
                    rn, rn, 0, 64u, 0,
                    AARCH64_OPERAND_FLAG_COMPOSITE) ||
                !AARCH64OperandsAddSynthetic(
                    set, "Rm",
                    AARCH64_CATALOG_OPERAND_GPR,
                    AARCH64_CATALOG_OPERAND_ROLE_SOURCE,
                    rm, rm, 0, 64u, 0,
                    AARCH64_OPERAND_FLAG_COMPOSITE))
                return 0;
        }
    }

    if (AARCH64OperandsStringEquals(form->operation_id, "ADDG") ||
        AARCH64OperandsStringEquals(form->operation_id, "SUBG")) {
        set->memory_tagged = 1u;
        set->memory_tag_granule_bytes = 16u;

        if (form->operand_descriptor_count == 0u) {
            uint8_t rd = (uint8_t)(set->encoding & 31u);
            uint8_t rn = (uint8_t)((set->encoding >> 5) & 31u);
            uint8_t uimm6 = (uint8_t)((set->encoding >> 10) & 63u);
            uint8_t uimm4 = (uint8_t)((set->encoding >> 22) & 15u);

            if (!AARCH64OperandsAddSynthetic(
                    set, "Rd",
                    AARCH64_CATALOG_OPERAND_GPR,
                    AARCH64_CATALOG_OPERAND_ROLE_DESTINATION,
                    rd, rd, 0, 64u, 0,
                AARCH64_OPERAND_FLAG_COMPOSITE) ||
            !AARCH64OperandsAddSynthetic(
                set, "Rn",
                AARCH64_CATALOG_OPERAND_GPR,
                AARCH64_CATALOG_OPERAND_ROLE_SOURCE,
                rn, rn, 0, 64u, 0,
                AARCH64_OPERAND_FLAG_COMPOSITE) ||
            !AARCH64OperandsAddSynthetic(
                set, "uimm6",
                AARCH64_CATALOG_OPERAND_IMMEDIATE,
                AARCH64_CATALOG_OPERAND_ROLE_SOURCE,
                uimm6, uimm6, 0, 6u, 0,
                AARCH64_OPERAND_FLAG_COMPOSITE) ||
            !AARCH64OperandsAddSynthetic(
                set, "uimm4",
                AARCH64_CATALOG_OPERAND_IMMEDIATE,
                AARCH64_CATALOG_OPERAND_ROLE_SOURCE,
                uimm4, uimm4, 0, 4u, 0,
                AARCH64_OPERAND_FLAG_COMPOSITE))
                return 0;
        }
    }

    return 1;
}

AARCH64OperandExtractionStatus AARCH64OperandsExtractWithProfile(
    const AARCH64CatalogMatch *match,
    AARCH64CatalogResult catalog_status,
    uint32_t encoding,
    uint64_t instruction_address,
    const AARCH64CatalogGuestProfile *profile,
    AARCH64OperandSet *operands
)
{
    uint16_t index;
    uint8_t saw_raw = 0;
    uint8_t saw_interpreted = 0;
    uint8_t saw_partial = 0;
    AARCH64CatalogResult alias_result;
    static const char *const shift_names[] = {
        "LSL", "LSR", "ASR", "ROR"
    };
    static const char *const extend_names[] = {
        "UXTB", "UXTH", "UXTW", "UXTX",
        "SXTB", "SXTH", "SXTW", "SXTX"
    };
    const AARCH64CatalogForm *form;

    if (operands == 0)
        return AARCH64_OPERAND_EXTRACTION_INVALID;
    AARCH64OperandsClear(operands, sizeof(*operands));
    operands->status = AARCH64_OPERAND_EXTRACTION_INVALID;
    operands->catalog_status = catalog_status;

    if (match == 0 || match->form == 0 || match->pattern == 0 ||
        (catalog_status != AARCH64_CATALOG_VALID_ENCODING &&
         catalog_status != AARCH64_CATALOG_PATTERN_MATCHED_UNVALIDATED &&
         catalog_status != AARCH64_CATALOG_EXTENSION_UNAVAILABLE) ||
        !match->raw_pattern_matched ||
        (encoding & match->pattern->mask) != match->pattern->value)
        return operands->status;

    form = match->form;
    if (catalog_status == AARCH64_CATALOG_PATTERN_MATCHED_UNVALIDATED &&
        AARCH64OperandsFormIsSystemOrSecurity(form) &&
        !AARCH64OperandsFormIsSystemGroup(form))
        return operands->status;
    if (!AARCH64CatalogFormEncodingValid(form, encoding))
        return operands->status;
    if (!AARCH64OperandsSpecializedMemoryEncodingValid(form, encoding))
        return operands->status;
    if (form->operand_descriptor_count == 0u ||
        form->operand_descriptors == 0) {
        if (!AARCH64OperandsFormIsSystemOrSecurity(form)) {
            operands->status = AARCH64_OPERAND_EXTRACTION_UNSUPPORTED;
            return AARCH64_OPERAND_EXTRACTION_UNSUPPORTED;
        }
    }
    if (form->operand_descriptor_count > AARCH64_OPERANDS_MAX_COUNT) {
        operands->status = AARCH64_OPERAND_EXTRACTION_UNSUPPORTED;
        return AARCH64_OPERAND_EXTRACTION_UNSUPPORTED;
    }

    operands->encoding = encoding;
    operands->instruction_address = instruction_address;
    operands->form = form;
    operands->canonical_mnemonic = form->mnemonic;
    operands->operation_id = form->operation_id;
    operands->family = form->family;
    operands->addressing_mode = form->addressing_mode;
    operands->status = AARCH64_OPERAND_EXTRACTION_COMPLETE;
    operands->operand_count = form->operand_descriptor_count;

    for (index = 0; index < form->operand_descriptor_count; index++) {
        const AARCH64CatalogOperandDescriptor *descriptor =
            &form->operand_descriptors[index];
        AARCH64Operand *operand = &operands->operands[index];
        uint32_t mask;

        if (descriptor->width == 0u || descriptor->width > 32u ||
            descriptor->lsb + descriptor->width > 32u ||
            descriptor->symbol_index >= AARCH64CatalogSymbolCount()) {
            operands->status = AARCH64_OPERAND_EXTRACTION_INVALID;
            return operands->status;
        }

        mask = descriptor->width == 32u
            ? UINT32_MAX
            : ((1u << descriptor->width) - 1u);
        operand->name =
            AARCH64CatalogSymbolName(descriptor->symbol_index);
        operand->symbol_index = descriptor->symbol_index;
        operand->kind = descriptor->kind;
        operand->role = descriptor->role;
        operand->field_width = descriptor->width;
        operand->raw_value =
            (encoding >> descriptor->lsb) & mask;
        operand->value = operand->raw_value;
        operand->status = AARCH64_OPERAND_VALUE_RAW;

        if (descriptor->kind == AARCH64_CATALOG_OPERAND_GPR) {
            uint64_t s_value = 0;

            operand->register_index = (uint8_t)operand->raw_value;
            operand->register_width = descriptor->register_width;
            if (AARCH64OperandsConventionalBranch(form)) {
                if (AARCH64OperandsFormHasGroup(form, "branch_reg"))
                    operand->register_width = 64u;
                else if (AARCH64OperandsFormHasGroup(form, "testbranch"))
                    operand->register_width = (encoding >> 31) != 0u
                        ? 64u : 32u;
                else if (AARCH64OperandsFormHasGroup(form,
                             "compbranch_regs2"))
                    operand->register_width = 32u;
                if (AARCH64OperandsStringEquals(operand->name, "Rt"))
                    operand->role = AARCH64_CATALOG_OPERAND_ROLE_SOURCE;
                if (AARCH64OperandsFormHasGroup(form, "branch_reg") &&
                    AARCH64OperandsStringEquals(operand->name, "Rm")) {
                    operand->kind = AARCH64_CATALOG_OPERAND_RAW;
                    operand->role = AARCH64_CATALOG_OPERAND_ROLE_FIELD;
                    operand->flags |= AARCH64_OPERAND_FLAG_ENCODING_SELECTOR;
                }
            }
            if (AARCH64OperandsExceptionalControl(form) &&
                AARCH64OperandsFormHasGroup(form, "branch_reg")) {
                uint8_t branch_target =
                    AARCH64OperandsStringEquals(form->operation_id, "BRA") ||
                    AARCH64OperandsStringEquals(form->operation_id, "BLRA");
                uint8_t modifier_register =
                    (branch_target && (encoding & (1u << 24)) != 0u) ||
                    AARCH64OperandsStringEquals(form->operation_id,
                                                "RETASPPCR_reg");
                operand->register_width = 64u;
                if ((AARCH64OperandsStringEquals(operand->name, "Rn") &&
                     !branch_target) ||
                    (AARCH64OperandsStringEquals(operand->name, "Rm") &&
                     !modifier_register)) {
                    operand->kind = AARCH64_CATALOG_OPERAND_RAW;
                    operand->role = AARCH64_CATALOG_OPERAND_ROLE_FIELD;
                    operand->flags |= AARCH64_OPERAND_FLAG_ENCODING_SELECTOR;
                }
            }
            if (form->family == AARCH64_CATALOG_FAMILY_DP_IMMEDIATE ||
                form->family == AARCH64_CATALOG_FAMILY_DP_REGISTER)
                operand->register_width = AARCH64OperandsIntegerRegisterWidth(
                    form, encoding, operand->name);
            if (AARCH64OperandsConventionalMemory(form) ||
                AARCH64OperandsSpecializedMemory(form) ||
                AARCH64OperandsForwardedMemory(form))
                operand->register_width = AARCH64OperandsMemoryRegisterWidth(
                    form, encoding, operand->name);
            if (AARCH64OperandsSpecializedMemory(form) &&
                !AARCH64OperandsConventionalMemory(form)) {
                if ((AARCH64OperandsStringEquals(operand->name, "Rt") ||
                     AARCH64OperandsStringEquals(operand->name, "Rt2")) &&
                    AARCH64OperandsFPDataWidth(form, encoding) != 0u) {
                    operand->kind = AARCH64_CATALOG_OPERAND_FP_SIMD;
                    operand->register_width =
                        AARCH64OperandsFPDataWidth(form, encoding);
                    operand->role =
                        form->operation_id[0] == 'S'
                        ? AARCH64_CATALOG_OPERAND_ROLE_SOURCE
                        : AARCH64_CATALOG_OPERAND_ROLE_DESTINATION;
                } else if (AARCH64OperandsFormHasGroup(form, "ldsttags") &&
                           (AARCH64OperandsStringEquals(operand->name, "Rt") ||
                            AARCH64OperandsStringEquals(operand->name, "Rn")))
                    operand->register_width = 64u;
                else if ((AARCH64OperandsFormHasGroup(form, "memop_unpriv") ||
                          AARCH64OperandsFormHasGroup(form, "ldst_unpriv")) &&
                         !AARCH64OperandsStringEquals(operand->name, "Rn")) {
                    const char *part = form->id;
                    while (*part != '\0' && *part != '_')
                        part++;
                    if (*part == '_')
                        part++;
                    operand->register_width =
                        part[0] == '6' && part[1] == '4' ? 64u : 32u;
                }
                if ((AARCH64OperandsAdvSIMDMemory(form) ||
                     AARCH64OperandsStringStartsWithIgnoreCase(form->id,
                         "RPRFM_")) &&
                    AARCH64OperandsStringEquals(operand->name, "Rm"))
                    operand->register_width = 64u;
            }
            operand->status = AARCH64_OPERAND_VALUE_INTERPRETED;
            if (operand->register_index == 31u) {
                if (descriptor->role == AARCH64_CATALOG_OPERAND_ROLE_BASE) {
                    operand->register_special = AARCH64_REGISTER_SPECIAL_SP;
                } else if ((descriptor->flags &
                            AARCH64_CATALOG_OPERAND_SP_OR_ZERO) != 0u) {
                    if (AARCH64OperandsStringEquals(operand->name, "Rn"))
                        operand->register_special =
                            AARCH64_REGISTER_SPECIAL_SP;
                    else if (AARCH64OperandsFindField(
                                 form, encoding, "S", &s_value, 0
                             ) && s_value != 0u)
                        operand->register_special =
                            AARCH64_REGISTER_SPECIAL_ZERO;
                    else
                        operand->register_special =
                            AARCH64_REGISTER_SPECIAL_SP;
                } else {
                    operand->register_special =
                        AARCH64_REGISTER_SPECIAL_ZERO;
                }
            }
            if (form->family == AARCH64_CATALOG_FAMILY_DP_REGISTER &&
                AARCH64OperandsFormHasGroup(form, "addsub_ext") &&
                AARCH64OperandsStringEquals(operand->name, "Rn") &&
                operand->register_index == 31u)
                operand->register_special = AARCH64_REGISTER_SPECIAL_SP;
            if (form->family == AARCH64_CATALOG_FAMILY_DP_IMMEDIATE &&
                AARCH64OperandsFormHasGroup(form, "addsub_immtags") &&
                operand->register_index == 31u &&
                (AARCH64OperandsStringEquals(operand->name, "Rn") ||
                 AARCH64OperandsStringEquals(operand->name, "Rd")))
                operand->register_special = AARCH64_REGISTER_SPECIAL_SP;
            if (operand->register_index == 31u &&
                form->family == AARCH64_CATALOG_FAMILY_DP_IMMEDIATE &&
                AARCH64OperandsFormHasGroup(form, "log_imm") &&
                AARCH64OperandsStringEquals(operand->name, "Rd") &&
                !AARCH64OperandsStringStartsWithIgnoreCase(form->id, "ANDS_"))
                operand->register_special = AARCH64_REGISTER_SPECIAL_SP;
            if (operand->register_index == 31u &&
                form->family == AARCH64_CATALOG_FAMILY_DP_REGISTER &&
                AARCH64OperandsFormHasGroup(form, "addsub_pt") &&
                (AARCH64OperandsStringEquals(operand->name, "Rd") ||
                 AARCH64OperandsStringEquals(operand->name, "Rn")))
                operand->register_special = AARCH64_REGISTER_SPECIAL_SP;
            if (form->family == AARCH64_CATALOG_FAMILY_DP_REGISTER &&
                ((AARCH64OperandsFormHasGroup(form, "dp_1src") &&
                  AARCH64OperandsStringEquals(operand->name, "Rn") &&
                  (AARCH64OperandsStringStartsWithIgnoreCase(form->id, "XPACI_") ||
                   AARCH64OperandsStringStartsWithIgnoreCase(form->id, "XPACD_") ||
                   AARCH64OperandsStringStartsWithIgnoreCase(form->id, "PACIZA_") ||
                   AARCH64OperandsStringStartsWithIgnoreCase(form->id, "PACIZB_") ||
                   AARCH64OperandsStringStartsWithIgnoreCase(form->id, "PACDZA_") ||
                   AARCH64OperandsStringStartsWithIgnoreCase(form->id, "PACDZB_") ||
                   AARCH64OperandsStringStartsWithIgnoreCase(form->id, "AUTIZA_") ||
                   AARCH64OperandsStringStartsWithIgnoreCase(form->id, "AUTIZB_") ||
                   AARCH64OperandsStringStartsWithIgnoreCase(form->id, "AUTDZA_") ||
                   AARCH64OperandsStringStartsWithIgnoreCase(form->id, "AUTDZB_"))) ||
                 (AARCH64OperandsFormHasGroup(form, "dp_1src") &&
                  (AARCH64OperandsStringEquals(operand->name, "Rn") ||
                   AARCH64OperandsStringEquals(operand->name, "Rd")) &&
                  (AARCH64OperandsStringStartsWithIgnoreCase(form->id, "PACIA171615_") ||
                   AARCH64OperandsStringStartsWithIgnoreCase(form->id, "PACIB171615_") ||
                   AARCH64OperandsStringStartsWithIgnoreCase(form->id, "AUTIA171615_") ||
                   AARCH64OperandsStringStartsWithIgnoreCase(form->id, "AUTIB171615_") ||
                   AARCH64OperandsStringStartsWithIgnoreCase(form->id, "PACIASPPC_") ||
                   AARCH64OperandsStringStartsWithIgnoreCase(form->id, "PACIBSPPC_") ||
                   AARCH64OperandsStringStartsWithIgnoreCase(form->id, "PACNBIASPPC_") ||
                   AARCH64OperandsStringStartsWithIgnoreCase(form->id, "PACNBIBSPPC_"))) ||
                 (AARCH64OperandsFormHasGroup(form, "dp_1src") &&
                  AARCH64OperandsStringEquals(operand->name, "Rd") &&
                  (AARCH64OperandsStringStartsWithIgnoreCase(form->id, "AUTIASPPCR_") ||
                   AARCH64OperandsStringStartsWithIgnoreCase(form->id, "AUTIBSPPCR_"))) ||
                 (AARCH64OperandsFormHasGroup(form, "dp_3src") &&
                  AARCH64OperandsStringEquals(operand->name, "Ra") &&
                  (AARCH64OperandsStringStartsWithIgnoreCase(form->id, "SMULH_") ||
                   AARCH64OperandsStringStartsWithIgnoreCase(form->id, "UMULH_")))))
                operand->flags |= AARCH64_OPERAND_FLAG_IMPLICIT_REGISTER;
            if (form->family == AARCH64_CATALOG_FAMILY_DP_IMMEDIATE &&
                AARCH64OperandsFormHasGroup(form, "dp_1src_imm") &&
                AARCH64OperandsStringEquals(operand->name, "Rd"))
                operand->flags |= AARCH64_OPERAND_FLAG_IMPLICIT_REGISTER;
            if (operand->register_index == 31u &&
                form->family == AARCH64_CATALOG_FAMILY_DP_REGISTER &&
                ((AARCH64OperandsStringStartsWithIgnoreCase(form->id, "IRG_") &&
                  (AARCH64OperandsStringEquals(operand->name, "Rd") ||
                   AARCH64OperandsStringEquals(operand->name, "Rn"))) ||
                 ((AARCH64OperandsStringStartsWithIgnoreCase(form->id, "GMI_") ||
                   AARCH64OperandsStringStartsWithIgnoreCase(form->id, "SUBP_") ||
                   AARCH64OperandsStringStartsWithIgnoreCase(form->id, "SUBPS_")) &&
                  AARCH64OperandsStringEquals(operand->name, "Rn")) ||
                 ((AARCH64OperandsStringStartsWithIgnoreCase(form->id, "SUBP_") ||
                   AARCH64OperandsStringStartsWithIgnoreCase(form->id, "SUBPS_")) &&
                  AARCH64OperandsStringEquals(operand->name, "Rm")) ||
                 (AARCH64OperandsStringStartsWithIgnoreCase(form->id, "PACGA_") &&
                  AARCH64OperandsStringEquals(operand->name, "Rm")) ||
                 ((AARCH64OperandsStringStartsWithIgnoreCase(form->id, "PACIA_") ||
                   AARCH64OperandsStringStartsWithIgnoreCase(form->id, "PACIB_") ||
                   AARCH64OperandsStringStartsWithIgnoreCase(form->id, "PACDA_") ||
                   AARCH64OperandsStringStartsWithIgnoreCase(form->id, "PACDB_") ||
                   AARCH64OperandsStringStartsWithIgnoreCase(form->id, "AUTIA_") ||
                   AARCH64OperandsStringStartsWithIgnoreCase(form->id, "AUTIB_") ||
                   AARCH64OperandsStringStartsWithIgnoreCase(form->id, "AUTDA_") ||
                   AARCH64OperandsStringStartsWithIgnoreCase(form->id, "AUTDB_")) &&
                  AARCH64OperandsStringEquals(operand->name, "Rn"))))
                operand->register_special = AARCH64_REGISTER_SPECIAL_SP;
            if (AARCH64OperandsConventionalMemory(form) &&
                AARCH64OperandsStringStartsWithIgnoreCase(form->id, "PRFM_") &&
                AARCH64OperandsStringEquals(operand->name, "Rt")) {
                operand->kind = AARCH64_CATALOG_OPERAND_IMMEDIATE;
                operand->role = AARCH64_CATALOG_OPERAND_ROLE_CONTROL;
                operand->register_width = 0u;
                operand->register_special = AARCH64_REGISTER_SPECIAL_NONE;
            }
            if (operand->kind == AARCH64_CATALOG_OPERAND_FP_SIMD)
                operand->register_special = AARCH64_REGISTER_SPECIAL_NONE;
            if (AARCH64OperandsAdvSIMDMemory(form) &&
                AARCH64OperandsStringEquals(operand->name, "Rm") &&
                operand->register_index == 31u) {
                operand->kind = AARCH64_CATALOG_OPERAND_RAW;
                operand->role = AARCH64_CATALOG_OPERAND_ROLE_FIELD;
                operand->register_special = AARCH64_REGISTER_SPECIAL_NONE;
                operand->flags |= AARCH64_OPERAND_FLAG_ENCODING_SELECTOR;
            }
            if (AARCH64OperandsSpecializedMemory(form) &&
                (AARCH64OperandsStringStartsWithIgnoreCase(form->id, "RPRFM_") ||
                 AARCH64OperandsStringStartsWithIgnoreCase(form->id, "PRFUM_")) &&
                AARCH64OperandsStringEquals(operand->name, "Rt")) {
                operand->kind = AARCH64_CATALOG_OPERAND_IMMEDIATE;
                operand->role = AARCH64_CATALOG_OPERAND_ROLE_CONTROL;
                operand->register_width = 0u;
                operand->register_special = AARCH64_REGISTER_SPECIAL_NONE;
                if (AARCH64OperandsStringStartsWithIgnoreCase(form->id,
                        "RPRFM_"))
                    operand->value = operand->raw_value & 7u;
            }
            if (AARCH64OperandsFormIsSystemOrSecurity(form) &&
                operand->kind == AARCH64_CATALOG_OPERAND_GPR &&
                operand->register_width == 0u)
                operand->register_width = 64u;
            if (operand->kind == AARCH64_CATALOG_OPERAND_GPR &&
                operand->register_width == 0u)
                operand->status = AARCH64_OPERAND_VALUE_PARTIAL;
        } else if (descriptor->kind == AARCH64_CATALOG_OPERAND_FP_SIMD ||
                   descriptor->kind == AARCH64_CATALOG_OPERAND_SVE_VECTOR ||
                   descriptor->kind == AARCH64_CATALOG_OPERAND_SME_TILE ||
                   descriptor->kind == AARCH64_CATALOG_OPERAND_PREDICATE) {
            operand->register_index = (uint8_t)operand->raw_value;
            operand->status = AARCH64_OPERAND_VALUE_PARTIAL;
            if (form->family == AARCH64_CATALOG_FAMILY_LOAD_STORE &&
                (AARCH64OperandsSpecializedMemory(form) ||
                 AARCH64OperandsConventionalMemory(form)))
                operand->status = AARCH64_OPERAND_VALUE_INTERPRETED;
            if (AARCH64OperandsStringEquals(operand->name, "Z") &&
                form->family == AARCH64_CATALOG_FAMILY_DP_REGISTER) {
                operand->status = AARCH64_OPERAND_VALUE_INTERPRETED;
                operand->kind = AARCH64_CATALOG_OPERAND_RAW;
                operand->flags |= AARCH64_OPERAND_FLAG_ENCODING_SELECTOR;
            }
        } else if (descriptor->kind ==
                   AARCH64_CATALOG_OPERAND_CONDITION) {
            operand->status = AARCH64_OPERAND_VALUE_INTERPRETED;
        } else if (descriptor->kind ==
                   AARCH64_CATALOG_OPERAND_IMMEDIATE &&
                   (descriptor->flags &
                    AARCH64_CATALOG_OPERAND_SIGNED) != 0u) {
            operand->signed_value = AARCH64OperandsSignExtend(
                operand->raw_value, descriptor->width
            );
            operand->value = (uint64_t)operand->signed_value;
            operand->status = AARCH64_OPERAND_VALUE_INTERPRETED;
            if ((descriptor->flags &
                  AARCH64_CATALOG_OPERAND_PC_RELATIVE) != 0u) {
                operand->scale = 4u;
                operand->value = (uint64_t)(operand->signed_value * 4);
                operand->signed_value *= 4;
                operand->address =
                    instruction_address + operand->value;
                operand->flags |= AARCH64_OPERAND_FLAG_PC_RELATIVE;
            }
            if (AARCH64OperandsConventionalBranch(form)) {
                operand->role = AARCH64_CATALOG_OPERAND_ROLE_OFFSET;
                operand->flags |= AARCH64_OPERAND_FLAG_SIGNED;
            }
        } else if (descriptor->kind ==
                   AARCH64_CATALOG_OPERAND_IMMEDIATE) {
            operand->status = AARCH64_OPERAND_VALUE_INTERPRETED;
            if (AARCH64OperandsExceptionalControl(form) &&
                AARCH64OperandsStringEquals(operand->name, "imm16"))
                operand->role =
                    AARCH64OperandsFormHasGroup(form, "exception")
                    ? AARCH64_CATALOG_OPERAND_ROLE_CONTROL
                    : AARCH64_CATALOG_OPERAND_ROLE_SOURCE;
            if (AARCH64OperandsConventionalBranch(form) &&
                ((AARCH64OperandsStringEquals(operand->name, "imm9") &&
                  (AARCH64OperandsFormHasGroup(form, "compbranch_imm") ||
                   AARCH64OperandsFormHasGroup(form, "compbranch_regs") ||
                   AARCH64OperandsFormHasGroup(form, "compbranch_regs2"))) ||
                 (AARCH64OperandsStringEquals(operand->name, "imm19") &&
                  AARCH64OperandsFormHasGroup(form, "compbranch")))) {
                uint8_t bits = AARCH64OperandsStringEquals(
                    operand->name, "imm9") ? 9u : 19u;
                operand->signed_value =
                    AARCH64OperandsSignExtend(operand->raw_value, bits) * 4;
                operand->value = (uint64_t)operand->signed_value;
                operand->address = instruction_address + operand->value;
                operand->scale = 4u;
                operand->role = AARCH64_CATALOG_OPERAND_ROLE_OFFSET;
                operand->flags |= AARCH64_OPERAND_FLAG_SIGNED |
                    AARCH64_OPERAND_FLAG_PC_RELATIVE;
            }
            if (AARCH64OperandsConventionalBranch(form) &&
                AARCH64OperandsFormHasGroup(form, "compbranch_imm") &&
                AARCH64OperandsStringEquals(operand->name, "imm6"))
                operand->role = AARCH64_CATALOG_OPERAND_ROLE_SOURCE;
            if (form->family == AARCH64_CATALOG_FAMILY_DP_IMMEDIATE &&
                AARCH64OperandsFormHasGroup(form, "addsub_immtags") &&
                AARCH64OperandsStringEquals(operand->name, "imm6")) {
                operand->value = operand->raw_value << 4;
                operand->scale = 4u;
            }
            if (form->family == AARCH64_CATALOG_FAMILY_DP_IMMEDIATE &&
                AARCH64OperandsFormHasGroup(form, "dp_1src_imm") &&
                AARCH64OperandsStringEquals(operand->name, "imm16")) {
                operand->signed_value = -(int64_t)(operand->raw_value << 2);
                operand->value = (uint64_t)operand->signed_value;
                operand->scale = 2u;
                operand->flags |= AARCH64_OPERAND_FLAG_SIGNED;
            }
            if (form->family == AARCH64_CATALOG_FAMILY_DP_IMMEDIATE &&
                AARCH64OperandsFormHasGroup(form, "minmax_imm") &&
                AARCH64OperandsStringEquals(operand->name, "imm8") &&
                (AARCH64OperandsStringStartsWithIgnoreCase(form->id, "SMAX_") ||
                 AARCH64OperandsStringStartsWithIgnoreCase(form->id, "SMIN_"))) {
                operand->signed_value = AARCH64OperandsSignExtend(
                    operand->raw_value, 8u);
                operand->value = (uint64_t)operand->signed_value;
                operand->flags |= AARCH64_OPERAND_FLAG_SIGNED;
            }
        } else if (descriptor->kind == AARCH64_CATALOG_OPERAND_SHIFT) {
            if (AARCH64OperandsStringEquals(operand->name, "sh") &&
                AARCH64OperandsFormHasGroup(form, "addsub_imm")) {
                operand->modifier_name = "LSL";
                operand->scale = operand->raw_value != 0u ? 12u : 0u;
                operand->value = operand->scale;
                operand->status = AARCH64_OPERAND_VALUE_INTERPRETED;
            } else if (operand->raw_value <
                       sizeof(shift_names) / sizeof(shift_names[0])) {
                operand->modifier_name = shift_names[operand->raw_value];
                operand->status = AARCH64_OPERAND_VALUE_INTERPRETED;
            } else {
                operand->status = AARCH64_OPERAND_VALUE_PARTIAL;
            }
        } else if (descriptor->kind == AARCH64_CATALOG_OPERAND_EXTEND) {
            if (operand->raw_value <
                sizeof(extend_names) / sizeof(extend_names[0])) {
                operand->modifier_name = extend_names[operand->raw_value];
                operand->status = AARCH64_OPERAND_VALUE_INTERPRETED;
            } else {
                operand->status = AARCH64_OPERAND_VALUE_PARTIAL;
            }
        } else if (descriptor->kind == AARCH64_CATALOG_OPERAND_LANE) {
            operand->status = AARCH64_OPERAND_VALUE_PARTIAL;
            if (form->family == AARCH64_CATALOG_FAMILY_LOAD_STORE &&
                (AARCH64OperandsSpecializedMemory(form) ||
                 AARCH64OperandsConventionalMemory(form)))
                operand->status = AARCH64_OPERAND_VALUE_INTERPRETED;
        }

        if ((AARCH64OperandsConventionalBranch(form) ||
              AARCH64OperandsExceptionalControl(form) ||
              AARCH64OperandsFormHasGroup(form, "hints")) &&
            operand->kind == AARCH64_CATALOG_OPERAND_RAW &&
            operand->role == AARCH64_CATALOG_OPERAND_ROLE_FIELD) {
            operand->status = AARCH64_OPERAND_VALUE_INTERPRETED;
            operand->flags |= AARCH64_OPERAND_FLAG_ENCODING_SELECTOR;
        }
        if ((AARCH64OperandsConventionalBranch(form) ||
             AARCH64OperandsExceptionalControl(form)) &&
            AARCH64OperandsFormHasGroup(form, "branch_reg") &&
            AARCH64OperandsStringEquals(operand->name, "Z")) {
            operand->kind = AARCH64_CATALOG_OPERAND_RAW;
            operand->status = AARCH64_OPERAND_VALUE_INTERPRETED;
            operand->flags |= AARCH64_OPERAND_FLAG_ENCODING_SELECTOR;
        }
        if ((AARCH64OperandsConventionalMemory(form) ||
             AARCH64OperandsSpecializedMemory(form) ||
             AARCH64OperandsForwardedMemory(form)) &&
            operand->role == AARCH64_CATALOG_OPERAND_ROLE_FIELD &&
            (operand->kind == AARCH64_CATALOG_OPERAND_RAW ||
             (operand->kind == AARCH64_CATALOG_OPERAND_FP_SIMD &&
              AARCH64OperandsStringEquals(operand->name, "VR")))) {
            operand->kind = AARCH64_CATALOG_OPERAND_RAW;
            operand->status = AARCH64_OPERAND_VALUE_INTERPRETED;
            operand->flags |= AARCH64_OPERAND_FLAG_ENCODING_SELECTOR;
        }

        if ((form->family == AARCH64_CATALOG_FAMILY_DP_IMMEDIATE ||
             form->family == AARCH64_CATALOG_FAMILY_DP_REGISTER) &&
            descriptor->kind == AARCH64_CATALOG_OPERAND_RAW &&
            AARCH64OperandsStringEquals(operand->name, "hw") &&
            AARCH64OperandsFormHasGroup(form, "movewide")) {
            operand->value = operand->raw_value * 16u;
            operand->modifier_name = "LSL";
            operand->status = AARCH64_OPERAND_VALUE_INTERPRETED;
        }
        if ((form->family == AARCH64_CATALOG_FAMILY_DP_IMMEDIATE ||
             form->family == AARCH64_CATALOG_FAMILY_DP_REGISTER) &&
            descriptor->kind == AARCH64_CATALOG_OPERAND_RAW &&
            (AARCH64OperandsStringEquals(operand->name, "nzcv") ||
             AARCH64OperandsStringEquals(operand->name, "mask"))) {
            operand->status = AARCH64_OPERAND_VALUE_INTERPRETED;
            operand->kind = AARCH64_CATALOG_OPERAND_IMMEDIATE;
        }

        if ((form->family == AARCH64_CATALOG_FAMILY_DP_IMMEDIATE ||
             form->family == AARCH64_CATALOG_FAMILY_DP_REGISTER) &&
            descriptor->kind == AARCH64_CATALOG_OPERAND_RAW &&
            AARCH64OperandsIntegerEncodingSelector(operand->name)) {
            operand->status = AARCH64_OPERAND_VALUE_INTERPRETED;
            operand->flags |= AARCH64_OPERAND_FLAG_ENCODING_SELECTOR;
        }

        if ((descriptor->flags &
             AARCH64_CATALOG_OPERAND_REGISTER_LIST) != 0u) {
            operand->flags |= AARCH64_OPERAND_FLAG_REGISTER_LIST;
            if (form->family == AARCH64_CATALOG_FAMILY_LOAD_STORE &&
                AARCH64OperandsSpecializedMemory(form))
                operand->status = AARCH64_OPERAND_VALUE_INTERPRETED;
            else
                operand->status = AARCH64_OPERAND_VALUE_PARTIAL;
        }
        if ((form->family == AARCH64_CATALOG_FAMILY_DP_IMMEDIATE ||
             form->family == AARCH64_CATALOG_FAMILY_DP_REGISTER) &&
            descriptor->kind == AARCH64_CATALOG_OPERAND_GPR &&
            (AARCH64OperandsFormHasGroup(form, "addsub_imm") ||
             AARCH64OperandsFormHasGroup(form, "addsub_ext")) &&
            operand->register_index == 31u &&
            AARCH64OperandsStringEquals(operand->name, "Rd") &&
            ((encoding >> 29) & 1u) != 0u)
            operand->register_special = AARCH64_REGISTER_SPECIAL_ZERO;
        if (operand->status == AARCH64_OPERAND_VALUE_PARTIAL)
            saw_partial = 1;
        else if (operand->status == AARCH64_OPERAND_VALUE_RAW)
            saw_raw = 1;
        else if (operand->status == AARCH64_OPERAND_VALUE_INTERPRETED)
            saw_interpreted = 1;
    }

    if (!AARCH64OperandsAddCompositeOperands(operands)) {
        operands->status = AARCH64_OPERAND_EXTRACTION_INVALID;
        return operands->status;
    }

    AARCH64OperandsSetPointerAuthMetadata(operands);
    AARCH64OperandsSetExceptionalControlMetadata(operands);
    AARCH64OperandsSetMemoryMetadata(operands);
    AARCH64OperandsSetSpecializedMemoryMetadata(operands);
    if (AARCH64OperandsFormHasGroup(form, "ldst_pac")) {
        int64_t offset = AARCH64OperandsSignExtend(
            ((encoding >> 12) & 0x1ffu) |
            (((encoding >> 22) & 1u) << 9), 10) * 8;
        operands->memory_access_width = 64u;
        operands->memory_base_register = (uint8_t)((encoding >> 5) & 31u);
        operands->memory_data_register = (uint8_t)(encoding & 31u);
        operands->memory_writeback = (encoding & (1u << 11)) != 0u;
        operands->memory_offset = (uint64_t)offset;
    }
    if (AARCH64OperandsConventionalBranch(form)) {
        uint16_t branch_index;
        operands->branch_condition = 0xffu;
        operands->branch_target_register = 0xffu;
        if (AARCH64OperandsFormHasGroup(form, "compbranch_regs2"))
            operands->branch_compare_width = (encoding & (1u << 14)) != 0u
                ? 16u : 8u;
        else if (AARCH64OperandsFormHasGroup(form, "compbranch") ||
                 AARCH64OperandsFormHasGroup(form, "compbranch_imm") ||
                 AARCH64OperandsFormHasGroup(form, "compbranch_regs") ||
                 AARCH64OperandsFormHasGroup(form, "testbranch"))
            operands->branch_compare_width = (encoding >> 31) != 0u
                ? 64u : 32u;
        if (AARCH64OperandsFormHasGroup(form, "branch_reg")) {
            operands->control_flow =
                AARCH64OperandsStringEquals(form->operation_id, "RET")
                ? AARCH64_CONTROL_FLOW_RETURN : AARCH64_CONTROL_FLOW_INDIRECT;
            operands->branch_link =
                AARCH64OperandsStringEquals(form->operation_id, "BLR");
            operands->branch_fallthrough = operands->branch_link;
        } else if (AARCH64OperandsFormHasGroup(form, "branch_imm")) {
            operands->control_flow = AARCH64_CONTROL_FLOW_DIRECT;
            operands->branch_link =
                AARCH64OperandsStringEquals(form->operation_id, "BL");
            operands->branch_fallthrough = operands->branch_link;
        } else {
            operands->control_flow = AARCH64_CONTROL_FLOW_CONDITIONAL;
            operands->branch_fallthrough = 1u;
            operands->branch_reads_nzcv =
                AARCH64OperandsFormHasGroup(form, "condbranch") &&
                (encoding & 15u) < 14u;
            if (AARCH64OperandsFormHasGroup(form, "condbranch") &&
                (encoding & 15u) >= 14u)
                operands->branch_fallthrough = 0u;
        }
        operands->branch_reads_pc =
            !AARCH64OperandsFormHasGroup(form, "branch_reg") ||
            operands->branch_link;
        if (operands->branch_fallthrough)
            operands->branch_fallthrough_address = instruction_address + 4u;
        for (branch_index = 0; branch_index < operands->operand_count;
             branch_index++) {
            const AARCH64Operand *branch_operand =
                &operands->operands[branch_index];
            if ((branch_operand->flags & AARCH64_OPERAND_FLAG_PC_RELATIVE) != 0u &&
                (AARCH64OperandsStringEquals(branch_operand->name, "imm26") ||
                 AARCH64OperandsStringEquals(branch_operand->name, "imm19") ||
                 AARCH64OperandsStringEquals(branch_operand->name, "imm14") ||
                 AARCH64OperandsStringEquals(branch_operand->name, "imm9")))
                operands->branch_target_address = branch_operand->address;
            if (AARCH64OperandsStringEquals(branch_operand->name, "Rn") &&
                operands->control_flow != AARCH64_CONTROL_FLOW_NONE) {
                operands->branch_target_register =
                    branch_operand->register_index;
                operands->branch_reads_lr =
                    operands->control_flow == AARCH64_CONTROL_FLOW_RETURN &&
                    branch_operand->register_index == 30u;
            }
            if (AARCH64OperandsStringEquals(branch_operand->name, "cond") ||
                AARCH64OperandsStringEquals(branch_operand->name, "cc"))
                operands->branch_condition =
                    (uint8_t)branch_operand->raw_value;
        }
    }
    alias_result = AARCH64CatalogFindPreferredAlias(
        match->form_index,
        encoding,
        profile,
        &operands->preferred_alias
    );
    if ((alias_result == AARCH64_CATALOG_PATTERN_MATCHED_UNVALIDATED ||
         alias_result == AARCH64_CATALOG_AMBIGUOUS) &&
        !AARCH64OperandsFormIsSystemOrSecurity(form))
        saw_partial = 1u;
    for (index = form->operand_descriptor_count;
         index < operands->operand_count;
         index++) {
        if (operands->operands[index].status ==
            AARCH64_OPERAND_VALUE_PARTIAL)
            saw_partial = 1;
        else if (operands->operands[index].status ==
                 AARCH64_OPERAND_VALUE_INTERPRETED)
            saw_interpreted = 1;
    }
    if (catalog_status != AARCH64_CATALOG_VALID_ENCODING &&
        !(catalog_status == AARCH64_CATALOG_PATTERN_MATCHED_UNVALIDATED &&
          AARCH64OperandsFormIsSystemOrSecurity(form)))
        saw_partial = 1;
    if (saw_partial ||
        (saw_raw && saw_interpreted &&
         !AARCH64OperandsFormIsSystemOrSecurity(form)))
        operands->status = AARCH64_OPERAND_EXTRACTION_PARTIAL;
    else if (saw_raw && !AARCH64OperandsFormIsSystemOrSecurity(form))
        operands->status = AARCH64_OPERAND_EXTRACTION_RAW;
    if ((form->family == AARCH64_CATALOG_FAMILY_DP_IMMEDIATE ||
         form->family == AARCH64_CATALOG_FAMILY_DP_REGISTER) &&
        operands->status == AARCH64_OPERAND_EXTRACTION_COMPLETE) {
        for (index = 0; index < operands->operand_count; index++) {
            const AARCH64Operand *operand = &operands->operands[index];
            if (operand->status != AARCH64_OPERAND_VALUE_INTERPRETED) {
                operands->status = AARCH64_OPERAND_EXTRACTION_PARTIAL;
                break;
            }
        }
    }
    return operands->status;
}

AARCH64OperandExtractionStatus AARCH64OperandsExtract(
    const AARCH64CatalogMatch *match,
    AARCH64CatalogResult catalog_status,
    uint32_t encoding,
    uint64_t instruction_address,
    AARCH64OperandSet *operands
)
{
    return AARCH64OperandsExtractWithProfile(
        match,
        catalog_status,
        encoding,
        instruction_address,
        0,
        operands
    );
}
