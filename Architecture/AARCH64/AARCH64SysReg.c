#include "Includes/AARCH64SysReg.h"

typedef struct {
    uint8_t op0;
    uint8_t op1;
    uint8_t crn;
    uint8_t crm;
    uint8_t op2;
    AARCH64SysRegIdentity identity;
    const char *name;
    AARCH64SysRegAccess access;
} AARCH64SysRegEntry;

static const AARCH64SysRegEntry AARCH64SysRegTable[] = {
    {3u, 0u, 0u, 0u, 0u, AARCH64_SYSREG_ID_MIDR_EL1, "MIDR_EL1", AARCH64_SYSREG_ACCESS_READ},
    {3u, 0u, 0u, 0u, 5u, AARCH64_SYSREG_ID_MPIDR_EL1, "MPIDR_EL1", AARCH64_SYSREG_ACCESS_READ},
    {3u, 0u, 0u, 0u, 6u, AARCH64_SYSREG_ID_REVIDR_EL1, "REVIDR_EL1", AARCH64_SYSREG_ACCESS_READ},
    {3u, 3u, 4u, 2u, 0u, AARCH64_SYSREG_ID_NZCV, "NZCV", AARCH64_SYSREG_ACCESS_WRITE},
    {3u, 3u, 4u, 2u, 1u, AARCH64_SYSREG_ID_DAIF, "DAIF", AARCH64_SYSREG_ACCESS_WRITE},
    {3u, 0u, 4u, 2u, 2u, AARCH64_SYSREG_ID_CURRENTEL, "CurrentEL", AARCH64_SYSREG_ACCESS_READ},
    {3u, 0u, 4u, 0u, 0u, AARCH64_SYSREG_ID_SPSR_EL1, "SPSR_EL1", AARCH64_SYSREG_ACCESS_WRITE},
    {3u, 4u, 4u, 0u, 0u, AARCH64_SYSREG_ID_SPSR_EL2, "SPSR_EL2", AARCH64_SYSREG_ACCESS_WRITE},
    {3u, 6u, 4u, 0u, 0u, AARCH64_SYSREG_ID_SPSR_EL3, "SPSR_EL3", AARCH64_SYSREG_ACCESS_WRITE},
    {3u, 0u, 4u, 0u, 1u, AARCH64_SYSREG_ID_ELR_EL1, "ELR_EL1", AARCH64_SYSREG_ACCESS_WRITE},
    {3u, 4u, 4u, 0u, 1u, AARCH64_SYSREG_ID_ELR_EL2, "ELR_EL2", AARCH64_SYSREG_ACCESS_WRITE},
    {3u, 6u, 4u, 0u, 1u, AARCH64_SYSREG_ID_ELR_EL3, "ELR_EL3", AARCH64_SYSREG_ACCESS_WRITE},
    {3u, 0u, 6u, 0u, 0u, AARCH64_SYSREG_ID_FAR_EL1, "FAR_EL1", AARCH64_SYSREG_ACCESS_WRITE},
    {3u, 4u, 6u, 0u, 0u, AARCH64_SYSREG_ID_FAR_EL2, "FAR_EL2", AARCH64_SYSREG_ACCESS_WRITE},
    {3u, 6u, 6u, 0u, 0u, AARCH64_SYSREG_ID_FAR_EL3, "FAR_EL3", AARCH64_SYSREG_ACCESS_WRITE},
    {3u, 0u, 5u, 2u, 0u, AARCH64_SYSREG_ID_ESR_EL1, "ESR_EL1", AARCH64_SYSREG_ACCESS_READ},
    {3u, 4u, 5u, 2u, 0u, AARCH64_SYSREG_ID_ESR_EL2, "ESR_EL2", AARCH64_SYSREG_ACCESS_READ},
    {3u, 6u, 5u, 2u, 0u, AARCH64_SYSREG_ID_ESR_EL3, "ESR_EL3", AARCH64_SYSREG_ACCESS_READ},
    {3u, 0u, 12u, 0u, 0u, AARCH64_SYSREG_ID_VBAR_EL1, "VBAR_EL1", AARCH64_SYSREG_ACCESS_WRITE},
    {3u, 4u, 12u, 0u, 0u, AARCH64_SYSREG_ID_VBAR_EL2, "VBAR_EL2", AARCH64_SYSREG_ACCESS_WRITE},
    {3u, 6u, 12u, 0u, 0u, AARCH64_SYSREG_ID_VBAR_EL3, "VBAR_EL3", AARCH64_SYSREG_ACCESS_WRITE},
    {3u, 0u, 1u, 0u, 0u, AARCH64_SYSREG_ID_SCTLR_EL1, "SCTLR_EL1", AARCH64_SYSREG_ACCESS_WRITE},
    {3u, 4u, 1u, 0u, 0u, AARCH64_SYSREG_ID_SCTLR_EL2, "SCTLR_EL2", AARCH64_SYSREG_ACCESS_WRITE},
    {3u, 6u, 1u, 0u, 0u, AARCH64_SYSREG_ID_SCTLR_EL3, "SCTLR_EL3", AARCH64_SYSREG_ACCESS_WRITE},
    {3u, 0u, 2u, 0u, 2u, AARCH64_SYSREG_ID_TCR_EL1, "TCR_EL1", AARCH64_SYSREG_ACCESS_WRITE},
    {3u, 4u, 2u, 0u, 2u, AARCH64_SYSREG_ID_TCR_EL2, "TCR_EL2", AARCH64_SYSREG_ACCESS_WRITE},
    {3u, 6u, 2u, 0u, 2u, AARCH64_SYSREG_ID_TCR_EL3, "TCR_EL3", AARCH64_SYSREG_ACCESS_WRITE},
    {3u, 0u, 2u, 0u, 0u, AARCH64_SYSREG_ID_TTBR0_EL1, "TTBR0_EL1", AARCH64_SYSREG_ACCESS_WRITE},
    {3u, 4u, 2u, 0u, 0u, AARCH64_SYSREG_ID_TTBR0_EL2, "TTBR0_EL2", AARCH64_SYSREG_ACCESS_WRITE},
    {3u, 6u, 2u, 0u, 0u, AARCH64_SYSREG_ID_TTBR0_EL3, "TTBR0_EL3", AARCH64_SYSREG_ACCESS_WRITE},
    {3u, 0u, 2u, 0u, 1u, AARCH64_SYSREG_ID_TTBR1_EL1, "TTBR1_EL1", AARCH64_SYSREG_ACCESS_WRITE},
    {3u, 4u, 2u, 0u, 1u, AARCH64_SYSREG_ID_TTBR1_EL2, "TTBR1_EL2", AARCH64_SYSREG_ACCESS_WRITE},
    {3u, 0u, 10u, 2u, 0u, AARCH64_SYSREG_ID_MAIR_EL1, "MAIR_EL1", AARCH64_SYSREG_ACCESS_WRITE},
    {3u, 4u, 10u, 2u, 0u, AARCH64_SYSREG_ID_MAIR_EL2, "MAIR_EL2", AARCH64_SYSREG_ACCESS_WRITE},
    {3u, 6u, 10u, 2u, 0u, AARCH64_SYSREG_ID_MAIR_EL3, "MAIR_EL3", AARCH64_SYSREG_ACCESS_WRITE},
    {3u, 0u, 10u, 3u, 0u, AARCH64_SYSREG_ID_AMAIR_EL1, "AMAIR_EL1", AARCH64_SYSREG_ACCESS_WRITE},
    {3u, 4u, 10u, 3u, 0u, AARCH64_SYSREG_ID_AMAIR_EL2, "AMAIR_EL2", AARCH64_SYSREG_ACCESS_WRITE},
    {3u, 6u, 10u, 3u, 0u, AARCH64_SYSREG_ID_AMAIR_EL3, "AMAIR_EL3", AARCH64_SYSREG_ACCESS_WRITE},
    {3u, 0u, 13u, 0u, 1u, AARCH64_SYSREG_ID_CONTEXTIDR_EL1, "CONTEXTIDR_EL1", AARCH64_SYSREG_ACCESS_WRITE},
    {3u, 3u, 13u, 0u, 2u, AARCH64_SYSREG_ID_TPIDR_EL0, "TPIDR_EL0", AARCH64_SYSREG_ACCESS_WRITE},
    {3u, 0u, 13u, 0u, 4u, AARCH64_SYSREG_ID_TPIDR_EL1, "TPIDR_EL1", AARCH64_SYSREG_ACCESS_WRITE},
    {3u, 4u, 13u, 0u, 2u, AARCH64_SYSREG_ID_TPIDR_EL2, "TPIDR_EL2", AARCH64_SYSREG_ACCESS_WRITE},
    {3u, 6u, 13u, 0u, 2u, AARCH64_SYSREG_ID_TPIDR_EL3, "TPIDR_EL3", AARCH64_SYSREG_ACCESS_WRITE},
    {3u, 0u, 4u, 1u, 0u, AARCH64_SYSREG_ID_SP_EL0, "SP_EL0", AARCH64_SYSREG_ACCESS_WRITE},
    {3u, 4u, 4u, 1u, 0u, AARCH64_SYSREG_ID_SP_EL1, "SP_EL1", AARCH64_SYSREG_ACCESS_WRITE},
    {3u, 6u, 4u, 1u, 0u, AARCH64_SYSREG_ID_SP_EL2, "SP_EL2", AARCH64_SYSREG_ACCESS_WRITE},
    {3u, 3u, 14u, 0u, 0u, AARCH64_SYSREG_ID_CNTFRQ_EL0, "CNTFRQ_EL0", AARCH64_SYSREG_ACCESS_WRITE},
    {3u, 3u, 14u, 0u, 2u, AARCH64_SYSREG_ID_CNTVCT_EL0, "CNTVCT_EL0", AARCH64_SYSREG_ACCESS_READ},
    {3u, 3u, 14u, 0u, 1u, AARCH64_SYSREG_ID_CNTPCT_EL0, "CNTPCT_EL0", AARCH64_SYSREG_ACCESS_READ},
};

static const AARCH64SysRegDescriptor AARCH64SysRegUnknown = {
    0u, 0u, 0u, 0u, 0u, 0u, AARCH64_SYSREG_ID_UNKNOWN, "unknown",
    AARCH64_SYSREG_ACCESS_NONE, 0u
};

uint16_t AARCH64SysRegEncode(
    uint8_t op0, uint8_t op1, uint8_t crn, uint8_t crm, uint8_t op2
)
{
    return (uint16_t)(
        ((op0 & AARCH64_SYSREG_OP0_MASK) << 14) |
        ((op1 & AARCH64_SYSREG_OP1_MASK) << 11) |
        ((crn & AARCH64_SYSREG_CRN_MASK) << 7) |
        ((crm & AARCH64_SYSREG_CRM_MASK) << 3) |
        (op2 & AARCH64_SYSREG_OP2_MASK)
    );
}

const AARCH64SysRegDescriptor *AARCH64SysRegResolve(
    uint8_t op0, uint8_t op1, uint8_t crn, uint8_t crm, uint8_t op2
)
{
    uint32_t index;
    uint32_t count = (uint32_t)(sizeof(AARCH64SysRegTable) / sizeof(AARCH64SysRegTable[0]));

    for (index = 0u; index < count; index++) {
        const AARCH64SysRegEntry *entry = &AARCH64SysRegTable[index];
        if (entry->op0 == op0 && entry->op1 == op1 &&
            entry->crn == crn && entry->crm == crm && entry->op2 == op2) {
            static AARCH64SysRegDescriptor resolved;
            resolved.op0 = entry->op0;
            resolved.op1 = entry->op1;
            resolved.crn = entry->crn;
            resolved.crm = entry->crm;
            resolved.op2 = entry->op2;
            resolved.encoding = AARCH64SysRegEncode(op0, op1, crn, crm, op2);
            resolved.identity = entry->identity;
            resolved.name = entry->name;
            resolved.access = entry->access;
            resolved.is_implementation_defined = 0u;
            return &resolved;
        }
    }

    {
        static AARCH64SysRegDescriptor unknown;
        unknown = AARCH64SysRegUnknown;
        unknown.op0 = op0;
        unknown.op1 = op1;
        unknown.crn = crn;
        unknown.crm = crm;
        unknown.op2 = op2;
        unknown.encoding = AARCH64SysRegEncode(op0, op1, crn, crm, op2);
        return &unknown;
    }
}

const char *AARCH64SysRegIdentityName(AARCH64SysRegIdentity identity)
{
    switch (identity) {
    case AARCH64_SYSREG_ID_MIDR_EL1: return "MIDR_EL1";
    case AARCH64_SYSREG_ID_MPIDR_EL1: return "MPIDR_EL1";
    case AARCH64_SYSREG_ID_REVIDR_EL1: return "REVIDR_EL1";
    case AARCH64_SYSREG_ID_NZCV: return "NZCV";
    case AARCH64_SYSREG_ID_DAIF: return "DAIF";
    case AARCH64_SYSREG_ID_CURRENTEL: return "CurrentEL";
    case AARCH64_SYSREG_ID_SPSR_EL1: return "SPSR_EL1";
    case AARCH64_SYSREG_ID_SPSR_EL2: return "SPSR_EL2";
    case AARCH64_SYSREG_ID_SPSR_EL3: return "SPSR_EL3";
    case AARCH64_SYSREG_ID_ELR_EL1: return "ELR_EL1";
    case AARCH64_SYSREG_ID_ELR_EL2: return "ELR_EL2";
    case AARCH64_SYSREG_ID_ELR_EL3: return "ELR_EL3";
    case AARCH64_SYSREG_ID_FAR_EL1: return "FAR_EL1";
    case AARCH64_SYSREG_ID_FAR_EL2: return "FAR_EL2";
    case AARCH64_SYSREG_ID_FAR_EL3: return "FAR_EL3";
    case AARCH64_SYSREG_ID_ESR_EL1: return "ESR_EL1";
    case AARCH64_SYSREG_ID_ESR_EL2: return "ESR_EL2";
    case AARCH64_SYSREG_ID_ESR_EL3: return "ESR_EL3";
    case AARCH64_SYSREG_ID_VBAR_EL1: return "VBAR_EL1";
    case AARCH64_SYSREG_ID_VBAR_EL2: return "VBAR_EL2";
    case AARCH64_SYSREG_ID_VBAR_EL3: return "VBAR_EL3";
    case AARCH64_SYSREG_ID_SCTLR_EL1: return "SCTLR_EL1";
    case AARCH64_SYSREG_ID_SCTLR_EL2: return "SCTLR_EL2";
    case AARCH64_SYSREG_ID_SCTLR_EL3: return "SCTLR_EL3";
    case AARCH64_SYSREG_ID_TCR_EL1: return "TCR_EL1";
    case AARCH64_SYSREG_ID_TCR_EL2: return "TCR_EL2";
    case AARCH64_SYSREG_ID_TCR_EL3: return "TCR_EL3";
    case AARCH64_SYSREG_ID_TTBR0_EL1: return "TTBR0_EL1";
    case AARCH64_SYSREG_ID_TTBR0_EL2: return "TTBR0_EL2";
    case AARCH64_SYSREG_ID_TTBR0_EL3: return "TTBR0_EL3";
    case AARCH64_SYSREG_ID_TTBR1_EL1: return "TTBR1_EL1";
    case AARCH64_SYSREG_ID_TTBR1_EL2: return "TTBR1_EL2";
    case AARCH64_SYSREG_ID_MAIR_EL1: return "MAIR_EL1";
    case AARCH64_SYSREG_ID_MAIR_EL2: return "MAIR_EL2";
    case AARCH64_SYSREG_ID_MAIR_EL3: return "MAIR_EL3";
    case AARCH64_SYSREG_ID_AMAIR_EL1: return "AMAIR_EL1";
    case AARCH64_SYSREG_ID_AMAIR_EL2: return "AMAIR_EL2";
    case AARCH64_SYSREG_ID_AMAIR_EL3: return "AMAIR_EL3";
    case AARCH64_SYSREG_ID_CONTEXTIDR_EL1: return "CONTEXTIDR_EL1";
    case AARCH64_SYSREG_ID_TPIDR_EL0: return "TPIDR_EL0";
    case AARCH64_SYSREG_ID_TPIDR_EL1: return "TPIDR_EL1";
    case AARCH64_SYSREG_ID_TPIDR_EL2: return "TPIDR_EL2";
    case AARCH64_SYSREG_ID_TPIDR_EL3: return "TPIDR_EL3";
    case AARCH64_SYSREG_ID_SP_EL0: return "SP_EL0";
    case AARCH64_SYSREG_ID_SP_EL1: return "SP_EL1";
    case AARCH64_SYSREG_ID_SP_EL2: return "SP_EL2";
    case AARCH64_SYSREG_ID_CNTFRQ_EL0: return "CNTFRQ_EL0";
    case AARCH64_SYSREG_ID_CNTVCT_EL0: return "CNTVCT_EL0";
    case AARCH64_SYSREG_ID_CNTPCT_EL0: return "CNTPCT_EL0";
    case AARCH64_SYSREG_ID_PMCR_EL0: return "PMCR_EL0";
    case AARCH64_SYSREG_ID_PMCNTENSET_EL0: return "PMCNTENSET_EL0";
    case AARCH64_SYSREG_ID_PMCNTENCLR_EL0: return "PMCNTENCLR_EL0";
    case AARCH64_SYSREG_ID_PMSWINC_EL0: return "PMSWINC_EL0";
    case AARCH64_SYSREG_ID_PMINTENSET_EL1: return "PMINTENSET_EL1";
    case AARCH64_SYSREG_ID_PMINTENCLR_EL1: return "PMINTENCLR_EL1";
    case AARCH64_SYSREG_ID_PMOVSSET_EL0: return "PMOVSSET_EL0";
    case AARCH64_SYSREG_ID_PMOVSCLR_EL0: return "PMOVSCLR_EL0";
    case AARCH64_SYSREG_ID_UNKNOWN:
    default: return "unknown";
    }
}